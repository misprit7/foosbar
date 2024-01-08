#include <Network.h>
#include <chrono>
#include <mutex>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/calib3d.hpp>


#include <cstddef>
#include <functional>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <algorithm>

#include "physical_params.hpp"

#include <clearpath/pubMotion.h>
#include <clearpath/pubSysCls.h>

#include <qualisys_cpp_sdk/RTProtocol.h>
#include <qualisys_cpp_sdk/RTPacket.h>

#include <uWebSockets/App.h>
#include <uWebSockets/PerMessageDeflate.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

/******************************************************************************
 * Defines
 ******************************************************************************/

#define ever ;;

#define ACC_LIM_RPM_PER_SEC    10000
#define VEL_LIM_RPM            1000

#define HOMING_TIMEOUT 10000

typedef enum state_t {
    state_defense,
    state_unknown,
    num_state_t
} state_t;

/******************************************************************************
 * Global Variables
 ******************************************************************************/
// Might not want these to be global later, whatever for now
sFnd::SysManager mgr;
vector<reference_wrapper<sFnd::INode>> lin_nodes;
vector<reference_wrapper<sFnd::INode>> rot_nodes;

/******************************************************************************
 * Definitions
 ******************************************************************************/
void onLowHChange(int, void*) {}
void onHighHChange(int, void*) {}
void onLowSChange(int, void*) {}
void onHighSChange(int, void*) {}
void onLowVChange(int, void*) {}
void onHighVChange(int, void*) {}

/******************************************************************************
 * Motor Wrappers
 ******************************************************************************/


void move_lin(int rod, double position_cm){
    int target_cnts = clamp(
            (int)(-lin_cm_to_cnts[rod] * position_cm),
            min(lin_range_cnts[rod][0], lin_range_cnts[rod][1]),
            max(lin_range_cnts[rod][0], lin_range_cnts[rod][1])
    );
    lin_nodes[rod].get().Motion.MovePosnStart(target_cnts, true);
}

void move_rot(int rod, double position_deg){
    int target_cnts = rot_deg_to_cnts[rod] * position_deg;
    rot_nodes[rod].get().Motion.MovePosnStart(target_cnts, true);
}

void set_speed_lin(int rod, double vel_cm_per_s, double acc_cm_per_s2){
    lin_nodes[rod].get().Motion.VelLimit = abs(vel_cm_per_s * lin_cm_to_cnts[rod]);
    lin_nodes[rod].get().Motion.AccLimit = abs(acc_cm_per_s2 * lin_cm_to_cnts[rod]);
}

void set_speed_rot(int rod, double vel_deg_per_s, double acc_deg_per_s2){
    rot_nodes[rod].get().Motion.VelLimit = vel_deg_per_s * rot_deg_to_cnts[rod];
    rot_nodes[rod].get().Motion.AccLimit = acc_deg_per_s2 * rot_deg_to_cnts[rod];
}

void close_all(){
    sFnd::IPort&port = mgr.Ports(0);
    for(int i = 0; i < port.NodeCount(); ++i){
        port.Nodes(i).EnableReq(false);
    }
    mgr.PortsClose();
}

int motors_init(){
    vector<string> comHubPorts;

    // Identify hubs
    sFnd::SysManager::FindComHubPorts(comHubPorts);
    printf("Found %zu SC Hubs\n", comHubPorts.size());

    // Find available ports
    size_t portCount = 0;
    for (portCount = 0; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++) {
        mgr.ComHubPort(portCount, comHubPorts[portCount].c_str());
    }

    if (portCount < 0) {
        printf("Unable to locate SC hub port\n");
        return -1;
    }

    // Open ports (hubs)
    mgr.PortsOpen(portCount);

    sFnd::IPort &port = mgr.Ports(0);
    printf(" Port[%d]: state=%d, nodes=%d\n",
        port.NetNumber(), port.OpenState(), port.NodeCount());

    // Arrange nodes
    for(int i = 0; i < num_rod_t; ++i){
        string lin_name = "lin-" + rod_names[i];
        string rot_name = "rot-" + rod_names[i];
        bool lin_found = false, rot_found = false;

        // Search to find correct names
        for(int j = 0; j < port.NodeCount(); ++j){
            string name = port.Nodes(j).Info.UserID.Value();
            if(!lin_found && lin_name == name){
                lin_nodes.push_back(port.Nodes(j));
                lin_found = true;
            }
            if(!rot_found && rot_name == name){
                rot_nodes.push_back(port.Nodes(j));
                rot_found = true;
            }
        }
        if(!lin_found || !rot_found){
            printf("Not all motors are present!\n");
            return -1;
        }
    }

    // Enable nodes
    for(int i = 0; i < lin_nodes.size(); ++i){
        lin_nodes[i].get().Status.AlertsClear();
        lin_nodes[i].get().Motion.NodeStopClear();
        lin_nodes[i].get().EnableReq(true);
    }

    for(int i = 0; i < rot_nodes.size(); ++i){
        rot_nodes[i].get().Status.AlertsClear();
        rot_nodes[i].get().Motion.NodeStopClear();
        rot_nodes[i].get().EnableReq(true);
    }

    // Wait for enable
    double timeout = mgr.TimeStampMsec() + 2000;
    for(ever){
        bool ready = true;
        for(int i = 0; i < lin_nodes.size(); ++i){
            if(!lin_nodes[i].get().Motion.IsReady()) ready = false;
        }
        for(int i = 0; i < rot_nodes.size(); ++i){
            if(!rot_nodes[i].get().Motion.IsReady()) ready = false;
        }
        if(ready) break;
        if (mgr.TimeStampMsec() > timeout) {
            printf("Timed out waiting for Nodes to enable\n");
            return -1;
        }
    }


    // Start homing
    for(int i = 0; i < lin_nodes.size(); ++i){
        if(!lin_nodes[i].get().Motion.Homing.HomingValid()) continue;
        if(!lin_nodes[i].get().Motion.Homing.WasHomed()) 
            lin_nodes[i].get().Motion.Homing.Initiate();
    }

    // Wait for homing
    timeout = mgr.TimeStampMsec() + HOMING_TIMEOUT;
    for(ever){
        bool homed = true;
        for(int i = 0; i < lin_nodes.size(); ++i){
            if(!lin_nodes[i].get().Motion.Homing.HomingValid()) continue;
            if(!lin_nodes[i].get().Motion.Homing.WasHomed()) homed = false;
        }
        if(homed) break;
        
        if(mgr.TimeStampMsec() > timeout){
            cout << "Homing timed out" << endl;
            close_all();
            return -1;
        }
    }

    // Set motion parameters
    for(int i = 0; i < lin_nodes.size(); ++i){
        lin_nodes[i].get().AccUnit(sFnd::INode::COUNTS_PER_SEC2);
        lin_nodes[i].get().VelUnit(sFnd::INode::COUNTS_PER_SEC);
        lin_nodes[i].get().Info.Ex.Parameter(98,1);
        set_speed_lin((rod_t)i, 100, 1000);
        /* set_speed_lin((rod_t)i, 100, 500); */
        /* cout << "Set linear speed for " << rod_names[i] << endl; */
        lin_nodes[i].get().Motion.PosnMeasured.AutoRefresh(true);
        move_lin(i, lin_range_cm[i]/2);
    }
    for(int i = 0; i < rot_nodes.size(); ++i){
        rot_nodes[i].get().AccUnit(sFnd::INode::COUNTS_PER_SEC2);
        rot_nodes[i].get().VelUnit(sFnd::INode::COUNTS_PER_SEC);
        if(i != goalie)
            rot_nodes[i].get().Info.Ex.Parameter(98,1);
        rot_nodes[i].get().Motion.PosnMeasured.AutoRefresh(true);
        /* set_speed_rot((rod_t)i, 10000, 100000); */
        set_speed_rot((rod_t)i, 5000, 50000);
        move_rot(i, 0);
    }

    return 0;
}

/******************************************************************************
 * Misc
 ******************************************************************************/

void signal_handler(int signal) {
    cout << "Got interrupt signal, closing..." << endl;
    if (signal == SIGINT) {
        close_all();
        cout << "Motors disabled" << endl;
        exit(signal);
    }
}

void print_status(string s){
    for(char c : s)
        if(c == '\n')
            cout << "\033[A\33[2K\r";
    cout << s;
}

/******************************************************************************
 * Main
 ******************************************************************************/
int main(int argc, char** argv){

    /**************************************************************************
     * Setup
     **************************************************************************/
    bool controller = false;
    std::signal(SIGINT, signal_handler);

    for(int i = 1; i < argc; ++i){
        string cmd(argv[i]);
        if(cmd == "--controller"){
            controller = true;
        }
    }

    /**************************************************************************
     * WebSocket Init
     **************************************************************************/

    struct socket_data {
        /* User data */
    };
    vector<uWS::WebSocket<false, true, socket_data>*> clients;
    mutex ws_mutex;

    // Webapp state
    double ws_pos[num_rod_t] = {0.5, 0.5, 0.5, 0.5};
    double ws_rot[num_rod_t] = {0, 0, 0, 0};
    bool ws_pos_updated[num_rod_t] = {0};
    bool ws_rot_updated[num_rod_t] = {0};
    int ws_selection = -1;

    struct uWS::Loop *loop;
    // Thread for web socket handling
    thread uws_thread([&]() {
        loop = uWS::Loop::get();
        // C++20 acting funky and makes me specificy every field
        uWS::App app;
        app.ws<socket_data>("/position", {
            .compression = uWS::DISABLED,
            .maxPayloadLength = 16 * 1024 * 1024,
            .idleTimeout = 16,
            .maxBackpressure = 1 * 1024 * 1024,
            .closeOnBackpressureLimit = false,
            .resetIdleTimeoutOnSend = false,
            .sendPingsAutomatically = true,
            .maxLifetime = 0,

            .upgrade = nullptr,
            .open = [&clients, &ws_mutex](auto *ws) {
                /* cout << "Connection! " << 9001 << endl; */
                lock_guard<mutex> lock(ws_mutex);
                clients.push_back(ws);
                /* nlohmann::json params = {}; */
                /* for(int i = 0; i < num_rod_t; ++i){ */
                /*     params["spacing-" + rod_names[i]] = plr_gap[i]; */
                /* } */

            },
            // Handles incoming packets
            .message = [&ws_mutex, &ws_pos, &ws_pos_updated, &ws_rot, &ws_rot_updated, &ws_selection]
                    (auto *ws, string_view message, uWS::OpCode opCode) {
                lock_guard<mutex> lock(ws_mutex);
                json packet = json::parse(message);

                if(packet["type"].get<string>() == "selection"){
                    ws_selection = packet["selection"].get<int>();

                } else if(packet["type"].get<string>() == "move"){
                    if(ws_selection >= 0 && ws_selection <=num_rod_t){
                        ws_pos[ws_selection] = clamp(ws_pos[ws_selection]+packet["pos"].get<double>(), 0.0, 1.0);
                        ws_rot[ws_selection] = ws_rot[ws_selection]+packet["rot"].get<double>();
                        ws_pos_updated[ws_selection] = abs(packet["pos"].get<double>()) > 0.001;
                        ws_rot_updated[ws_selection] = abs(packet["rot"].get<double>()) > 0.001;
                    }
                }
            },
            .drain = [](auto * /*ws*/) {},
            .ping = [](auto * /*ws*/, string_view) {},
            .pong = [](auto * /*ws*/, string_view) {},
            .close = [&clients, &ws_mutex](auto* ws, int /*code*/, string_view /*message*/) {
                /* cout << "Client disconnected" << endl; */
                lock_guard<mutex> lock(ws_mutex);
                clients.erase(remove(clients.begin(), clients.end(), ws), clients.end());
            }
        }).listen(9001, [](auto *listen_socket) {
            if (listen_socket) {
                cout << "Listening on port " << 9001 << endl;
            }
        });

        app.run(); // Run the event loop
    });

    /**************************************************************************
     * QTM Init
     **************************************************************************/

    mutex qtm_mutex;
    vector<float> ball_pos = {0, 0, 0};

    thread qtm_thread([&qtm_mutex, &ball_pos]() {
        CRTProtocol rtProtocol;

        const char           serverAddr[] = "192.168.155.1";
        const unsigned short basePort = 22222;
        const int            majorVersion = 1;
        const int            minorVersion = 19;
        const bool           bigEndian = false;

        unsigned short udpPort = 6734;
        
        while (!rtProtocol.Connected())
        {
            if (!rtProtocol.Connect(serverAddr, basePort, &udpPort, majorVersion, minorVersion, bigEndian))
            {
                printf("rtProtocol.Connect: %s\n\n", rtProtocol.GetErrorString());
                sleep(1);
            }
        }

        if(!rtProtocol.StreamFrames(CRTProtocol::RateAllFrames, 0, udpPort, nullptr, CRTProtocol::cComponent3dNoLabels)){
            printf("Failed streaming!\n");
            return -1;
        }

        for(ever){
            CRTPacket::EPacketType packetType;
            if(rtProtocol.Receive(packetType, true, 0) == CNetwork::ResponseType::success){
                if(packetType != CRTPacket::PacketData) continue;
                lock_guard<mutex> lock(qtm_mutex);

                CRTPacket *rtPacket = rtProtocol.GetRTPacket();

                /* printf("Frame %d\n", rtPacket->GetFrameNumber()); */

                unsigned int n;

                rtPacket->Get3DNoLabelsMarker(0, ball_pos[0], ball_pos[1], ball_pos[2], n);
                for(int i = 0; i < 3; ++i){
                    ball_pos[i] /= 10; // convert to mm from cm
                    ball_pos[i] -= cal_offset[i];
                }

            }
        }
    });

    /**************************************************************************
     * Clearpath Init
     **************************************************************************/

    int init_err = motors_init();
    if(init_err < 0) return init_err;

    /**************************************************************************
     * Main Event Loop
     **************************************************************************/

    try{

    cout << endl;
    
    state_t state = state_defense;

    for(ever){
        stringstream status;

        // Updates in a thread safe and fast way
        vector<float> ball_pos_lcl;
        {
            lock_guard<mutex> lock(qtm_mutex);
            ball_pos_lcl = ball_pos;
        }

        vector<double> red_pos;
        vector<double> red_rot;
        status << "Linear position: ";
        for(int i = 0; i < num_rod_t; ++i){
            red_pos.push_back(abs(lin_nodes[i].get().Motion.PosnMeasured.Value()
                    / lin_cm_to_cnts[i]));
            red_rot.push_back(rot_nodes[i].get().Motion.PosnMeasured.Value() / rot_rad_to_cnts[i]);
            status << lin_nodes[i].get().Motion.PosnMeasured.Value() << ", ";
        }
        status << endl;

        json positionData = {
            {"type", "pos"},
            {"bluepos", {
                0.5, 0.5, 0.5, 0.5
            }},
            {"redpos", {
                red_pos[three_bar] / lin_range_cm[three_bar],
                red_pos[five_bar] / lin_range_cm[five_bar],
                red_pos[two_bar] / lin_range_cm[two_bar],
                red_pos[goalie] / lin_range_cm[goalie],
            }},
            {"bluerot", {
                0,0,0,0
            }},
            {"redrot", {
                red_rot[three_bar],
                red_rot[five_bar],
                red_rot[two_bar],
                red_rot[goalie],
            }},
            {"ballpos", {
                ball_pos_lcl[0]/(play_height)+0.5,
                ball_pos_lcl[1]/(play_width)+0.5,
                0,
            }}
        };
        status << "Ball position: " << ball_pos_lcl[0] << ", " << ball_pos_lcl[1] << ", " << ball_pos_lcl[2] << endl;
        string message = positionData.dump();

        {
            lock_guard<mutex> lock(ws_mutex);
            for (auto* client : clients) {
                loop->defer([client, message](){
                    client->send(message, uWS::OpCode::TEXT);
                });
                /* cout << "Update!" << endl; */
            }
        }

        if(controller){
            for(int i = 0; i < num_rod_t; ++i){
                double ws_pos_lcl, ws_pos_updated_lcl, ws_rot_lcl, ws_rot_updated_lcl;
                {
                    lock_guard<mutex> lock(ws_mutex);
                    ws_pos_lcl = ws_pos[i];
                    ws_pos_updated_lcl = ws_pos_updated[i];
                    ws_rot_lcl = ws_rot[i];
                    ws_rot_updated_lcl = ws_rot_updated[i];
                }
                if(ws_pos_updated_lcl){
                    move_lin(i, lin_range_cm[i] * ws_pos_lcl);
                    ws_pos_updated_lcl = false;
                }
                if(ws_rot_updated_lcl){
                    move_rot(i, ws_rot_lcl / deg_to_rad);
                    ws_rot_updated_lcl = false;
                }
            }
        // Yes, else switch is just as much as a things as else if
        } else switch(state){
            case state_defense:
                int front;
                if(ball_pos_lcl[1] > 2*plr_dist) front = three_bar;
                else if(ball_pos_lcl[1] > 0) front = five_bar;
                else if(ball_pos_lcl[1] > -2*plr_dist) front = two_bar;

                status << front << ": ";
                for(int i = 0; i + front < num_rod_t; ++i){
                    int rod = i + front;
                    double target_cm = ball_pos_lcl[0] + play_height / 2;

                    // Offset so no double blocking
                    if(i == 1){
                        target_cm += (ball_pos_lcl[0] > 0 ? -1 : 1) * plr_width;
                    } else if (i == 2){
                        target_cm += (ball_pos_lcl[0] > 0 ? 1 : -1) * plr_width;
                    }

                    int plr = floor(num_plrs[rod] * target_cm / play_height);

                    // Override since switching is awkward
                    if(rod == two_bar) plr = 0;
                    
                    plr = clamp(plr, 0, num_plrs[rod]);

                    double plr_offset_cm = bumper_width + plr_width/2 + plr*plr_gap[rod];
                    status << plr << ", " << plr_offset_cm << ", ";
                    double move_cm = target_cm - plr_offset_cm;
                    if(abs(move_cm - red_pos[rod]) > 0.3)
                        move_lin(rod, target_cm - plr_offset_cm);
                    
                }
                status << endl;

                break;
            case state_unknown:
            default:
                break;
        }

        print_status(status.str());

        this_thread::sleep_for(chrono::microseconds(5'000));
    }

    } catch (sFnd::mnErr& theErr)
	{
		printf("Failed to disable Nodes n\n");
		//This statement will print the address of the error, the error code (defined by the mnErr class), 
		//as well as the corresponding error message.
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);

		return 0;  //This terminates the main program
	}

    uws_thread.join();

    return 0;
}

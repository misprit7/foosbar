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
#include <deque>
#include <queue>

#include <clearpath/pubMotion.h>
#include <clearpath/pubSysCls.h>

#include <qualisys_cpp_sdk/RTProtocol.h>
#include <qualisys_cpp_sdk/RTPacket.h>

#include <uWebSockets/App.h>
#include <uWebSockets/PerMessageDeflate.h>
#include <nlohmann/json.hpp>

#include "physical_params.hpp"
#include "algo.hpp"

using namespace std;
using json = nlohmann::json;

/******************************************************************************
 * Defines/const
 ******************************************************************************/

#define ever ;;
#define eps 1e-3

const int init_vel_lin_cm_s = 100;
const int init_accel_lin_cm_ss = 1000;

const int init_vel_rot_deg_s = 5000;
const int init_accel_rot_deg_ss = 50000;

const int homing_timeout_ms = 10000;

/******************************************************************************
 * Global Variables
 ******************************************************************************/

// Might not want these to be global later, whatever for now
sFnd::SysManager mgr;
vector<reference_wrapper<sFnd::INode>> nodes[num_axis_t];

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
    nodes[lin][rod].get().Motion.MovePosnStart(target_cnts, true);
}

void move_rot(int rod, double position_deg){
    int target_cnts = rot_deg_to_cnts[rod] * position_deg;
    nodes[rot][rod].get().Motion.MovePosnStart(target_cnts, true);
}

function<void(int, double)> mtr_move[num_axis_t] = {move_lin, move_rot};

void set_speed_lin(int rod, double vel_cm_per_s, double acc_cm_per_s2){
    nodes[lin][rod].get().Motion.VelLimit = abs(vel_cm_per_s * lin_cm_to_cnts[rod]);
    nodes[lin][rod].get().Motion.AccLimit = abs(acc_cm_per_s2 * lin_cm_to_cnts[rod]);
}

void set_speed_rot(int rod, double vel_deg_per_s, double acc_deg_per_s2){
    nodes[rot][rod].get().Motion.VelLimit = vel_deg_per_s * rot_deg_to_cnts[rod];
    nodes[rot][rod].get().Motion.AccLimit = acc_deg_per_s2 * rot_deg_to_cnts[rod];
}

function<void(int, double, double)> mtr_set_speed[num_axis_t] = {set_speed_lin, set_speed_rot};

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
        mgr.ComHubPort(portCount, comHubPorts[portCount].c_str(), MN_BAUD_48X);
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
                nodes[lin].push_back(port.Nodes(j));
                lin_found = true;
            }
            if(!rot_found && rot_name == name){
                nodes[rot].push_back(port.Nodes(j));
                rot_found = true;
            }
        }
        if(!lin_found || !rot_found){
            printf("Not all motors are present!\n");
            return -1;
        }
    }

    // Enable nodes
    for(int i = 0; i < nodes[lin].size(); ++i){
        nodes[lin][i].get().Status.AlertsClear();
        nodes[lin][i].get().Motion.NodeStopClear();
        nodes[lin][i].get().EnableReq(true);
    }

    for(int i = 0; i < nodes[rot].size(); ++i){
        nodes[rot][i].get().Status.AlertsClear();
        nodes[rot][i].get().Motion.NodeStopClear();
        nodes[rot][i].get().EnableReq(true);
    }

    // Wait for enable
    double timeout = mgr.TimeStampMsec() + 2000;
    for(ever){
        bool ready = true;
        for(int i = 0; i < nodes[lin].size(); ++i){
            if(!nodes[lin][i].get().Motion.IsReady()) ready = false;
        }
        for(int i = 0; i < nodes[rot].size(); ++i){
            if(!nodes[rot][i].get().Motion.IsReady()) ready = false;
        }
        if(ready) break;
        if (mgr.TimeStampMsec() > timeout) {
            printf("Timed out waiting for Nodes to enable\n");
            return -1;
        }
    }


    // Start homing
    for(int i = 0; i < nodes[lin].size(); ++i){
        if(!nodes[lin][i].get().Motion.Homing.HomingValid()) continue;
        if(!nodes[lin][i].get().Motion.Homing.WasHomed()) 
            nodes[lin][i].get().Motion.Homing.Initiate();
    }

    // Wait for homing
    timeout = mgr.TimeStampMsec() + homing_timeout_ms;
    for(ever){
        bool homed = true;
        for(int i = 0; i < nodes[lin].size(); ++i){
            if(!nodes[lin][i].get().Motion.Homing.HomingValid()) continue;
            if(!nodes[lin][i].get().Motion.Homing.WasHomed()) homed = false;
        }
        if(homed) break;
        
        if(mgr.TimeStampMsec() > timeout){
            cout << "Homing timed out" << endl;
            close_all();
            return -1;
        }
    }

    // Set motion parameters
    for(int i = 0; i < nodes[lin].size(); ++i){
        nodes[lin][i].get().AccUnit(sFnd::INode::COUNTS_PER_SEC2);
        nodes[lin][i].get().VelUnit(sFnd::INode::COUNTS_PER_SEC);
        nodes[lin][i].get().Info.Ex.Parameter(98,1);
        set_speed_lin((rod_t)i, init_vel_lin_cm_s, init_accel_lin_cm_ss);
        /* set_speed_lin((rod_t)i, 100, 500); */
        /* cout << "Set linear speed for " << rod_names[i] << endl; */
        nodes[lin][i].get().Motion.PosnMeasured.AutoRefresh(true);
        /* move_lin(i, lin_range_cm[i]/2); */
    }
    for(int i = 0; i < nodes[rot].size(); ++i){
        nodes[rot][i].get().AccUnit(sFnd::INode::COUNTS_PER_SEC2);
        nodes[rot][i].get().VelUnit(sFnd::INode::COUNTS_PER_SEC);
        if(i != goalie)
            nodes[rot][i].get().Info.Ex.Parameter(98,1);
        nodes[rot][i].get().Motion.PosnMeasured.AutoRefresh(true);
        /* set_speed_rot((rod_t)i, 10000, 100000); */
        set_speed_rot((rod_t)i, init_vel_rot_deg_s, init_accel_rot_deg_ss);
        /* move_rot(i, 0); */
    }

    return 0;
}

/******************************************************************************
 * Misc
 ******************************************************************************/

bool should_terminate(){
    fd_set set;
    struct timeval timeout;

    // Initialize the file descriptor set
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    // Initialize the timeout data structure
    timeout.tv_sec = 0;  // Wait for up to 1 second
    timeout.tv_usec = 500;

    // Check if input is available
    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
        string command;
        cin >> command;
        if (command == "q") {
            return true;
        }
    }
    return false;
}

void signal_handler(int signal) {
    cout << "Got interrupt signal, closing..." << endl;
    if (signal == SIGINT) {
        /* close_all(); */
        /* cout << "Motors disabled" << endl; */
        /* exit(signal); */
        /* quit = true; */
    }
}

void print_status(string s, bool erase = true){
    if(erase)
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
    bool controller = false, no_motors = false;

    for(int i = 1; i < argc; ++i){
        string cmd(argv[i]);
        if(cmd == "--controller"){
            controller = true;
        } else if(cmd == "--no-motors"){
            no_motors = true;
        } else {
            cout << "Unrecognized argument: " << cmd << endl;
            return -1;
        }
    }

    // For some reason first call gives garbage value
    (void) mgr.TimeStampMsec();

    /**************************************************************************
     * WebSocket Init
     **************************************************************************/

    struct socket_data {
        /* User data */
    };
    vector<uWS::WebSocket<false, true, socket_data>*> clients;
    mutex ws_mutex;

    // Webapp state
    double tgt_pos[num_rod_t] = {0.5, 0.5, 0.5, 0.5};
    double tgt_rot[num_rod_t] = {0, 0, 0, 0};
    bool in_shot = false;
    bool tgt_pos_updated[num_rod_t] = {0};
    bool tgt_rot_updated[num_rod_t] = {0};
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
            .message = [&ws_mutex, &tgt_pos, &tgt_pos_updated, &tgt_rot, &tgt_rot_updated, &ws_selection, &in_shot]
                    (auto *ws, string_view message, uWS::OpCode opCode) {
                lock_guard<mutex> lock(ws_mutex);
                json packet = json::parse(message);

                if(packet["type"].get<string>() == "selection"){
                    ws_selection = packet["selection"].get<int>();

                } else if(packet["type"].get<string>() == "move"){
                    if(ws_selection >= 0 && ws_selection <=num_rod_t){
                        tgt_pos[ws_selection] = clamp(tgt_pos[ws_selection]+packet["pos"].get<double>(), 0.0, 1.0);
                        tgt_rot[ws_selection] = tgt_rot[ws_selection]+packet["rot"].get<double>();
                        if(abs(packet["rot"].get<double>()) > 1) in_shot = true;
                        tgt_pos_updated[ws_selection] = abs(packet["pos"].get<double>()) > 0.001;
                        tgt_rot_updated[ws_selection] = abs(packet["rot"].get<double>()) > 0.001;
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
    vector<double> ball_pos_fast = {0, 0, 0};
    vector<double> ball_pos_slow = {0, 0, 0};
    vector<double> ball_vel = {0, 0, 0};

    double rod_pos[num_axis_t][num_rod_t] = {{0,0,0,0}, {0,0,0,0}};
    double rod_in_vision[num_rod_t] = {false, false, false, false};

    double qtm_time = 0;

    thread qtm_thread([&qtm_mutex, &ball_pos_fast, &ball_pos_slow, &ball_vel, &rod_pos, &qtm_time, &rod_in_vision]() {
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

        
        deque<pair<double, vector<double>>> pos_buffer;
        const int buf_cap = vision_fps;
        // Higher = more noise, less latency
        /* const double gamma_vel = 0.2; */
        const double gamma_vel = 0.05;
        /* const double gamma_pos = 0.05; */
        const double gamma_pos = 0.1;
        for(ever){
            CRTPacket::EPacketType packetType;
            if(rtProtocol.Receive(packetType, true, 0) == CNetwork::ResponseType::success){
                if(packetType != CRTPacket::PacketData) continue;
                lock_guard<mutex> lock(qtm_mutex);
                double t_start = mgr.TimeStampMsec();

                CRTPacket *rtPacket = rtProtocol.GetRTPacket();

                bool ball_seen = false;
                for(int r = 0; r < num_rod_t; ++r)
                    rod_in_vision[r] = false;
                for(int m = 0; m < rtPacket->Get3DNoLabelsMarkerCount(); ++m){
                    
                    // Not read directly since we want doubles not floats
                    vector<float> marker_pos = {0, 0, 0};
                    unsigned int n;
                    rtPacket->Get3DNoLabelsMarker(m, marker_pos[0], marker_pos[1], marker_pos[2], n);
                    for(int i = 0; i < 3; ++i){
                        marker_pos[i] = marker_pos[i] / 10; // convert to mm from cm
                        marker_pos[i] -= cal_offset[i];
                    }
                    // Offset convention, 0 at edge of table
                    marker_pos[0] += play_height / 2;

                    if(marker_pos[2] < 2 && !ball_seen){
                        // ball
                        for(int i = 0; i < 3; ++i) ball_pos_fast[i] = marker_pos[i];
                        pos_buffer.push_front({mgr.TimeStampMsec(), ball_pos_fast});
                        ball_seen = true;
                    } else if(marker_pos[2] > plr_height){
                        // hat
                        auto [side, rod] = closest_rod(marker_pos[1]);
                        if(side != human) continue;
                        double dy = marker_pos[1] - (-rod_coord[rod]);
                        /* cout << marker_pos[1] << ", " << */ 
                        rod_pos[rot][rod] = asin(clamp(dy/hat_height, -1.0, 1.0)) / deg_to_rad;
                        rod_pos[lin][rod] = marker_pos[0] - bumper_width - plr_width / 2;
                        rod_in_vision[rod] = true;
                    }


                }
                if(!ball_seen){
                    if(pos_buffer.size() <= 0) continue;
                    pos_buffer.push_front({mgr.TimeStampMsec(), pos_buffer[0].second});
                }
                if(pos_buffer.size() < buf_cap) continue;
                pos_buffer.pop_back();
                // EWMA
                double vel[3] = {0,0,0}, pos[3] = {0,0,0}, denom_vel = 0, denom_pos = 0;
                for(int i = 0; i < pos_buffer.size()-1; ++i){
                    double scale_vel = exp(-gamma_vel * i);
                    double scale_pos = exp(-gamma_pos * i);
                    denom_vel += scale_vel;
                    denom_pos += scale_pos;
                    for(int j = 0; j < 3; ++j){
                        // Important: the first one seems like it should be better, but it actually isn't
                        // Network delay is unpredictable, so sometimes timing we get it doesn't represent
                        // timing the video was captured. This can give extraneous high spikes in velocity.
                        // It does however rely on having a hardcoded fps which isn't ideal
                        /* num[j] += (pos_buffer[i].second[j] - pos_buffer[i+1].second[j]) * scale */
                        /*     / ((pos_buffer[i].first - pos_buffer[i+1].first)/1000); */
                        vel[j] += (pos_buffer[i].second[j] - pos_buffer[i+1].second[j]) * scale_vel * vision_fps;
                        pos[j] += pos_buffer[i].second[j] * scale_pos;
                    }
                }
                for(int j = 0; j < 3; ++j){
                    ball_vel[j] = vel[j] / denom_vel;
                    ball_pos_slow[j] = pos[j] / denom_pos;
                }
                qtm_time = mgr.TimeStampMsec() - t_start;
            }
        }
    });

    /**************************************************************************
     * Clearpath thread
     **************************************************************************/

    // Do this on the main thread just to make sure that everything is initialized
    mutex mtr_mutex;

    const struct motor_cmd null_cmd = {NAN, NAN, NAN};

    // Could be fancier with some kind of a priority queue, but I think this is fine
    vector<motor_cmd> mtr_cmds[num_axis_t];

    vector<double> mtr_t_last_update[num_axis_t];
    vector<double> mtr_t_last_cmd[num_axis_t];

    vector<motor_cmd> mtr_last_cmd[num_axis_t];

    bool disable_motor_updates = false;

    vector<double> cur_pos[num_axis_t];

    for(int a = 0; a < num_axis_t; ++a){
        for(int r = 0; r < num_rod_t; ++r){
            mtr_cmds[a].push_back(null_cmd);
            mtr_t_last_update[a].push_back(mgr.TimeStampMsec());
            mtr_t_last_cmd[a].push_back(mgr.TimeStampMsec());
            if(a == rot){
                mtr_last_cmd[a].push_back({0, init_vel_lin_cm_s, init_accel_lin_cm_ss});
                cur_pos[a].push_back(lin_range_cm[r]/2);
            }
            else{
                mtr_last_cmd[a].push_back({0, init_vel_rot_deg_s, init_accel_rot_deg_ss});
                cur_pos[a].push_back(0);
                /* move_lin(r, 0); */
            }
        }
    }

    if(!no_motors){
        int init_err = motors_init();
        if(init_err < 0) return init_err;

    }

    // This is the only thread that should ever query motors directly
    thread mtr_thread([no_motors, &mtr_mutex, &mtr_cmds, &mtr_t_last_update, &mtr_t_last_cmd, &mtr_last_cmd, &cur_pos, &disable_motor_updates]() {
        if(no_motors) return;

        const double mtr_refresh_t_ms = 100;

        auto exec_cmds = [&](){
            for(int a = 0; a < num_axis_t; ++a){
                for(int r = 0; r < num_rod_t; ++r){
                    // Awkward to make sure thread safe
                    vector<function<void(void)>> moves;
                    {
                        lock_guard<mutex> lock(mtr_mutex);
                        motor_cmd cmd = mtr_cmds[a][r];
                        motor_cmd last_cmd = mtr_last_cmd[a][r];

                        if((!isnan(cmd.vel) && abs(cmd.vel - last_cmd.vel) > eps)
                                || (!isnan(cmd.accel) && abs(cmd.accel - last_cmd.accel) > eps)){
                            moves.push_back([a, r, cmd](){
                                mtr_set_speed[a](r, cmd.vel, cmd.accel);
                            });
                            if(!isnan(cmd.vel))
                                mtr_last_cmd[a][r].vel = cmd.vel;
                            if(!isnan(cmd.accel))
                                mtr_last_cmd[a][r].accel = cmd.accel;
                            mtr_t_last_cmd[a][r] = mgr.TimeStampMsec();
                        }

                        if(!isnan(cmd.pos) && abs(cmd.pos - last_cmd.pos) > eps){
                            moves.push_back([a, r, cmd](){
                                mtr_move[a](r, cmd.pos);
                            });
                            mtr_last_cmd[a][r].pos = cmd.pos;
                            mtr_t_last_cmd[a][r] = mgr.TimeStampMsec();
                        }
                    }
                    // This is outside the lock's scope to avoid holding mutex too long
                    for(auto fn : moves){

                        try{
                            fn();
                        } catch (sFnd::mnErr& theErr)
                        {
                            printf("Caught mnErr\n");
                            printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
                            cout << endl << endl << endl << endl << endl << endl << endl << endl;
                        }                    }
                }
            }
        };
        for(ever){
            for(int a = 0; a < num_axis_t; ++a){
                for(int r = 0; r < num_rod_t; ++r){
                    exec_cmds();
                    if(mgr.TimeStampMsec() - mtr_t_last_update[a][r] > mtr_refresh_t_ms && !disable_motor_updates){
                        lock_guard<mutex> lock(mtr_mutex);
                        if(a == lin){
                            cur_pos[a][r] = abs(nodes[lin][r].get().Motion.PosnMeasured.Value()
                                    / lin_cm_to_cnts[r]);
                        } else {
                            cur_pos[a][r] = nodes[rot][r].get().Motion.PosnMeasured.Value()
                                    / rot_rad_to_cnts[r] / deg_to_rad;
                        }
                        mtr_t_last_update[a][r] = mgr.TimeStampMsec();
                    } else {
                        this_thread::sleep_for(chrono::microseconds(100));
                    }
                }
            }
        }
    });

    /**************************************************************************
     * Main Event Loop
     **************************************************************************/

    cout << endl;
    cout << fixed << setprecision(2);
    
    state_t state = state_controlled_five_bar;
    /* state_t state = state_test; */
    /* state_t state = state_defense; */
    /* state_t state = state_snake; */
    /* mtr_cmds[lin][three_bar] = { */
    /*     .pos = play_height / 2 - plr_offset(1, three_bar), */
    /*     .vel = 50, */
    /*     .accel = 500, */
    /* }; */

    c3b_t c3b_task = c3b_init;
    c5b_t c5b_task = c5b_init;
    double control_task_timer = mgr.TimeStampMsec();
    // 1 = right, -1 = left, 0 = not shooting
    int control_task_shoot_dir = 0;


    /* for(int i = 0; i < 2; ++i){ */
    /*     double start_t = mgr.TimeStampMsec(); */
    /*     move_rot(i, 90); */
    /*     cout << mgr.TimeStampMsec() - start_t << endl; */
    /* } */

    // So that motor cur_pos is updated by the first loop
    this_thread::sleep_for(chrono::microseconds(100000));
    double time_ms = mgr.TimeStampMsec();

    for(ever){

        if(should_terminate()) break;

        double start_t = mgr.TimeStampMsec();

        stringstream status;

        lock_guard<mutex> qtm_lock(qtm_mutex);
        lock_guard<mutex> mtr_lock(mtr_mutex);

        

        json positionData = {
            {"type", "pos"},
            {"bluepos", {
                0.5, 0.5, 0.5, 0.5
            }},
            {"redpos", {
                cur_pos[lin][three_bar] / lin_range_cm[three_bar],
                cur_pos[lin][five_bar] / lin_range_cm[five_bar],
                cur_pos[lin][two_bar] / lin_range_cm[two_bar],
                cur_pos[lin][goalie] / lin_range_cm[goalie],
            }},
            {"bluerot", {
                0,0,0,0
            }},
            {"redrot", {
                cur_pos[rot][three_bar],
                cur_pos[rot][five_bar],
                cur_pos[rot][two_bar],
                cur_pos[rot][goalie],
            }},
            {"ballpos", {
                ball_pos_fast[0]/(play_height),
                ball_pos_fast[1]/(play_width),
                0,
            }}
        };
        status << fixed << setprecision(3) << setw(10) << showpos;
        status << "Ball position fast: " << ball_pos_fast[0] << ", " << ball_pos_fast[1] << ", " << ball_pos_fast[2] << "; ";
        status << "Ball position slow: " << ball_pos_slow[0] << ", " << ball_pos_slow[1] << ", " << ball_pos_slow[2] << "; ";
        status << "Ball velocity: " << ball_vel[0] << ", " << ball_vel[1] << ", " << ball_vel[2] << endl;
        status << "Marker positions: " << rod_pos[lin][three_bar] << ", " << rod_pos[lin][five_bar] << ", " << rod_pos[lin][two_bar] << ", " << rod_pos[lin][goalie] << "; ";
        status << "Marker rotations: " << rod_pos[rot][three_bar] << ", " << rod_pos[rot][five_bar] << ", " << rod_pos[rot][two_bar] << ", " << rod_pos[rot][goalie] << endl;
        status << "State: " << state << endl;
        status << "Three bar pos: " << cur_pos[lin][three_bar] << ", rot: " << cur_pos[rot][three_bar] << endl;
        status << "Blocked from three bar: " << is_blocked(three_bar, ball_pos_fast[0], rod_pos[lin]) << endl;

        /* static int frame = 0; */
        /* status << "Frame: " << ++frame << endl; */
        /* status << "QTM time: " << qtm_time << endl; */
        string message = positionData.dump();

        {
            lock_guard<mutex> lock(ws_mutex);
            for (auto* client : clients) {
                loop->defer([client, message](){
                    client->send(message, uWS::OpCode::TEXT);
                });
            }
        }

        double dt_ms = mgr.TimeStampMsec() - time_ms;
        time_ms = mgr.TimeStampMsec();


        if(controller){
            for(int r = 0; r < num_rod_t; ++r){

                lock_guard<mutex> lock(ws_mutex);
                if(tgt_pos_updated[r]){
                    mtr_cmds[lin][r] = {
                        .pos = lin_range_cm[r] * tgt_pos[r],
                        .vel = 100,
                        .accel = 100,
                    };
                }
                if(tgt_rot_updated[r]){
                    mtr_cmds[rot][r] = {
                        .pos = tgt_rot[r] / deg_to_rad,
                        .vel = in_shot ? 10000.0 : 500,
                        .accel = in_shot ? 100000.0 : 1000,
                    };
                }
            }
            if(in_shot) in_shot = false;
        // Yes, else switch is just as much as a thing as else if
        } else switch(state){
        case state_defense:
        {
            int front;
            pair<side_t, rod_t> closest = closest_rod(ball_pos_fast[1]);
            if(closest.first == bot){
                state = state_uncontrolled;
                break;
            }
            if(closest.second == three_bar) front = two_bar;
            else if(closest.second == five_bar) front = five_bar;
            else front = three_bar;

            /* ball_vel = {20,-200,0}; */
            double cooldown_time = 25;

            for(int i = 0; i + front < num_rod_t; ++i){
                int rod = i + front;

                // Don't send commands too often
                if(time_ms - mtr_t_last_cmd[lin][rod] < cooldown_time) continue;

                double target_cm = ball_pos_fast[0];

                if(i > 0){
                    // Offset so no double blocking
                    double offset_gap_cm = 4.5;
                    // 1 = bot side, -1 = human
                    double side = ball_pos_fast[0] > play_height / 2 ? 1 : -1;
                    // Override if currently moving and already at one side
                    if(ball_vel[1] > 50 && mtr_cmds[lin][rod].pos >= mtr_cmds[lin][rod-1].pos)
                        side = 1;
                    if(ball_vel[1] < -50 && mtr_cmds[lin][rod].pos <= mtr_cmds[lin][rod-1].pos)
                        side = -1;
                    /* if(front == two_bar && abs(ball_vel[0]) >= 50){ */
                    /*     offset_gap *= 1.5; */
                    /* } */
                    if(i == 1){
                        target_cm += -side * offset_gap_cm;
                    } else if (i == 2){
                        target_cm += side * offset_gap_cm;
                    }
                }

                /* if(abs(ball_vel[0]) > 50){ */
                /*     int sgn = ball_vel[0] > 0 ? 1 : -1; */
                /*     target_cm += sgn*4; */
                /* } */

                // Don't block the wall behind
                if(front == two_bar)
                    target_cm = clamp(target_cm, play_height/3, play_height*2/3);

                int plr = closest_plr(rod, target_cm, cur_pos[lin][rod]);
                
                /* if(rod == goalie && front == two_bar) plr = 1; */

                double plr_offset_cm = plr_offset(plr, rod);
                // How much of a change from previous command this is
                double move_cm = abs(target_cm - plr_offset_cm - mtr_last_cmd[lin][rod].pos);

                // Hysteresis to prevent rapid commands
                /* cout << cur_pos[lin][rod] << endl; */
                if(move_cm > 1){
                    double accel = 1000 * clamp(move_cm / 2, 0.0, 1.0);
                    if(front < two_bar && rod >= two_bar){
                        accel = 50;
                    }
                    mtr_cmds[lin][rod] = {
                        .pos = target_cm - plr_offset_cm,
                        .vel = 100,
                        .accel = accel,
                    };
                }

                double target_deg = -25;
                if(front == two_bar && rod == two_bar){
                    if(target_cm >= play_height * 11.5/18 || target_cm <= play_height * 6.5/18)
                        target_deg = 25;
                }

                if(abs(mtr_last_cmd[rot][rod].pos - target_deg) >= eps){
                    mtr_cmds[rot][rod] = {
                        .pos = target_deg,
                        .vel = NAN,
                        .accel = NAN,
                    };
                }
                
            }
            status << endl;

            if(ball_vel[1] < -100) state = state_shot_defense;

            break;
        }
        case state_shot_defense:
        {
            for(int r = 0; r < num_rod_t; ++r){
                // If ball is already past this rod, do nothing
                if(ball_pos_fast[1] < rod_coord[r]) continue;

                // Predict trajectory
                double target_cm = ball_pos_fast[0];
                // ball_vel[1] is negative so this is positive
                double dt = (rod_coord[r] - ball_pos_fast[1]) / ball_vel[1];
                target_cm += ball_vel[0] * dt;
                /* cout << "dt: " << dt << ", ball_vel[0]: " << ball_vel[0] << ", ball_vel[1]: " << ball_vel[1] << ", target_cm: " << target_cm << endl; */

                int plr = closest_plr(r, target_cm, cur_pos[lin][r]);
                if(r == goalie) plr = 1;

                double plr_offset_cm = plr_offset(plr, r);

                mtr_cmds[lin][r] = {
                    .pos = target_cm - plr_offset_cm,
                    .vel = 150.0,
                    .accel = 1500.0,
                };
            }
            if(ball_vel[1] > -50) state = state_defense;
            break;
        }
        case state_uncontrolled:
        {
            pair<side_t, rod_t> closest = closest_rod(ball_pos_fast[1]);
            if(closest.first != bot){
                state = state_defense;
                break;
            }
            int rod = closest.second;
            double target_cm = ball_pos_fast[0];

            int plr = closest_plr(rod, target_cm, cur_pos[lin][rod]);
            double plr_offset_cm = plr_offset(plr, rod);
            double move_cm = abs(target_cm - plr_offset_cm - mtr_last_cmd[lin][rod].pos);

            if(move_cm >= 0.5){
                mtr_cmds[lin][rod] = {
                    .pos = target_cm - plr_offset_cm,
                    .vel = 50,
                    .accel = 500,
                };
            }
            double target_deg = ball_vel[1] > 0 ? -25 : 25;
            if(abs(ball_vel[1]) < 20) target_deg = mtr_last_cmd[rot][rod].pos;
            if(abs(mtr_last_cmd[rot][rod].pos - target_deg) >= eps){
                mtr_cmds[rot][rod] = {
                    .pos = target_deg,
                    .vel = NAN,
                    .accel = NAN,
                };
            }

            static double t_last_move = mgr.TimeStampMsec();

            if(abs(ball_vel[1]) > 20){
                t_last_move = time_ms;
            }
            if(time_ms - t_last_move > 1000){
                if(rod == three_bar){
                    state = state_controlled_three_bar;
                } else if(rod == five_bar){
                    state = state_controlled_five_bar;
                }
            }

            break;
        }
        // Convenience macros for waiting for rot/lin motions to finish respectively
#define wait_rot if(abs(cur_pos[rot][rod] - mtr_last_cmd[rot][rod].pos) < 1.0 && time_ms - control_task_timer > 100)
#define wait_lin if(abs(cur_pos[lin][rod] - mtr_last_cmd[lin][rod].pos) < 0.1 && time_ms - control_task_timer > 100)
#define wait_time(T) if(time_ms - control_task_timer >= (T))
        case state_controlled_three_bar:
        {
            if(no_motors) break;


            pair<side_t, rod_t> closest = closest_rod(ball_pos_fast[1]);
            /* if(closest.first != bot){ */
            /*     state = state_defense; */
            /*     break; */
            /* } */
            rod_t rod = closest.second;

            // Small extra offset for a bit of tolerance without hitting the edge
            const double setup_offset = bumper_width + plr_gap[rod] + plr_width/2 + ball_rad + foot_width/2 + 1;
            double goal_cm = play_height - setup_offset;
            if(ball_pos_slow[0] < play_height / 2){
                /* goal_cm = play_height / 2 + 2; */
                goal_cm = setup_offset;
            }

            int plr = 0;
            int dir = 1;
            bool ball_in_pos = abs(ball_pos_slow[0] - goal_cm) <= 1;
            bool ball_in_rot = abs(ball_pos_slow[1] - rod_coord[rod]) <= 1;
            // If already close we want to prepare the shot
            if(control_task_shoot_dir != 0){
                plr = 1, dir = control_task_shoot_dir;
            } else if(ball_in_pos){
                plr = 1, dir = ball_pos_slow[0] >= play_height / 2 ? -1 : 1;
            } else if(ball_pos_slow[0] < setup_offset){
                plr = 0, dir = 1;
            } else if(ball_pos_slow[0] >= setup_offset && ball_pos_slow[0] <= play_height - setup_offset){
                plr = 1, dir = ball_pos_slow[0] >= play_height / 2 ? 1 : -1;
            } else {
                plr = 2, dir = -1;
            }

            double plr_offset_cm = plr_offset(plr, rod);

            // Location of the ball in degrees of rotation of the player
            // Using \sin\theta \approx \theta
            // h\sin\theta = dx \implies \theta \approx dx/h
            double ball_deg = (rod_coord[rod] - ball_pos_fast[1]) / plr_height / deg_to_rad;

            // Whether to move clockwise or not
            // This is only relevant for a few of the modes, but it's shared so it's defined here
            // Only move clockwise if both in front of the ball and we're on the inconvenient side
            bool cw = abs(cur_pos[lin][rod] + plr_offset_cm - ball_pos_slow[0]) < ball_rad + plr_width/2
                && cur_pos[rot][rod] * deg_to_rad * plr_height <= rod_coord[rod] - ball_pos_slow[1];

            // Second state machine specifically for micro control tasks
            // Could combine this but these are much lower level tasks than high level states
            switch(c3b_task){
            case c3b_init:
                mtr_cmds[rot][rod] = {
                    .pos = cw ? -360.0 + 90 : 90,
                    .vel = 500,
                    .accel = 5000,
                };
                // Clear previously issued command
                mtr_cmds[lin][rod] = {
                    .pos = NAN,
                    .vel = NAN,
                    .accel = NAN,
                };
                c3b_task = c3b_raise;
                control_task_timer = time_ms;
                cout << "raise" << endl;
                break;
            case c3b_raise:
                wait_rot{
                    double offset = ball_in_pos ? 0.35 : 0.5;
                    double target_cm = ball_pos_slow[0] - dir*(ball_rad + foot_width/2 + offset);
                    // If in position but not rotated, go directly to the ball to adjust it
                    if(ball_in_pos && !ball_in_rot){
                        /* double sign = ball_pos[0] > goal_cm ? 1 : -1; */
                        /* target_cm = ball_pos[0] + sign * dir * ball_rad / 2; */
                        target_cm = ball_pos_slow[0];
                        c3b_task = c3b_adjust_move;
                        control_task_timer = time_ms;
                    } else {
                        c3b_task = c3b_move_lateral;
                        control_task_timer = time_ms;
                    }

                    mtr_cmds[lin][rod] = {
                        .pos = clamp(target_cm - plr_offset_cm, 0.0, lin_range_cm[rod]),
                        /* .vel = ball_in_pos ? 20.0 : 50, */
                        /* .accel = ball_in_pos ? 200.0 : 500, */
                        .vel = 50,
                        .accel = 500,
                    };
                    cout << "move_lateral" << endl;
                }
                break;
            case c3b_move_lateral:
                wait_lin{
                    mtr_cmds[rot][rod] = {
                        .pos = ball_deg + 6,
                        .vel = 50,
                        .accel = 1000,
                    };
                    c3b_task = c3b_lower;
                    control_task_timer = time_ms;
                    cout << "lower" << endl;
                    cout << ball_deg << endl;
                }
                break;
            case c3b_adjust_move:
                wait_lin{
                    double pos = ball_pos_slow[1] < rod_coord[rod] ? 55 : 360 - 55;
                    mtr_cmds[rot][rod] = {
                        .pos = pos,
                        .vel = 150,
                        .accel = 1000,
                    };
                    c3b_task = c3b_adjust_down;
                    control_task_timer = time_ms;
                    cout << "adjust down" << endl;
                }

                break;
            case c3b_adjust_down:
                wait_rot{
                    double last_deg = mtr_last_cmd[rot][rod].pos;
                    // Check for first, fast section of move and finish slowly
                    if(last_deg >= 45 && last_deg <= 360 - 45){
                        double pos = ball_pos_slow[1] < rod_coord[rod] ? 30 : 360 - 30;
                        mtr_cmds[rot][rod] = {
                            .pos = pos,
                            .vel = 10,
                            .accel = 200,
                        };
                        break;
                    }
                    mtr_cmds[rot][rod] = {
                        .pos = cw ? -360.0 + 90 : 90,
                        .vel = 500,
                        .accel = 5000,
                    };
                    cout << "raise" << endl;
                    c3b_task = c3b_raise;
                    control_task_timer = time_ms;
                }
                break;
            case c3b_lower:
                wait_rot{
                    if(ball_in_pos){
                        double target_cm = ball_pos_slow[0] - dir*(ball_rad + foot_width/2);
                        if(abs(cur_pos[lin][0] + plr_offset_cm - target_cm) > 1){
                            mtr_cmds[rot][rod] = {
                                .pos = 90,
                                .vel = 500,
                                .accel = 5000,
                            };
                            cout << "Failed lower, raising..." << endl;
                            c3b_task = c3b_raise;
                            control_task_timer = time_ms;
                        } else {
                            cout << "shoot" << endl;
                            c3b_task = c3b_shoot_wait;
                            control_task_timer = time_ms;
                            control_task_shoot_dir = dir;
                            // Random 0-2s, plus 500ms minimum delay
                            cout << "Time: " << time_ms << endl;
                            cout << mtr_last_cmd[rot][three_bar].pos << endl;
                            control_task_timer = time_ms + 500 + (rand() % 2000);
                        }
                    } else {
                        cout << "push" << endl;
                        mtr_cmds[lin][rod] = {
                            .pos = goal_cm - plr_offset_cm - dir * (ball_rad + foot_width/2),
                            .vel = 2.5,
                            .accel = 3,
                        };
                        c3b_task = c3b_push;
                        control_task_timer = time_ms;
                    }
                }
                break;
            case c3b_push:
            {
                // Don't rapid fire commands
                if(time_ms - mtr_t_last_cmd[lin][rod] > 50){
                    double target_deg = 1.4 * ball_deg;
                    // Empirical offset from player shape
                    target_deg += 4;
                    mtr_cmds[rot][rod] = {
                        .pos = target_deg,
                        .vel = 500,
                        .accel = 5000,
                    };
                }
                if(abs(cur_pos[lin][rod] - mtr_last_cmd[lin][rod].pos) < 0.1){
                    cout << "raise" << endl;
                    mtr_cmds[rot][rod] = {
                        .pos = 90,
                        .vel = 500,
                        .accel = 5000,
                    };
                    c3b_task = c3b_raise;
                    control_task_timer = time_ms;
                }
                break;
            }
            case c3b_shoot_wait:
            {
                bool in_vision = true;
                for(int r = two_bar; r < num_rod_t; ++r){
                    if(!rod_in_vision[r]) in_vision = false;
                }
                // Expected time before straight shot when open
                /* double r = (rand() % 10'000) / 10'000.0; */
                double r_straight = (rand() % 10'000) / 10'000.0;
                double et_straight = 500;

                // Use different r than previously so they're not correlated
                double r_middle = (rand() % 10'000) / 10'000.0;
                /* double r_middle = 1; */
                double et_middle = 1'000;
                if(r_straight > 1 - dt_ms / et_straight && in_vision && !is_blocked(rod, ball_pos_slow[0], rod_pos[lin], 2)){
                    mtr_cmds[rot][rod] = {
                        .pos = 80,
                        .vel = 20'000,
                        .accel = 200'000,
                    };
                    cout << "shoot straight" << endl;
                    cout << "Time: " << time_ms << endl;
                    cout << dt_ms << ", " << r_straight << endl;
                    c3b_task = c3b_shoot_straight;
                    control_task_timer = time_ms;
                    disable_motor_updates = true;
                } else if(r_middle > 1 - dt_ms / et_middle && in_vision && !is_blocked(rod, play_height/2, rod_pos[lin], 1)){
                    mtr_cmds[lin][rod] = {
                        .pos = play_height/2 - plr_offset_cm,
                        .vel = 200,
                        .accel = 2000,
                    };
                    cout << "shoot middle" << endl;
                    c3b_task = c3b_shoot_middle;
                    control_task_timer = time_ms;
                    disable_motor_updates = true;
                }
                wait_time(5000){
                /* wait_time(0){ */
                    double target_cm = control_task_shoot_dir == -1 ? setup_offset : play_height - setup_offset;
                    /* mtr_cmds[lin][rod] = { */
                    /*     .pos = target_cm - plr_offset_cm, */
                    /*     .vel = 300, */
                    /*     .accel = 3000, */
                    /* }; */
                    mtr_cmds[lin][rod] = {
                        .pos = target_cm - plr_offset_cm,
                        .vel = 100,
                        .accel = 3000,
                    };
                    cout << "shoot end" << endl;
                    c3b_task = c3b_shoot_end_slow;
                    control_task_timer = time_ms;
                    disable_motor_updates = true;
                }
                break;
            }
            case c3b_shoot_straight:
                wait_time(20){
                    mtr_cmds[lin][rod] = {
                        .pos = ball_pos_fast[0] - plr_offset_cm,
                        .vel = 300,
                        .accel = 3000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_shoot_straight_2;
                }
                break;
            case c3b_shoot_straight_2:
                wait_time(25){
                    // Abort!
                    if(is_blocked(rod, ball_pos_fast[0], rod_pos[lin], 0)){
                        c3b_task = c3b_raise;
                        cout << "straight abort" << endl;
                    } else {
                        mtr_cmds[rot][rod] = {
                            .pos = -90,
                            .vel = 20000,
                            .accel = 200000,
                        };
                        control_task_timer = time_ms;
                        cout << "idle" << endl;
                        c3b_task = c3b_idle;
                    }
                    disable_motor_updates = false;
                }
                break;
            case c3b_shoot_middle:
                wait_time(30){
                    mtr_cmds[rot][rod] = {
                        .pos = 55,
                        .vel = 20000,
                        .accel = 200000,
                    };
                    mtr_cmds[lin][rod] = {
                        .pos = play_height/2 - plr_offset_cm,
                        .vel = 200,
                        .accel = 3000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_shoot_middle_2;
                }
                break;
            case c3b_shoot_middle_2:
                wait_time(50){
                    mtr_cmds[rot][rod] = {
                        .pos = -90,
                        .vel = 20000,
                        .accel = 200000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_idle;
                    disable_motor_updates = false;
                }
                break;
            case c3b_shoot_end:
                wait_time(45){
                    mtr_cmds[rot][rod] = {
                        .pos = 90,
                        .vel = 22000,
                        .accel = 220000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_shoot_end_2;
                }
                break;
            case c3b_shoot_end_2:
                wait_time(10){
                    double target_cm = control_task_shoot_dir == -1 ? setup_offset-3 : play_height - setup_offset;
                    mtr_cmds[lin][rod] = {
                        .pos = target_cm - plr_offset_cm,
                        .vel = 400,
                        .accel = 4000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_shoot_end_3;
                }
                break;
            case c3b_shoot_end_3:
                wait_time(50){
                    mtr_cmds[rot][rod] = {
                        .pos = -90,
                        .vel = 22000,
                        .accel = 220000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_idle;
                    disable_motor_updates = false;
                }
                break;
            case c3b_shoot_end_slow:
                wait_time(45){
                    mtr_cmds[rot][rod] = {
                        .pos = 90,
                        .vel = 22000,
                        .accel = 220000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_shoot_end_slow_2;
                }
                break;
            case c3b_shoot_end_slow_2:
                wait_time(10){
                    double target_cm = control_task_shoot_dir == -1 ? setup_offset-2 : play_height - setup_offset;
                    mtr_cmds[lin][rod] = {
                        .pos = target_cm - plr_offset_cm,
                        .vel = 300,
                        .accel = 3000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_shoot_end_slow_3;
                }
                break;
            case c3b_shoot_end_slow_3:
                wait_time(115){
                    mtr_cmds[rot][rod] = {
                        .pos = -90,
                        .vel = 22000,
                        .accel = 220000,
                    };
                    control_task_timer = time_ms;
                    c3b_task = c3b_idle;
                    disable_motor_updates = false;
                }
                break;
            case c3b_idle:
            default:
                break;
            }
            break;
        }
        case state_controlled_five_bar:
        {
            pair<side_t, rod_t> closest = closest_rod(ball_pos_fast[1]);
            side_t side = closest.first;
            rod_t rod = closest.second;
            if(rod != five_bar || side != bot){
                /* state = state_unknown; */
            }

            int plr = closest_plr(rod, ball_pos_slow[0], cur_pos[lin][rod]);
            double plr_offset_cm = plr_offset(plr, rod);

            static int tic_tac_dir = 1;
            int ball_dir = ball_vel[0] > 0 ? 1 : -1;

            const double hit_thresh = 4;

            const double tic_tac_vel = 20;
            const double tic_tac_accel = 200;
            
            const double left_cm = 2;
            const double right_cm = 12;


            double hit_cm = ball_dir == 1 ? right_cm - hit_thresh/2 : left_cm + hit_thresh/2;
            double dt = abs(ball_pos_slow[0] - hit_cm) / ball_vel[0];
            /* double ball_cm = ball_pos_slow[1] + ball_vel[1]*dt; */
            double ball_cm = ball_pos_slow[1];

            /* double ball_deg = (rod_coord[rod] - ball_pos_fast[1]) / plr_height / deg_to_rad - 3; */
            double ball_deg = atan((rod_coord[rod] - ball_cm) / (plr_height+plr_levitate-ball_rad/2)) / deg_to_rad-3;

            switch(c5b_task){
            case c5b_init:
                mtr_cmds[lin][rod] = {
                    .pos = right_cm,
                    .vel = tic_tac_vel,
                    .accel = tic_tac_accel,
                };
                mtr_cmds[rot][rod] = {
                    .pos = ball_deg,
                    .vel = 4'000,
                    .accel = 40'000,
                };
                tic_tac_dir = 1;
                c5b_task = c5b_tic_tac;
                /* c5b_task = c5b_threaten; */
                break;
            case c5b_tic_tac:
            {
                /* double tol = 2; */

                if(time_ms - mtr_t_last_cmd[rot][rod] > 20){
                    mtr_cmds[rot][rod] = {
                        /* .pos = ball_deg + (ball_deg > 6 ? 3 : 0) + (ball_deg < -6 ? -3 : 0), */
                        /* .pos = ball_deg*1.2 - ball_vel[1] * 1, */
                        .pos = ball_deg * 1.3,
                        /* .pos = ball_deg + pow(abs(ball_deg), 0.5)*(ball_deg>0?1:-1), */
                        .vel = 2'000,
                        .accel = 20'000,
                    };
                }
                if(tic_tac_dir == 1 && ball_dir == 1 && ball_pos_fast[0] > right_cm + plr_offset(1, rod) - (ball_rad+foot_width/2) - hit_thresh){
                    tic_tac_dir = -1;
                    mtr_cmds[lin][rod] = {
                        .pos = left_cm,
                        .vel = tic_tac_vel,
                        .accel = tic_tac_accel,
                    };
                    /* cout << "left" << endl; */
                } else if(tic_tac_dir == -1 && ball_dir == -1 && ball_pos_fast[0] < left_cm + plr_offset(0, rod) + (ball_rad+foot_width/2) + hit_thresh){
                    tic_tac_dir = 1;
                    mtr_cmds[lin][rod] = {
                        .pos = right_cm,
                        .vel = tic_tac_vel,
                        .accel = tic_tac_accel,
                    };
                    /* cout << "right" << endl; */
                }
                
                break;
            }
            case c5b_threaten:
            {

            }
            case c5b_idle:
                break;
            }
            break;
        }
        case state_snake: {
            static double t_up = time_ms+2000;
            static double t_shoot = time_ms + 10000;
            static bool up = false;
            static int dir = 1;

            double plr_offset_cm = plr_offset(1, three_bar);

            if(time_ms > t_shoot && cur_pos[lin][three_bar] - plr_offset_cm < 0.5){
                mtr_cmds[rot][three_bar] = {
                    .pos = -450,
                    .vel = 20000,
                    .accel = 200000,
                };
                mtr_cmds[lin][three_bar] = {
                    .pos = play_height / 2 - plr_offset_cm - 2,
                    .vel = 100,
                    .accel = 1000,
                };
                state = state_unknown;
            } else if(time_ms > t_up && time_ms < t_up + 200){
                mtr_cmds[rot][three_bar] = {
                    .pos = -55,
                    .vel = 3600,
                    .accel = 36000,
                };
                up = true;
            } else {
                if(up){
                    t_up += 2000;
                    up = false;
                }
                mtr_cmds[rot][three_bar] = {
                    .pos = -49.5,
                    .vel = 3600,
                    .accel = 36000,
                };
            }
            if(abs(cur_pos[lin][three_bar] - mtr_last_cmd[lin][three_bar].pos) < 0.1){
                dir *= -1;
                mtr_cmds[lin][three_bar] = {
                    .pos = play_height / 2 - plr_offset_cm + dir * 1,
                    .vel = 25,
                    .accel = 250,
                };
            }
            break;
        }
        case state_test:
            // For misc testing
            mtr_cmds[lin][three_bar] = {
                /* .pos = ball_pos[0] - plr_offset(1, three_bar), */
                .pos = play_height/2 - plr_offset(1, three_bar),
                .vel = 25,
                .accel = 250,
            };
            break;
        case state_unknown:
        default:
            break;
        }

        /* status << cur_pos[lin][three_bar] << ", " << cur_pos[lin][goalie] << endl; */
        print_status(status.str(), true);
        /* cout << ball_pos_lcl[0] << ", " << ball_pos_lcl[1] << "; v: " << ball_vel_lcl[0] << ", " << ball_vel_lcl[1] << endl; */
        /* cout << time_ms - start_t << endl; */

        this_thread::sleep_for(chrono::microseconds(500));
    }


    cout << "Got terminate command, quitting..." << endl;
    close_all();
    terminate();

    return 0;
}

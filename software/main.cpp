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
 * Defines/const
 ******************************************************************************/

#define ever ;;
#define eps 1e-5

const int init_vel_lin_cm_s = 100;
const int init_accel_lin_cm_ss = 1000;

const int init_vel_rot_deg_s = 5000;
const int init_accel_rot_deg_ss = 50000;

const int homing_timeout_ms = 10000;

typedef enum state_t {
    state_defense,
    state_shot_defense,
    state_shot_offense,
    state_goalie_possess,
    state_two_bar_possess,
    state_five_bar_possess,
    state_three_bar_possess,
    state_unknown,
    num_state_t
} state_t;

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
        move_lin(i, lin_range_cm[i]/2);
    }
    for(int i = 0; i < nodes[rot].size(); ++i){
        nodes[rot][i].get().AccUnit(sFnd::INode::COUNTS_PER_SEC2);
        nodes[rot][i].get().VelUnit(sFnd::INode::COUNTS_PER_SEC);
        if(i != goalie)
            nodes[rot][i].get().Info.Ex.Parameter(98,1);
        nodes[rot][i].get().Motion.PosnMeasured.AutoRefresh(true);
        /* set_speed_rot((rod_t)i, 10000, 100000); */
        set_speed_rot((rod_t)i, init_vel_rot_deg_s, init_accel_rot_deg_ss);
        move_rot(i, 0);
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
            .message = [&ws_mutex, &tgt_pos, &tgt_pos_updated, &tgt_rot, &tgt_rot_updated, &ws_selection]
                    (auto *ws, string_view message, uWS::OpCode opCode) {
                lock_guard<mutex> lock(ws_mutex);
                json packet = json::parse(message);

                if(packet["type"].get<string>() == "selection"){
                    ws_selection = packet["selection"].get<int>();

                } else if(packet["type"].get<string>() == "move"){
                    if(ws_selection >= 0 && ws_selection <=num_rod_t){
                        tgt_pos[ws_selection] = clamp(tgt_pos[ws_selection]+packet["pos"].get<double>(), 0.0, 1.0);
                        tgt_rot[ws_selection] = tgt_rot[ws_selection]+packet["rot"].get<double>();
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
    vector<double> ball_pos = {0, 0, 0};
    vector<double> ball_vel = {0, 0, 0};

    thread qtm_thread([&qtm_mutex, &ball_pos, &ball_vel]() {
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
        const double gamma = 0.5; // Higher = more noise, less latency
        for(ever){
            CRTPacket::EPacketType packetType;
            if(rtProtocol.Receive(packetType, true, 0) == CNetwork::ResponseType::success){
                if(packetType != CRTPacket::PacketData) continue;
                lock_guard<mutex> lock(qtm_mutex);

                CRTPacket *rtPacket = rtProtocol.GetRTPacket();

                if(rtPacket->Get3DNoLabelsMarkerCount() < 1){
                    // Assume ball stays in place
                    if(pos_buffer.size() <= 0) continue;
                    pos_buffer.push_front({mgr.TimeStampMsec(), pos_buffer[0].second});
                } else {

                    // Not read directly since we want doubles not floats
                    vector<float> ball_pos_tmp = {0, 0, 0};
                    unsigned int n;
                    rtPacket->Get3DNoLabelsMarker(0, ball_pos_tmp[0], ball_pos_tmp[1], ball_pos_tmp[2], n);
                    for(int i = 0; i < 3; ++i){
                        ball_pos[i] = ball_pos_tmp[i] / 10; // convert to mm from cm
                        ball_pos[i] -= cal_offset[i];
                    }

                    pos_buffer.push_front({mgr.TimeStampMsec(), ball_pos});
                }
                if(pos_buffer.size() < buf_cap) continue;
                pos_buffer.pop_back();
                // EWMA
                double num[3] = {0,0,0}, denom = 0;
                for(int i = 0; i < pos_buffer.size()-1; ++i){
                    double scale = exp(-gamma * i);
                    denom += scale;
                    for(int j = 0; j < 3; ++j){
                        // Important: the first one seems like it should be better, but it actually isn't
                        // Network delay is unpredictable, so sometimes timing we get it doesn't represent
                        // timing the video was captured. This can give extraneous high spikes in velocity
                        /* num[j] += (pos_buffer[i].second[j] - pos_buffer[i+1].second[j]) * scale */
                        /*     / ((pos_buffer[i].first - pos_buffer[i+1].first)/1000); */
                        num[j] += (pos_buffer[i].second[j] - pos_buffer[i+1].second[j]) * scale * vision_fps;
                    }
                }
                for(int j = 0; j < 3; ++j){
                    ball_vel[j] = num[j] / denom;
                }
                // Sometimes useful for debugging by printing out buffer
                /* if(abs(ball_vel[1]) > 75){ */
                /*     for(int i = 0; i < pos_buffer.size()-1; ++i){ */
                /*         cout << pos_buffer[i].first << "; "; */
                /*         for(int j = 0; j < 3; ++j){ */
                /*             cout << pos_buffer[i].second[j] << ", "; */
                /*         } */
                /*         cout << endl; */
                /*     } */
                /*     cout << endl; */
                /*     for(int j = 0; j < 3; ++j){ */
                /*         cout << num[j] << ", " << denom << ", " << ball_vel[j] << endl; */
                /*     } */
                /*     terminate(); */
                /* } */

            }
        }
    });

    /**************************************************************************
     * Clearpath thread
     **************************************************************************/

    // Do this on the main thread just to make sure that everything is initialized
    int init_err = motors_init();
    if(init_err < 0) return init_err;

    mutex mtr_mutex;

    struct motor_cmd {
        // NAN for unchanged
        double pos;
        double vel;
        double accel;
    };
    const struct motor_cmd null_cmd = {NAN, NAN, NAN};

    // Could be fancier with some kind of a priority queue, but I think this is fine
    vector<motor_cmd> mtr_cmds[num_axis_t];

    vector<double> mtr_t_last_update[num_axis_t];
    vector<double> mtr_t_last_cmd[num_axis_t];

    vector<motor_cmd> mtr_last_cmd[num_axis_t];


    vector<double> cur_pos[num_axis_t];

    for(int a = 0; a < num_axis_t; ++a){
        for(int r = 0; r < num_rod_t; ++r){
            mtr_cmds[a].push_back(null_cmd);
            mtr_t_last_update[a].push_back(mgr.TimeStampMsec());
            mtr_t_last_cmd[a].push_back(mgr.TimeStampMsec());
            if(a == rot){
                mtr_last_cmd[a].push_back({lin_range_cm[r]/2, init_vel_lin_cm_s, init_accel_lin_cm_ss});
                cur_pos[a].push_back(lin_range_cm[r]/2);
            }
            else{
                mtr_last_cmd[a].push_back({0, init_vel_rot_deg_s, init_accel_rot_deg_ss});
                cur_pos[a].push_back(0);
            }
        }
    }

    // This is the only thread that should ever query motors directly
    thread mtr_thread([&mtr_mutex, &mtr_cmds, &mtr_t_last_update, &mtr_t_last_cmd, &mtr_last_cmd, &cur_pos]() {

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
                            mtr_last_cmd[a][r].vel = cmd.vel;
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
                        fn();
                    }
                }
            }
        };
        for(ever){
            for(int a = 0; a < num_axis_t; ++a){
                for(int r = 0; r < num_rod_t; ++r){
                    exec_cmds();
                    if(mgr.TimeStampMsec() - mtr_t_last_update[a][r] > mtr_refresh_t_ms){
                        lock_guard<mutex> lock(mtr_mutex);
                        if(a == lin){
                            cur_pos[a][r] = abs(nodes[lin][r].get().Motion.PosnMeasured.Value()
                                    / lin_cm_to_cnts[r]);
                        } else {
                            cur_pos[a][r] = nodes[rot][r].get().Motion.PosnMeasured.Value()
                                    / rot_rad_to_cnts[r];
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

    try{

    cout << endl;
    cout << fixed << setprecision(2);
    
    state_t state = state_defense;

    for(int i = 0; i < 3; ++i){
        double start_t = mgr.TimeStampMsec();
        move_rot(i, 90);
        cout << mgr.TimeStampMsec() - start_t << endl;
    }

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
                ball_pos[0]/(play_height)+0.5,
                ball_pos[1]/(play_width)+0.5,
                0,
            }}
        };
        status << fixed << setprecision(3) << setw(10) << showpos;
        status << "Ball position: " << ball_pos[0] << ", " << ball_pos[1] << ", " << ball_pos[2] << "; ";
        status << "Ball velocity: " << ball_vel[0] << ", " << ball_vel[1] << ", " << ball_vel[2] << endl;
        string message = positionData.dump();

        {
            lock_guard<mutex> lock(ws_mutex);
            for (auto* client : clients) {
                loop->defer([client, message](){
                    client->send(message, uWS::OpCode::TEXT);
                });
            }
        }

        if(controller){
            for(int i = 0; i < num_rod_t; ++i){
                double ws_pos, ws_pos_updated, ws_rot, ws_rot_updated;
                {
                    lock_guard<mutex> lock(ws_mutex);
                    ws_pos = tgt_pos[i];
                    ws_pos_updated = tgt_pos_updated[i];
                    ws_rot = tgt_rot[i];
                    ws_rot_updated = tgt_rot_updated[i];
                }
                if(ws_pos_updated){
                    move_lin(i, lin_range_cm[i] * ws_pos);
                    ws_pos_updated = false;
                }
                if(ws_rot_updated){
                    move_rot(i, ws_rot / deg_to_rad);
                    ws_rot_updated = false;
                }
            }
        // Yes, else switch is just as much as a thing as else if
        } else switch(state){
            case state_defense:
            {
                int front;
                /* if(ball_pos[1] > 2*plr_dist) front = three_bar; */
                /* else if(ball_pos[1] > 0) front = five_bar; */
                /* else if(ball_pos[1] > -2*plr_dist) front = two_bar; */
                front = goalie;

                /* ball_vel = {20,-200,0}; */
                bool shot_firing = ball_vel[1] < -100;
                double cooldown_time = shot_firing ? 1 : 25;

                for(int i = 0; i + front < num_rod_t; ++i){
                    int rod = i + front;

                    // Don't send commands too often
                    if(mgr.TimeStampMsec() - mtr_t_last_cmd[lin][rod] < cooldown_time) continue;

                    double target_cm = ball_pos[0] + play_height / 2;
                    if(shot_firing && rod == goalie){
                        double rod_y = -rod_gap * 3.5;
                        double dt = (rod_y - ball_pos[1]) / ball_vel[1];
                        target_cm += ball_vel[0] * dt;
                    }

                    // Offset so no double blocking
                    if(i == 1){
                        target_cm += (ball_pos[0] > 0 ? -1 : 1) * plr_width;
                    } else if (i == 2){
                        target_cm += (ball_pos[0] > 0 ? 1 : -1) * plr_width;
                    }

                    int plr = floor(num_plrs[rod] * target_cm / play_height);

                    // Override since switching is awkward
                    if(rod == two_bar) plr = 0;
                    
                    plr = clamp(plr, 0, num_plrs[rod]);

                    double plr_offset_cm = bumper_width + plr_width/2 + plr*plr_gap[rod];
                    double move_cm = target_cm - plr_offset_cm;

                    // Hysteresis to prevent rapid commands
                    /* cout << cur_pos[lin][rod] << endl; */
                    if((shot_firing || abs(move_cm - cur_pos[lin][rod]) > 0.5) && !no_motors){
                        mtr_cmds[lin][rod] = {
                            .pos = target_cm - plr_offset_cm,
                            .vel = shot_firing ? 150.0 : 100,
                            .accel = shot_firing ? 1500.0 : 1000,
                        };
                    }
                    
                }


                break;
            }
            case state_shot_defense:
            {

            }
            case state_unknown:
            default:
                break;
        }

        /* print_status(status.str(), false); */
        /* cout << ball_pos_lcl[0] << ", " << ball_pos_lcl[1] << "; v: " << ball_vel_lcl[0] << ", " << ball_vel_lcl[1] << endl; */
        /* cout << mgr.TimeStampMsec() - start_t << endl; */

        this_thread::sleep_for(chrono::microseconds(500));
    }

    } catch (sFnd::mnErr& theErr)
	{
		printf("Failed to disable Nodes n\n");
		//This statement will print the address of the error, the error code (defined by the mnErr class), 
		//as well as the corresponding error message.
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);

		return 0;  //This terminates the main program
	}

    cout << "Got terminate command, quitting..." << endl;
    close_all();
    terminate();

    return 0;
}

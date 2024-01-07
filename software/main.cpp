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
/* using namespace cv; */
using namespace sFnd;
using json = nlohmann::json;

/******************************************************************************
 * Defines
 ******************************************************************************/

#define ever ;;

#define ACC_LIM_RPM_PER_SEC    10000
#define VEL_LIM_RPM            1000

#define HOMING_TIMEOUT 5000

typedef enum state_t {
    state_defense,
    state_unknown,
    num_state_t
} state_t;

/******************************************************************************
 * Global Variables
 ******************************************************************************/
// Might not want these to be global later, whatever for now
SysManager mgr;
vector<reference_wrapper<INode>> lin_nodes;
vector<reference_wrapper<INode>> rot_nodes;

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
    IPort&port = mgr.Ports(0);
    for(int i = 0; i < port.NodeCount(); ++i){
        port.Nodes(i).EnableReq(false);
    }
    mgr.PortsClose();
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

    struct PerSocketData {
        /* User data */
    };
    vector<uWS::WebSocket<false, true, PerSocketData>*> clients;
    mutex clientsMutex;

    // Webapp state
    double ws_pos[num_rod_t] = {0.5, 0.5, 0.5, 0.5};
    double ws_rot[num_rod_t] = {0, 0, 0, 0};
    bool ws_pos_updated[num_rod_t] = {0};
    bool ws_rot_updated[num_rod_t] = {0};
    int ws_selection = -1;

    struct uWS::Loop *loop;
    // Thread for web socket handling
    thread uwsThread([&]() {
        loop = uWS::Loop::get();
        // C++20 acting funky and makes me specificy every field
        uWS::App app;
        app.ws<PerSocketData>("/position", {
            .compression = uWS::DISABLED,
            .maxPayloadLength = 16 * 1024 * 1024,
            .idleTimeout = 16,
            .maxBackpressure = 1 * 1024 * 1024,
            .closeOnBackpressureLimit = false,
            .resetIdleTimeoutOnSend = false,
            .sendPingsAutomatically = true,
            .maxLifetime = 0,

            .upgrade = nullptr,
            .open = [&clients, &clientsMutex](auto *ws) {
                /* cout << "Connection! " << 9001 << endl; */
                lock_guard<mutex> lock(clientsMutex);
                clients.push_back(ws);
                /* nlohmann::json params = {}; */
                /* for(int i = 0; i < num_rod_t; ++i){ */
                /*     params["spacing-" + rod_names[i]] = plr_gap[i]; */
                /* } */

            },
            // Handles incoming packets
            .message = [&clientsMutex, &ws_pos, &ws_pos_updated, &ws_rot, &ws_rot_updated, &ws_selection]
                    (auto *ws, string_view message, uWS::OpCode opCode) {
                lock_guard<mutex> lock(clientsMutex);
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
            .close = [&clients, &clientsMutex](auto* ws, int /*code*/, string_view /*message*/) {
                /* cout << "Client disconnected" << endl; */
                lock_guard<mutex> lock(clientsMutex);
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

    /**************************************************************************
     * Clearpath Init
     **************************************************************************/
    vector<string> comHubPorts;

    // Identify hubs
    SysManager::FindComHubPorts(comHubPorts);
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

    IPort &port = mgr.Ports(0);
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
        lin_nodes[i].get().AccUnit(INode::COUNTS_PER_SEC2);
        lin_nodes[i].get().VelUnit(INode::COUNTS_PER_SEC);
        lin_nodes[i].get().Info.Ex.Parameter(98,1);
        /* set_speed_lin((rod_t)i, 100, 1000); */
        set_speed_lin((rod_t)i, 50, 500);
        /* cout << "Set linear speed for " << rod_names[i] << endl; */
        lin_nodes[i].get().Motion.PosnMeasured.AutoRefresh(true);
        move_lin(i, lin_range_cm[i]/2);
    }
    for(int i = 0; i < rot_nodes.size(); ++i){
        rot_nodes[i].get().AccUnit(INode::COUNTS_PER_SEC2);
        rot_nodes[i].get().VelUnit(INode::COUNTS_PER_SEC);
        if(i != goalie)
            rot_nodes[i].get().Info.Ex.Parameter(98,1);
        rot_nodes[i].get().Motion.PosnMeasured.AutoRefresh(true);
        /* set_speed_rot((rod_t)i, 10000, 100000); */
        set_speed_rot((rod_t)i, 5000, 50000);
        move_rot(i, 0);
    }

    /**************************************************************************
     * Main Event Loop
     **************************************************************************/

    try{

    CRTPacket::EPacketType packetType;
    cout << endl;
    vector<float> ball_pos = {0, 0, 0};
    

    for(ever){
        stringstream status;

        while(rtProtocol.Receive(packetType, true, 0) == CNetwork::ResponseType::success){
            if(packetType != CRTPacket::PacketData) continue;

            CRTPacket *rtPacket = rtProtocol.GetRTPacket();

            /* printf("Frame %d\n", rtPacket->GetFrameNumber()); */

            unsigned int n;

            rtPacket->Get3DNoLabelsMarker(0, ball_pos[0], ball_pos[1], ball_pos[2], n);

        }
        status << ball_pos[0] << ", " << ball_pos[1] << ", " << ball_pos[2] << endl;

        vector<double> red_pos;
        vector<double> red_rot;
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
                (ball_pos[0]-cal_offset_x*10)/(play_height*10)+0.5,
                (ball_pos[1]-cal_offset_y*10)/(play_width*10)+0.5,
                0,
            }}
        };
        string message = positionData.dump();

        {
            lock_guard<mutex> lock(clientsMutex);
            for (auto* client : clients) {
                loop->defer([client, message](){
                    client->send(message, uWS::OpCode::TEXT);
                });
                /* cout << "Update!" << endl; */
            }
        }

        if(controller){
            for(int i = 0; i < num_rod_t; ++i){
                double ws_pos_local, ws_pos_updated_local, ws_rot_local, ws_rot_updated_local;
                {
                    lock_guard<mutex> lock(clientsMutex);
                    ws_pos_local = ws_pos[i];
                    ws_pos_updated_local = ws_pos_updated[i];
                    ws_rot_local = ws_rot[i];
                    ws_rot_updated_local = ws_rot_updated[i];
                }
                if(ws_pos_updated_local){
                    move_lin(i, lin_range_cm[i] * ws_pos_local);
                    ws_pos_updated_local = false;
                }
                if(ws_rot_updated_local){
                    move_rot(i, ws_rot_local / deg_to_rad);
                    ws_rot_updated_local = false;
                }
            }
        } else {
            
        }

        print_status(status.str());

        this_thread::sleep_for(chrono::microseconds(100'000));
    }

    }catch (mnErr& theErr)
	{
		printf("Failed to disable Nodes n\n");
		//This statement will print the address of the error, the error code (defined by the mnErr class), 
		//as well as the corresponding error message.
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);

		return 0;  //This terminates the main program
	}

    uwsThread.join();

    return 0;

#if 0

    // Linear node tracking
    int lin_pos_cnts[num_rod_t] = {0}; // Homing sets to 0

    // Open video capture
    cv::VideoCapture cap(2);

    if (!cap.isOpened())
    {
        cerr << "Error: Couldn't open the camera." << endl;
        lin_nodes[three_bar].get().EnableReq(false);
        return -1;
    }

    // Retrieve calibration parameters
    cv::Mat cameraMatrix, distCoeffs;

    cv::FileStorage fs("../assets/calibration/calibration.yml", cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        cerr << "Failed to open calibration.yml" << endl;
        lin_nodes[three_bar].get().EnableReq(false);
        return -1;
    }

    fs["camera_matrix"] >> cameraMatrix;
    fs["dist_coeff"] >> distCoeffs;

    fs.release();

    // Aruco symbols used
    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

    // Make threshold window
    cv::namedWindow("Original", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Thresholded", cv::WINDOW_AUTOSIZE);

    int lowH = 10, highH = 20;
    int lowS = 139, highS = 255;
    int lowV = 92, highV = 221;

    cv::createTrackbar("LowH", "Thresholded", &lowH, 180, onLowHChange);
    cv::createTrackbar("LowS", "Thresholded", &lowS, 255, onLowSChange);
    cv::createTrackbar("LowV", "Thresholded", &lowV, 255, onLowVChange);
    cv::createTrackbar("HighH", "Thresholded", &highH, 180, onHighHChange);
    cv::createTrackbar("HighS", "Thresholded", &highS, 255, onHighSChange);
    cv::createTrackbar("HighV", "Thresholded", &highV, 255, onHighVChange);

    // Tracks current guess of where play area is based on fiducials
    vector<cv::Point2f> play_area = {{0,0},{0,0},{0,0},{0,0}};

    for(ever)
    {
        cv::Mat frame, flatFrame, hsvFrame, thresholdedFrame, transformedFrame;

        bool ret = cap.read(frame);
        if (!ret)
        {
            cerr << "Error: Couldn't read a frame from the camera." << endl;
            break;
        }

        // Undistort
        cv::Mat newcameramtx;
        cv::Rect roi;
        newcameramtx = cv::getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, frame.size(), 1, frame.size(), &roi);
        cv::undistort(frame, flatFrame, cameraMatrix, distCoeffs, newcameramtx);


        // Fetching values directly from the trackbars inside the loop
        lowH = cv::getTrackbarPos("LowH", "Thresholded");
        highH = cv::getTrackbarPos("HighH", "Thresholded");
        lowS = cv::getTrackbarPos("LowS", "Thresholded");
        highS = cv::getTrackbarPos("HighS", "Thresholded");
        lowV = cv::getTrackbarPos("LowV", "Thresholded");
        highV = cv::getTrackbarPos("HighV", "Thresholded");


        // Detect the markers
        vector<int> ids;
        vector<vector<cv::Point2f>> corners;
        cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();

        cv::aruco::ArucoDetector detector(dictionary, detectorParams);
        detector.detectMarkers(flatFrame, corners, ids);

        if (ids.size() > 0) 
        {
            cv::aruco::drawDetectedMarkers(flatFrame, corners, ids);
        }

        // Execute transform
        if(corners.size() == 4) for(int i = 0; i < 4; ++i){
            play_area[ids[i]] = corners[i][0];
        }

        vector<cv::Point2f> dst_pts;
        const int upscale = 5;
        int play_width_px = play_width*upscale;
        int play_height_px = play_height*upscale;
        int edge_width_px = 10*upscale, edge_height_px = 2*upscale;
        dst_pts.push_back(cv::Point2f(-edge_width_px, -edge_height_px));
        dst_pts.push_back(cv::Point2f(play_width_px+edge_width_px, -edge_height_px));
        dst_pts.push_back(cv::Point2f(play_width_px+edge_width_px, play_height_px+edge_height_px));
        dst_pts.push_back(cv::Point2f(-edge_width_px, play_height_px+edge_height_px));

        cv::Mat matrix = cv::getPerspectiveTransform(play_area, dst_pts);

        cv::warpPerspective(flatFrame, transformedFrame, matrix, cv::Size(play_width_px, play_height_px));

        // Threshold
        cv::cvtColor(transformedFrame, hsvFrame, cv::COLOR_BGR2HSV);
        cv::inRange(hsvFrame, cv::Scalar(lowH, lowS, lowV), cv::Scalar(highH, highS, highV), thresholdedFrame);

        // Erode and dilate
        cv::Mat ballFrame;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
        cv::erode(cv::Mat(thresholdedFrame, cv::Rect(0,0,30*upscale,play_height_px)), ballFrame, element);
        cv::dilate(ballFrame, ballFrame, element);

        // Find contours
        vector<vector<cv::Point>> contours;
        cv::findContours(ballFrame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // Find the largest contour
        double maxArea = 0;
        int largestContourIdx = -1;
        for (int i = 0; i < contours.size(); i++) {
            double area = cv::contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                largestContourIdx = i;
            }
        }

        // Convert to color
        cv::Mat ballFrameClr;
        cv::cvtColor(ballFrame, ballFrameClr, cv::COLOR_GRAY2BGR);

        // Calculate the centroid of the largest contour
        if(largestContourIdx >= 0){
            cv::Moments m = cv::moments(contours[largestContourIdx]);
            cv::Point2f centroid(m.m10/m.m00, m.m01/m.m00);
            cv::circle(ballFrameClr, centroid, 10, cv::Scalar(0, 0, 255), -1);

            // Send move
            /* linear_node.Motion.MoveWentDone(); // Clear move done register */
            double ball_pos = (centroid.y / ballFrame.size().height-1.0/2) * play_height;
            int target_cnts = lin_mid_cnts[three_bar] - ball_pos * lin_cm_to_cnts[three_bar];

            if(ball_pos + play_height/2 < plr_gap[three_bar] + plr_width/2 + bumper_width){
                target_cnts -= plr_gap[three_bar] * lin_cm_to_cnts[three_bar];
            } else if(ball_pos + play_height/2 > play_height - (plr_gap[three_bar] + plr_width/2 + bumper_width)){
                target_cnts += plr_gap[three_bar] * lin_cm_to_cnts[three_bar];
            }
             
            if(abs(target_cnts-lin_pos_cnts[three_bar]) > 500){
            /* if(false){ */
                double est_duration_ms = lin_nodes[three_bar].get().Motion.MovePosnDurationMsec(abs(target_cnts-lin_pos_cnts[three_bar]));
                lin_nodes[three_bar].get().Motion.MovePosnStart(target_cnts, true);
                /* printf("Target cnts: %d, estimated time: %f.\n", target_cnts, est_duration_ms); */
                timeout = mgr.TimeStampMsec() + est_duration_ms + 300;

                /* while (!lin_nodes[three_bar].get().Motion.MoveIsDone()) { */
                /*     if (cp_mgr.TimeStampMsec() > timeout) { */
                /*         printf("Error: Timed out waiting for move to complete\n"); */
                /*         lin_nodes[three_bar].get().EnableReq(false); */
                /*         return -2; */
                /*     } */
                /* } */
                lin_pos_cnts[three_bar] = target_cnts;
            }
        }

        cv::imshow("Original", transformedFrame);
        cv::imshow("Thresholded", thresholdedFrame);
        cv::imshow("Ball area", ballFrameClr);

        if ((cv::waitKey(30) & 0xFF) == 'q') // Press any key to exit
            break;
    }

    cap.release();
    cv::destroyAllWindows();

    for(int i = 0; i < lin_nodes.size(); ++i){
        lin_nodes[i].get().EnableReq(false);
    }
    for(int i = 0; i < rot_nodes.size(); ++i){
        rot_nodes[i].get().EnableReq(false);
    }

    mgr.PortsClose(); 

    return -1;
#endif
}

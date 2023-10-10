#include <cstddef>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>
#include <iostream>
#include <cstring>

#include "clearpath/pubSysCls.h"    

using namespace std;
using namespace cv;
using namespace sFnd;


#define ACC_LIM_RPM_PER_SEC    10000
#define VEL_LIM_RPM            200
#define MOVE_DISTANCE_CNTS     -2400    

#define HOMING_TIMEOUT 10000

void onLowHChange(int, void*) {}
void onHighHChange(int, void*) {}
void onLowSChange(int, void*) {}
void onHighSChange(int, void*) {}
void onLowVChange(int, void*) {}
void onHighVChange(int, void*) {}


int main( int argc, char** argv ){

    SysManager cp_mgr;
    std::vector<std::string> comHubPorts;

#if 0
    try
    {
        // Identify hubs
        SysManager::FindComHubPorts(comHubPorts);
        printf("Found %zu SC Hubs\n", comHubPorts.size());

        // Find available ports
        size_t portCount = 0;
        for (portCount = 0; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++) {
            cp_mgr.ComHubPort(portCount, comHubPorts[portCount].c_str());
        }

        if (portCount < 0) {
            printf("Unable to locate SC hub port\n");
            return -1;
        }

        // Open ports (hubs)
        cp_mgr.PortsOpen(portCount);

        IPort &hub0 = cp_mgr.Ports(0);
        printf(" Port[%d]: state=%d, nodes=%d\n",
            hub0.NetNumber(), hub0.OpenState(), hub0.NodeCount());

        // Open nodes (motors)
        INode &linear_node = hub0.Nodes(0);
        linear_node.EnableReq(false);
        cp_mgr.Delay(200);

        printf("   Node[%d]: type=%d\n", 0, linear_node.Info.NodeType());
        printf("            userID: %s\n", linear_node.Info.UserID.Value());
        printf("        FW version: %s\n", linear_node.Info.FirmwareVersion.Value());
        printf("          Serial #: %d\n", linear_node.Info.SerialNumber.Value());
        printf("             Model: %s\n", linear_node.Info.Model.Value());

        // Enable motor
        linear_node.Status.AlertsClear();
        linear_node.Motion.NodeStopClear();
        linear_node.EnableReq(true);
        printf("Node \t%i enabled\n", 0);

        double timeout = cp_mgr.TimeStampMsec() + 2000;
        while (!linear_node.Motion.IsReady()) {
            if (cp_mgr.TimeStampMsec() > timeout) {
                printf("Error: Timed out waiting for Node %d to enable\n", 0);
                return -2;
            }
        }

        // Homing
        if (linear_node.Motion.Homing.HomingValid()){
            linear_node.Motion.Homing.Initiate();

            timeout = cp_mgr.TimeStampMsec() + HOMING_TIMEOUT;            

            while (!linear_node.Motion.Homing.WasHomed()) {
                if (cp_mgr.TimeStampMsec() > timeout) {
                    printf("Homing failed!\n");
                    return -2;
                }
            }
        }

        // Set motion parameters
        linear_node.Motion.MoveWentDone(); // Clear move done register
        linear_node.AccUnit(INode::RPM_PER_SEC);
        linear_node.VelUnit(INode::RPM);
        linear_node.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
        linear_node.Motion.VelLimit = VEL_LIM_RPM;

        // Send move
        printf("Moving Node \t%i \n", 0);
        linear_node.Motion.MovePosnStart(MOVE_DISTANCE_CNTS);
        printf("%f estimated time.\n", linear_node.Motion.MovePosnDurationMsec(MOVE_DISTANCE_CNTS));
        timeout = cp_mgr.TimeStampMsec() + linear_node.Motion.MovePosnDurationMsec(MOVE_DISTANCE_CNTS) + 100;

        while (!linear_node.Motion.MoveIsDone()) {
            if (cp_mgr.TimeStampMsec() > timeout) {
                printf("Error: Timed out waiting for move to complete\n");
                return -2;
            }
        }
        printf("Node \t%i Move Done\n", 0);


        // Disable motor
        linear_node.EnableReq(false);


    }
    catch (mnErr& theErr)
    {
        printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
        return -1;

    }

    cp_mgr.PortsClose(); 

#endif

    cv::VideoCapture cap(2);

    if (!cap.isOpened())
    {
        std::cerr << "Error: Couldn't open the camera." << std::endl;
        return -1;
    }

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

    cv::namedWindow("Original", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Thresholded", cv::WINDOW_AUTOSIZE);

    int lowH = 8, highH = 22;
    int lowS = 121, highS = 255;
    int lowV = 92, highV = 255;

    cv::createTrackbar("LowH", "Thresholded", &lowH, 180, onLowHChange);
    cv::createTrackbar("LowS", "Thresholded", &lowS, 255, onLowSChange);
    cv::createTrackbar("LowV", "Thresholded", &lowV, 255, onLowVChange);
    cv::createTrackbar("HighH", "Thresholded", &highH, 180, onHighHChange);
    cv::createTrackbar("HighS", "Thresholded", &highS, 255, onHighSChange);
    cv::createTrackbar("HighV", "Thresholded", &highV, 255, onHighVChange);

    while (true)
    {
        cv::Mat frame, hsvFrame, thresholdedFrame;

        bool ret = cap.read(frame);
        if (!ret)
        {
            std::cerr << "Error: Couldn't read a frame from the camera." << std::endl;
            break;
        }

        // Fetching values directly from the trackbars inside the loop
        lowH = cv::getTrackbarPos("LowH", "Thresholded");
        highH = cv::getTrackbarPos("HighH", "Thresholded");
        lowS = cv::getTrackbarPos("LowS", "Thresholded");
        highS = cv::getTrackbarPos("HighS", "Thresholded");
        lowV = cv::getTrackbarPos("LowV", "Thresholded");
        highV = cv::getTrackbarPos("HighV", "Thresholded");

        cv::cvtColor(frame, hsvFrame, cv::COLOR_BGR2HSV);
        cv::inRange(hsvFrame, cv::Scalar(lowH, lowS, lowV), cv::Scalar(highH, highS, highV), thresholdedFrame);


        // Detect the markers
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
        cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
        /* cv::aruco::detectMarkers(frame, &dictionary, corners, ids); */
        cv::aruco::ArucoDetector detector(dictionary, detectorParams);
        detector.detectMarkers(frame, corners, ids);

        if (ids.size() > 0) 
        {
            cv::aruco::drawDetectedMarkers(frame, corners, ids);
        }

        cv::imshow("Original", frame);
        cv::imshow("Thresholded", thresholdedFrame);

        if ((cv::waitKey(30) & 0xFF) == 'q') // Press any key to exit
            break;
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}

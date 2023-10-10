#include <cstddef>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/calib3d.hpp>
#include <iostream>
#include <cstring>

#include "clearpath/pubSysCls.h"    

using namespace std;
using namespace cv;
using namespace sFnd;

#define ever ;;

#define ACC_LIM_RPM_PER_SEC    1000
#define VEL_LIM_RPM            200

#define HOMING_TIMEOUT 10000

const int linear_min_cnts = -2400;
const int linear_max_cnts = 0;

const double play_height_cm = 68;
const double play_width_cm = 116.6;

void onLowHChange(int, void*) {}
void onHighHChange(int, void*) {}
void onLowSChange(int, void*) {}
void onHighSChange(int, void*) {}
void onLowVChange(int, void*) {}
void onHighVChange(int, void*) {}

int main( int argc, char** argv ){

    SysManager cp_mgr;
    std::vector<std::string> comHubPorts;

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
    linear_node.AccUnit(INode::RPM_PER_SEC);
    linear_node.VelUnit(INode::RPM);
    linear_node.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
    linear_node.Motion.VelLimit = VEL_LIM_RPM;

    // Linear node tracking
    int linear_pos_cnts = 0; // Homing sets to 0

    // Open video capture
    cv::VideoCapture cap(2);

    if (!cap.isOpened())
    {
        std::cerr << "Error: Couldn't open the camera." << std::endl;
        return -1;
    }

    // Retrieve calibration parameters
    cv::Mat cameraMatrix, distCoeffs;

    cv::FileStorage fs("../assets/calibration/calibration.yml", cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cerr << "Failed to open calibration.yml" << std::endl;
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
    std::vector<cv::Point2f> play_area = {{0,0},{0,0},{0,0},{0,0}};

    for(ever)
    {
        cv::Mat frame, flatFrame, hsvFrame, thresholdedFrame, transformedFrame;

        bool ret = cap.read(frame);
        if (!ret)
        {
            std::cerr << "Error: Couldn't read a frame from the camera." << std::endl;
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
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
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

        std::vector<cv::Point2f> dst_pts;
        const int upscale = 5;
        int play_width_px = play_width_cm*upscale;
        int play_height_px = play_height_cm*upscale;
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
        cv::erode(cv::Mat(thresholdedFrame, Rect(0,0,30*upscale,play_height_px)), ballFrame, element);
        cv::dilate(ballFrame, ballFrame, element);

        // Find contours
        std::vector<std::vector<cv::Point>> contours;
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
            printf("Moving Node \t%i \n", 0);

            int target_cnts = linear_max_cnts - (centroid.y / ballFrame.size().height)
                    * (linear_max_cnts - linear_min_cnts);
            if(abs(target_cnts-linear_pos_cnts) > 30){
                double est_duration_ms = linear_node.Motion.MovePosnDurationMsec(abs(target_cnts-linear_pos_cnts));
                linear_node.Motion.MovePosnStart(target_cnts, true);
                printf("Target cnts: %d, estimated time: %f.\n", target_cnts, est_duration_ms);
                timeout = cp_mgr.TimeStampMsec() + est_duration_ms + 100;

                while (!linear_node.Motion.MoveIsDone()) {
                    if (cp_mgr.TimeStampMsec() > timeout) {
                        printf("Error: Timed out waiting for move to complete\n");
                        return -2;
                    }
                }
                printf("Node \t%i Move Done\n", 0);
                linear_pos_cnts = target_cnts;
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

    // Close motors
    linear_node.EnableReq(false);
    cp_mgr.PortsClose(); 

    return 0;
}

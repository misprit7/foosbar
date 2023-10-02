#include <cstddef>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <cstring>

#include "clearpath/pubSysCls.h"    

using namespace std;
using namespace cv;
using namespace sFnd;


#define ACC_LIM_RPM_PER_SEC	10000
#define VEL_LIM_RPM			100
#define MOVE_DISTANCE_CNTS	100	


int main( int argc, char** argv ){

    SysManager cp_mgr;
    std::vector<std::string> comHubPorts;

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

		// Open ports
		cp_mgr.PortsOpen(portCount);

        IPort &hub0 = cp_mgr.Ports(0);
		printf(" Port[%d]: state=%d, nodes=%d\n",
			hub0.NetNumber(), hub0.OpenState(), hub0.NodeCount());

		INode &linear_node = hub0.Nodes(0);
		linear_node.EnableReq(false);
		cp_mgr.Delay(200);

		printf("   Node[%d]: type=%d\n", 0, linear_node.Info.NodeType());
		printf("            userID: %s\n", linear_node.Info.UserID.Value());
		printf("        FW version: %s\n", linear_node.Info.FirmwareVersion.Value());
		printf("          Serial #: %d\n", linear_node.Info.SerialNumber.Value());
		printf("             Model: %s\n", linear_node.Info.Model.Value());


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

		linear_node.Motion.MoveWentDone();

		linear_node.AccUnit(INode::RPM_PER_SEC);
		linear_node.VelUnit(INode::RPM);
		linear_node.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
		linear_node.Motion.VelLimit = VEL_LIM_RPM;

		printf("Moving Node \t%i \n", 0);
		linear_node.Motion.MovePosnStart(MOVE_DISTANCE_CNTS);
		printf("%f estiomated time.\n", linear_node.Motion.MovePosnDurationMsec(MOVE_DISTANCE_CNTS));
		timeout = cp_mgr.TimeStampMsec() + linear_node.Motion.MovePosnDurationMsec(MOVE_DISTANCE_CNTS) + 100;

		while (!linear_node.Motion.MoveIsDone()) {
			if (cp_mgr.TimeStampMsec() > timeout) {
				printf("Error: Timed out waiting for move to complete\n");
				return -2;
			}
		}
		printf("Node \t%i Move Done\n", 0);


		linear_node.EnableReq(false);


    }
	catch (mnErr& theErr)
	{
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		return -1;

    }

    cp_mgr.PortsClose(); 

    return 0;

    /* // show help */
    /* if(argc<2){ */
    /*     cout<< */
    /*         " Usage: tracker <video_name>\n" */
    /*         " examples:\n" */
    /*         " example_tracking_kcf Bolt/img/%04d.jpg\n" */
    /*         " example_tracking_kcf faceocc2.webm\n" */
    /*         << endl; */
    /*     return 0; */
    /* } */
    /* // declares all required variables */
    /* Rect roi; */
    /* Mat frame; */
    /* // create a tracker object */
    /* cv::Ptr<Tracker> tracker = TrackerKCF::create(); */
    /* // set input video */
    /* std::string video = argv[1]; */
    /* VideoCapture cap(video); */
    /* // get bounding box */
    /* cap >> frame; */
    /* roi=selectROI("tracker",frame); */
    /* //quit if ROI was not selected */
    /* if(roi.width==0 || roi.height==0) */
    /*     return 0; */
    /* // initialize the tracker */
    /* tracker->init(frame,roi); */
    /* // perform the tracking process */
    /* printf("Start the tracking process, press ESC to quit.\n"); */
    /* for ( ;; ){ */
    /*     // get frame from the video */
    /*     cap >> frame; */
    /*     // stop the program if no more images */
    /*     if(frame.rows==0 || frame.cols==0) */
    /*         break; */
    /*     // update the tracking result */
    /*     tracker->update(frame,roi); */
    /*     // draw the tracked object */
    /*     rectangle( frame, roi, Scalar( 255, 0, 0 ), 2, 1 ); */
    /*     // show image with the tracked object */
    /*     imshow("tracker",frame); */
    /*     //quit on ESC button */
    /*     if(waitKey(1)==27)break; */
    /* } */
    /* return 0; */
}

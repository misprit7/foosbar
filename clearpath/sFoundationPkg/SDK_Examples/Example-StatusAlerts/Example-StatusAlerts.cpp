///*******************************************************************************
/**
	Example-StatusAlerts

	This example demonstrates checking Clearpath-SC node's status and Alert Registers.
**/
//******************************************************************************

#include <stdio.h>
#include <string>
#include <iostream>
#include "pubSysCls.h"

using namespace sFnd;
using namespace std;


// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}


int main(int argc, char* argv[])
{
	msgUser("Status Alerts Example starting. Press Enter to continue.");
	size_t portCount = 0;
	std::vector<std::string> comHubPorts;

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager myMgr;							//Create System Manager myMgr

	//This will try to open the port. If there is an error/exception during the port opening,
	//the code will jump to the catch loop where detailed information regarding the error will be displayed;
	//otherwise the catch loop is skipped over
	try
	{ 
		
		SysManager::FindComHubPorts(comHubPorts);
		printf("Found %d SC Hubs\n", comHubPorts.size());

		for (portCount = 0; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++) {
			
			myMgr.ComHubPort(portCount, comHubPorts[portCount].c_str()); 	//define the first SC Hub port (port 0) to be associated 
											// with COM portnum (as seen in device manager)
		}

		if (portCount > 0) {
			//printf("\n I will now open port \t%i \n \n", portnum);
			myMgr.PortsOpen(portCount);				//Open the port

			for (size_t i = 0; i < portCount; i++) {
				IPort &myPort = myMgr.Ports(i);

				printf(" Port[%d]: state=%d, nodes=%d\n",
					myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());
			}
		}
		else {
			printf("Unable to locate SC hub port\n");

			msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key

			return -1;  //This terminates the main program
		}
	

		for (size_t iPort = 0; iPort < portCount; iPort++) {
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr.Ports(iPort);

			char alertList[256];

			printf("Checking for Alerts: \n");

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++) {
				// Get a reference to the node, to make accessing it easier
				INode &theNode = myPort.Nodes(iNode);

				// make sure our registers are up to date
				theNode.Status.RT.Refresh();
				theNode.Status.Alerts.Refresh();

				printf("---------\n");
				printf(" Checking node %i for Alerts:\n", iNode);

				// Check the status register's "AlertPresent" bit
				// The bit is set true if there are alerts in the alert register
				if (!theNode.Status.RT.Value().cpm.AlertPresent) {
					printf("   Node has no alerts!\n");
				}
				//Check to see if the node experienced torque saturation
				if (theNode.Status.HadTorqueSaturation()) {
					printf("      Node has experienced torque saturation since last checking\n");
				}
				// get an alert register reference, check the alert register directly for alerts
				if (theNode.Status.Alerts.Value().isInAlert()) {
					// get a copy of the alert register bits and a text description of all bits set
					theNode.Status.Alerts.Value().StateStr(alertList, 256);
					printf("   Node has alerts! Alerts:\n%s\n", alertList);

					// can access specific alerts using the method below
					if (theNode.Status.Alerts.Value().cpm.Common.EStopped) {
						printf("      Node is e-stopped: Clearing E-Stop\n");
						theNode.Motion.NodeStopClear();
					}
					if (theNode.Status.Alerts.Value().cpm.TrackingShutdown) {
						printf("      Node exceeded Tracking error limit\n");
					}

					

					// Check for more alerts and Clear Alerts
					theNode.Status.Alerts.Refresh();
					if (theNode.Status.Alerts.Value().isInAlert()) {
						theNode.Status.Alerts.Value().StateStr(alertList, 256);
						printf("      Node has non-estop alerts: %s\n", alertList);
						printf("      Clearing non-serious alerts\n");
						theNode.Status.AlertsClear();

						// Are there still alerts?
						theNode.Status.Alerts.Refresh();
						if (theNode.Status.Alerts.Value().isInAlert()) {
							theNode.Status.Alerts.Value().StateStr(alertList, 256);
							printf("   Node has serious, non-clearing alerts: %s\n", alertList);
						}
						else {
							printf("   Node %d: all alerts have been cleared\n", theNode.Info.Ex.Addr());
						}
					}
					else {
						printf("   Node %d: all alerts have been cleared\n", theNode.Info.Ex.Addr());
					}

				}
				printf("=========\n");
			}

			printf("\nShutting down network\n");

			// Close down the ports
			myMgr.PortsClose();
		}
	}
	catch (mnErr& theErr) {
		fprintf(stderr, "Caught error: addr=%d, err=0x%0x\nmsg=%s\n",
				theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n",
				theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		msgUser("Press any key to continue.");
		// keep the console output readable
		printf("\n");
		return(2);
	}
	catch (...) {
		fprintf(stderr, "Error generic caught\n");
		printf("Generic error caught\n");
		msgUser("Press any key to continue.");
		// keep the console output readable
		printf("\n");
		return(3);
	}

	// Good-bye
	msgUser("Press any key to continue.");
	// keep the console output readable
	printf("\n");
	return 0;
}
//																			   *
//******************************************************************************


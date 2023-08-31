//Required include files
#include <stdio.h>	
#include <string>
#include <iostream>
#include "pubSysCls.h"	

using namespace sFnd;

// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}

//*********************************************************************************
//This program will load configuration files onto each node connected to the port, then executes
//sequential repeated moves on each axis.
//*********************************************************************************

#define CHANGE_NUMBER_SPACE	2000	//The change to the numberspace after homing (cnts)
#define TIME_TILL_TIMEOUT	10000	//The timeout used for homing(ms)


int main(int argc, char* argv[])
{
	msgUser("Homing Example starting. Press Enter to continue.");

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


		//Once the code gets past this point, it can be assumed that the Port has been opened without issue
		//Now we can get a reference to our port object which we will use to access the node 

		for (size_t iPort = 0; iPort < portCount; iPort++) {
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr.Ports(iPort);

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++) {

				//////////////////////////////////////////////////////////////////////////////////////////////////////////
				//Here we identify the first Node, enable and home the node, then adjust the position reference
				//////////////////////////////////////////////////////////////////////////////////////////////////////////

						// Create a shortcut reference for the first node
				INode &theNode = myPort.Nodes(iNode);

				//theNode.EnableReq(false);				//Ensure Node is disabled before starting

				printf("   Node[%d]: type=%d\n", iNode, theNode.Info.NodeType());
				printf("            userID: %s\n", theNode.Info.UserID.Value());
				printf("        FW version: %s\n", theNode.Info.FirmwareVersion.Value());
				printf("          Serial #: %d\n", theNode.Info.SerialNumber.Value());
				printf("             Model: %s\n", theNode.Info.Model.Value());

				//The following statements will attempt to enable the node.  First,
				// any shutdowns or NodeStops are cleared, finally the node in enabled
				theNode.Status.AlertsClear();					//Clear Alerts on node 
				theNode.Motion.NodeStopClear();	//Clear Nodestops on Node  				
				theNode.EnableReq(true);					//Enable node 

				double timeout = myMgr.TimeStampMsec() + TIME_TILL_TIMEOUT;	//define a timeout in case the node is unable to enable
																			//This will loop checking on the Real time values of the node's Ready status
				while (!theNode.Motion.IsReady()) {
					if (myMgr.TimeStampMsec() > timeout) {
						printf("Error: Timed out waiting for Node %d to enable\n", iNode);
						msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
						return -2;
					}
				}
				//At this point the Node is enabled, and we will now check to see if the Node has been homed
				//Check the Node to see if it has already been homed, 
				if (theNode.Motion.Homing.HomingValid())
				{
					if (theNode.Motion.Homing.WasHomed())
					{
						printf("Node %d has already been homed, current position is: \t%8.0f \n", iNode, theNode.Motion.PosnMeasured.Value());
						printf("Rehoming Node... \n");
					}
					else
					{
						printf("Node [%d] has not been homed.  Homing Node now...\n", iNode);
					}
					//Now we will home the Node
						theNode.Motion.Homing.Initiate();

						timeout = myMgr.TimeStampMsec() + TIME_TILL_TIMEOUT;	//define a timeout in case the node is unable to enable
					   // Basic mode - Poll until disabled
						while (!theNode.Motion.Homing.WasHomed()) {
							if (myMgr.TimeStampMsec() > timeout) {
								printf("Node did not complete homing:  \n\t -Ensure Homing settings have been defined through ClearView. \n\t -Check for alerts/Shutdowns \n\t -Ensure timeout is longer than the longest possible homing move.\n");
								msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
								return -2;
							}
						}
						theNode.Motion.PosnMeasured.Refresh();		//Refresh our current measured position
						printf("Node completed homing, current position: \t%8.0f \n", theNode.Motion.PosnMeasured.Value());
						printf("Soft limits now active\n");

						printf("Adjusting Numberspace by %d\n", CHANGE_NUMBER_SPACE);

						theNode.Motion.AddToPosition(CHANGE_NUMBER_SPACE);			//Now the node is no longer considered "homed, and soft limits are turned off

						theNode.Motion.Homing.SignalComplete();		//reset the Node's "sense of home" soft limits (unchanged) are now active again

						theNode.Motion.PosnMeasured.Refresh();		//Refresh our current measured position
						printf("Numberspace changed, current position: \t%8.0f \n", theNode.Motion.PosnMeasured.Value());
					}
					else {
						printf("Node[%d] has not had homing setup through ClearView.  The node will not be homed.\n", iNode);
					}

					printf("Disabling Node\n");
					theNode.EnableReq(false);
				}
			}
		}
	catch (mnErr& theErr)
	{
		//This statement will print the address of the error, the error code (defined by the mnErr class), 
		//as well as the corresponding error message.
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);

		msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key

		return 0;  //This terminates the main program
	}
		

	// Close down the ports
	myMgr.PortsClose();

	msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
	return 0;			//End program
}


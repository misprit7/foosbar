/******************************************************************************/
/** 
	Using Communications Hub for General Purpose Outputs, and Inputs

	This example demonstrates setting the Brake Control mode to use a
	communication hub Brake output for general purpose usages.  This example
	also reads the local General purpose inputs at the Nodes.
**/
/******************************************************************************/

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
	msgUser("GPIO Example starting. Press Enter to continue.");
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
		//Now we can get a reference to our port object which we will use to access the node objects

		for (size_t iPort = 0; iPort < portCount; iPort++) {
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr.Ports(iPort);



			// Print out some information about the port
			printf(" Port[%d]: state=%d, nodes=%d\n", myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());

			// Set GPO to ON for the first brake control, 
			myPort.BrakeControl.BrakeSetting(0, GPO_ON);
			// Can set GPO to OFF for the second brake control using the method below
			//myPort.BrakeControl.BrakeSetting(1, GPO_OFF);

			for (size_t iBrake = 0; iBrake < 2; iBrake++) {
				// Print out brake mode for the first brake
				size_t controlMode = myPort.BrakeControl.BrakeSetting(iBrake);
				if (controlMode == 3) {
					printf("\nPort[%d] Brake 0 control mode: GPO ON", myPort.NetNumber());
				}
				else if (controlMode == 4) {
					printf("\nPort[%d] Brake 0 control mode: GPO OFF", myPort.NetNumber());
				}
				else {
					printf("\nPort[%d] Brake 0 control mode: GPO not set", myPort.NetNumber());
				}

				// keep the console output readable
				printf("\n");
			}

			printf("\n");

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++) {

				INode &theNode = myPort.Nodes(iNode);

				//Checking Inputs to each Node
				if (theNode.Status.RT.Value().cpm.InA)
				{
					printf("Node %i Input A is asserted \n", iNode);
				}
				else
				{
					printf("Node %i Input A is not asserted \n", iNode);
				}
				if (theNode.Status.RT.Value().cpm.InB)
				{
					printf("Node %i Input B is asserted \n", iNode);
				}
				else
				{
					printf("Node %i Input B is not asserted\n", iNode);
				}

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
		msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
		// keep the console output readable
		printf("\n");
		return(2);
	}
	catch (...) {
		fprintf(stderr, "Error generic caught\n");
		printf("Generic error caught\n");
		msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
		// keep the console output readable
		printf("\n");
		return(3);
	}

	// Good-bye
	msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
	// keep the console output readable
	printf("\n");
	return 0;
}
//																			   *
//******************************************************************************


///*******************************************************************************
/**
	Basic Operations Example

	The main program for single threaded ClearPath-SC example. The only command
	line argument is the port number where the network is attached. This
	main function opens the port, prints some basic information about the
	nodes that are found, checks that they are all in full-access mode,
	then creates the Axis objects to run the nodes individually.
**/
//******************************************************************************

#include <stdio.h>
#include <ctime>
#include <string>
#include <iostream>
#include "Axis.h"

// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}

/*****************************************************************************
*  Function Name: main
*	Description:	The main function for this single threaded example. 
*					One command-line argument is expected, which is the COM
*					port number for where the SC network is attached.
*		Return:		0 if successful; non-zero if there was a problem
*****************************************************************************/
int main(int argc, char* argv[])
{
	msgUser("Single-threaded Example starting. Press Enter to continue.");

	size_t portCount = 0;
	size_t returnVal = 0;
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

			returnVal = -1;  //This terminates the main program
		}

		// Create a list of axes - there will be one Axis per node
		vector<Axis*> listOfAxes;

		// Assume that the nodes are of the right type and that this app has full control
		bool nodeTypesGood = true, accessLvlsGood = true;

		for (size_t iPort = 0; iPort < portCount; iPort++) {
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr.Ports(iPort);

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++) {
				// Get a reference to the node, to make accessing it easier
				INode &theNode = myPort.Nodes(iNode);

				// Make sure we are talking to a ClearPath SC (advanced or basic model will work)
				if (theNode.Info.NodeType() != IInfo::CLEARPATH_SC_ADV
					&& theNode.Info.NodeType() != IInfo::CLEARPATH_SC) {
					printf("---> ERROR: Uh-oh! Node %d is not a ClearPath-SC Motor\n", iNode);
					nodeTypesGood = false;
				}

				if (nodeTypesGood) {
					// Create an axis for this node
					listOfAxes.push_back(new Axis(myMgr, &theNode));

					// Make sure we have full access
					if (!theNode.Setup.AccessLevelIsFull()) {
						printf("---> ERROR: Oh snap! Access level is not good for node %u\n", iNode);
						accessLvlsGood = false;
					}
				}
			}
		}

		// If we have full access to the nodes and they are all ClearPath-SC nodes, 
		// then continue with the example
		if (nodeTypesGood && accessLvlsGood) {
			for (Uint16 iAxis = 0; iAxis < listOfAxes.size(); iAxis++) {
				// Tell each axis to do its thing
				listOfAxes.at(iAxis)->AxisMain();
			}
		}
		else {
			// If something is wrong with the nodes, tell the user about it
			if (!nodeTypesGood) {
				printf("\n\tFAILURE: Please attach only ClearPath-SC nodes.\n\n");
				returnVal = -5;
			}
			else if (!accessLvlsGood) {
				printf("\n\tFAILURE: Please get full access on all your nodes.\n\n");
				returnVal = -6;
			}
		}

		// Delete the list of axes that were created
		for (size_t iAxis = 0; iAxis < listOfAxes.size(); iAxis++) {
			delete listOfAxes.at(iAxis);
		}

		// Close down the ports
		myMgr.PortsClose();

	}
	catch (mnErr& theErr) {
		fprintf(stderr, "Caught error: addr=%d, err=0x%0x\nmsg=%s\n",
			theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n",
			theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		returnVal = -3;
	}
	catch (...) {
		fprintf(stderr, "Error generic caught\n");
		printf("Generic error caught\n");
		returnVal = -4;
	}

	// Good-bye
	msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
	return returnVal;
}
//																			   *
//******************************************************************************


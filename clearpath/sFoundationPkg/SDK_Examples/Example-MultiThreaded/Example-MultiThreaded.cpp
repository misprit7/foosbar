///*******************************************************************************
/**
    Example-MultiThreaded

	The main function for Multi-Threaded ClearPath-SC example. The only command
	line argument is the port number where the network is attached. This
	main function opens the port, prints some basic information about the
	nodes that are found, checks that they are all in full-access mode,
	then creates the Supervisor object (which has its own thread) to run
	the show. This main thread then waits for the user to hit a key to
	end the program.
**/
//******************************************************************************

#include <stdio.h>
#include <ctime>
#include <chrono>
#include <string>
#include <iostream>
#include "Axis.h"
#include "Supervisor.h"

// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}

/*****************************************************************************
*  Function Name: AttentionDetected
*	 Description:	This is the port-level attention handler function.
*					This handler simply prints out the attention information
*					to the console window. 
*	  Parameters:
*          Input:	detected		- contains the attention information
*         Return:		none
*****************************************************************************/
void MN_DECL AttentionDetected(const mnAttnReqReg &detected)
{
	// Make a local, non-const copy for printing purposes
	mnAttnReqReg myAttns = detected;
	// Create a buffer to hold the attentionReg information
	char attnStringBuf[512];
	// Load the buffer with the string representation of the attention information
	myAttns.AttentionReg.StateStr(attnStringBuf, 512);
	// Print it out to the console
	printf("  --> ATTENTION: port %d, node=%d, attn=%s\n",
		detected.MultiAddr >> 4, detected.MultiAddr, attnStringBuf);
}


#if _MSC_VER
#pragma warning(disable:4996)
#endif
// A nice way of printing out the system time
string CurrentTimeStr() {
	time_t now = time(NULL);
	return string(ctime(&now));
}
#define CURRENT_TIME_STR CurrentTimeStr().c_str()
#if _MSC_VER
#pragma warning(default:4996)
#endif

int main(int argc, char* argv[])
{
	msgUser("Multithreaded Example starting. Press Enter to continue.");

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

		// Create a list of axes - one per node
		vector<Axis*> listOfAxes;
		// Assume that the nodes are of the right type and that this app has full control
		bool nodeTypesGood = true, accessLvlsGood = true; 

		for (size_t iPort = 0; iPort < portCount; iPort++){
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr.Ports(iPort);

			// Enable the attentions for this port
			myPort.Adv.Attn.Enable(true);
			// The attentions will be handled by the individual nodes, but register
			// a handler at the port level, just for illustrative purposes.
			myPort.Adv.Attn.AttnHandler(AttentionDetected);

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++){

				// Get a reference to the node, to make accessing it easier
				INode &theNode = myPort.Nodes(iNode);
				theNode.Motion.AccLimit = 100000;

				// Make sure we are talking to a ClearPath SC
				if (theNode.Info.NodeType() != IInfo::CLEARPATH_SC_ADV) {
					printf("---> ERROR: Uh-oh! Node %d is not a ClearPath-SC Advanced Motor\n", iNode);
					nodeTypesGood = false;
				}

				if (nodeTypesGood) {
					// Create an axis for this node
					listOfAxes.push_back(new Axis(&theNode));

					if (!theNode.Setup.AccessLevelIsFull()) {
						printf("---> ERROR: Oh snap! Access level is not good for node %u\n", iNode);
						accessLvlsGood = false;
					}
					else {
						// Set the move distance based on where it is in the network
						listOfAxes.at(iNode)->SetMoveRevs((iNode + 1) * 2);
						// Set the trigger group indicator
						theNode.Motion.Adv.TriggerGroup(1);
					}
				}

			}

#if 0
			// Set the last node in the ring to a longer move
			listOfAxes.at(listOfAxes.size()-1)->SetMoveRevs(listOfAxes.size()*10);
#endif
		}

		// If we have full access to the nodes and they are all ClearPath-SC advanced nodes, 
		// then continue with the example
		if (nodeTypesGood && accessLvlsGood){

			// Create the supervisor thread, giving it access to the list of axes
			Supervisor theSuper(listOfAxes, myMgr);
			theSuper.CreateThread();

			printf("\nMachine starting: %s\n", CURRENT_TIME_STR);

			// Everything is running - wait for the user to press a key to end it
			msgUser("Press any key to stop the machine."); //pause so the user can see the error message; waits for user to press a key
			
			printf("Machine stopping: %s\n", CURRENT_TIME_STR);

			// Tell the supervisor to stop
			theSuper.Quit();
			theSuper.Terminate();

			printf("\nFinalStats:\tnumMoves\t\tPosition\n");
		}
		else{
			// If something is wrong with the nodes, tell the user about it and quit
			if (!nodeTypesGood){
				printf("\n\tFAILURE: Please attach only ClearPath-SC Advanced nodes.\n\n");
			}
			else if (!accessLvlsGood){
				printf("\n\tFAILURE: Please get full access on all your nodes.\n\n");
			}
		}

		// Delete the list of axes that were created
		for (size_t iAxis = 0; iAxis < listOfAxes.size(); iAxis++){
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
		msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
		return(2);
	}
	catch (...) {
		fprintf(stderr, "Error generic caught\n");
		printf("Generic error caught\n");
		msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
		return(3);
	}

	// Good-bye
	msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
	return 0;
}
//																			   *
//******************************************************************************


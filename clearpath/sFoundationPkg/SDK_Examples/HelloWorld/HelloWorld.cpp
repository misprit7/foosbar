//Required include files
#include <stdio.h>		
#include <iostream>
#include "pubSysCls.h"	

using namespace sFnd;	

// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}

/*****************************************************************************
*	Description:	This program is the bare minimum to open a port and identify the number of nodes
*		on the port.
*	
*
*		Return:		0 if the system was initialized properly.
*					-1 if there was a problem accessing the port.
*****************************************************************************/

int main(int argc, char* argv[]){
	size_t portCount = 0;
	std::vector<std::string> comHubPorts;

	msgUser("Hello World Example starting. Press Enter to continue.");

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager myMgr;							//Create System Manager myMgr
		
	printf("Hello World, I am SysManager\n");	//Print to console

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
			// Close down the ports
			myMgr.PortsClose();	
		}
		else {
			printf("Unable to locate SC hub port\n");

			msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key

			return -1;  //This terminates the main program
		}
	}			
	catch(mnErr& theErr)	//This catch statement will intercept any error from the Class library
	{
		printf("Port Failed to open, Check to ensure correct Port number and that ClearView is not using the Port\n");	
		//This statement will print the address of the error, the error code (defined by the mnErr class), 
		//as well as the corresponding error message.
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);

		msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
    
		return -1;  //This terminates the main program
	}

	msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key

	return 0;			//End program
}
//

//******************************************************************************
// $Archive: /ClearPath SC/User Driver/src/meridianNet.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: meridianNet.cpp $
//
// DESCRIPTION
/**
	\file
	\cond ISC_DOC
	\brief System Setup Programming Interface Implementation

	This module implements the system startup/shutdown and node inventory
	features.

	\defgroup SetupGrp System Setup Programming Interface
	\brief Meridian Link Setup, Shutdown and Node Discovery Functions.
	- Include "meridianHdrs.h" in your project to use these functions.

	This group of functions sets up a system of Meridian nodes that may
	span multiple serial ports. Once a system is initialized an
	application can query for the quantities and addresses of specific
	types of node in the system.

	There are two groups for functions noted by the inclusion of \e
	"Sys" or \e "Net" in their names. The \e "Sys" group of functions
	targets all serial ports atomically. The \e "Net" group targets a
	specific serial port channel alone.

	On larger systems, breaking functionality up by individual serial
	ports can lead to improved performance or the ability to adapt to
	new machine features in a modular fashion. 	i.e. an additional
	elevator sub-system is added and its motion and I/O is provided by
	its communication link on a separate port. Alternatively within the
	node count limitations, this could be appended to an existing
	sub-system and the presence of additional nodes an indication of
	this potentially optional feature.
@{
**/
//		Provides the following main functions:
//			mnInitializeSystem
//			mnInitializeNets
//				System Initialization
//
//			mnShutdown
//				Orderly shutdown fucntion
//
//			mnRestartNet
//				Restart either cold or warm of specified network and nodes.
//
// CREATION DATE:
//		10/30/1998 09:55:47
//		06/11/2009 10:52:00 - Renamed and cleaned out irrelavent CP stuff
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
// 																			   *
//******************************************************************************


//******************************************************************************
//  NAME																       *
// 	meridianNet.c headers
//
	#include "meridianHdrs.h"
	#ifndef DOXYGEN_SHOULD_SKIP_THIS
		#include "lnkAccessCommon.h"
		#include "netCmdPrivate.h"
		#include "pubSysCls.h"
		#if (defined(_WIN32)||defined(_WIN64))
			#include <crtdbg.h>
			#include <winbase.h>
		#else
			#include <stdio.h>
			#include <string.h>
		#endif
	#endif
// 																	           *
//******************************************************************************



//******************************************************************************
//  NAME																       *
// 	meridianNet.c constants
//
#if (defined(_WIN32)||defined(_WIN64))
#define DBG_LOG(x) OutputDebugString(_T(x))
#else
#define DBG_LOG(x) fprintf(stderr, x)
#endif
// 																	           *
//******************************************************************************



//******************************************************************************
//  NAME																       *
// 	meridianNet.c static

/// \cond	INTERNAL_DOC

	// Number of specified ports on last mnInitializeNets
	unsigned SysPortCount = 0;
	// System inventory of nodes types and addresses
	mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];

	// Variables for debug thread locking
	CCEvent debugGate;
	CCEvent debugThreadLockResponseGate;
	bool inDebugging;
	nodeulong lockedThreadID;
	CCMutex threadLockMutex;

//																			   *
//******************************************************************************


//******************************************************************************
//  NAME																       *
//		mnInitializeProc
//
//  CREATION DATE
//		Modified on 01/12/2001 18:27:40
//		Modified 08/05/09 2:37:45 - from ControlPoint implementation/new diags
//
//	DESCRIPTION
/**
	Initialize the internal databases for port specified by \a pPortSpec.

	/return MN_OK on success
**/
//	SYNOPSIS:
cnErrCode MN_DECL mnInitializeProc(
		nodebool resetNodes,		// TRUE if a nodes are to reset
		nodebool singleNode,		// TRUE if we want to reset single node
		multiaddr addr,				// Net/Node to reset
		const portSpec *pPortSpec,	// Port information
		mnNetInvRecords *initInfo)	// Ptr to information return

{
	cnErrCode lastErr=MN_OK, initErr=MN_OK, startErr, netBreakErr = MN_OK;
	cnErrCode resetErr=MN_OK;
	netaddr cNum = NET_NUM(addr);	// Controller number
	nodebool foundNetworkNodes;

	nodebool diagsOff;

	// Bounds check controller destination
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	// If controller is open and auto-discover running we return
	// our normal "still looking" case.
	if (infcOnline(cNum) && infcAutoDiscoverRunning(cNum) && !resetNodes)
		return(MN_ERR_NO_NET_CONNECTIVITY);

	//_RPT3(_CRT_WARN, "%.1f mnInitializeProc(%d) reset=%d\n",
	//				 infcCoreTime(), cNum, resetNodes);
	infcFireNetEvent(cNum, NODES_RESETTING);
	infcSetInitializeMode(cNum, TRUE, MN_OK);

	// Halt the offline recovery tickler if running
	infcHaltAutoNetDiscovery(cNum);

	// Reset the inventory count and invalidate the inventory records
	mnNetInvRecords &theNet = SysInventory[cNum];
	// Create our port class for the location if we can
	switch (pPortSpec->PortType) {
	case CPM_COMHUB:
	case GENERIC_SERIAL:
		if (theNet.pPortCls)
			theNet.pPortCls->NetNumber(cNum);
		else
			theNet.pPortCls = new sFnd::SysCPMport(cNum);
		break;
	case MERIDIAN_CONMOD:
		break;
	default:
		_RPT3(_CRT_WARN,"%.1f mnInitializeProc(%d) no port class for this type %d\n",
						 infcCoreTime(), cNum, pPortSpec->PortType);
	}
	// Assume port is still open at this point, stop controller will 
	// definitively set this.
	theNet.clearNodes(false);
	int16 stackIn = theNet.Initializing;
	startErr = infcStopController(cNum);
	if (startErr == MN_OK) {
		startErr = infcStartController(cNum);
	}
	// We are always one away from here
	theNet.Initializing = stackIn;
		
	// Reset the global stop setup to the default state
	for (size_t i=0; i < MN_API_MAX_NODES; i++) {
		ShutdownInfo &sInfo = theNet.GroupShutdownInfo[NODE_ADDR(i)];
		sInfo.enabled = true;
		sInfo.statusMask.clear();
		//sInfo.theStopType.bits = 0;
		sInfo.theStopType.cpm.MotionAction = MG_STOP_STYLE_RAMP;
		sInfo.theStopType.cpm.Disable = true;
		sInfo.theStopType.cpm.MotionLock = true;
	}
	// Serial Port startup OK?
	if (startErr == MN_OK) {

		// Reset all the nodes once	if desired
		if (resetNodes && initErr == MN_OK) {
			// Start error will reflect reset failure
			resetErr = netReset(addr, singleNode ? FALSE : TRUE);
			if (resetErr != MN_OK) {
				infcErrInfo errInfo;
				_RPT3(_CRT_WARN,"%.1f mnInitializeProc(%d) reset failed code 0x%X\n",
								 infcCoreTime(), cNum, startErr);
				// Convert code to a more specific one and send failure
				// back to application.
				resetErr = MN_ERR_RESET_FAILED;
				errInfo.cNum = cNum;
				errInfo.errCode = resetErr;
				errInfo.node = MN_API_UNKNOWN_ADDR;
				// Force a message back to UI or application
				infcFireErrCallback(&errInfo, true);
			}
			// Insure failed reset gets net @ same rate
			lastErr = infcResetNetRate(cNum);
		}
		// Let stuff settle out
		infcSleep(100);
		// Kill any data that trickled in
		infcFlush(cNum);
		// Fire up the nodes and note the inventory count
		theNet.InventoryNow.NumOfNodes = 0;			// Start net @ 0
		initErr = netSetAddress(cNum, &theNet.InventoryNow.NumOfNodes);
		// Set boolean to reflect the basic presence of nodes
		if (initErr == MN_OK) {
			foundNetworkNodes = theNet.InventoryNow.NumOfNodes >= 1;
			// Set the network rate to requested rate
			initErr = infcSetNetRate(cNum, pPortSpec->PortRate);
			if (initErr != MN_OK) {
				// Unwind init stack and report error, too serious to run
				infcErrInfo errInfo;
				errInfo.cNum = cNum;
				errInfo.errCode = initErr;
				errInfo.node = MN_API_UNKNOWN_ADDR;
				infcSetInitializeMode(cNum, FALSE, initErr);
				// Force a message back to UI or application
				infcFireErrCallback(&errInfo, true);
				return(initErr);
			}
		}
		else {
			foundNetworkNodes = FALSE;
		}

		// Get system diagnostic blocker state
		netBreakErr = infcGetDiagBlock(cNum, &diagsOff);
		// If diagnostics are not blocked and we did not have
		// a successful "set address" bid, run diagnostics to see if
		// we can determine the problem.
		if (!diagsOff && initErr != MN_OK)  {
			// Run the network diagnostics to detect broken ring
			initErr = netBreakErr
				= infcBrokenRingDiag(cNum, foundNetworkNodes,
										  theNet.InventoryNow.NumOfNodes);
		}
		else {
			netBreakErr = MN_OK;		// Assume OK if not run
		}

		// If we failed the reset and there are net problems, return them
		// and create a dump file
		if (IsNetBreakErr(netBreakErr)) {
			infcTraceDumpNext(cNum);
			infcSetInitializeMode(cNum, FALSE, netBreakErr);
			//infcFireNetEvent(cNum, NODES_ONLINE_NO_TEST);
			return(netBreakErr);
		}
		// ---- END - Diagnostics: Run if node initialize not successful ----


		// We got some nodes, enumerate them
		if (initErr == MN_OK && foundNetworkNodes) {
			startErr = netEnumerate(cNum);
		}
		else
			lastErr = startErr = initErr;

		// Convert node command type errors into nothing bad. The node exists
		// and will talk.
		if (startErr >= MN_ERR_CMD_INTERNAL && startErr <= MN_ERR_CMD_ERR_END)
			startErr = MN_OK;

		switch(startErr) {
			case MN_OK:
				// Net break issues drive return error code
				lastErr = netBreakErr;
				// If otherwise OK but reset failed, note it
				if (lastErr == MN_OK && resetErr != MN_OK)
					lastErr = resetErr;
				infcFlushProc(cNum);
				// If problems, start the search to wait for it repair
				infcSetInitializeMode(cNum, FALSE, lastErr);
				if (lastErr != MN_OK)
					// Start looking for network to re-appear
					infcStartAutoNetDiscovery(cNum, lastErr);
				break;
			case MN_ERR_NO_NET_CONNECTIVITY:
			case MN_ERR_RESP_TIMEOUT:
				// Could not detect anything, assume we don't know
				theNet.OpenStateNext(OPENED_SEARCHING);
				if (initInfo != NULL) {
					*initInfo = SysInventory[cNum];
				}
				infcSetInitializeMode(cNum, FALSE, lastErr);
				// Start looking for network to re-appear
				infcStartAutoNetDiscovery(cNum, lastErr);
				return(lastErr);
			default:
				_RPT1(_CRT_WARN,
					"mnInitializeProc: netEnumerate failed code=0x%0X\n",
					lastErr);
				// Do final cleanup
				infcSetInitializeMode(cNum, FALSE, lastErr);
				break;
		}
	}

	else {
		lastErr = startErr;
		// We are done initialization now, let auto-discover run
		infcSetInitializeMode(cNum, FALSE, lastErr);
	}
	// NOTE: infcSetInitializeMode(FALSE) should all have been done

	// Last error reflects the final initialization outcome
	// Return whether this data is OK
	if (!((lastErr == MN_OK) || (lastErr == MN_ERR_TEST_INCOMPLETE))) 
		theNet.OpenStateNext(OPENED_SEARCHING);

	if (theNet.OpenState == OPENED_ONLINE) {
		infcUpdateInventory(cNum, &SysInventory[cNum]);
	}
	
//#ifdef DUMP_INIT
//	infcTraceDump(cNum, L"#mn_init.log");
//#endif
	// If they requested inventory copy, give them it
	if (initInfo != NULL) {
		*initInfo = SysInventory[cNum];
	}
	// Something wrong enroute, create dump
	if (theNet.OpenState != OPENED_ONLINE) {
		infcTraceDumpNext(cNum);
	}

	return(lastErr);
}
// 																	           *
//******************************************************************************

/// \endcond


//||||||||||||||||||||||||||||||||||||
// MULTINET INITIALIZATION INTERFACE
//||||||||||||||||||||||||||||||||||||


//******************************************************************************
//  NAME																       *
//		mnInitializeNets
//
//	DESCRIPTION:
/**
	This function initializes the Meridian system and is used to
	start communications on the channel(s).

	\deprecated	This function has been replaced with #mnInitializeSystem
	to allow initialization of systems with a combination of Meridian 
	and ClearPath-SC nodes.

	\param[in] resetNodes
		- True = Reset all individual nodes before establishing channel
		- False = Just establish communication
	\param[in] netCount
		Number of communication channels to initialize (e.g. 1..2)
	\param[in]	controllers[]
		Ptr to controller array of \a netCount elements. Each item in the array
		identifies the PortRate (speed) and the PortNumber (COM #) that
		the function will try to initialize.

	\return
		#cnErrCode; MN_OK if all channels and nodes are ready for
		interactions.  Some errors return bit-mapped fields to signal the
		channels that are broken.

	\see \ref SFinitPage
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnInitializeNets(
		nodebool resetNodes,
		nodeulong netCount,
		const controllerSpec controllers[])
{
	portSpec converted[NET_CONTROLLER_MAX];
	// Bounds check the table
	if (netCount>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);
	for (size_t cNum = 0; cNum < netCount; cNum++) {
		converted[cNum] = controllers[cNum];
	}
	return (mnInitializeSystem(resetNodes, netCount, converted));
}
//																			   *
//******************************************************************************


//******************************************************************************
//  NAME																       *
//		mnInitializeSystem
//
//	DESCRIPTION:
/**
	This function initializes a system consisting of Meridian and ClearPath-SC 
	nodes. The function arguments describe the serial ports to utilize, the
	type of nodes attached and the desired communications speed.

	\param[in] resetNodes
		- True = Reset all individual nodes before establishing channel
		- False = Just establish communication
	\param[in] netCount
		Number of communication channels to initialize (e.g. 1..2)
	\param[in]	controllers[]
		Ptr to controller array of \a netCount elements. Each item in the array
		identifies the PortRate (speed), the PortNumber (COM #) and
		the types of nodes to expect on that port. 

	\return
		#cnErrCode; MN_OK if all channels and nodes are ready for
		interactions.  Some errors return bit-mapped fields to signal the
		channels that are broken or the serial ports do not exist.

	\see \ref SFinitPage
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnInitializeSystem(
		nodebool resetNodes,
		nodeulong netCount,
		const portSpec controllers[])
{
	// Overall initializing state, 0==System ONLINE, else
	// initialization still in progress.
	extern int InitializingGlobal;
	unsigned cNum, initFailures, portFailures, baudFailures;
	cnErrCode initErr[NET_CONTROLLER_MAX], theErr;

	//DBG_LOG("mnInitializeSystem\n");
	// Bounds check the table
	if (netCount>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	// Insure past failed initializations are cleared and all ports closed
	for (cNum=0; cNum<NET_CONTROLLER_MAX; cNum++) {
		theErr = infcStopController(cNum);
		#ifdef _DEBUG
			if (theErr != MN_OK) {
				_RPT3(_CRT_WARN, "%.1f mnInitializeSystem stopping error @%d, code=0x%0X\n",
					  infcCoreTime(), cNum, theErr);
			}
		#endif
		SysInventory[cNum].clearNodes(true);
		SysInventory[cNum].Initializing = 0;
	}
	// Block error callbacks
	InitializingGlobal = 1;

	// Start with no ports
	SysPortCount = 0;
	baudFailures = initFailures = portFailures = 0;
	// Initialize them in turn
	for (netaddr i=0 ; i<netCount ; i++ ) {
		// Defer "online/offline/error" events until completed
		infcSetInitializeMode(i, TRUE, MN_OK);
		// Save our port and speed information for later
		infcSetPortSpecifier(i, &controllers[i]);
		// Perform net enumeration and node class initialization if possible
		initErr[i] = mnInitializeProc(resetNodes, false, MULTI_ADDR(i, 0),
									  &controllers[i], NULL);
		// How did this initialization go?
		switch (initErr[i]) {
			case MN_OK:
				// Great, allow us to go online immediately
				SysInventory[i].OpenStateNext(OPENED_ONLINE);
				break;
			case MN_ERR_PORT_PROBLEM:
				// Record as specific issue, serial port not found
				portFailures |= (1<<i);
				SysInventory[i].OpenStateNext(PORT_UNAVAILABLE);
				break;
			case MN_ERR_BAUD_NOT_SUPPORTED:
				// Record as specific issue, serial port baud rate not supported
				baudFailures |= (1<<i);
				SysInventory[i].OpenStateNext(PORT_UNAVAILABLE);
				break;
			default:
				// Record as generic sort of failure, port probably OK, nodes
				// not detected yet.
				initFailures |= (1<<i);
				SysInventory[i].OpenStateNext(OPENED_SEARCHING);
				break;

		}
		// Record our requested configuration count
		SysPortCount++;
	}
	// Insure past port objects are cleared
	for (size_t i=netCount; i<NET_CONTROLLER_MAX; i++) {
		delete SysInventory[i].pPortCls;
		SysInventory[i].pPortCls = (sFnd::IPort *)NULL;
	}
	// Return bit oriented failure code based on initialization results
	if (portFailures) {
		// Return failure source(s)
		theErr = (cnErrCode)(MN_ERR_PORT_FAILED_BASE + portFailures);
	}
	else if (baudFailures) {
		// Return failure source(s)
		theErr = (cnErrCode)(MN_ERR_BAUD_FAILED_BASE + baudFailures);
	}
	else if (initFailures) {
		// Return failure source(s)
		theErr = (cnErrCode)(MN_ERR_INIT_FAILED_BASE + initFailures);
	}
	else
		theErr = MN_OK;

	// Allow global operations to occur
	InitializingGlobal = 0;

	// Allow the online/offline/error events now
	for (netaddr i=0 ; i<netCount ; i++ ) {
		// Record the last error in case the port isn't online
		SysInventory[i].OfflineRootErr = initErr[i];
		// Allow successful ports to go online
		infcSetInitializeMode(i, FALSE, initErr[i]);
	}
	// Make sure background poller (re)starts
	if (theErr == MN_OK) {
		//DBG_LOG("mnInitializeSystem OK\n");
		for (netaddr i=0 ; i<netCount ; i++ ) {
			// Start keep alive polling if enabled
			if (SysInventory[i].KeepAlivePollEnable
			&& SysInventory[i].OpenState == OPENED_ONLINE) {
				infcBackgroundPollControl(i, TRUE);
			}
		}
	}
	#ifdef _DEBUG
	else { 
		// Start auto-discovery threads on the failed ports
		for (netaddr i = 0; i < netCount; i++) {
			if (SysInventory[i].OpenState == OPENED_SEARCHING)
				infcStartAutoNetDiscovery(i, initErr[i]);
		}
		_RPT1(_CRT_WARN, "mnInitializeSystem err=0x%X\n", theErr);
	}
	infcHeapCheck("mnInitializeSystem");
	#endif
	return(theErr);
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		mnRestartNet
//
//	DESCRIPTION:
/**
	Restart the previously-initialized network.

	\param[in] cNum Channel number to restart. The first channel is zero.
	\param[in] restartNodes
	- True = Reset all individual nodes
	- False = Just establish communication

	\return
		#cnErrCode; MN_OK if system restarted without problems.
**/
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnRestartNet(
	netaddr cNum,
	nodebool restartNodes)
{
	cnErrCode theErr;
	portSpec comPortSpec;

	DBG_LOG("mnRestartNet\n");
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);
	// Prevent interference
	nodebool pollIn = SysInventory[cNum].KeepAlivePollEnable;
	infcBackgroundPollControl(cNum, false);
	// Signal online->offline transition
	infcSetInitializeMode(cNum, TRUE, MN_OK);
	// Rebuild the network
	theErr = infcGetPortSpecifier(cNum, &comPortSpec);
	if (theErr ==  MN_OK) {
		theErr = mnInitializeProc(restartNodes, false, MULTI_ADDR(cNum, 0),
								  &comPortSpec, NULL);
	}
	// Signal offline->online or error condition as a result of restart
	infcSetInitializeMode(cNum, FALSE, theErr);
	// Restore background keep alive polling.
	SysInventory[cNum].KeepAlivePollEnable = pollIn;
	if (theErr == MN_OK && SysInventory[cNum].KeepAlivePollEnable 
	&& SysInventory[cNum].Initializing == 0) {
		cnErrCode pollErr = infcBackgroundPollControl(cNum, true);
		theErr = (theErr == MN_OK) ? pollErr : theErr; 
	}
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnRestartNode
//
//	DESCRIPTION:
/**
	Restart the specified node.

	\param[in] addr multi-net address of node to restart.

	\return
		#cnErrCode; MN_OK if node started without problems.

	\see \ref MULTI_ADDR for macros to build specific node addresses
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnRestartNode(
	multiaddr addr)
{
	cnErrCode theErr;
	portSpec comPortSpec;
	nodeaddr cNum = NET_NUM(addr);

	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);
	// Signal online->offline transition
	infcSetInitializeMode(cNum, TRUE, MN_OK);
	// Rebuild the network
	theErr = infcGetPortSpecifier(cNum, &comPortSpec);
	if (theErr ==  MN_OK) {
		theErr = mnInitializeProc(true, true, addr,
								  &comPortSpec, NULL);
		// Signal offline->online or error condition as a result of restart
		infcSetInitializeMode(cNum, FALSE, theErr);
	}
	return(theErr);
}
//																			   *
//******************************************************************************


/// \cond INTERNAL_DOC
//******************************************************************************
//	NAME																	   *
//		mnGetClassItem
//
//	DESCRIPTION:
/**
	Return a pointer to the node class record for this channel.

	\param[in] cNum Channel number to restart. The first channel is zero.
	\param[in] deviceType device type

	\return mnClassInfo Ptr to the class information record for
			the specified node type.  NULL is returned if no devices of this
			type can be present.
**/
//	SYNOPSIS:
mnClassInfo *mnGetClassItem(
	netaddr cNum,
	nodeIDs deviceType)
{
	switch(deviceType)  {
		case NODEID_MD:
			return &SysInventory[cNum].InventoryNow.mnDrvInfo;
		case NODEID_CP:
			return &SysInventory[cNum].InventoryNow.mnClearPathInfo;
		case NODEID_CS:
			return &SysInventory[cNum].InventoryNow.mnCpScInfo;
		default:
			return(NULL);
	}
}
//																			   *
//******************************************************************************
/// \endcond


//******************************************************************************
//	NAME																	   *
//		mnNetInventoryCount
//
//	DESCRIPTION:
/**
	Return the inventory count of the specified device type on the
	specified channel.

	\param[in] cNum Zero-based channel index.
	\param[in] deviceType Device type to find.
	\param[out] pCount When return is MN_OK, the user's \a pCount is
			    updated with the count of these \a deviceTypes present on
				this channel.
	\return #cnErrCode; If MN_OK returned, \a pCount was updated with count of
				\a deviceTypes found on this channel.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnNetInventoryCount(
	netaddr cNum,
	nodeIDs deviceType,
	nodeulong *pCount)
{
	mnClassInfo *ip;

	if (pCount == NULL)
		return(MN_ERR_VALUE);
	// Reset user's counter
	*pCount = 0;
	// Accumulate the counts
	ip = mnGetClassItem(cNum, deviceType);
	if (ip == NULL)
		return(MN_ERR_VALUE);
	*pCount = ip->count;
	return(MN_OK);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnNetInventoryRecord
//
//	DESCRIPTION:
/**
	Get the address list for the specified node type on the specified channel.
	The \a devices list is filled up to \a maxDevices.	This is useful for
	identifying the locations of certain types of nodes on a specific channel.

	Use \c mnNetInventoryCount to aid in memory allocation for the \a
	devices argument.

	\param[in] cNum Zero-based channel index.
	\param[in] deviceType Device type to find.
	\param[in] maxDevices Maximum number of devices in list.
	\param[out] devices Ptr to result list, filled in on successful completion.

	\return #cnErrCode; MN_OK if \a devices was updated with locations of \a
	deviceTypes on this channel.

	\see mnNetInventoryCount
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnNetInventoryRecord(
	netaddr cNum,
	nodeIDs deviceType,
	nodeulong maxDevices,
	multiaddr devices[])
{
	mnClassInfo *ip;
	// Make sure we have a place to write!
	if (devices == NULL)
		return(MN_ERR_VALUE);
	// Scan through the inventory records picking up items

	ip = mnGetClassItem(cNum, deviceType);
	if (ip == NULL)
		return(MN_ERR_VALUE);
	if (maxDevices<ip->count)
		return(MN_ERR_VALUE);
	// Copy addresses over
	memcpy(devices, ip->node, ip->count*sizeof(multiaddr));
	return(MN_OK);
}
//																			   *
//******************************************************************************


//******************************************************************************
// NAME																       	   *
//		mnNetNodeCount
//
// DESCRIPTION:
/**
	Return the number of nodes on the network.

	\param[in] cNum Zero-based channel index.
	\param[out] pCount Pointer to the count location.

	\return #cnErrCode; MN_OK if \a pCount was updated.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnNetNodeCount(
	netaddr cNum,
	nodeulong *pCount)
{
	if (pCount == NULL || cNum > SysPortCount)
		return(MN_ERR_VALUE);
	*pCount= SysInventory[cNum].InventoryNow.NumOfNodes;
	return(MN_OK);
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		mnSysInventoryCount
//
//	DESCRIPTION:
/**
	Return the system inventory count for this type of device.

	\param[in] deviceType Device type to find.
	\param[out] pCount Ptr to return count.

	\return #cnErrCode; MN_OK if \a pCount was updated.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnSysInventoryCount(
	nodeIDs deviceType,
	nodeulong *pCount)		// Ptr to result
{
	unsigned i;
	mnClassInfo *ip;

	// Prevent crash
	if (!pCount)
		return(MN_ERR_VALUE);
	// Reset user's count
	*pCount = 0;
	for (i=0; i<SysPortCount; i++) {
		// Accumulate the counts
		ip = mnGetClassItem(i, deviceType);
		if (ip == NULL)
			return(MN_ERR_VALUE);
		*pCount += ip->count;
	}
	return(MN_OK);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnSysInventoryRecord
//
//	DESCRIPTION:
/**
	Get the address list for all the specified node types in the system. The
	\a devices list is filled up to \a maxDevices.  This is useful for
	identifying the locations of certain types of nodes in the system.

	Use \c mnSysInventoryCount to aid in memory allocation for the \a
	devices argument.

	\param[in] deviceType Device type to find.
	\param[in] maxDevices Maximum number of devices in list
	\param[out] devices Ptr to user's list of addresses in the system of this
		   type, filled in up to \a maxDevices.

	\return #cnErrCode; MN_OK if \a devices was updated with locations
	of \a deviceTypes.

	\see mnSysInventoryCount
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnSysInventoryRecord(
	nodeIDs deviceType,			// Which device type
	nodeulong maxDevices,	// Maximum number of devices in list
	multiaddr devices[])		// Ptr to result list
{
	unsigned i;
	mnClassInfo *ip;
	netaddr cNum;
	// Make sure we have a place to write!
	if (devices == NULL)
		return(MN_ERR_VALUE);
	// Scan through the inventory records picking up items

	for (i=0; i<SysPortCount; i++) {
		// Get this class record for this net
		cNum = i;
		ip = mnGetClassItem(cNum, deviceType);
		if (ip == NULL)
			return(MN_ERR_VALUE);
		if (maxDevices<ip->count)
			return(MN_ERR_VALUE);
		// Copy addresses over
		memcpy(devices, ip->node, ip->count*sizeof(multiaddr));
		// Move pointer down
		devices+=ip->count;
	}
	return(MN_OK);
}
//																			   *
//******************************************************************************



//******************************************************************************
//  NAME																       *
//		mnSysNodeCount
//
//  DESCRIPTION:
/**
	Return the count of all nodes detected in the system.

	\param[out] pCount Ptr to returned count area.

	\return #cnErrCode; MN_OK if \a pCount was updated with total system
				node count.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnSysNodeCount(
			nodeulong *pCount)		// Ptr to result area
{
	unsigned i;
	nodeulong thisNetCnt;
	cnErrCode theErr;

	// Prevent crash on stupid
	if (!pCount)
		return (MN_ERR_VALUE);
	// Start with no nodes
	*pCount = 0;
	for (i=0; i<SysPortCount; i++) {
		if ((theErr = mnNetNodeCount(i, &thisNetCnt)) == MN_OK) {
			*pCount += thisNetCnt;
		}
		else
			return(theErr);
	}
	return(MN_OK);
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		mnShutdown
//
//	DESCRIPTION:
/**
	Shutdown the networks and release any resources.

	\return #cnErrCode; MN_OK if system is shutdown and ports are released.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL mnShutdown(void)
{
	cnErrCode theErr, compositeErr = MN_OK;

	//_RPT1(_CRT_WARN, "%.1f mnShutdown started\n", infcCoreTime());
	for (netaddr i=0; i<NET_CONTROLLER_MAX; i++) {
		if (infcOnline(i))
			// Tell everyone to stop using us
			infcFireNetEvent(i, NODES_STOP_NET_CONFIG);
		// Test for NCS and insure complete shutdown if not in special mode
		theErr = infcStopController(i);
		if (theErr != MN_OK && compositeErr == MN_OK)
			compositeErr = theErr;
	}
	SysPortCount = 0;
	//_RPT1(_CRT_WARN, "%.1f mnShutdown done\n", infcCoreTime());
	return(compositeErr);
}
//																			   *
//******************************************************************************


// End of Group: SetupGrp
/// @}
// cond ISC_DOC
/// \endcond

//==============================================================================
//	END OF FILE meridianNet.c
//==============================================================================

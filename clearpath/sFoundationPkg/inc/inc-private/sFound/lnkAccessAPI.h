//****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/lnkAccessAPI.h $
// $Revision: 127 $ $Date: 12/09/16 1:46p $
// $Workfile: lnkAccessAPI.h $
//
// DESCRIPTION:
/**
	\file 
	\brief Definitions and function prototypes for the low
	level setup functions used to set up Meridian links and serial port
	access.

	\ingroup SetupGrp
	@{

	This header file describes the public data types and API used
	to access the link layer of the network.

**/
// 	The items in this file should also isolate platform specific 
// 	implementations.
//		
//
// CREATION DATE:
//		2/11/1998 15:19:31 - was ioCore.h
//		6/11/2008 10:11:00 - renamed lnkAccessAPI.h
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-1999, 2009-2010  Teknic, Inc.  All rights reserved.
//		
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			 *
//****************************************************************************

#ifndef __LNKACCESSAPI_H__
#define __LNKACCESSAPI_H__

//***************************************************************************
// NAME																		*
// 	lnkAccessAPI.h headers
//
	#include "pubMnNetDef.h"
	#include "mnErrors.h"
	#include "mnDiags.h"

	#include "pubCoreRegs.h"
	#include "pubDataAcq.h"
	#include "pubIscRegs.h"
	#include "pubDataAcq.h"
	#include "pubMotion.h"
	#include "pubNetAPI.h"

	#include <sys/types.h>
	#include <sys/timeb.h>
	#include "TeknicDevID.h"
	#if defined(_WIN32)||defined(_WIN64)
		#include <sys/types.h>
		#ifndef MAX_PATH
			 //#include "windows.h"
		#endif
 	#else
 		#include <string.h>
	#endif
// 																	         *
//****************************************************************************


//***************************************************************************
// NAME																		*
// 	lnkAccessAPI.h constants
//	
//#define USE_ACTIVEX					// Define for ActiveX support
/// \cond (INTERNAL_DOC||ISC_DOC)
/// Extract the physical node address from multinet address selector
#define NODE_ADDR(x) ((x)&(MN_API_MAX_NODES-1))
/// Extracts the net number from the multinet address
#define NET_NUM(multiAddr) (((multiAddr)/MN_API_MAX_NODES))
/// Create a multinet address from a link number and physical address
#define MULTI_ADDR(net, node) ((multiaddr)(((netaddr)(net))*MN_API_MAX_NODES)+(nodeaddr)(node))
/// \endcond

/// \cond INTERNAL_DOC
// Unknown address marker
const multiaddr MN_API_UNKNOWN_ADDR = (multiaddr)0xffffffff;

// This list should match the TekNetLibX.NetAdapter list
typedef enum NetworkChanges {
    NODES_OFFLINE = 0,
    NODES_ONLINE = 1,
    NODES_RESETTING = 2,
    NODES_SENDING = 3,
    NODES_NO_NET_CONTROLLER = 4,
    NODES_CHANGING_NET_CONTROLLER = 5,
    NODES_NO_PORT = 6,
    NODES_ONLINE_NO_TEST = 7,
	NODES_FLASH_MODE = 8,
	NODES_STOP_NET_CONFIG = 9,
	NODES_BAUD_UNSUPPORTED = 10,
	NODES_BAUD_CHANGING = 11,
	NODES_DRIVER_ONLINE = 12
} NetworkChanges;



/// \endcond
//																			  *
//*****************************************************************************

// 																	         *
//****************************************************************************
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//							   NODE/NET INFORMATION
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/// \cond INTERNAL_DOC
// This structure holds the serial port information.
typedef struct _serPortInfo {
	char mfg[64];
	char adapter[64];
} serPortInfo;
/// \endcond

#ifdef __cplusplus
extern "C" {				// Use C linkage for wide flexibility of targets
#endif

/// \cond INTERNAL_DOC
// Open the ports and setup for communications at standard speed
MN_EXPORT cnErrCode MN_DECL infcStartController(
		netaddr cNum);

// Close down the communications port and release all resources
MN_EXPORT cnErrCode MN_DECL infcStopController(
		netaddr cNum);
		
// Reset the network speed to default rate		
MN_EXPORT cnErrCode MN_DECL infcResetNetRate(
		netaddr cNum);
		
// Set the network speed to given rate
MN_EXPORT cnErrCode MN_DECL infcSetNetRate(
		netaddr cNum,
		netRates newRate);
		
// Main command/response execution API with logging.
MN_EXPORT cnErrCode MN_DECL infcRunCommand(
		netaddr cNum,						// Port index
		packetbuf *theCommand,				// Ptr to buffered command
		packetbuf *theResponse);			// Ptr to response area
		
// Microsecond level time stamp
MN_EXPORT double MN_DECL infcCoreTime(void);

// Millisecond level thread sleep function
MN_EXPORT void MN_DECL infcSleep(Uint32 milliseconds);

// Return TRUE if this network is online and able to communicate with nodes.
MN_EXPORT nodebool MN_DECL infcOnline(
		netaddr cNum);

// Access standard error message strings for cnErrCodes.
MN_EXPORT cnErrCode MN_DECL infcErrCodeStrA(
	cnErrCode lookupCode,
	Uint16 maxLen,
	char resultStr[]);

MN_EXPORT cnErrCode MN_DECL infcErrCodeStrW(
	cnErrCode lookupCode,
	Uint16 maxLen,
	WCHAR resultStr[]);

// Driver versioning information and location
MN_EXPORT nodeulong MN_DECL infcVersion(void);
MN_EXPORT void MN_DECL infcFileNameA(
		char *fname, long len);
		
// Get port information about our controller
MN_EXPORT cnErrCode MN_DECL infcGetPortInfo(
		const char *portName, 
		serPortInfo *portInfo);

// Returns OS version string
MN_EXPORT void MN_DECL infcOSversionW(
		nodeulong strSize, 
		nodeUNIstr pszOS);

MN_EXPORT void MN_DECL infcOSversionA(
		nodeulong strSize, 
		char *pszOS);

// Get the channel type for this controller
MN_EXPORT mnNetSrcs MN_DECL infcGetNetSource(
		netaddr cNum);

// Get our thread ID
MN_EXPORT Uint64 MN_DECL infcThreadID();

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LOW LEVEL DRIVER ACCESS API
//		These functions are generally used for diagnostics
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Flush all tracking and network data
nodeulong MN_DECL infcFlush(
		netaddr cNum);
		
// Low Level command transmission without tracking
MN_EXPORT cnErrCode MN_DECL infcSendCommand(
		netaddr cNum,					// Port Index
		packetbuf *theCommand);			// Ptr to buffered command

// Get Low Level logged response without tracking
MN_EXPORT cnErrCode MN_DECL infcGetResponse(
		netaddr cNum,					// Port Index
		packetbuf *theResponse);		// Ptr to response area


// Wait for all communications from this thread to stop
void MN_DECL infcWaitForNetStop(netaddr cNum);

// Get/Set Network Auto-Discovery mode
MN_EXPORT cnErrCode MN_DECL infcSetAutoNetDiscovery(
		netaddr cNum, 
		nodebool AutoNetDiscovery);

MN_EXPORT cnErrCode MN_DECL infcGetAutoNetDiscovery(
		netaddr cNum, 
		nodebool *AutoNetDiscovery);

// Control the background polling thread
MN_EXPORT cnErrCode MN_DECL infcBackgroundPollControl(
		netaddr cNum,
		nodebool start);

// Set the cmd queue limit for the network
MN_EXPORT cnErrCode MN_DECL infcSetCmdQueueLimit(
		netaddr cNum,				// Network 
		nodeulong nCmds);			// Number of commands allowed at once
		
MN_EXPORT cnErrCode MN_DECL infcGetOnlineState(
		netaddr cNum,
		nodebool *state);

// Return expert online state
MN_EXPORT cnErrCode MN_DECL infcGetOnlineStateEx(
		netaddr cNum,
		openStates *state);
				
// Low level packet formatter for the network set address command
cnErrCode MN_DECL infcSetAddressFmt(
		nodeulong startingAddr,		// Net address start here
		packetbuf *theCmd);

MN_EXPORT nodelong MN_DECL infcDbgDepth(netaddr cNum);

// Get serial port stats
MN_EXPORT cnErrCode MN_DECL infcSerialStats(
		netaddr cNum, 
		nodeulong &rxCnt, 
		nodeulong &txCnt,
		nodeulong &rxPktCnt,
		nodeulong &txPktCnt);

// NOTE: nc aka cNum and node address are long for ActiveX alignment and compatibility
// reasons.
typedef struct _packetbuf18 {
	int32 bufferSize;
	nodechar buffer[18];
} packetbuf18;

///\endcond
/**
	Error Callback information
**/
typedef struct _infcErrInfo { 
	
	Uint32 cNum;			///< Controller reporting error
	Uint32 node;			///< Node on that network
	cnErrCode errCode;		///< Error code reported
	packetbuf18 response;	///< Command that caused error
} infcErrInfo;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DIAGNOSTICS & CONTROL INTERFACES
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
///\cond INTERNAL_DOC
// Command Trace Interface
// Trace enable/disable
MN_EXPORT void MN_DECL infcSetTraceEnable(
		netaddr cNum, 
		nodebool newState);
///\endcond
// Save information / UNICODE filename
MN_EXPORT cnErrCode MN_DECL infcTraceDump(
		netaddr cNum, 
		const wchar_t *filePath);

// Save information / ANSI filename
MN_EXPORT cnErrCode MN_DECL infcTraceDumpA(
		netaddr cNum, 
		const char *filePath);

// Get errors self-reported from nodes
MN_EXPORT cnErrCode MN_DECL infcGetBackgroundErrs(
	multiaddr theMultiAddr,
	nodebool *pIsSet,
	mnNetDiagStats *pResult);	

// Get errors detected at host port		
MN_EXPORT cnErrCode MN_DECL infcGetHostErrStats(
	netaddr cNum,
	nodebool *pIsSet,
	mnNetDiagStats *pResult);
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// C EVENT CALLBACKS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Provides callbacks on the following events:
//	* On network error detected 
//	* After response from command detected
//  * On a node's parameter being changed, signifing that the local cache
//	  should be invalidated
//	* Network Online/Offline state change
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Callback function definitions
							  
// ---------------------
// On Parameter Change
///\cond INTERNAL_DOC

// Callback types for status and parameter value changes
// Parameter change info block. NOTE: node and net are not necessarily the
// same as internal types due to VB/C structure aligment issues.
typedef struct _mnParamChg {
	double newValue;				// New "numeric" value
									// NOTE: VB/C aligment forces double first
	nodeulong node;					// Node registering change (multiAddr)
	nodeulong net;					// Net registering change
	nodeparam paramNum;				// Parameter changing
} mnParamChg;

typedef void (nodeCallback *paramChangeFunc)
				(const mnParamChg *chgItem);

///\endcond
// ---------------------
/**
	\typedef infcErrCallback
	\brief Error Callback Function Type

	The #infcSetErrorFunc registers a function of this type that will
	be called whenever the S-Foundation detects an error.
**/
typedef void (nodeCallback *infcErrCallback)(
						infcErrInfo *infcErrInfo);
						
MN_EXPORT infcErrCallback MN_DECL infcSetErrorFunc(
	  		infcErrCallback newFunc);				// Ptr to callback function

// ---------------------
// Post-command execution

///\cond INTERNAL_DOC
// Information returned by the completion function. Provides central
// statistics for QC and tuning purposes.
typedef struct _mnCompletionInfo {
	packetbuf cmd;					///< Original command if known, else NULL
	packetbuf *pResp;				///< Response if known, else NULL
	double execTime;				///< Execution time less pacing
	double pacedTime;				///< Executoon time inclusive of pacing
	double sendTime;				///< Time required to send command
	double rxTime;					///< Time required to get response
	nodeulong ringDepth;			///< Ring depth
	long spare[4];					// Padding for future
	#ifdef __cplusplus
	// Construction initialization
	_mnCompletionInfo() {
		pacedTime = rxTime = execTime = sendTime = 0;
		ringDepth = 0;
		pResp = NULL;
		spare[0] = spare[1] = spare[2] = spare[3] = 0;
	};
	#endif
} mnCompletionInfo;

typedef void (nodeCallback *infcCmdCompleteCallback)(
						netaddr cNum,
					    const mnCompletionInfo *pStats);
					    
MN_EXPORT infcCmdCompleteCallback MN_DECL infcSetCmdCompleteFunc(
	  		infcCmdCompleteCallback newFunc);	// Ptr to callback function

// ---------------------
// Change of parameter; need to invalidate cache

typedef void (nodeCallback *infcInvalCacheCallback)(
						netaddr cNum,
					    nodeaddr node);
					    
MN_EXPORT infcInvalCacheCallback MN_DECL infcSetInvalCacheFunc(
	  		infcInvalCacheCallback newFunc);	// Ptr to callback function

// ---------------------
// Change of network state
typedef void (nodeCallback *infcNetStateCallback)(
						netaddr cNum,
						nodebool online);
						
MN_EXPORT infcNetStateCallback MN_DECL infcSetNetStateFunc(
	  		infcNetStateCallback newFunc);		// Ptr to callback function



// ---------------------------------
// BRAKE INTERFACE API
// ---------------------------------

// Direct user control of the indicated brake number
// NOTE: If the brake mode is not set to BRAKE_AUTOCONTROL the
//			auto-brake feature will not have control of the brake
MN_EXPORT cnErrCode MN_DECL infcBrakeControlSet(
		netaddr cNum,
		size_t brakeNum,
		BrakeControls brakeMode);

MN_EXPORT cnErrCode MN_DECL infcBrakeControlGet(
		netaddr cNum,
		size_t brakeNum,
		BrakeControls *pBrakeMode);


// When theNode becomes disabled, engage the indicated brake number
MN_EXPORT cnErrCode MN_DECL infcAutoBrakeSet(
		netaddr cNum,
		size_t brakeNum,
		nodebool enabled,
		multiaddr theMultiAddr);

MN_EXPORT cnErrCode MN_DECL infcAutoBrakeGet(
		netaddr cNum,
		size_t brakeNum,
		nodebool *pEnabled,
		multiaddr *pTheMultiAddr);


MN_EXPORT cnErrCode MN_DECL infcCommUnitPowered(
		netaddr cNum,
		nodebool *pIsPowered);



// ---------------------------------
// NODESTOP ON SHUTDOWN API
// ---------------------------------
// If any of the nodes in the group have a event included in the mask
// send the specified nodestop type to each of the nodes in the group
// setting the mask
// NOTE: This feature waits for all shutdown conditions to clear to re-arm

MN_EXPORT cnErrCode MN_DECL infcShutdownInfoSet(
		multiaddr theAddr,
		const ShutdownInfo *pTheInfo);

MN_EXPORT cnErrCode MN_DECL infcShutdownInfoGet(
		multiaddr theAddr,
		ShutdownInfo *pTheInfo);

MN_EXPORT cnErrCode MN_DECL infcShutdownInitiate(
		netaddr cNum);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DATA ACQUISITION INTERFACE. 
// Provide a polled data acquisition FIFO interface. This needs to be
// VB friendly.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		
MN_EXPORT cnErrCode MN_DECL infcGetDataAcqPt(
		multiaddr multiAddr, 
		nodeulong ptsToRead, 
		mnDataAcqPt pTheDataAcqPt[],
		nodeulong *pPtsRead);
		 
MN_EXPORT cnErrCode MN_DECL infcGetDataAcqPtCount(
		multiaddr multiAddr, 
		nodeulong *pPointCount);
		
MN_EXPORT cnErrCode MN_DECL infcFlushDataAcq(
		multiaddr multiAddr);
/// \endcond

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// ATTENTION (NET INTERRUPT) INTERFACE. 
// Provide a polled interface for getting the oldest attention request.
// This needs to be VB friendly.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Detected Attention Packet Function
MN_EXPORT cnErrCode MN_DECL infcNetGetAttnReq(
		netaddr cNum,								// Channel number
		mnAttnReqReg *pAttnReq);					// Ptr Oldest request copy

// Flush all attentions detected so far
MN_EXPORT cnErrCode MN_DECL infcNetAttnFlush(
		netaddr cNum);								// Channel number
		
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// VB POLLED INTERFACE. 
// This is necessary to provide thread safe interface with this ActiveX 
// client which want's to Apartment Marshall anything we want to do and 
// can only serially processes Events and Function calls in a way that can
// lead to dead-lock.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/// \cond INTERNAL_DOC
// Polled error interface
MN_EXPORT nodebool MN_DECL infcGetNextError(
			netaddr cNum, 
			infcErrInfo *pErrInfo);
// Polled net change interface
MN_EXPORT nodebool MN_DECL infcGetNextNetChange(
			netaddr cNum, 
			NetworkChanges *pNetChange);
// Polled param changed check interface
MN_EXPORT nodebool MN_DECL infcParamsHaveChanged(
			multiaddr multiAddr);

// Polled autonomous error reporting interface
MN_EXPORT cnErrCode MN_DECL infcGetNetErrorStats(
	multiaddr theMultiAddr,			// Node to lookup
	nodebool *pIsSet,				// Updated to TRUE if set
	mnNetDiagStats *pResult);		// Result area				

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DEBUGGING INTERFACE
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Check heap for corruption
MN_EXPORT void MN_DECL infcHeapCheck(const char *msg);
// Force unload of the library
MN_EXPORT bool MN_DECL infcUnloadLib();
/// \endcond
#ifdef __cplusplus
}
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// NATIVE C++ INTERFACE
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifdef __cplusplus
#include <vector>
#include <string>
// Return a list of SC hubs currently connected
MN_EXPORT cnErrCode MN_DECL infcGetHubPorts(
		std::vector<std::string> &comHubPorts);
#endif

/// @}
#endif //ifndef __LNKACCESSAPI_H__
// 																			   *
//******************************************************************************

//============================================================================== 
//	END OF FILE lnkAccessAPI.h
//==============================================================================
 

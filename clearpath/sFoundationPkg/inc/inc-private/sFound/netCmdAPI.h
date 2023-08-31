//******************************************************************************
// NAME
//		netCmdAPI.h
//
// DESCRIPTION:
//		This file decribes the network command level interface. It provides
//		a common API for executing node commands and accessing network 
//		facilities such as high-priority trigger commands and the like.
//
// CREATION DATE:
//		09/22/2004 13:44:07 - was netCoreAPI.h
//		06/11/2008 10:33:00 - renamed to avoid CP conflicts
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2004-2009  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			   *
//******************************************************************************
#ifndef __NETCMDAPI_H__
#define	__NETCMDAPI_H__


//******************************************************************************
// NAME																	       *   
// 	netCmdAPI.h headers
//
	#include "pubMnNetDef.h"
	#include "pubCoreRegs.h"
	#include "mnErrors.h"
	#include "TeknicDevID.h"
	#include "mnDiags.h"
//																			   *
//******************************************************************************



//******************************************************************************
// NAME																	       *   
// 	netCmdAPI.h constants
//
//

/**
	This enumeration choses the format for the configuration file.
**/
typedef enum _configFmts {
	CLASSIC,							///< Creates setup compatible file
	ALL_NON_VOLATILE,					///< CLASSIC + all non-volatile items
	CLASSIC_NO_RESET					///< Setup compatible file, no reset at end
} configFmts;
//																			   *
//******************************************************************************



//******************************************************************************
// NAME																	       *   
// 	netCmdAPI.h function prototypes
//
// API functions
#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------
// NETWORK SETUP
//----------------------------------
// Issue the set address control packet
MN_EXPORT cnErrCode MN_DECL netSetAddress(
		netaddr cNum,
		nodeulong *maxNode);

// Issue the reverse set address control packet
MN_EXPORT cnErrCode MN_DECL netReverseAddress(
		netaddr cNum,
		nodeaddr nodeCountLessOne);
		
// Scan the network and build up the inventory lists
MN_EXPORT cnErrCode MN_DECL netEnumerate(
		netaddr cNum);

//----------------------------------
// COMMAND INTERFACE
//----------------------------------
// Run a command and get the response on <cNum>
MN_EXPORT cnErrCode MN_DECL netRunCommand(
		netaddr cNum,				// Controller number
		packetbuf *theCommand,		// pointer to filled in command
		packetbuf *theResponse);	// pointer to response area
					
//----------------------------------
// PARAMETER ACCESS AND MANIPULATION
//----------------------------------
// Get a parameter with various options
MN_EXPORT cnErrCode MN_DECL netGetParameter(
		multiaddr theMultiAddr, 		// device address
		nodeparam theParam, 		// parameter index
		packetbuf *paramInfo);		// ptr for returned param

// Set a parameter via a number of methods
MN_EXPORT cnErrCode MN_DECL netSetParameter(
		multiaddr theMultiAddr, 		// node address
		nodeparam theParamNum, 		// parameter index
		packetbuf *pNewValue);		// ptr to new parameter data					

MN_EXPORT cnErrCode MN_DECL netSetParameterEx(
		multiaddr theMultiAddr, 		// node address
		nodeparam theParamNum, 		// parameter index
		packetbuf *pNewValue);		// ptr to new parameter data
		
// Force re-read of parameter on next request
MN_EXPORT cnErrCode MN_DECL netClrParamCache(
		multiaddr theMultiAddr,
		mnParams parameter);

// C++ Overloaded parameter accessors with multiaddr argument for numeric
// parameters.
MN_EXPORT cnErrCode MN_DECL netGetParameterDbl(
		multiaddr theMultiAddr,
		mnParams theParamNum,
		double *pValue);
MN_EXPORT cnErrCode MN_DECL netSetParameterDbl(
		multiaddr theMultiAddr,
		mnParams theParamNum,
		double value);

typedef struct _paramInfo paramInfo;
typedef struct _paramValue paramValue;

// Get info enhanced value
MN_EXPORT cnErrCode MN_DECL netGetParameterInfo(
		multiaddr multiAddr,		// Node address
		nodeparam theParam,			// Parameter number
		paramInfo *pRetValInfo,		// Ptr to returning value info or NULL
		paramValue *pRetVal);		// Ptr to returning value or NULL
	
//----------------------------------
// ALERT INTERFACE
//----------------------------------
// Time stamp epoch encoding starts here.
#define MERIDIAN_EPOCH "2010-01-01T00:00:00Z"
// Number of seconds since 1970-01-01T00:00:00Z to MERIDIAN_EPOCH
#define MERIDIAN_EPOCH_CTIME 1262304000

// Clear the selected node's alert log history
MN_EXPORT cnErrCode MN_DECL netAlertLogClear(
		multiaddr theMultiAddr);		// Node address

// Read an alert log item. Set <restart> to TRUE get oldest, FALSE to get remaining.
MN_EXPORT cnErrCode MN_DECL netAlertLogGet(
		multiaddr theMultiAddr,		// Node address
		nodebool restart,			// Restart the list from oldest
		nodebool *pEntryExists,		// Updated with TRUE if <pEntry> OK
		alertLogEntry *pEntry);		// Update user's buffer	
		
// Set Alert log epoch time
MN_EXPORT cnErrCode MN_DECL netAlertLogEpoch(
		multiaddr theMultiAddr);		// Node address

// Clear and/or acknowledge non-serious Alerts
MN_EXPORT cnErrCode MN_DECL netAlertClear(
		multiaddr theMultiAddr);		// Node address

// Get the current alert register
MN_EXPORT cnErrCode MN_DECL netGetAlertReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pAlertReg);		// Current setting

// Get and clear the current warning register
MN_EXPORT cnErrCode MN_DECL netGetWarnReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pWarnReg);		// Current setting

//----------------------------------
// STATUS REGISTER INTERFACE
//----------------------------------
MN_EXPORT cnErrCode MN_DECL netGetAttnStatusRiseReg(
			multiaddr theMultiAddr,			// Node address
			mnStatusReg *pReg);				// Current setting

MN_EXPORT cnErrCode MN_DECL netGetStatusRTReg(
			multiaddr theMultiAddr,			// Node address
			mnStatusReg *pReg);				// Current setting

MN_EXPORT cnErrCode MN_DECL netGetStatusAccumReg(
			multiaddr theMultiAddr,			// Node address
			mnStatusReg *pReg);				// Current setting

//----------------------------------
// DATA ACQUISITION INTERFACE
//----------------------------------
// Start acquisition
MN_EXPORT cnErrCode MN_DECL netEnterDataAcqMode(
		multiaddr theMultiAddr,
		double *startTime);
	
// Stop Acquisition
MN_EXPORT cnErrCode MN_DECL netExitDataAcqMode(
		multiaddr theMultiAddr);
	
//----------------------------------
// NODE STOPS
//----------------------------------
// Stop command		
MN_EXPORT cnErrCode MN_DECL netNodeStop(
		multiaddr theMultiAddr, 	// node address
		mgNodeStopReg stopType, 		// stop strategy
		nodebool broadcast);		// hi prio broadcast


//----------------------------------
// TRIGGER INTERFACES
//----------------------------------
// Trigger a single or group of nodes
MN_EXPORT cnErrCode MN_DECL netTrigger(
		netaddr cNum,				// Net number
		nodeaddr nodeOrGroupAddr,	// Node or Group Addr
		nodebool usingGroup);		// Node(s) selection(s)

//----------------------------------
// PLA INTERFACES
//----------------------------------
// Clear PLA fuses
MN_EXPORT cnErrCode MN_DECL netPLAclear(
		multiaddr theMultiAddr); 	// node address

// Get PLA fuses
MN_EXPORT cnErrCode MN_DECL netPLAget(
		multiaddr theMultiAddr, 	// node address
		plaFuseBuffer *pFuses);		// Ptr to fuse buffer
		
// Set PLA fuses
MN_EXPORT cnErrCode MN_DECL netPLAset(
		multiaddr theMultiAddr, 	// node address
		plaFuseBuffer *pFuses);		// Ptr to fuse buffer


//----------------------------------
// DIAGNOSTICS INTERFACE
//----------------------------------
MN_EXPORT cnErrCode MN_DECL netGetErrStats(
		multiaddr theMultiAddr,
		mnNetErrStats *pNetStats);
	
// Run a network diagnostic run
MN_EXPORT cnErrCode MN_DECL netDiagsRun(
		netaddr theNet,
		mnNetDiagResults *stats);		// Net stats results

// Return driver's diagnostic run results
MN_EXPORT cnErrCode MN_DECL netDiagsGet(
		netaddr theNet,
		mnNetDiagResults *stats);		// Net stats results

MN_EXPORT cnErrCode MN_DECL netDiagsClr(
		netaddr theNet);

// Motion Data Collected
MN_EXPORT cnErrCode MN_DECL netGetDataCollected(
		multiaddr theMultiAddr,			// Destination node
		iscDataCollect *pReturnData);	// Returned data

//----------------------------------
// ACCESS LEVEL INTERFACE
//----------------------------------
MN_EXPORT cnErrCode MN_DECL netSetNetAccessLvl(
		netaddr theNet,
		mnNetStatus theAccessLevel);

MN_EXPORT cnErrCode MN_DECL netGetNetAccessLvl(
		netaddr theNet,
		mnNetStatus *pAccessLevel);

// Change and get the node's feature access level.
MN_EXPORT cnErrCode MN_DECL netSetNodeAccessLvl(
	multiaddr theMultiAddr,				// Destination node
	mnNetStatus newAccessLevel);

MN_EXPORT cnErrCode MN_DECL netGetNodeAccessLvl(
	multiaddr theMultiAddr,				// Destination node
	mnNetStatus *pAccessLevel);

//--------------------------------------
// NODE MANAGEMENT INTERFACE
//--------------------------------------
// Get the part number of the specified node into user's buffer
MN_EXPORT cnErrCode MN_DECL netGetPartNumber(
		multiaddr theMultiAddr,
		char *partNumBuf,
		Uint16 bufferSize);
		
// Get the type of device present at <theMultiAddr>
MN_EXPORT nodeIDs MN_DECL netGetDevType(
		multiaddr theMultiAddr);

//--------------------------------------
// USER ID MANAGEMENT INTERFACE
//--------------------------------------
MN_EXPORT cnErrCode MN_DECL netSetUserID(
		multiaddr theMultiAddr,
		const char *pNewID);

MN_EXPORT cnErrCode MN_DECL netGetUserID(
		multiaddr theMultiAddr,
		char *pCurrentID,
		Uint16 maxBufSize);

//--------------------------------------
// USER DESCRIPTION MANAGEMENT INTERFACE
//--------------------------------------
MN_EXPORT cnErrCode MN_DECL netSetUserDescription(
		multiaddr theMultiAddr,
		const char *pNewDescr);

MN_EXPORT cnErrCode MN_DECL netGetUserDescription(
		multiaddr theMultiAddr,
		char *pUserDescrStr,
		Uint16 maxBufSize);

//--------------------------------------
// CONFIGURATION FILE INTERFACE
//--------------------------------------
MN_EXPORT cnErrCode MN_DECL netConfigSave(
		multiaddr theMultiAddr,
		configFmts saveFmt,
		const char *pFilePath);

MN_EXPORT cnErrCode MN_DECL netConfigLoad(
		multiaddr theMultiAddr,
		configFmts loadFmt,
		const char *pFilePath);
#ifdef __cplusplus
}
#endif
//																			   *
//******************************************************************************
#endif  // #ifndef __NETCMDAPI_H__

//============================================================================== 
//	END OF FILE netCmdAPI.h
//==============================================================================

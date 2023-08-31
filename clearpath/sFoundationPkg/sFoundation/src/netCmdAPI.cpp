//******************************************************************************
// $Archive: /ClearPath SC/User Driver/src/netCmdAPI.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: netCmdAPI.cpp $
//
// DESCRIPTION:
/**
	\file
	\brief 	Link Function Programming Interface Implementation
	This module implements functions that act on all nodes or generically
	access common features among all node types.

	\defgroup LinkGrp Link Function Programming Interface
	@{
	\brief Link oriented feature functions. These act upon all nodes or
	target nodes generically. Link oriented features such as, Trigger
	and Stop, are accessed by functionality in the group of functions.
	- Include "meridianHdrs.h" in your project to use these functions.

	This module defines the common link command set and utilities that all
	network devices will utilize in the programming interface. It
	provides the utilities that:
		- retrieve the generic device information
		- set and retrieve parameters from devices in a generic sense.
	@}
**/
//	All I/O is handled by linking to the transport layer module. That
//	module provides the core functions of opening/closing communications
//	to the network controller, provides base transfer of command
//	information and responses and provides a hook to field an
//	asynchronous request for service by the network controller.
//
// CREATION DATE:
//		02/13/1998 12:22:30
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			   *
//******************************************************************************

/// \cond	INTERNAL_DOC

//******************************************************************************
// NAME																		   *
// 	netCore.c headers
//
	#include "meridianHdrs.h"
	#include "mnParamDefs.h"
	#include "sFoundResource.h"
	#include "lnkAccessCommon.h"
	#include "netCmdPrivate.h"
	#include "pubIscAPI.h"
	#include "pubCpmAPI.h"
	#include "iscRegs.h"
	#include "pubSysCls.h"
	#include "pubCpmCls.h"
	#include "iniparser.h"
#if defined(IMPL_NODE_CP)
	#include "clearPathRegs.h"
	#include "pubClearPathAPI.h"
#endif
	#include "dictionary.h"
	#include "iniparser.h"
	#include "cpmRegs.h"
	#include "converterLib.h"
	#include "pubMonPort.h"
	#include "SerialEx.h"
	#include <math.h>
	#include <assert.h>
	#include <time.h>
	#if !(defined(_WIN32)||defined(_WIN64))
		#include <string.h>
		#include <stdio.h>
		#include <stdlib.h>
	#else
	    #include <CRTDBG.H>
		#include <stdio.h>
		#include <io.h>
		#include <fcntl.h>
		#include <sys/stat.h>
	#endif
	// Fix up windows naming for VS8
	#if defined(_WIN32)||defined(_WIN64)
		#define access _access
		#if defined(_MSC_VER) && _MSC_VER<=1800
			#define snprintf sprintf_s
		#endif
		#define open _open
		#define W_OK 2
		#define R_OK 4
	#endif

	#ifdef __linux
	#ifdef __LP64__
		#define THREAD_RADIX "0x%lx"
	#else
		#define THREAD_RADIX "%d"
	#endif
	#else
		#define THREAD_RADIX "0x%x"
	#endif

//																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
// 	netCore.c global variables
//
	// Reference to channel and node information
	extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];
	// This is the master configuration file key list as enumerated by
	// the ParameterNumbers enumeration. This is used to locate or store
	// configuration parameters in configuration files.	This file
	// is maintained by the ValuesLib.dll make file.
	#include "valkeys.h"
	#ifdef __QNX__
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	#endif
//																			   *
//******************************************************************************



//******************************************************************************
// NAME																		   *
// 	netCore.c local variables
//
#define IOC_HZ_MIN			1.0							// Minimum frequency
// Maximum allowable time(ms) difference between FC Out on a given node and
// FC IN on the neighboring(lower address number) node
#define NET_CHECKSUMS_BAD 	1
#define NET_FRAGS_BAD 		1
#define NET_STRAYS_BAD 		1
#define NET_OVERRUN_BAD 	1

//- - - - - - - - - - - - - - - - - - - -
// TRACING/DEBUG SETUP
//- - - - - - - - - - - - - - - - - - - -
#ifdef _DEBUG
#define T_ON TRUE
#else
#define T_ON FALSE
#endif
#define T_OFF FALSE
#define TRACE_HIGH_LEVEL	T_OFF
#define TRACE_SERIAL		T_ON
#define TRACE_HEAP			T_OFF

// The base level by node information
mnNetDiagResults NetDiags[NET_CONTROLLER_MAX];

// Configuration File Strings
const char *CNFG_VER_KEY = "Chip Version";
const char *CNFG_DATE_KEY = "File Written";
const char *CNFG_MOD_VER_KEY = "Writer";
const char *CNFG_SYS_VCS_KEY = "SysWriter";
const char *CNFG_LOAD_MTR_RATIO = "LoadMtrRatio";
// Motor file key for user description
const char *CNFG_USER_DESC = "UserDesc";
const char *CNFG_USERID = "UserID0";
const char *MOTOR_SECTION = "Motor Info";
const char *CNFG_PN = "PN";

// Parameter location for the user description; define here so 
// it's not exposed publicly
const int CPM_P_USER_DESCR_BASE = 540;

// Cracker for long form version strings
typedef union _lvers {
	nodeulong	bits;
	struct {
		unsigned build	: 16;
		unsigned minor	: 8;
		unsigned major	: 8;
	} fld;
} longVers;
//																			   *
//******************************************************************************



//******************************************************************************
// NAME																		   *
// 	netCore.c function prototypes
//
// Local Function Prototype
// Convert a buffer area to a double based on the parameter information
//cnErrCode coreBufToValue(
//		const paramInfoLcl *info,	// Parameter information
//		paramValue *bufValue);		// Effective value

// Convert a double to a buffer area based on the parameter information
void coreDoubleToBuf(
		const paramInfoLcl *info, 	// Parameter information
		packetbuf *buffer,			// Ptr to start of buffer
		double newValue);			// Value to set

double coreBufToDouble(
	const paramInfoLcl *info,		// Information about this param
	paramValue *bufValue);			// Effective value

// Convert value in user units to base node value
void toBaseUnit(
		multiaddr theMultiAddr,		// Node address
		appNodeParam paramNum,		// the parameter number
		byNodeDB *nodeInfo,			// the node information
		paramInfoLcl const *pFixedInfoDB,	// the fixed parameter info
		paramBank *pBankInfo,		// the parameter bank
		paramValue *userVal);		// user value in their unit

// Convert base node value to a value in user specified units
void fromBaseUnit(
		multiaddr theMultiAddr,		// Node address
		appNodeParam paramNum,		// the parameter number
		byNodeDB *nodeInfo,			// the node information
		paramInfoLcl const *pFixedInfoDB,	// the fixed parameter info
		paramBank *pBankInfo,		// the parameter bank
		paramValue *pNodeVal);		// node's base value

cnErrCode coreBaseGetParameterLong(netaddr cNum, nodeaddr theNode,
								  nodeparam paramNum, int *pVal);


// Returns the firmware ID for this node
cnErrCode netGetFirmwareID(
		multiaddr theMultiAddr,		// Node address
		size_t bufSizeMax,			// sizeof theID buffer
		char *theID);
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		hexToDec
//
//	DESCRIPTION:
///		Parse the string for hex chars and convert to decimal.
///
/// 	\param pDest Destination area of compatibility 18 octet buffer
///		\param pSrc  Source of the copy
///
/// 	Detailed description.
//
//	SYNOPSIS:
nodelong hexToDec(char *theStr)
{
	size_t theLen = strlen(theStr);
	unsigned char *p = (unsigned char *)theStr
		+ theLen - 1;
	nodelong lVal = 0;
	for (size_t i=0; i<theLen; i++) {
		*p = toupper(*p);
		if (*p>='0' && *p<='9')
			lVal = lVal | ((*p-- - '0')<<(4*i));
		else if(*p>='A' && *p<='F')
			lVal = lVal | ((*p-- - 'A' + 10)<<(4*i));
		else  {
			p--;
			throw "Bad character";			// Skip this bad char
		}
	}
	return lVal;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcCopyPktToPkt18
//
//	DESCRIPTION:
///		Copy a full packet buffer to truncated one.
///
/// 	\param pDest Destination area of compatibility 18 octet buffer
///		\param pSrc  Source of the copy
///
/// 	Detailed description.
//
//	SYNOPSIS:
static void infcCopyPktToPkt18(packetbuf18 *pDest, packetbuf *pSrc)
{
	pDest->bufferSize = pSrc->Byte.BufferSize < 18 ? pSrc->Byte.BufferSize : 18;
	memcpy(&pDest->buffer[0], &pSrc->Byte.Buffer[0],
		   pDest->bufferSize);
}
//																			   *
//******************************************************************************


//****************************************************************************
//	NAME
//		coreController
//
//	DESCRIPTION:
//		Return the controller based on the node address. If the node address
//		is between [0..31] the current controller is returned else the new
//		algorithm
//
//	RETURNS:
//		netaddr (controller target)
//
//	SYNOPSIS:
netaddr coreController(multiaddr theMultiAddr)
{
	// Make sure they are not using old "currentController" form
	assert(NET_NUM(theMultiAddr)<NET_CONTROLLER_MAX);
	return(NET_NUM(theMultiAddr));
}
/****************************************************************************/




//****************************************************************************
//	NAME
//		netEnumerate
//
//	DESCRIPTION:
//		Bring nodes online/re-address and enumerate the inventory list.
//
//		SysInventory[cNum] is updated with the results
//
//	RETURNS:
//		#cnErrCode: MN_OK if there is at least an NC
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netEnumerate(
	netaddr cNum)				// Controller number
{
	cnErrCode theErr = MN_OK, lastErr = MN_OK;
	nodeulong i;
	nodeulong maxNode;
	packetbuf theResp;
	mnClassInfo *pClassInfo;
	mnNetInvRecords &netInv = SysInventory[cNum];

	// Get current controller
	infcSetInitializeMode(cNum, TRUE, MN_OK);
	infcFireNetEvent(cNum, NODES_SENDING);
	// Clear our initial set of counts until we rediscover
	netInv.clearNodes(false);

	maxNode=0;
	lastErr = netSetAddress(cNum, &maxNode);

	// Assign nodes their addresses
	netInv.InventoryNow.NumOfNodes = maxNode;
	// Collect array of basic devices, if NC OK
	if (theErr == MN_OK && lastErr == MN_OK) {
		// Reset to user's desired speed for this if not already there
		if (netInv.PhysPortSpecifier.PortRate != netInv.pNCS->pSerialPort->GetBaudrate()) {
			theErr = infcSetNetRate(cNum, netInv.PhysPortSpecifier.PortRate);
			#if defined(_DEBUG) && ((TRACE_CALLBACK&&TRACE_LOW_LEVEL) || LCL_INIT_PR || TRACE_SERIAL)
			_RPT4(_CRT_WARN, "%.1f netEnumerate(%d): restoring baud, rate=%d, err=0x%x\n",
				infcCoreTime(), cNum, netInv.PhysPortSpecifier.PortRate, theErr);
			#endif
			if (theErr != MN_OK)
				return theErr;
		}
		for (i=0; i<maxNode; i++) {
			pClassInfo = NULL;
			// Get the device type from the
			theErr = netGetParameter(MULTI_ADDR(cNum, i),
									  MN_P_NODEID, &theResp);
			// Correct packet and size?
			if (theErr == MN_OK && theResp.Byte.BufferSize==2) {
				theErr = netGetNodeAccessLvl(MULTI_ADDR(cNum, i),
											 &netInv.AccessInfo[i]);
				if (theErr == MN_OK) {
					// Setup data for coreGetDevType
					netInv.NodeInfo[i].theID =
						*((devID_t *)&theResp.Byte.Buffer[0]);
					// Initialize the nodes and reverse ranking
					switch(netInv.NodeInfo[i].theID.fld.devType) {
						case NODEID_MD:
							pClassInfo = &netInv.InventoryNow.mnDrvInfo;
							lastErr = iscInitializeEx(MULTI_ADDR(cNum,i), TRUE);
							// Adjust the cleanup masks
							for (size_t j=0; j<MN_API_MAX_NODES; j++)
								netInv.attnCleanupMask[j] = 0xffffff;
							break;
	#if defined(IMPL_NODE_CP)
						case NODEID_CP:
							pClassInfo = &netInv.InventoryNow.mnClearPathInfo;
							lastErr = clearPathInitializeEx(MULTI_ADDR(cNum,i), TRUE);
							// Adjust the cleanup masks
							for (size_t j=0; j<MN_API_MAX_NODES; j++)
								netInv.attnCleanupMask[j] = 0;
							break;
	#endif
						case NODEID_CS:
							pClassInfo = &netInv.InventoryNow.mnCpScInfo;
							lastErr = cpmInitializeEx(MULTI_ADDR(cNum,i), TRUE);
							// If we have a port, setup nodes
							if (netInv.pPortCls) {
								try {
									netInv.pNodes[i]
										= new sFnd::CPMnode(*netInv.pPortCls,
											MULTI_ADDR(cNum, i));
								}
								catch (sFnd::mnErr theErr) {
									// Something failed
									lastErr = theErr.ErrorCode;
								}
								catch (...) {
									return MN_ERR_FAIL;
								}
							}
							// Adjust the cleanup masks
							for (size_t j=0; j<MN_API_MAX_NODES; j++)
								netInv.attnCleanupMask[j] = 0xffffffff;
							break;
						default:
							lastErr = MN_ERR_WRONG_NODE_TYPE;
							break;
					}
				}
				// Update the inventory records
				if (pClassInfo != NULL) {
					// Build up the reverse link from the address
					netInv.NodeInfo[i].rank = pClassInfo->count;
					// Add one to the bunch and mark the location
					pClassInfo->node[pClassInfo->count++] = MULTI_ADDR(cNum,i);
				}
			}
			else {
				netInv.NodeInfo[i].theID.fld.devType = NODEID_UNK;
				lastErr = theErr;
			}
		}
	}
	// Initialize the remaining nodes to 'unknown'
	for (i=maxNode+1 ; i<MN_API_MAX_NODES; i++) {
		netInv.NodeInfo[i].theID.fld.devType = NODEID_UNK;
		netInv.NodeInfo[i].rank = 0;
	}

	// Force Initialize mode to determine if online
	netInv.OpenStateNext(OPENED_ONLINE);

	// No more commands to protect
	infcSetInitializeMode(cNum, FALSE, lastErr);

	return lastErr;
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		netGetUserDescription
//
//	DESCRIPTION:
/**
	Get the user description for this node. An error is returned if the
	node does not support this feature.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pUserDescrStr Return buffer area
	\param[in] maxBufSize Size of \p pUserIdStr area

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetUserDescription(
		multiaddr theMultiAddr,
		char *pUserDescrStr,
		Uint16 maxBufSize)
{
	cnErrCode theErr = MN_ERR_CMD_INTERNAL;
	nodeparam theBasePnum;
	paramValue paramBytes;
	//Uint16 nProcessed = 0;
	char *bufEnd = pUserDescrStr+maxBufSize-1;
	size_t len;

	// Args make sense?
	if (!pUserDescrStr || !maxBufSize)
		return MN_ERR_BADARG;
	netaddr cNum = NET_NUM(theMultiAddr);
	if (cNum>NET_CONTROLLER_MAX)
		return MN_ERR_BADARG;
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	if (SysInventory[cNum].InventoryNow.NumOfNodes < theNode) {
		return MN_ERR_ADDR_RANGE;
	}
	// Does this node support this
	switch(SysInventory[cNum].NodeInfo[theNode].theID.fld.devType ) {
#if defined(IMPL_NODE_CP)
	case NODEID_CP:
		theBasePnum = CP_P_USER_DESCR_BASE;
		break;
#endif
	case NODEID_CS:
		theBasePnum = CPM_P_USER_DESCR_BASE;
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}
	// Insure null-term inside buffer. Link pkt is smaller than buffer
	// so there will always be one just off the end.
	memset(paramBytes.raw.Byte.Buffer, 0, sizeof(paramBytes.raw.Byte.Buffer));
	// Get and concatenate chunks
	for (size_t i=0; i<SC_USER_DESCR_SPAN; i++) {
		theErr = netGetParameterInfo(theMultiAddr, nodeparam(theBasePnum+i),
									NULL, &paramBytes);
		if (theErr != MN_OK)
			break;

		// Do we have the null-term block?
		len = strlen((const char *)paramBytes.raw.Byte.Buffer);
		if (len>=maxBufSize) {
			memcpy(pUserDescrStr, paramBytes.raw.Byte.Buffer, maxBufSize);
			pUserDescrStr += maxBufSize;
			break;
		}
		else {
			memcpy(pUserDescrStr, paramBytes.raw.Byte.Buffer, SC_USER_DESCR_CHUNK);
			maxBufSize -= SC_USER_DESCR_CHUNK;
			pUserDescrStr += SC_USER_DESCR_CHUNK;
		}
	}
	// Insure null-terminated
	*bufEnd = '\0';
	return(theErr);
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		netSetUserDescription
//
//	DESCRIPTION:
/**
	Set the user description for this node. Any ANSI character string maybe used
	up to 125 characters.
	
	\param[in] theMultiAddr The address code for this node.
	\param[in] pNewDescr Ptr to NULL terminated ANSI string.

	\return
		#cnErrCode; MN_OK if successful
**/
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netSetUserDescription(
		multiaddr theMultiAddr,
		const char *pNewDescr)
{
	cnErrCode theErr = MN_ERR_CMD_INTERNAL;
	packetbuf paramBytes;
	size_t len;
	if (!pNewDescr)
		return MN_ERR_BADARG;
	netaddr cNum = NET_NUM(theMultiAddr);
	if (cNum>NET_CONTROLLER_MAX)
		return MN_ERR_BADARG;
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	nodeparam theBasePnum;
	if (SysInventory[cNum].InventoryNow.NumOfNodes < theNode) {
		return MN_ERR_ADDR_RANGE;
	}
	// Does this node support this
	switch(SysInventory[cNum].NodeInfo[theNode].theID.fld.devType) {
#if defined(IMPL_NODE_CP)
	case NODEID_CP:
		theBasePnum = CP_P_USER_DESCR_BASE;
		break;
#endif
	case NODEID_CS:
		theBasePnum = CPM_P_USER_DESCR_BASE;
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}
	// Setup parameter buffer for set operation
	paramBytes.Byte.BufferSize = SC_USER_DESCR_CHUNK;
	len = strlen(pNewDescr);
	for (size_t i=0; i<SC_USER_DESCR_SPAN; i++) {
		// Insure null terminate and end of string bytes
		memset(paramBytes.Byte.Buffer, 0, SC_USER_DESCR_CHUNK);
		if (len > 0) {
			strncpy((char *)paramBytes.Byte.Buffer,
					(const char *)pNewDescr, SC_USER_DESCR_CHUNK);
			// Insure last char is always NUL
			if (i==(SC_USER_DESCR_SPAN-1)) {
				paramBytes.Byte.Buffer[SC_USER_DESCR_CHUNK-1] = '\0';
			}
			pNewDescr += SC_USER_DESCR_CHUNK;
			len -= SC_USER_DESCR_CHUNK; 
		}
		// Send chunks to drive
		theErr = netSetParameterEx(theMultiAddr, nodeparam(theBasePnum+i),
									&paramBytes);
		if (theErr != MN_OK)
			break;
	}
	return(theErr);
}
/****************************************************************************/




//****************************************************************************
//	NAME
//		netGetUserID
//
//	DESCRIPTION:
/**
	Get the User ID for this node. This is an identifier the application
	may use for "plug-and-play" or diagnostic purpose. 

	\param[in] theMultiAddr The address code for this node.
	\param[out] pUserIdStr Return buffer area
	\param[in] maxBufSize Size of \p pUserIdStr area

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetUserID(
		multiaddr theMultiAddr,
		char *pUserIdStr,
		Uint16 maxBufSize)
{
	packetbuf nameBuf, cmdBuf;
	cnErrCode theErr;
	size_t i;
	char *pNextChar = pUserIdStr;
 	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);

	cmdBuf.Byte.Buffer[CMD_LOC] = MN_CMD_USER_ID;
	
	// Clear out buffer with 0 padding
	for(i=CMD_LOC+1; i<sizeof(nameBuf.Byte.Buffer); i++)
		nameBuf.Byte.Buffer[i] = 0;
	cmdBuf.Fld.Addr = theNode;
	cmdBuf.Fld.PktLen = 1;
	theErr = netRunCommand(cNum, &cmdBuf, &nameBuf); 
	if (theErr == MN_OK) {
		// Limit return size to buffer size plus room for terminating null.
		i = nameBuf.Fld.PktLen+RESP_LOC;		// End char loc
		if (nameBuf.Fld.PktLen > size_t(maxBufSize-1)) 
			i = size_t(maxBufSize-1);
		for (size_t j=RESP_LOC; j<i; j++) {
			if (nameBuf.Byte.Buffer[j] >= ' ' 
			&& nameBuf.Byte.Buffer[j] < 127)
				*pNextChar++ = nameBuf.Byte.Buffer[j];
			else
				*pNextChar++ = 0;
		}
		// Ensure buffer is null terminated
		*pNextChar = 0;
	}
	else {
		pUserIdStr[0] = 0;
	}
	// If the UserID is null, fill it with our default
	if (!pUserIdStr[0]) {
		char modelStr[MN_PART_NUM_SIZE+1];
		theErr = netGetPartNumber(theMultiAddr, modelStr, sizeof(modelStr));
		if (theErr != MN_OK)
			return theErr;
		snprintf(pUserIdStr, maxBufSize, "Unloaded <%s>",
			modelStr);
	}

	return(theErr);
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		netSetUserID
//
//	DESCRIPTION:
/**
	Set the User ID for this node. Any ANSI character string maybe used
	up to 13 characters.
	
	This maybe used as an identifier for application "plug-and-play" or
	diagnostic purposes. If set to NULL, the APS application will apply
	a generic name "Axis \<addr\>" on its display.  A typical name could
	be "X Axis".

	\param[in] theMultiAddr The address code for this node.
	\param[in] pNewName Ptr to NULL terminated ANSI string.

	\return
		#cnErrCode; MN_OK if successful
**/
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netSetUserID(
		multiaddr theMultiAddr,
		const char *pNewName)
{
	packetbuf nameBuf, respBuf;
	size_t i, nSize;
	cnErrCode theErr = MN_OK;
	char newName[MN_USER_ID_SIZE_V2] = "";
	
	if(pNewName == NULL)
		return(MN_ERR_BADARG);
	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	nameBuf.Byte.BufferSize = 0;
	nameBuf.Byte.Buffer[CMD_LOC] = MN_CMD_USER_ID;
	// Setup buffer based on model
	switch (SysInventory[cNum].NodeInfo[theNode].theID.fld.devType) {
	case NODEID_MD:
		nameBuf.Fld.PktLen = MN_USER_ID_SIZE+1;
		// 02/12/2016 JS: This section of code will have to be tested the next
		//                time we test the Meridian codebase!
		if (strncmp(pNewName, "Axis ", strlen("Axis ")) != 0) {
			// If it doesn't look like the default name, copy it over
			strncpy(newName, pNewName, MN_USER_ID_SIZE);
		}
		break;
	case NODEID_CP:
	case NODEID_CS:
	default:
		nameBuf.Fld.PktLen = MN_USER_ID_SIZE_V2+1;
		if (strncmp(pNewName, "Unloaded ", strlen("Unloaded ")) != 0) {
			// If it doesn't look like the default name, copy it over
			strncpy(newName, pNewName, MN_USER_ID_SIZE_V2);
		}
		break;
	}
	
	nSize = strlen(newName);
	if (nSize > nameBuf.Fld.PktLen) nSize = nameBuf.Fld.PktLen;

	// Clear out buffer with 0 padding
	for(i=nSize; i<nameBuf.Fld.PktLen; i++)
		nameBuf.Byte.Buffer[i+CMD_LOC+1] = 0;
	
	strncpy((char *)&nameBuf.Byte.Buffer[CMD_LOC+1], 
			(char *)&newName[0], nSize);
	cleanForXML((char *)&nameBuf.Byte.Buffer[CMD_LOC+1]);
	nameBuf.Fld.Addr = theNode;

	theErr = netRunCommand(NET_NUM(theMultiAddr), &nameBuf, &respBuf); 

	return(theErr);
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		netRunCommand
//
//	DESCRIPTION:
//		This function will send the specified command on the <cNum>
//		and wait for the response and stores its response in <pTheResp>.
//
//		If <pTheResp> is NULL, this command returns MN_OK if the network
//		controller accepts the command.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netRunCommand(
		netaddr cNum,				// Controller number
		packetbuf *pTheCmd,		// pointer to filled in command
		packetbuf *pTheResp)
{
#define RUN_CMD_ERR_DBG (_DEBUG&&1)			// Show err src
	cnErrCode theErr = MN_OK;
	infcErrInfo errInfo;					// Error reporting buffer

	if (pTheResp==NULL)
		return(MN_ERR_BADARG);

	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	// Adjust the frame length to match command length information
	// Initialize the command invariant information
	pTheCmd->Fld.PktType = MN_PKT_TYPE_CMD;
	pTheCmd->Fld.Src = MN_SRC_HOST;
	pTheCmd->Fld.Mode = 0;
	pTheCmd->Fld.Zero1 = 0;
	pTheCmd->Byte.BufferSize = pTheCmd->Fld.PktLen + MN_API_PACKET_HDR_LEN;

	// Run the command and check the response for acceptance.
	if ((theErr = infcRunCommand(cNum, pTheCmd, pTheResp)) == MN_OK)  {
		// Make sure the packet is consistent with the length
		if (pTheResp->Byte.BufferSize
		&& (pTheResp->Fld.PktLen + MN_API_PACKET_HDR_LEN)
		   != pTheResp->Byte.BufferSize) {
			//_RPT4(_CRT_WARN, "%.1f netRunCommand(%d): pkt buf mismatch,"
			//				 "buf size=%d, pkt size=%d\n",
			//				infcCoreTime(), cNum,
			//				pTheResp->Byte.BufferSize,
			//				pTheResp->Fld.PktLen + MN_API_PACKET_HDR_LEN);
			theErr = MN_ERR_PKT_ERR;
		}
		else {
			// Check that the response was legit
			theErr = coreGenErrCode(cNum, pTheResp, pTheCmd->Fld.Addr);
			#ifdef RUN_CMD_ERR_DBG
			if (theErr != MN_OK) {
				_RPT3(_CRT_WARN, "%.1f netRunCommand(%d): parse error 0x%x\n",
								infcCoreTime(), cNum, theErr);
			}
			#endif

		}
	}
	#ifdef RUN_CMD_ERR_DBG
	else {
		_RPT4(_CRT_WARN, 
			"%.1f netRunCommand(%d): exec error 0x%x, thread=" THREAD_RADIX "\n",
			infcCoreTime(), cNum, theErr, infcThreadID());
	}
	#endif
	// If we find this in the normal coarse of events, fire off the callbacks
	if (theErr != MN_OK) {
		// Fire off the error callback
		errInfo.cNum = cNum;
		errInfo.node = MULTI_ADDR(cNum, pTheCmd->Fld.Addr);
		infcCopyPktToPkt18(&errInfo.response, pTheResp);
		if (theErr == MN_ERR_OFFLINE)
			errInfo.errCode = (cnErrCode)(MN_ERR_OFFLINE_00+pTheCmd->Fld.Addr);
		else
			errInfo.errCode = theErr;
		infcFireErrCallback(&errInfo);
		// Insure response buffer is cleared
		pTheResp->Fld.PktLen = 0;
		pTheResp->Byte.BufferSize = 0;
		_RPT4(_CRT_WARN, "%.1f netRunCommand(%d): response err 0x%x, thread=" THREAD_RADIX "\n",
						infcCoreTime(), cNum, errInfo.errCode, infcThreadID());
		// Create dump file on this error
		infcTraceDumpNext(cNum);
	}
	return(theErr);
}
/****************************************************************************/



/*****************************************************************************
 *	NAME
 *		coreGenErrCode
 *
 *	DESCRIPTION:
 *		Create the appropriate error code for this buffer
 *
 *	RETURNS:
 *		cnErrCode
 *
 *	SYNOPSIS: 															    */
cnErrCode coreGenErrCode(netaddr cNum, packetbuf *pTheResp, nodeaddr cmdNode)
{
	nodebool initMode;
	netErrGeneric *pErrPkt;
	//char msg[100];

	// Is the response plausible?
	if (pTheResp->Byte.BufferSize < MN_API_PACKET_HDR_LEN)  {
		return(MN_ERR_RESP_FMT);
	}
	// Get initialization mode flag
	infcGetInitializeMode(cNum, &initMode);
	// Is the address plausable and we are not in initialization phase?
	if (pTheResp->Fld.PktType != MN_PKT_TYPE_SET_ADDR
	&& (pTheResp->Fld.Addr > SysInventory[cNum].InventoryNow.NumOfNodes)
	&& !initMode)
		return(MN_ERR_ADDR_RANGE);
	// Is this response from a known source?
	if (pTheResp->Fld.Src != MN_SRC_HOST)
		return(MN_ERR_WRONG_SRC);
	// Has it been processed and OK?
	switch(pTheResp->Fld.PktType) {
		case MN_PKT_TYPE_CMD:
			// Make sure response from proper node
			if(pTheResp->Fld.Addr != cmdNode)
				return(MN_ERR_RESP_ADDR);
//			if(!initMode)
//				return(MN_ERR_OFFLINE);
			// Let upper levels declare this un-executed
			return(MN_ERR_OFFLINE);
		case MN_PKT_TYPE_ERROR:
			pErrPkt = (netErrGeneric *)&pTheResp->Byte.Buffer[RESP_LOC];
			switch (pErrPkt->Fld.ErrCls) {
				case ND_ERRCLS_CMD:
					return((cnErrCode)(MN_ERR_CMD_ERR_BASE
									   + pErrPkt->Fld.ErrCode));
				case ND_ERRCLS_NET:
					return((cnErrCode)(MN_ERR_NET_ERR_BASE
									   + pErrPkt->Fld.ErrCode));
				default:
					return(MN_ERR_FRAME);
			}
			break;
		case MN_PKT_TYPE_RESP:
			// If basically OK, make sure it came from proper node
			if (pTheResp->Fld.Addr != cmdNode)
				return(MN_ERR_RESP_ADDR);
			return(MN_OK);
		// Assume other are OK
		case MN_PKT_TYPE_ATTN_IRQ:				// Attn Request
		case MN_PKT_TYPE_SET_ADDR:				// Set Address
		case MN_PKT_TYPE_TRIGGER:				// Motion trigger
		case MN_PKT_TYPE_EXTEND_HIGH:			// Extended control frames
		case MN_PKT_TYPE_EXTEND_LOW:
			return(MN_OK);
		default:
			_RPT2(_CRT_WARN, "coreGenErrcode(%d), type unknown %d\n",
				pTheResp->Fld.Addr, pTheResp->Fld.PktType);
	}
	return(MN_ERR_FAIL);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netGetParameterDbl
//
//	DESCRIPTION:
/**
	This function will get the parameter specified by \a theParamNum on
	device \a theNode. The parameter is returned in the buffer pointed to
	by \a paramInfo. The actual length and information will be updated
	when this function returns MN_OK.

	\param[in] cNum Channel number to for node. The first channel is zero.
	\param[in] theNode Node address on the channel.
	\param[in] theParamNum The parameter code number for retrieval.
	\param[out] pValue Response area update with the scaled double
				parameter value.

	\return MN_OK if succeed and \p pValue updated
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetParameterDbl(
		multiaddr theMultiAddr,
		mnParams theParamNum,
		double *pValue)
{
	cnErrCode theErr;
	paramValue theValue;
	theErr = netGetParameterInfo(theMultiAddr, theParamNum,
								  NULL, &theValue);
	*pValue = theValue.value;
	return theErr;
}


//cnErrCode MN_DECL netGetParameterDbl(
//		netaddr cNum,				// network controller
//		nodeaddr theNode, 			// device address
//		mnParams theParamNum,		// parameter index
//		double *pValue)				// ptr for returned param
//
//{
//	cnErrCode theErr;
//	paramValue theValue;
//	theErr = netGetParameterInfo(MULTI_ADDR(cNum, theNode), theParamNum,
//								  NULL, &theValue);
//	*pValue = theValue.value;
//	return theErr;
//}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netSetParameterDbl
//
//  DESCRIPTION:
/**
	This function will get the parameter specified by \a theParamNum on
	device \a theNode. The parameter is returned in the buffer pointed to
	by \a paramInfo. The actual length and information will be updated
	when this function returns MN_OK.

	\param[in] cNum Channel number to for node. The first channel is zero.
	\param[in] theNode Node address on the channel.
	\param[in] theParamNum The parameter code number for retrieval.
	\param[in] value New value to set at the node.

	\return MN_OK if the node's parameter is updated
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netSetParameterDbl(
		multiaddr theMultiAddr,
		mnParams theParamNum,
		double value)
{
	cnErrCode theErr;
	theErr = netSetParameterInfo(theMultiAddr,
								  theParamNum, value, FALSE);
	return theErr;
}

//cnErrCode MN_DECL netSetParameterDbl(
//		netaddr cNum,				// network controller
//		nodeaddr theNode, 			// device address
//		mnParams theParamNum,		// parameter index
//		double value)				// ptr for returned param
//
//{
//	cnErrCode theErr;
//	theErr = netSetParameterInfo(MULTI_ADDR(cNum, theNode),
//								  theParamNum, value, FALSE);
//	return theErr;
//}
//																			 *
//****************************************************************************


//******************************************************************************
//	NAME
//		netSetParameterEx
//
//	DESCRIPTION:
/**
	This function will run the set parameter command for device \a
	theMultiAddr at parameter index \a theParamNum. The parameter
	database is updated and any change callbacks issued for this node.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theParamNum The parameter code number for retrieval.
	\param[in] pParamBuf Ptr to value area

	\return MN_OK if node updated successfully
**/
//	SYNOPSIS:

MN_EXPORT cnErrCode MN_DECL netSetParameterEx(
		multiaddr theMultiAddr, 		// node address
		nodeparam theParamNum, 		// parameter index
		packetbuf *pNewValue)		// ptr to new parameter data
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;
	byNodeDB *pNodeInfo;			// Node information
	paramBank *pParamBank;			// Current parameter bank
	paramInfoLcl const *pTheInfoDB;	// Current parameter fixed info
	paramValue *pTheValDB;			// Current value DB
	netaddr cNum;
	mnParamChg paramChgObj;			// Parameter change object
	appNodeParam appParam;			// Parameter info extractor instance

	cNum = coreController(theMultiAddr);
	// Get a pointer to this node's information
	pNodeInfo = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
	// Bail if port closed/offline
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS || !(pNCS->pSerialPort) || !pNCS->pSerialPort->IsOpen())
		return(MN_ERR_CLOSED);

	if (pNodeInfo->paramBankList == NULL)
		return(MN_ERR_PARAM_NOT_INIT);

	// Request makes sense?
	appParam.bits = theParamNum;
	if (appParam.fld.bank >= pNodeInfo->bankCount) {
		return MN_ERR_PARAM_RANGE;
	}

	if ((theErr = netSetParameterFmt(&theCmd,
								      theMultiAddr,
								      theParamNum,
								      pNewValue)) == MN_OK) {
		// Can/should we update the value table
		pParamBank = &(pNodeInfo->paramBankList)[appParam.fld.bank];
		if (pParamBank->nParams > appParam.fld.param) {
			pTheInfoDB = pParamBank->fixedInfoDB+appParam.fld.param;
			pTheValDB =  pParamBank->valueDB+appParam.fld.param;
		}
		else {
			pTheInfoDB = NULL;
			pTheValDB =  NULL;
		}


		// Update the parameter database if necessary
		if (theErr == MN_OK && pTheValDB) {
			// If non-volatile, make sure the length match
			if (appParam.fld.param < pParamBank->nParams) {
				// Too big for parameter?
				if ((unsigned)labs(pTheInfoDB->info.paramSize) < pNewValue->Byte.BufferSize)
					return(MN_ERR_VALUE);
				// If positive, it must match
				if (pTheInfoDB->info.paramSize > 0
				&& (pTheInfoDB->info.paramSize != pNewValue->Byte.BufferSize))
					return(MN_ERR_VALUE);
				// Only fail if Read-Only past here
				if (!(pTheInfoDB->info.paramType & PT_RO)) {
					pTheValDB->raw = *pNewValue;
				}
				// If VA, the double value is meaningless, make 0
				if (pTheInfoDB->info.paramSize <= 0) {
					pTheValDB->value = 0;
				}
				// Call the parameter handler if it exists
				if (pTheInfoDB->converter) {
					(*pTheInfoDB->converter)(false, theMultiAddr, appParam, 0, pNodeInfo);
				}
			}
		}
		// If buffer is still OK, run the command
		if (theErr == MN_OK) {
			//if (verify)
				theErr = netRunCommand(cNum, &theCmd, &theResp);
			//else
			//	theErr = netRunCommand(cNum, &theCmd, NULL);
			// Mark that we set the value
			if (theErr == MN_OK) {
				if (pTheValDB) {
					pTheValDB->exists = TRUE;
					paramChgObj.newValue = pTheValDB->value;
					pTheValDB->value = coreBufToDouble(pTheInfoDB, pTheValDB);
				}
				else {
					paramValue pv;
					pv.raw = *pNewValue;
					paramChgObj.newValue = coreBufToDouble(pTheInfoDB, &pv);
				}

				paramChgObj.net = cNum;
				paramChgObj.node = theMultiAddr;
				paramChgObj.paramNum = theParamNum;
				// Call the callback
				infcFireParamCallback(pNodeInfo->pClassInfo->paramChngFunc,
									  &paramChgObj);
			}
		}
		else {
			_RPT1(_CRT_WARN, "netSetParameterEx error: %0X\n", theErr);
		}
	}
	return (theErr);
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME
//		netSetAddress
//
//	DESCRIPTION:
//		Re-address the selected network and return the quantity of nodes
//		detected.
//
//		NOTE:	The network is addressed starting from <maxNode>.  For normal
//				operation caller should initialize <maxNode> to 0.
//
//	RETURNS:
//		MN_OK and *maxNode set, else the error condition
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netSetAddress(
		netaddr cNum,
		nodeulong *maxNode)
{
	packetbuf theCmd, theResp;
	unsigned i;
	cnErrCode theErr = MN_ERR_FAIL;

	theCmd.Byte.BufferSize = 2;						// This is always length 2

	// Build set address packet
	infcSetAddressFmt(*maxNode, &theCmd);

	SysInventory[cNum].InventoryNow.NumOfNodes = *maxNode = 0;		// Assume no response
	theErr = MN_ERR_FAIL;
	// Run the set address control frame a few times
	for (i=0; i<3 && theErr != MN_OK; i++) {
		if ((theErr = infcRunCommand(cNum, &theCmd, &theResp)) == MN_OK)  {
			// Make sure this is the correct response type
			if ((theResp.Fld.PktType != MN_PKT_TYPE_SET_ADDR
			|| theResp.Byte.BufferSize != theCmd.Byte.BufferSize)) {
				theErr = MN_ERR_RESP_FMT;
			}
			// We did get a valid response
			else if (((setAddrPkt *) &theResp)->Fld.Err != 0 && theResp.Fld.Addr > 0) {
				theErr = MN_ERR_TOO_MANY;
			}
			else {
				// Extract the address/# of nodes (0 means 16 nodes online)
				*maxNode = theResp.Fld.Addr > 0 ? theResp.Fld.Addr :
												// <err> will be set if there are 16
												// else we got back what we sent
												(theResp.Fld.Mode ? 16 : 0);
				SysInventory[cNum].InventoryNow.NumOfNodes = *maxNode;
				#if TRACE_HIGH_LEVEL
				_RPT3(_CRT_WARN, "%.1f netSetAddress(%d): found %d\n",
								infcCoreTime(), cNum, *maxNode);
				#endif
			}
		}
		#ifdef _DEBUG
		else {
		//	infcFlush();
			_RPT3(_CRT_WARN, "%.1f netSetAddress(%d): failed %0X!\n",
							  infcCoreTime(), cNum, theErr);
		}
		#endif
	}
	// Reverse the addresses if this is the diagnostic network to align
	// numbering with the Application net.
	if (theErr == MN_OK && *maxNode>0) {
		// Ask the first node on our net to find out what net it is
		// attached to.
		mnNetStatus netState;
		theErr = netGetNodeAccessLvl(MULTI_ADDR(cNum,0), &netState);
		if (theErr == MN_OK) {
			// Tell driver for diagnostics purposes
			infcSetNetSource(cNum, (mnNetSrcs)netState.Fld.NetSource);
			if (netState.Fld.NetSource != MN_SRC_APP_NET) {
				_RPT2(_CRT_WARN, "%.1f netSetAddress(%d): reversing addresses of diag net\n",
								  infcCoreTime(), cNum);
				theErr = netReverseAddress(cNum, *maxNode-1);
			}
		}
	}
	return(theErr);
}
/****************************************************************************/



//****************************************************************************
//	NAME
//		netReverseAddress
//
//	DESCRIPTION:
//		Re-address the selected network by taking the node's current address
//		and subtracting the <nodeCountLessOne> value from it. The affect is
//		to reverse address the targeted network. This is done to allow a
//		reverse wired diagnostic net to have the same addresses as the forward
//		direction.
//
//	RETURNS:
//		MN_OK if successfully executed
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netReverseAddress(
		netaddr cNum,
		nodeaddr nodeCountLessOne)
{
	packetbuf theCmd, theResp;
	unsigned i;
	cnErrCode theErr = MN_ERR_FAIL;

	theCmd.Byte.BufferSize = 3;						// This is always length 3

	// Build set address control frame
	theCmd.Fld.PktLen = 1;
	theCmd.Fld.Addr = nodeCountLessOne;
	theCmd.Fld.PktType = MN_PKT_TYPE_EXTEND_HIGH;
	theCmd.Fld.Mode = theCmd.Fld.Zero1 = 0;	// Fill in unused with 0
	theCmd.Fld.Src = MN_SRC_HOST;
	theCmd.Byte.Buffer[EXTEND_CODE_LOC] = MN_CTL_EXT_REV_ADDR;

	theErr = MN_ERR_FAIL;
	// Run the set address control frame a few times
	for (i=0; i<3 && theErr != MN_OK; i++) {
		if ((theErr = infcRunCommand(cNum, &theCmd, &theResp)) == MN_OK)  {
			// Make sure this is the correct response type
			if ((theResp.Fld.PktType != MN_PKT_TYPE_EXTEND_HIGH)) {
				theErr = MN_ERR_RESP_FMT;
			}
			// We did get a valid response
		}
		else {
		//	infcFlush();
			_RPT1(_CRT_WARN, "netReverseAddress: failed %0X!\n", theErr);
		}
	}
	return(theErr);
}
/****************************************************************************/


//*****************************************************************************
//	NAME																	  *
//		netReset
//
//	DESCRIPTION:
//		Format and run the node reset command. This command will return with a
//		time-out if the network is broken. The node waits for the completion
//		of the re-transmission of this command before actually resetting.
//
//		<theMultiAddr> = 0 will reset all nodes on the network except the NC
//		else the specific node is reset.
//
//		NOTE: DS 2/3/2010 deprecating this for public use as it requires
//		scheduling of a baud rate change and restoration to prevent the node
//		from being isolated at an incompatible baud rate.
//
//	RETURNS:
//		ND_ERR_TIMEOUT normally
//
//	SYNOPSIS:
cnErrCode MN_DECL netReset(
		multiaddr theMultiAddr,
		nodebool broadCast) 				// node address
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;
	int i;
	netaddr cNum = coreController(theMultiAddr);

	// Try reset a couple of times (this is a hack!)
	for (i=0; i<3; i++) {

		theCmd.Fld.SetupHdr(MN_PKT_TYPE_EXTEND_HIGH,
							  NODE_ADDR(theMultiAddr),
							  MN_SRC_HOST);
		theCmd.Fld.PktLen = 1;
		theCmd.Fld.Mode=broadCast;
		theCmd.Byte.Buffer[CMD_LOC] = MN_CTL_EXT_RESET;
		theCmd.Byte.BufferSize = theCmd.Fld.PktLen + MN_API_PACKET_HDR_LEN;
		theErr = infcRunCommand(cNum, &theCmd, &theResp);
		if (theErr == MN_OK)
			break;
	}

	// Show reset state
	infcFireNetEvent(cNum, NODES_RESETTING);
	// Wait for nodes to reset
	infcSleep(MN_MAX_RESET_TIME);
	return (theErr);
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		netGetNodeAccessLvl
//
//	DESCRIPTION:
//		Gets the the node access level of the specified node on the specified
//		node.
//
//		Parameter
//			theMultiAddr - the node on the network to get the access level
//
//			theAccessLevel - this is the new access level to be set at
//				0 = MN_ACCESS_LVL_MONITOR
//				1 = MN_ACCESS_LVL_TUNE
//				2 = MN_ACCESS_LVL_FULL
//
//	RETURNS:
//		MN_OK on success or cnErrCode reason.
//
//	SYNOPSIS:
cnErrCode MN_DECL netGetNodeAccessLvl(
	multiaddr theMultiAddr,							// Destination node
	mnNetStatus *pAccessLevel)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	// Build the command packet
	theCmd.Fld.PktLen = 1;
	theCmd.Fld.Addr = NODE_ADDR(theMultiAddr);
	theCmd.Fld.PktType = MN_PKT_TYPE_CMD;
	theCmd.Fld.Mode = theCmd.Fld.Zero1 = 0;	// Fill in unused with 0
	theCmd.Fld.Src = MN_SRC_HOST;
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_NET_ACCESS;
	theCmd.Byte.BufferSize = theCmd.Fld.PktLen + MN_API_PACKET_HDR_LEN;

	if ((theErr = infcRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp)) == MN_OK) {
		// Make sure this is the correct response type
		if ((theResp.Fld.PktType != MN_PKT_TYPE_RESP)) {
			_RPT1(_CRT_WARN, "netSetNodeAccessLvl: failed %0X!\n", theErr);
		}
	}
	pAccessLevel->bits = theResp.Byte.Buffer[RESP_LOC];
	return(theErr);
}
//																		      *
//*****************************************************************************


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*  PRIVATE DLL UTILITY FUNCTIONS 										   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*****************************************************************************
 *	NAME
 *		coreBufToDouble
 *
 *	DESCRIPTION:
 *		Convert the buffer to a double precision floating point value.
 *
 *		NOTE: This code depends on little-endian architecture
 *
 *	RETURNS:
 *		double precision value of buffer contents
 *
 *	SYNOPSIS: 															    */
double coreBufToDouble(
	const paramInfoLcl *info,			// Information about this param
	paramValue *bufValue)				// Effective value
{
	hwVers_t hwVersInfo;
	versID_t fwVersInfo;
	devID_t theID;
	const char *devIDstr;
	char devIDunkn[5];
	double val;
	unsigned long lVal;

	// Perform sign extended conversion right from buffer
	switch (bufValue->raw.Byte.BufferSize)  {
		case 1:
			val = (nodechar)bufValue->raw.Byte.Buffer[0];
			break;
		case 2:
			val = *((nodeshort *)&bufValue->raw.Byte.Buffer[0]);
			break;
		case 3:
			// Sign extend and convert to long
			if (bufValue->raw.Byte.Buffer[2] & 0x80)
				bufValue->raw.Byte.Buffer[3] = (nodechar)0xff;
			else
				bufValue->raw.Byte.Buffer[3] = 0x0;
			// no break OK (special line for Code Analyzer to suppress warning)
		case 4:
			val = *((nodelong *)&bufValue->raw.Byte.Buffer[0]);
			break;
		case 6:
			// Sign extend and convert to long
			if ((bufValue->raw.Byte.Buffer[5] & 0x80) &&
				(info->info.signextend ==ST_SIGNED)) {
				bufValue->raw.Byte.Buffer[6] = (nodechar)0xff;
				bufValue->raw.Byte.Buffer[7] = (nodechar)0xff;
			} else {
				bufValue->raw.Byte.Buffer[6] = 0x00;
				bufValue->raw.Byte.Buffer[7] = 0x00;
			}
			// no break OK (special line for Code Analyzer to suppress warning)
		case 8:
#if (defined(_MSC_VER))
			#pragma warning(push)
			#pragma warning(disable: 4244)
#endif
			val = *((long long *)&bufValue->raw.Byte.Buffer[0]);
#if (defined(_MSC_VER))
			#pragma warning(pop)
#endif
			break;
		default:
			bufValue->exists = FALSE;
			val = 0;
			break;
	}
	// This a generic buffer conversion?
	if (info == NULL) {
		return(val);
	}
	// Is this non-defined type conversion?
	if (info->info.paramType == PT_NONE) {
		// Return as plain signed integer
		bufValue->exists = TRUE;
		return(val);
	}
	bufValue->exists = TRUE;			// Assume good until the end

	if (info->info.paramSize <= 0)
		return (0);						// There is no double view

	// Do we know anything about this parameter?
	lVal = (long)val;
	switch(info->info.unitType) {
		// Note: These are special cases, where the buffer is transformed
		// into an intermediate form.
		case DEV_ID:
			theID = *((devID_t *)(&lVal));
			switch(theID.fld.devType) {
				case NODEID_MD:
					devIDstr = "MD";
					break;
				case NODEID_CP:
					devIDstr = "CP";
					break;
				case NODEID_CS:
					devIDstr = "CS";
					break;
				default:
					devIDstr = &devIDunkn[0];
					sprintf((char *)devIDstr, "%d.", theID.fld.devType);
					break;
			}
			bufValue->raw.Byte.BufferSize
			= sprintf((char *)bufValue->raw.Byte.Buffer,
					"%s.%d",
					devIDstr, theID.fld.devModel);
			return val;
		case FW_VERS:
			fwVersInfo = *((versID_t *)(&lVal));
			bufValue->raw.Byte.BufferSize
			= sprintf((char *)bufValue->raw.Byte.Buffer,
					"%d.%d.%d",
					fwVersInfo.fld.majorVers, fwVersInfo.fld.minorVers, fwVersInfo.fld.buildVers);
			return val;
		case HW_VERS:
			hwVersInfo = *((hwVers_t *)(&lVal));
			if (hwVersInfo.fld.Major<26) {
				bufValue->raw.Byte.BufferSize
				= sprintf((char *)bufValue->raw.Byte.Buffer,
						"%c%d",
						hwVersInfo.fld.Major+'A', hwVersInfo.fld.Minor);
			}
			else {
				bufValue->raw.Byte.BufferSize
				= sprintf((char *)bufValue->raw.Byte.Buffer,
						"%c%c%d",
						hwVersInfo.fld.Major/26+'A', hwVersInfo.fld.Major%26+'A',
						hwVersInfo.fld.Minor);
			}
			return val;
		default:
			// Convert buffer to signed integer
			switch(info->info.signextend)  {
				case ST_SIGNED:
					// <val> was already converted this way
					break;
				case ST_UNSIGNED:
				case ST_POS_ONLY:
					// Unsigned
					switch (bufValue->raw.Byte.BufferSize) {
						case 1:
							val = (nodechar)bufValue->raw.Byte.Buffer[0];
							break;
						case 2:
							val = *((nodeushort *)&bufValue->raw.Byte.Buffer[0]);
							break;
						case 3:
							bufValue->raw.Byte.Buffer[3] = 0x0;
							// no break OK (special line for Code Analyzer to suppress warning)
						case 4:
							val = *((nodelong *)&bufValue->raw.Byte.Buffer[0]);
							break;
						case 6:
						case 8:
							break;
						default:
							bufValue->exists = FALSE;
							return(0);
					}
					break;
			}
			//
			// Scale the value by the appropriate amount and take reciprical flag
			// into consideration.
			//
			if (!info->info.reciprical && info->info.scaleFromBase != 0) {
				// Avoid divide by 0 by leaving value @ 0
				if (val != 0)
					val /= info->info.scaleFromBase;
				else
					val	= 0;
			}
			else {
				val *= info->info.scaleFromBase;
			}
			return(val);
	}
	//// If we got here, the value was not OK
	//bufValue->exists = FALSE;
	//return(0);
}
/*																 !end!		*/
/****************************************************************************/


 
/*****************************************************************************
 *	NAME
 *		coreDoubleToBuf
 *
 *	DESCRIPTION:
 *		This function will convert a double precision floating point value
 *		<newValue> into a nodechar buffer area. If <info> is defined that
 *		value is scaled appropriately into the proper fixed point value.
 *
 *		If <info> is NULL, the value is assumed to be an signed integer
 *		value whose length is defined by <bufsize>.
 *
 *		NOTE: This code depends on little-endian architecture
 *
 *	RETURNS:
 *		cnErrCode, MN_OK if successful
 *
 *	SYNOPSIS: 															    */
void coreDoubleToBuf(
	const paramInfoLcl *info, 			// Parameter information
	packetbuf *buffer,					// Ptr to start of buffer
	double newValue)					// Value to set
{
	int64 lValue;
	Uint64 uMax;

	if (info == NULL)  {
		// Default value to at least a long
		lValue = (int64)(newValue+0.5);			// Convert to a long
	}
	else {
		double scaleFact = info->info.scaleFromBase;
		// Round only scaled items
		if (!info->info.reciprical)  {
			lValue = (int64)(newValue * scaleFact 
				+ (scaleFact > 1.0? 0.5 : 0));
		}
		else {
			// Avoid the divide by 0 case
			if (newValue != 0)
				lValue = (int64)(newValue / scaleFact 
				+ (scaleFact > 1.0? 0.5 : 0));
			else
				lValue = 0;
		}
		#define BIG1 (Uint64(1))
		switch (info->info.signextend) {
			case ST_SIGNED:
				uMax = (BIG1<<(8*buffer->Byte.BufferSize-1))-1;
				if (lValue > 0 && lValue > (int64)uMax) lValue = (Uint64)uMax;
				if (lValue < 0 && lValue < -((int64)uMax))
					lValue = -((int64)uMax+1);
				break;
			case ST_UNSIGNED:
				uMax = (BIG1<<(8*buffer->Byte.BufferSize))-1;
				lValue &= uMax;
				break;
			case ST_POS_ONLY:
				uMax = (BIG1<<(8*buffer->Byte.BufferSize-1))-1;
				if (lValue > (int64)uMax) lValue = uMax;
				if (lValue < 0) lValue = 0;
				break;
		}
	}
	//
	// Convert this nominal long into the buffer area
	//
	switch (labs(buffer->Byte.BufferSize)) {
		case 1:
			buffer->Byte.Buffer[0] = (nodechar)lValue;
			break;
		case 3:
			buffer->Byte.Buffer[2] = *((nodechar *)(&lValue)+2);
			// Set LSBs with int case
			// no break OK (special line for Code Analyzer to suppress warning)
		case 2:
			*((int16 *)&buffer->Byte.Buffer[0])
			= (int16)lValue;
			break;
		case 4:
			*((int32 *)&buffer->Byte.Buffer[0])
			= (int32)lValue;
			break;
		case 8:
			*((int64 *)&buffer->Byte.Buffer[0])
			= (int64)lValue;
			break;
		default:
			// Leave the buffer along, we can't do this
			_RPT2(_CRT_WARN, "coreDoubleToBuf: Unknown conversion size %d, desc ID=%d\n",
								buffer->Byte.BufferSize, info->info.descID);
			break;
	}
}
/*																 !end!		*/
/****************************************************************************/


 
/*****************************************************************************
 *	NAME
 *		coreUpdateParamInfo
 *
 *	DESCRIPTION:
 *		This function initializes the parameter manager for a particular
 *		node class.
 *
 *	RETURNS:
 *		cnErrCode, MN_OK if successful
 *
 *	SYNOPSIS: 															    */
cnErrCode coreUpdateParamInfo(
	multiaddr theMultiAddr)			// Node address
{
	nodeparam pNum;					// Parameter loop counter
	cnErrCode lastErr = MN_OK;
	netaddr cNum;
	//cnErrCode errRet;
	byNodeDB *pNodeInfo;			// Node information
	paramBank *pParamBank;			// Current parameter bank
	paramInfoLcl const *pFixedInfoDB;	// Current parameter fixed info
	paramValue *pValueDB;			// Current value DB
	unsigned bank;
	#ifdef PRE_CACHE
	appNodeParam coreParam;
	coreParam.bits = 0;
	#endif


	cNum = coreController(theMultiAddr);
	// Get a pointer to this node's information
	pNodeInfo = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
	for(bank=0; bank<pNodeInfo->bankCount; bank++) {
		// Get fixed and current parameter value information pointers
		pParamBank = &(pNodeInfo->paramBankList)[bank];
		// Now get the values from the node and update the value DB
		for(pNum=0; pNum<pParamBank->nParams; pNum++) {
            #ifdef PRE_CACHE
                coreParam.fld.bank = bank;
                coreParam.fld.param = pNum;
            #endif
			pFixedInfoDB = &pParamBank->fixedInfoDB[pNum];
			pValueDB = &pParamBank->valueDB[pNum];
			pValueDB->isPolled = FALSE;
			pValueDB->exists = FALSE;			// Mark "un-read"
			pValueDB->value = 0;
			if (pFixedInfoDB->info.paramType != PT_NONE)  {
#ifdef PRE_CACHE
				errRet = netGetParameter(theMultiAddr, coreParam.bits,
										  &pValueDB->raw);
				// Convert this buffer to a double
				if (errRet == MN_OK)  {
					// Scale the base units to the user's units
					fromBaseUnit(theMultiAddr, coreParam, pNodeInfo,
						   	   pFixedInfoDB, pParamBank, pValueDB);
				}
				else {
					_RPT2(_CRT_WARN,"coreUpdateParamInfo error %x on param %d\n",
						  errRet, coreParam.bits);
				}
				// Mark the value as valid
				pValueDB->exists = errRet == MN_OK;
				// Mark the error
				if (pValueDB->exists) {
					// Force a callback if defined
					paramChgObj.node = theMultiAddr;
					paramChgObj.net = cNum;
					paramChgObj.paramNum = coreParam.bits;
					paramChgObj.newValue = pValueDB->value;
					infcFireParamCallback(pNodeInfo->pClassInfo->paramChngFunc,
									      &paramChgObj);
				}
#endif
			}
			else {
				//lastErr = errRet;
			}
		} // for pNum
	}
	return(lastErr);
}
/*																 !end!		*/
/****************************************************************************/



/*****************************************************************************
 *	NAME
 *		netGetParameterInfo
 *
 *	DESCRIPTION:
 *		This is a generic parameter read function that gets augmented
 *		parameter information.
 *
 *	RETURNS:
 *		cnErrCode, MN_OK if successful
 *
 *	SYNOPSIS: 															    */
MN_EXPORT cnErrCode MN_DECL netGetParameterInfo(
		multiaddr theMultiAddr,			// Node address
		nodeparam theParam,			// Parameter number
		paramInfo *pRetValInfo,		// Ptr to returning value info or NULL
		paramValue *pRetVal)		// Ptr to returning value or NULL
{
	appNodeParam coreParam;			// The core parameter number
	cnErrCode theErr=MN_OK;
	double initialVal;				// Initial value for this parameter
	mnParamChg paramChgObj;			// Parameter change object
	byNodeDB *pNodeInfo;			// Node information
	paramBank *pParamBank;			// Current parameter bank
	paramInfoLcl const *pFixedInfoDB;	// Current parameter fixed info
	paramInfoLcl dummyInfo;			// Dummy information
	paramValue *pValueDB;			// Current value DB
	netaddr cNum;					// Current network

	cNum = coreController(theMultiAddr);

#if 0
	if (NODE_ADDR(theMultiAddr) == 4 && theParam == ISC_P_STATUSRT_REG) {
		_RPT0(_CRT_WARN, "StatusRT\n");
	}
#endif
	// Bail if port closed/offline
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS || !(pNCS->pSerialPort) || !pNCS->pSerialPort->IsOpen())
		return(MN_ERR_CLOSED);
	pNodeInfo = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
	if (pNodeInfo->paramBankList == NULL)
		return(MN_ERR_PARAM_NOT_INIT);

	// Initialize the parameter cracker
	coreParam.bits = theParam;
	// Request makes sense?
	if (coreParam.fld.bank >= pNodeInfo->bankCount) {
		return MN_ERR_PARAM_RANGE;
	}
	// Create shortcut to the proper parameter bank
	pParamBank = &(pNodeInfo->paramBankList)[coreParam.fld.bank];

	if (coreParam.fld.param < pParamBank->nParams)  {
		paramValue optVal;
		// We are within our database, copy the basic information
		// to the user if requested. First speed up the operations
		// and clutter by initializing local values.

		// Initialize shortcuts
		pFixedInfoDB = pParamBank->fixedInfoDB + coreParam.fld.param;
		if (coreParam.fld.option)
			pValueDB = &optVal;
		else
			pValueDB = pParamBank->valueDB + coreParam.fld.param;
		// Save initial value
		initialVal = pValueDB->value;

		// Are they requesting extra information?
		if (pRetValInfo) {
			*pRetValInfo = pFixedInfoDB->info;
		} // if (pRetValInfo)

		//
		// If this is a real time parameter, never been read or the
		// Option Bit is set update the value DB now. If option bit
		// is set the acquired value is not cached.
		//
		if (((pFixedInfoDB->info.paramType & PT_RT) != 0)
		  || coreParam.fld.option
		  || !pValueDB->exists
		  || (pFixedInfoDB->info.paramType == PT_NONE)) {

			// Read from the node directly, real-time, EEPROM type or first time
			theErr = netGetParameter(theMultiAddr, theParam, &pValueDB->raw);
			// Kill the command execute bit for all opstates
			//_RPT2(_CRT_WARN, "netCore: get param(RT) node=%d, param=%d\n", theMultiAddr, theParam);
			// If we got value OK, convert to value
			if (theErr == MN_OK)  {
				// Update the database entry
				fromBaseUnit(theMultiAddr, coreParam, pNodeInfo,
						   pFixedInfoDB, pParamBank, pValueDB);
			}
		}

		// This parameter converted OK
		pValueDB->exists = (theErr == MN_OK);
		// So far so good?
		if (theErr != MN_OK)  {
			// Nope, make sure value is marked as junk
			if (pRetValInfo) {
				pRetValInfo->unitType = NO_UNIT;
				pRetValInfo->paramType  = PT_UNKNOWN;
			}
			return(theErr);
		}

		//
		// We have a value in pValueDB, return it via pRetVal after
		// performing possible scaling.
		//

		// Copy raw parts to user's buffer, assume no scaling
		*pRetVal = *pValueDB;
		// If this is a clear-on-read type, OR in with previous value(s)
		// and kill the DB copy now
		if ((pFixedInfoDB->info.paramType & PT_CLR) != 0)  {
			pRetVal->value = (unsigned long)pRetVal->value
								   | (unsigned long)initialVal;
			pValueDB->value = 0;
		}
		// Signal parameter change if this occurred	using user unit values
		if(initialVal != pRetVal->value) {
			paramChgObj.net = cNum;
			paramChgObj.node = theMultiAddr;
			paramChgObj.paramNum = coreParam.bits;
			paramChgObj.newValue = pRetVal->value;
			// Call the callback
			infcFireParamCallback(pNodeInfo->pClassInfo->paramChngFunc,
								  &paramChgObj);
			// If clear-on-read, kill the old value
			if (pFixedInfoDB->info.paramType & PT_CLR)
				pValueDB->value = 0;
		}
	}
	else {
		//
		// Off the end of the table.
		// Dummy up a parameter descriptor making this value an integer.
		//
		dummyInfo.info.reciprical = FALSE;
		dummyInfo.info.signextend = FALSE;
		dummyInfo.info.unitType = NO_UNIT;
		dummyInfo.info.paramType = PT_UNKNOWN;
		dummyInfo.info.scaleFromBase = 1;
		dummyInfo.info.signextend = ST_SIGNED;
		dummyInfo.info.descID = STR_UNKNOWN;
		dummyInfo.converter = NULL;

		// Get value in real-time from node into local area and return
		theErr = netGetParameter(theMultiAddr, theParam, &pRetVal->raw);
		if (theErr == MN_OK)  {
			// Dummy up the size from the received data
			dummyInfo.info.paramSize = pRetVal->raw.Byte.BufferSize;
			pRetVal->value = coreBufToDouble(&dummyInfo, pRetVal);
		}
		if (pRetValInfo)
			*pRetValInfo = dummyInfo.info;
		// Fire the callback if it exists
		paramChgObj.net = cNum;
		paramChgObj.node = theMultiAddr;
		paramChgObj.paramNum = coreParam.bits;
		paramChgObj.newValue = pRetVal->value;
		infcFireParamCallback(pNodeInfo->pClassInfo->paramChngFunc,
								  &paramChgObj);
	}
#ifdef TRACE_PARAMS
	_RPT3(_CRT_WARN, "netGetParameterInfo(0x%x,p=%d)=>%f\n", theMultiAddr, theParam, pRetVal->value);
#endif
	return(theErr);
}
/*																 !end!		*/
/****************************************************************************/

 
/*****************************************************************************
 *	NAME
 *		netSetParameterInfo
 *
 *	DESCRIPTION:
 *		This function will take a VB friendly double precision floating point
 *		value and convert it to the proper internal representation for the
 *		node and update the node and if this succeeds the value database will
 *		be updated to match this value.
 *
 *	RETURNS:
 *		cnErrCode, MN_OK if successful
 *
 *	SYNOPSIS: 															    */
cnErrCode netSetParameterInfo(
		multiaddr theMultiAddr,		// Node address
		nodeparam theParam,		   	// Parameter number
		const double theValue,	   	// New value
		nodebool verify)			// TRUE, verify write worked OK
{
	cnErrCode theErr;
	paramValue newParamValue, *pVal;
//	double oldValue;
	mnParamChg paramChgObj;			// Parameter change
	appNodeParam coreParam;			// Parameter cracker
	byNodeDB *pNodeInfo;			// Node Data
	paramBank *pParamBank;			// Current parameter bank
	paramInfoLcl const *pFixedInfoDB;	// Current parameter fixed info
	netaddr cNum;				// Current network
	// Vars for manipulating unknown parameters
	paramInfoLcl dummyInfo;			// Unknown parameter dummy info
	packetbuf dummyBuf;				// Scratch buffer
#ifdef TRACE_PARAMS
	_RPT3(_CRT_WARN, "netSetParameterInfo(0x%x,p=%d)<=%f\n", theMultiAddr, theParam,theValue);
#endif
	// Get node base info
	cNum = coreController(theMultiAddr);
	coreParam.bits = theParam;		// Initialize the parameter cracker

	pNodeInfo = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
	if (coreParam.fld.bank >= pNodeInfo->bankCount) {
		return(MN_ERR_BADARG);
	}
	// Create shortcut to the proper parameter bank
	pParamBank = &(pNodeInfo->paramBankList)[coreParam.fld.bank];
	//
	// Find out if this is a known or unknown parameter
	//

	if (coreParam.fld.param >= pParamBank->nParams
	|| pParamBank->fixedInfoDB[coreParam.fld.param].info.paramType==PT_NONE) {
		//
		// If parameter is out of range don't update the paramDB, get
		// a value to establish parameter size and pretend that it is an
		// integer.
		theErr = netGetParameter(theMultiAddr, theParam, &dummyBuf);
		if (theErr == MN_OK)  {
			// Read OK, assign set size to equal read size
			pVal = &newParamValue;
			pVal->raw.Byte.BufferSize = dummyBuf.Byte.BufferSize;
			// Dummy up an information record for this parameter
			pFixedInfoDB = &dummyInfo;
			dummyInfo.info.paramSize = dummyBuf.Byte.BufferSize;	// Size from get
			dummyInfo.info.paramType = PT_UNKNOWN;	// Unknown type
			dummyInfo.info.unitType = NO_UNIT;		// No units
			dummyInfo.info.reciprical = FALSE;		// Standard value
			dummyInfo.info.scaleFromBase = 1;		// No scaling
			dummyInfo.info.signextend = TRUE;		// Sign-extended
			dummyInfo.info.descID = STR_UNKNOWN;	// Description
			dummyInfo.converter = NULL;				// No converter
		}
		else {
			// Nothing returned is meaningful, just error out
			return(theErr);
		}
	}
	else {
		// Point at our parameter information to convert to bits
		pFixedInfoDB = pParamBank->fixedInfoDB+coreParam.fld.param;
		// Using the option bit?
		if (!coreParam.fld.option)  {
			// No, use the value database as the basis
			pVal = pParamBank->valueDB + coreParam.fld.param;
			// Save the old value
//			oldValue = pVal->value;
		}
		else {
			// Yes, dummy up the database entry for proper conversions
			pVal = &newParamValue;
			// Produce the callback for non-zero
//			oldValue = -theValue;
		}
	}
	//
	// We have parameter specifics, now convert to buffer format
	//
	pVal->raw.Byte.BufferSize = pFixedInfoDB->info.paramSize;
	// Set the proposed value in the database
	pVal->value = theValue;
	// Convert value to base unit in the "raw" buffer.
	toBaseUnit(theMultiAddr, coreParam, pNodeInfo, pFixedInfoDB, pParamBank, pVal);
	// Convert the "raw" base value to the standard display value
	fromBaseUnit(theMultiAddr, coreParam, pNodeInfo, pFixedInfoDB, pParamBank, pVal);
	//
	// The buffer is now loaded, send it off to the node.
	//
	theErr = netSetParameter(theMultiAddr, theParam, &pVal->raw);
	// Is there a registered callback and a value change or
	// this is off the end of the parameter table.
	if (theErr == MN_OK	|| (coreParam.fld.param > pParamBank->nParams)) {
		// Mark value OK if we sent OK
		pVal->exists = (theErr == MN_OK);
		// Make copy of the value
		newParamValue = *pVal;
		paramChgObj.net = cNum;
		paramChgObj.node = theMultiAddr;
		paramChgObj.paramNum = coreParam.bits;
		paramChgObj.newValue = newParamValue.value;
		// Call the callback
		infcFireParamCallback(pNodeInfo->pClassInfo->paramChngFunc,
							  &paramChgObj);
	}
	else {
		pVal->exists = FALSE;
	}
	return(theErr);
}
/*																 !end!		*/
/****************************************************************************/

 
/*****************************************************************************
 *	NAME
 *		toBaseUnit
 *
 *	DESCRIPTION:
 *		Convert the paramValue's value member to a properly scaled unit
 *		in the raw member's buffer.
 *
 *	SYNOPSIS: 															    */
void toBaseUnit(
	multiaddr theMultiAddr,					// Node address
	appNodeParam appParam,				// parameter number of interest
	byNodeDB *pNodeDB,					// ptr to this node's param data
	paramInfoLcl const *pFixedInfoDB,	// Current parameter fixed info
	paramBank *pBankInfo,				// Current bank
	paramValue *userVal)				// user value in their unit
{
	double convVal;

	// Custom converter defined?
	if (pFixedInfoDB->converter != NULL)  {
		// Yes, run it to convert the value member to "bits"
		convVal =
			(pFixedInfoDB->converter)(FALSE, theMultiAddr, appParam,
									  userVal->value, pNodeDB);
	}
	else {
		// Value requires no special conversions
		convVal = userVal->value;
	}
	// Convert the "base" number in the value field to the "raw" value
	coreDoubleToBuf(pFixedInfoDB, &userVal->raw, convVal);
}
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	NAME
 *		fromBaseUnit
 *
 *	DESCRIPTION:
 *		Convert the node's base unit value to the user's unit.
 *
 *	SYNOPSIS: 															    */
void fromBaseUnit(
	multiaddr theMultiAddr,				// Node address
	appNodeParam appParam,				// parameter number of interest
	byNodeDB *pNodeDB,					// ptr to this node's param data
	paramInfoLcl const *pFixedInfoDB,	// ptr to fixed info
	paramBank *pBankInfo,				// ptr to current parameter bank
	paramValue *pNodeVal)				// ptr to node's base value
{
	double pVal;

	// Using a custom converter?
	if (pFixedInfoDB->converter != NULL)  {
		// Do base level conversion
		pVal = coreBufToDouble(pFixedInfoDB, pNodeVal);
		// Convert using base level conversion of buffer
		pNodeVal->value = (pFixedInfoDB->converter)(TRUE, theMultiAddr, appParam,
									                pVal, pNodeDB);
		return;
	}
	else {
		// Do base level conversion
		pNodeVal->value = coreBufToDouble(pFixedInfoDB, pNodeVal);
	}
}
//																			 *
//****************************************************************************



//****************************************************************************
//	NAME																	 *
//		netEnterDataAcqMode
//
//	DESCRIPTION:
//		Start a node's data acquisition mode.
//
//	RETURNS:
//		cnErrCode, <startTime> set to node's time stamp at start of
//		acquisition.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netEnterDataAcqMode(multiaddr theMultiAddr,
												double *startTime)
{
	packetbuf theCmd, aResp;
	double theSampPeriod;
	cnErrCode theErr;

	if (startTime==NULL)
		return(MN_ERR_BADARG);
	//#if TRACE_HIGH_LEVEL
	//	_RPT2(_CRT_WARN, "%.1f netEnterDataAcqMode(%d)\n",
	//					  infcCoreTime(), theMultiAddr);
	//#endif
	netGetParameterDbl(theMultiAddr,
						MN_P_SAMPLE_PERIOD, &theSampPeriod);
	// Remember sample rate for return data
	infcInitDataAcq(theMultiAddr, theSampPeriod);

	// Kill old data
	infcFlushDataAcq(theMultiAddr);

	// Build command
	theCmd.Fld.Src = MN_SRC_HOST;
	theCmd.Fld.PktType = MN_PKT_TYPE_CMD;
	theCmd.Fld.Addr = NODE_ADDR(theMultiAddr);
	theCmd.Fld.PktLen = 2;
	theCmd.Fld.Zero1 = theCmd.Fld.Mode = 0;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_DATA_ACQ ;
	theCmd.Byte.Buffer[CMD_LOC+1] = SCP_DRATE;

	// Expect a response - it is a timestamp
	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &aResp);
	if (theErr == MN_OK) {
		*startTime = theSampPeriod*(*(nodeushort *)&aResp.Byte.Buffer[RESP_LOC]);
	}
	return(theErr);
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME																	 *
//		netExitDataAcqMode
//
//	DESCRIPTION:
//		Stop a node's data acquisition mode.
//
//	RETURNS:
//		cnErrCode, <startTime> set to node's time stamp at start of
//		acquisition.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netExitDataAcqMode(multiaddr theMultiAddr)
{
	packetbuf theCmd, aResp;

	//#if TRACE_HIGH_LEVEL
	//	_RPT2(_CRT_WARN, "%.1f netExitDataAcqMode(%d)\n",
	//					  infcCoreTime(), theMultiAddr);
	//#endif
	theCmd.Fld.Src = MN_SRC_HOST;
	theCmd.Fld.PktType = MN_PKT_TYPE_EXTEND_HIGH;
	theCmd.Fld.Addr = NODE_ADDR(theMultiAddr);
	theCmd.Fld.PktLen = 2;
	theCmd.Fld.Zero1 = theCmd.Fld.Mode = 0;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_DATA_ACQ;
	theCmd.Byte.Buffer[CMD_LOC+1] = SCP_OFF;
	return(netRunCommand(NET_NUM(theMultiAddr), &theCmd, &aResp));
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME
//		netGetErrStats
//
//	DESCRIPTION:
//		Get the network statistics for the specified node. The node's values
//		are cleared upon execution.
//
//	RETURNS:
//		cnErrCode, <netStats> updated
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetErrStats(
	multiaddr theMultiAddr,
	mnNetErrStats *pNetStats)
{
	cnErrCode theErr = MN_OK;

	double theDbl;
	mnParams appChksParam, appFragParam, appStrayParam, appOverrunParam;
	mnParams diagChksParam, diagFragParam, diagStrayParam, diagOverrunParam;
	mnParams netUnderVoltParam;

	mnNetStatus netState;
	theErr = netGetNodeAccessLvl(theMultiAddr, &netState);
	// Get each diagnostic element from the node
	switch(netState.Fld.NetParamVer) {
	case 0:
		appChksParam = MN_P_NETERR_APP_CHKSUM;
		appFragParam = MN_P_NETERR_APP_FRAG;
		appStrayParam = MN_P_NETERR_APP_STRAY;
		appOverrunParam = MN_P_NETERR_APP_OVERRUN;
		diagChksParam = MN_P_NETERR_DIAG_CHKSUM;
		diagFragParam = MN_P_NETERR_DIAG_FRAG;
		diagStrayParam = MN_P_NETERR_DIAG_STRAY;
		diagOverrunParam = MN_P_NETERR_DIAG_OVERRUN;
		netUnderVoltParam = MN_P_NETERR_LOW_VOLTS;
		break;
	default:
		// Newer compact base parameter map
		appChksParam = (mnParams)MN_P1_NETERR_APP_CHKSUM;
		appFragParam = (mnParams)MN_P1_NETERR_APP_FRAG;
		appStrayParam = (mnParams)MN_P1_NETERR_APP_STRAY;
		appOverrunParam = (mnParams)MN_P1_NETERR_APP_OVERRUN;
		diagChksParam = (mnParams)MN_P1_NETERR_DIAG_CHKSUM;
		diagFragParam = (mnParams)MN_P1_NETERR_DIAG_FRAG;
		diagStrayParam = (mnParams)MN_P1_NETERR_DIAG_STRAY;
		diagOverrunParam = (mnParams)MN_P1_NETERR_DIAG_OVERRUN;
		netUnderVoltParam = (mnParams)MN_P1_NETERR_LOW_VOLTS;
		break;
	}
	// Application Port
	theErr = netGetParameterDbl(theMultiAddr,
								appChksParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->ApplicationPort.nChecksumBad=(nodeulong)theDbl;

	theErr = netGetParameterDbl(theMultiAddr,
								appFragParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->ApplicationPort.nFragPkts=(nodeulong)theDbl;

	theErr = netGetParameterDbl(theMultiAddr,
								appStrayParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->ApplicationPort.nStrayChars=(nodeulong)theDbl;

	theErr = netGetParameterDbl(theMultiAddr,
								appOverrunParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->ApplicationPort.nOverflows=(nodeulong)theDbl;


	// Diagnostic Port
	theErr = netGetParameterDbl(theMultiAddr,
								diagChksParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->DiagnosticPort.nChecksumBad=(nodeulong)theDbl;

	theErr = netGetParameterDbl(theMultiAddr,
								diagFragParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->DiagnosticPort.nFragPkts=(nodeulong)theDbl;

	theErr = netGetParameterDbl(theMultiAddr,
								diagStrayParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->DiagnosticPort.nStrayChars=(nodeulong)theDbl;

	theErr = netGetParameterDbl(theMultiAddr,
								diagOverrunParam,&theDbl);
	if(theErr != MN_OK)
		return theErr;
	pNetStats->DiagnosticPort.nOverflows=(nodeulong)theDbl;

	// New items to try
	nodeIDs thisNodeType = netGetDevType(theMultiAddr);
	bool hasNetLowFeature = false;
	switch(thisNodeType) {
		case NODEID_MD:
			theErr = netGetParameterDbl(theMultiAddr,
										MN_P_FW_VERSION, &theDbl);
			hasNetLowFeature = (theErr == MN_OK
				&& (theDbl >= FW_MILESTONE_LINK_LOW));
			break;
		default:
			hasNetLowFeature = true;
			break;
	}
	// Get the accumlated counts
	if (hasNetLowFeature) {
		theErr = netGetParameterDbl(theMultiAddr,
									netUnderVoltParam,&theDbl);
		if (theErr == MN_OK)
			pNetStats->LowPowerEvents = (nodeulong)theDbl;
	}
	else {
		pNetStats->LowPowerEvents = 0;		// Assume zero
	}

	return(theErr);
}
//																			 *
//****************************************************************************



//****************************************************************************
//	NAME
//		netRunNetDiag
//
//	DESCRIPTION:
//		Run the network diagnostic function and accumulate data updating
//		the user's <stats> buffer.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netDiagsRun(
	netaddr cNum,
	mnNetDiagResults *stats)
{
	unsigned i;
	nodeulong nodesNow;
	cnErrCode theErr = MN_OK;
	mnNetDiagResult zeroes;

	//if (!SysInventory.valid)
	//	return(MN_ERR_PARAM_NOT_INIT);

	// Read the results
	//infcSetInitializeMode(cNum, TRUE, MN_OK);

	// Record the time we read this stuff
	NetDiags[cNum].sampleTime = infcCoreTime();
	NetDiags[cNum].nSamples++;

	// See if there are currently any nodes and if communications OK
	nodesNow = 0;
	theErr = infcProbeForOffline(cNum, &nodesNow);

	// Detect if network is not online - exit
	if (theErr != MN_OK) {
		return(theErr);
	}

	// Query the diagnostic data from each node and interpret it properly
	for (i=0; i<SysInventory[cNum].InventoryNow.NumOfNodes; i++) {
		// Invalidate the results until analyzed
		theErr = netGetErrStats(MULTI_ADDR(cNum,i), &NetDiags[cNum].node[i].lastErrs);
		NetDiags[cNum].node[i].netOK = NET_TEST_UNTESTED;
		if(theErr != MN_OK)
			return(theErr);

	}

	// Zero out "off the net items"
	for(i=SysInventory[cNum].InventoryNow.NumOfNodes; i<MN_API_MAX_NODES; i++) {
		NetDiags[cNum].node[i] = zeroes;
	}

	// Accumulate and analyze the results
	for (i=0; i<SysInventory[cNum].InventoryNow.NumOfNodes && SysInventory[cNum].InventoryNow.NumOfNodes>1; i++) {
		if (NetDiags[cNum].node[i].collectCode == MN_OK) {
			mnNetDiagResult	*mErr;
			mErr = &NetDiags[cNum].node[i];
			// Update the network problem statistics
			mErr->accumErrs.ApplicationPort.nChecksumBad
			+= mErr->lastErrs.ApplicationPort.nChecksumBad;
			mErr->accumErrs.ApplicationPort.nFragPkts
			+= mErr->lastErrs.ApplicationPort.nFragPkts;
			mErr->accumErrs.ApplicationPort.nStrayChars
			+= mErr->lastErrs.ApplicationPort.nStrayChars;
			mErr->accumErrs.ApplicationPort.nOverflows
			+= mErr->lastErrs.ApplicationPort.nOverflows;

			mErr->accumErrs.DiagnosticPort.nChecksumBad
			+= mErr->lastErrs.DiagnosticPort.nChecksumBad;
			mErr->accumErrs.DiagnosticPort.nFragPkts
			+= mErr->lastErrs.DiagnosticPort.nFragPkts;
			mErr->accumErrs.DiagnosticPort.nStrayChars
			+= mErr->lastErrs.DiagnosticPort.nStrayChars;
			mErr->accumErrs.DiagnosticPort.nOverflows
			+= mErr->lastErrs.DiagnosticPort.nOverflows;
			mErr->accumErrs.LowPowerEvents += mErr->lastErrs.LowPowerEvents;

			// Mark as problem if we see too many frags or strays
			if (mErr->accumErrs.ApplicationPort.nChecksumBad > NET_CHECKSUMS_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.ApplicationPort.nFragPkts > NET_FRAGS_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.ApplicationPort.nStrayChars > NET_STRAYS_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.ApplicationPort.nOverflows > NET_OVERRUN_BAD)
				mErr->netOK = NET_TEST_FAILED;

			if (mErr->accumErrs.DiagnosticPort.nChecksumBad > NET_CHECKSUMS_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.DiagnosticPort.nFragPkts > NET_FRAGS_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.DiagnosticPort.nStrayChars > NET_STRAYS_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.DiagnosticPort.nOverflows > NET_OVERRUN_BAD)
				mErr->netOK = NET_TEST_FAILED;
			if (mErr->accumErrs.LowPowerEvents > 1)
				mErr->netOK = NET_TEST_FAILED;

			// If we didn't detect any problems set the node state to PASSED
			if (mErr->netOK == NET_TEST_UNTESTED)
				mErr->netOK = NET_TEST_PASSED;
		}
	}
	// Return results if they want them
	if (stats != NULL) {
		*stats = NetDiags[cNum];
	}

	if (theErr != MN_OK) {
		return(theErr);
	}
	//infcSetInitializeMode(cNum, FALSE, theErr);
	return(theErr);
}
//																			 *
//****************************************************************************





//****************************************************************************
//	NAME
//		netDiagsGet
//
//	DESCRIPTION:
//		Return the driver's network diagnostics data.
//
//	RETURNS:
//		cnErrCode, *stats updated with driver state
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netDiagsGet(netaddr cNum,
    mnNetDiagResults *pStats)
{
	_RPT1(_CRT_WARN, "sizeof(mnNetDiagResults)=%ld\n", sizeof(mnNetDiagResults));
	*pStats = NetDiags[cNum];
	return(MN_OK);
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		netDiagsClr
//
//	DESCRIPTION:
//		Clear the accumulated network diagnostic information.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netDiagsClr(
	netaddr cNum)
{
	unsigned i;
	mnNetDiagResult zeroes;

	for (i=0; i<MN_API_MAX_NODES; i++) {
		NetDiags[cNum].node[i] = zeroes;
	}
	NetDiags[cNum].startTime = infcCoreTime();
	NetDiags[cNum].sampleTime = NetDiags[cNum].startTime;
	NetDiags[cNum].nSamples = 0;
	return(MN_OK);
}
/****************************************************************************/



//*****************************************************************************
//	NAME																	  *
//		coreFreeAllNodeInfo
//
//	DESCRIPTION:
//		Return the entire node information DB for this net.
//
//	SYNOPSIS:
void coreFreeAllNodeInfo(netaddr cNum)
{
	unsigned nodeAddr;

#if TRACE_HEAP
	infcHeapCheck("coreFreeAllNodeInfo start");
#endif

	for (nodeAddr=0 ; nodeAddr<MN_API_MAX_NODES ; nodeAddr++) {
		//WCHAR foo[200];
		byNodeDB *pNodeDB = &SysInventory[cNum].NodeInfo[nodeAddr];
		if (pNodeDB->delFunc) {
			//wsprintf(foo,L"coreFreeAllNodeInfo net %d node %d\n", cNum, nodeAddr);
			//OutputDebugString(foo);
			(*pNodeDB->delFunc)(MULTI_ADDR(cNum, nodeAddr));
		}
	}
#if TRACE_HEAP
	infcHeapCheck("coreFreeAllNodeInfo exit");
#endif
}
//																			  *
//*****************************************************************************


/*****************************************************************************
 *	NAME
 *		coreInvalidateValCache
 *
 *	DESCRIPTION:
 *		Invalidate all the value cache items for this controller
 *
 *	SYNOPSIS: 															    */
void coreInvalidateValCache(netaddr cNum)
{
	unsigned nodeAddr;

	for (nodeAddr=0 ; nodeAddr<MN_API_MAX_NODES ; nodeAddr++) {
            coreInvalidateValCacheByNode(cNum, nodeAddr);
	}
}
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	NAME
 *		coreInvalidateValCacheByNode
 *
 *	DESCRIPTION:
 *		Invalidate all the value cache items for a particular node
 *
 *	SYNOPSIS: 															    */
void coreInvalidateValCacheByNode(netaddr cNum, nodeaddr nodeAddr)
{
	unsigned bank, param;
	paramBank *pBank;

        for (bank=0; bank < SysInventory[cNum].NodeInfo[nodeAddr].bankCount; bank++) {
            pBank = &SysInventory[cNum].NodeInfo[nodeAddr].paramBankList[bank];
			for (param=0; pBank && param < pBank->nParams; param++) {
				if (pBank && pBank->valueDB) {
					pBank->valueDB[param].exists = FALSE;
					pBank->valueDB[param].value = 0;
				}
            }
        }
}
/*																 !end!		*/
/****************************************************************************/


//****************************************************************************
//	NAME
//		netClrParamCache
//
//	DESCRIPTION:
//		Invalidate the parameter cache for this node and parameter. This will
//		cause the driver to re-read the value on the next get parameter
//		 request.
//
//	RETURNS:
//		#cnErrCode: MN_OK on success, otherwise a specific error
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netClrParamCache(
	multiaddr theMultiAddr,
	mnParams parameter)
{
	paramBank *pBank;
	appNodeParam pCrack;

	pCrack.bits = parameter;

	netaddr cNum = coreController(theMultiAddr);
	nodeaddr nodeAddr = NODE_ADDR(theMultiAddr);

	// Is the address ok for this net
	if (nodeAddr >= SysInventory[cNum].InventoryNow.NumOfNodes)
		return(MN_ERR_PARAM_RANGE);

	// Param out of range?
	if (pCrack.fld.bank > SysInventory[cNum].NodeInfo[nodeAddr].bankCount)
		return(MN_ERR_PARAM_RANGE);

	// Get bank list for this parameter
	pBank = &SysInventory[cNum].NodeInfo[nodeAddr].paramBankList[pCrack.fld.bank];

	// Param out of range?
	if (!pBank || pBank->nParams <= pCrack.fld.param)
		return(MN_ERR_PARAM_RANGE);

	// Kill the cached value and mark as unread
	pBank->valueDB[pCrack.fld.param].exists = FALSE;
	pBank->valueDB[pCrack.fld.param].value = 0;

	return(MN_OK);
}
/****************************************************************************/



//****************************************************************************
//	NAME
//		coreSetParamFromBytes
//
//	DESCRIPTION:
//		Set the parameter database as though the SetParameter was run using
//		the specified value byte string <pVal>.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
cnErrCode coreSetParamFromBytes(multiaddr theMultiAddr,
								 nodeparam paramNum,
								 nodeuchar *pVal,
								 unsigned sizeInBytes)
{
	cnErrCode theErr = MN_OK;
	netaddr cNum;
	paramValue *pTheVal,uVal;			// Ptr to current value item
	double oldValue;
	mnParamChg paramChgObj;				// Parameter change object
	appNodeParam coreParam;				// Parameter cracker
	byNodeDB *pNodeInfo;				// Node Data
	paramBank *pParamBank;				// Current parameter bank
	paramInfoLcl const *pFixedInfoDB;	// Current parameter fixed info

	// Get node number and current controller
	cNum = coreController(theMultiAddr);
	// Bail if port closed/offline
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS || !(pNCS->pSerialPort) || !pNCS->pSerialPort->IsOpen())
		return(MN_ERR_CLOSED);
	pNodeInfo = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
	if (pNodeInfo == NULL || pNodeInfo->paramBankList == NULL)
		return MN_ERR_PARAM_NOT_INIT;
	coreParam.bits = paramNum;
	if (pNodeInfo->bankCount <= coreParam.fld.bank)
		return MN_ERR_PARAM_RANGE;
	// Shortcut to the parameter bank
	pParamBank = pNodeInfo->paramBankList + coreParam.fld.bank;
	// Calc ptr to value area
	pTheVal = &pParamBank->valueDB[coreParam.fld.param];
	pFixedInfoDB = &pParamBank->fixedInfoDB[coreParam.fld.param];
	oldValue = pTheVal->value;
	// Update the raw information area
	cpymem(pVal, (nodeuchar *)&pTheVal->raw.Byte.Buffer[0], sizeInBytes);
	pTheVal->raw.Byte.BufferSize = sizeInBytes;
	fromBaseUnit(theMultiAddr, coreParam, pNodeInfo,
			   pFixedInfoDB, pParamBank, pTheVal);
	// Register the change
	if (oldValue != pTheVal->value
	   || ((pFixedInfoDB->info.paramType & PT_CLR)
		   && (pTheVal->value != 0))) {
		uVal = *pTheVal;
		// Call the user callback
		paramChgObj.node = theMultiAddr;
		paramChgObj.net = cNum;
		paramChgObj.paramNum = paramNum;
		paramChgObj.newValue = uVal.value;
		infcFireParamCallback(pNodeInfo->pClassInfo->paramChngFunc,
							  &paramChgObj);
		// If clear-on-read, kill the old value
		if (pFixedInfoDB->info.paramType & PT_CLR)
			pTheVal->value = 0;
	}
	return(theErr);
}
/****************************************************************************/



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*  C O M M O N    C O N V E R T E R S   								   */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//****************************************************************************
//	NAME
//		coreBaseGetParameterInt
//
//	DESCRIPTION:
//		Get a 16-bit parameter using just base commands. This function only
//		works when the read thread is not running.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
cnErrCode coreBaseGetParameterInt(netaddr cNum, nodeaddr theNode,
								  nodeparam paramNum, Uint16 * pVal)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr = MN_OK;

	#if MN_PKT_TYPE_CMD!=0
	#error "assumption on ADDR_LOC not true"
	#endif

	theCmd.Byte.BufferSize = 4;
	theCmd.Byte.Buffer[ADDR_LOC] = 0;
	theCmd.Byte.Buffer[LEN_LOC] = 0;
	theCmd.Fld.Addr = theNode;
	theCmd.Fld.PktLen = 2;
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_GET_PARAM0;
	theCmd.Byte.Buffer[CMD_LOC+1] = (nodechar)paramNum;
	//theErr = infcRunCommandRaw(cNum, &theCmd, &theResp);
	theErr = infcRunCommand(cNum, &theCmd, &theResp);
	if (theErr == MN_OK)  {
		*pVal = *(nodeshort *)&theResp.Byte.Buffer[RESP_LOC] & 0xFFFF;
	}
	return(theErr);
}
cnErrCode coreBaseGetParameterLong(netaddr cNum, nodeaddr theNode,
								  nodeparam paramNum, int *pVal)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr = MN_OK;

	#if MN_PKT_TYPE_CMD!=0
	#error "assumption on ADDR_LOC not true"
	#endif

	theCmd.Byte.BufferSize = 4;
	theCmd.Byte.Buffer[ADDR_LOC] = 0;
	theCmd.Byte.Buffer[LEN_LOC] = 0;
	theCmd.Fld.Addr = theNode;
	theCmd.Fld.PktLen = 2;
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_GET_PARAM0;
	theCmd.Byte.Buffer[CMD_LOC+1] = (nodechar)paramNum;
//	theErr = infcRunCommandRaw(cNum, &theCmd, &theResp);
	theErr = infcRunCommand(cNum, &theCmd, &theResp);
	if (theErr == MN_OK)  {
		*pVal = *(nodelong *)&theResp.Byte.Buffer[RESP_LOC] & 0xFFFFFFFF;
	}
	return(theErr);
}
/****************************************************************************/



//****************************************************************************
//	NAME
//		coreBaseSetParameterInt
//
//	DESCRIPTION:
//		Set a 16-bit parameter using just base commands. This function only
//		works when the read thread is not running.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
cnErrCode coreBaseSetParameterInt(netaddr cNum, nodeaddr theNode,
								  nodeparam paramNum, int pVal)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr = MN_OK;

	#if MN_PKT_TYPE_CMD!=0
	#error "assumption on ADDR_LOC not true"
	#endif

	theCmd.Byte.BufferSize = 6;
	theCmd.Byte.Buffer[ADDR_LOC] = 0;
	theCmd.Byte.Buffer[LEN_LOC] = 0;
	theCmd.Fld.Addr = theNode;
	theCmd.Fld.PktLen = 4;
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_SET_PARAM0;
	theCmd.Byte.Buffer[CMD_LOC+1] = (nodechar)paramNum;
	theCmd.Byte.Buffer[CMD_LOC+2] = pVal & 0xff;
	theCmd.Byte.Buffer[CMD_LOC+3] = (pVal>>8) & 0xff;
	//theErr = infcRunCommandRaw(cNum, &theCmd, &theResp);
	theErr = infcRunCommand(cNum, &theCmd, &theResp);
	if (theErr == MN_OK)  {
		theErr = coreGenErrCode(cNum, &theResp, theResp.Fld.Addr);
	}
	return(theErr);
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		coreInitializeEx
//
//	DESCRIPTION:
//		Perform common initializing functions for the specified node class.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
cnErrCode MN_DECL coreInitializeEx(
		multiaddr theMultiAddr,
		nodebool warmInitialize,
		coreParamSetupFuncPtr classSetupFunc)
{
	cnErrCode errRet = MN_OK;
	netaddr cNum = NET_NUM(theMultiAddr);

	#if TRACE_HIGH_LEVEL&0
	_RPT2(_CRT_WARN, "%.1f coreInitializeEx(%d)\n",
			infcCoreTime(), theMultiAddr);
	#endif

	//
	// Wait for commands to sync in this thread, if we are talking on the net
	//
	infcWaitForNetStop(cNum);

	// Signal we are initializing
	infcSetInitializeMode(cNum, TRUE, MN_OK);

	infcFireNetEvent(cNum, NODES_SENDING);

	// Setup the node
	errRet = (*classSetupFunc)(theMultiAddr);
	// Relieve another init count / this error
	infcSetInitializeMode(cNum, FALSE, errRet);
	return(errRet);
}
/****************************************************************************/


//*****************************************************************************
//	NAME																	  *
//		netAlertLogEpoch
//
//	DESCRIPTION:
///		Setup the node's Alert epoch entry to the current system time.
///
/// 	\param[in] theMultiAddr The address code for this node.
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netAlertLogEpoch(
		multiaddr theMultiAddr)		// Node address
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;
	time_t timeNowZ;				// Current time in UTC
	time_t *pktTime
		= reinterpret_cast<time_t *>(&theCmd.Byte.Buffer[CMD_LOC+1]);

	time(&timeNowZ);

	// Convert to Merdian epoch
	timeNowZ = MERIDIAN_EPOCH_CTIME - timeNowZ;

	// Start with basic command
	theCmd.Fld.SetupHdr(MN_PKT_TYPE_CMD, NODE_ADDR(theMultiAddr), MN_SRC_HOST);
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_ALERT_LOG;
	// Signal epoch adjust by sending 4 octets
	theCmd.Fld.PktLen = 4;
	// Copy our epoch adjustment
	*pktTime = timeNowZ;
	// Run it
	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp);
	// Send to node and wait for null response.
	if (theErr == MN_OK) {
		// Should return null packet
		if (theResp.Fld.PktLen)
			return(MN_ERR_RESP_FMT);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************

/// \endcond

// =============================================================================
// DOCUMENTED FUNCTIONS BELOW HERE
// =============================================================================

/**
	\addtogroup LinkGrp
	@{
**/

//******************************************************************************
//	NAME																	   *
//		netGetParameter
//
//	DESCRIPTION:
/**
	This function will get the bit-oriented parameter specified by \a
	theParamNum on device \a theMultiAddr. The parameter is returned in the
	buffer pointed to by \a paramInfo. The actual length and information
	will be updated when this function returns MN_OK.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theParamNum The parameter code number for retrieval.
	\param[out] pParamBuf Ptr to result area

	\return	MN_OK if the parameter was retrieved and the \p pParamBuf

	\see mnSysInventoryRecord function
	\see MULTI_ADDR macro
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetParameter(
		multiaddr theMultiAddr, 			// device address
		nodeparam theParamNum,		// parameter index
		packetbuf *pParamBuf)		// ptr for returned param

{
	packetbuf cmd,resp;				// Input and output buffers
	cnErrCode theErr;
	netaddr cNum;

	cNum = coreController(theMultiAddr);

	if ((theErr = netGetParameterFmt(&cmd, NODE_ADDR(theMultiAddr), theParamNum)) == MN_OK)  {
		if ((theErr = netRunCommand(cNum, &cmd, &resp)) == MN_OK)  {
			theErr = netGetParameterExtract(&resp, pParamBuf);
		}
		else {
			pParamBuf->Fld.PktLen = 0;
			pParamBuf->Byte.BufferSize = 0;
		}
	}
#if _DEBUG
	if (theErr != MN_OK) {
			_RPT3(_CRT_WARN,
				"netGetParameter(multiaddr=%d,param=%d failed: 0x%X\n",
				theMultiAddr, theParamNum, theErr);	
			DUMP_PKT(cNum, "...the cmd", &cmd);
	}
#endif
	return theErr;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netSetParameter
//
//	DESCRIPTION:
/**
	This function will run the set parameter command for device \a theMultiAddr
	at parameter index \a theParamNum. The parameter value is defined in the
	\a paramInfo area and the \a paramSize field defines the actual size.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theParamNum The parameter code number for retrieval.
	\param[in] pNewValue Bit-oriented area that will be sent to node to update
			   the parameter.

	\return MN_OK if the value was successfully updated.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netSetParameter(
		multiaddr theMultiAddr, 			// node address
		nodeparam theParamNum, 		// parameter index
		packetbuf *pNewValue)		// ptr to new parameter data
{
	packetbuf theCmd;
	cnErrCode theErr;
	netaddr cNum;
	packetbuf theResp;
	nodeaddr respAddr;
	cNum = coreController(theMultiAddr);

	if ((theErr = netSetParameterFmt(&theCmd,
								     NODE_ADDR(theMultiAddr),
								     theParamNum,
								     pNewValue)) == MN_OK) {
		theErr = netRunCommand(cNum, &theCmd, &theResp);
		if(theErr == MN_OK) {	// Figure out if return is
			respAddr = theResp.Fld.Addr;
			theErr= coreGenErrCode(cNum, &theResp, respAddr);
		}
	}
#if _DEBUG	
	if (theErr != MN_OK) {
		_RPT3(_CRT_WARN,
			"netSetParameter(multiaddr=%d,param=%d) failed: 0x%X\n",
			theMultiAddr, theParamNum, theErr);	
		DUMP_PKT(cNum, "..the cmd: ", &theCmd);
	}
#endif
	return (theErr);
}
//																			 *
//****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		netGetPartNumber
//
//	DESCRIPTION:
/**
	This function will fill the user's buffer with the addressed node's
	part number.

	\param[in] theMultiAddr The address code for this node.
	\param partNumBuf Ptr to response are
	\param maxBufSize size of the \p paramNumBuf

	\return MN_OK if the \p paramNumBuf is updated from the node.
**/
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetPartNumber(
	multiaddr theMultiAddr,
	char *partNumBuf,
	Uint16 maxBufSize)
{
	cnErrCode theErr;
	unsigned i, nChars;
	int nVerChars;
	paramValue versionCode, options, partNum;
	const unsigned verStrMax = 15;
	char verStr[verStrMax];
	optionReg opts;

	// Get version code
	theErr = netGetParameterInfo(theMultiAddr, MN_P_FW_VERSION, NULL, &versionCode);
	if (theErr != MN_OK)
		return(theErr);

	// Get this device's option register
	theErr = netGetParameterInfo(theMultiAddr, MN_P_OPTION_REG, NULL, &options);
	if (theErr != MN_OK)
		return(theErr);

	// Get the part number information
	theErr = netGetParameterInfo(theMultiAddr, MN_P_PART_NUM, NULL, &partNum);

	if (theErr == MN_OK) {
		// Limit return size to buffer size
		nChars = (partNum.raw.Byte.BufferSize < maxBufSize)
			? partNum.raw.Byte.BufferSize : maxBufSize;
		// Copy and test characters for sanity until end of string
		for (i=0; i<nChars && partNum.raw.Byte.Buffer[i]; i++) {
			// Make sure we are ANSI like
			if (partNum.raw.Byte.Buffer[i] > 127
			|| partNum.raw.Byte.Buffer[i] < ' ') {
				strncpy(partNumBuf, "N/A", maxBufSize);
				return(MN_OK);
			}
			partNumBuf[i] = partNum.raw.Byte.Buffer[i];
		}
		// Save as the exit location
		nChars = i;
		// Make <partNum>-
		if (nChars<maxBufSize) {
			partNumBuf[nChars++] = '-';
		}
		// Create version string
		versID_t verCracker;
		verCracker.verCode = (Uint16)versionCode.value;
		opts.bits = (unsigned)options.value;
		if(opts.DrvFld.FWverMask) {
			nVerChars = sprintf(verStr, "%d-%d-D", verCracker.fld.majorVers,
													verCracker.fld.minorVers)+1;
		} else {
			nVerChars = sprintf(verStr, "%d-%d-%d", verCracker.fld.majorVers,
													verCracker.fld.minorVers,
													verCracker.fld.buildVers)+1;
		}
		// Insure it will fit
		if ((nVerChars+nChars)>maxBufSize)
			nVerChars = maxBufSize-nChars;		// Rest of buffer
		if (nVerChars > 0) {
			// Append to base part
			strncpy(&partNumBuf[nChars], verStr, nVerChars);
		}
		// Insure string terminated even if truncated
		partNumBuf[maxBufSize-1] = 0;
	}
	else {
		partNumBuf[0] = 0;
	}
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netNodeStop
//
//	DESCRIPTION:
/**
	For broadcast style usage:
	- the stopType is inspected and if not STOP_TYPE_IGNORE the
	short form high priority packet is sent forcing the node to use
	it's default stop type upon command reception. The network is
	derived from the the \a theMultiAddr argument and the node address
	is ignored. Therefore the address of any node on that
	network can be used as the argument.
	.

	For addressed
	- Run the specified \a stopType on the addressed node.
	.

	To clear an existing node stop condition such as E-Stop, the
	clear type must be specified with the modifier bits set for
	the condition you want to clear.

	\param[in] theMultiAddr The address code for this node.
	\param[in] stopType Stopping strategy
	\param[in] broadcast Send to all nodes, the address is ignored in this mode.

	\return MN_OK when the node stop has been received by all the nodes.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netNodeStop(
		multiaddr theMultiAddr, 	// node address/net
		mgNodeStopReg stopType,		// stop strategy
		nodebool broadcast)			// Send as high prio-broadcast
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;
	netaddr cNum;
	cNum = coreController(theMultiAddr);
	if (!broadcast) {
		// Send via regular command
		theCmd.Fld.Addr = NODE_ADDR(theMultiAddr);
		theCmd.Fld.PktLen = 2;
		theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_NODE_STOP;
		theCmd.Byte.Buffer[CMD_LOC+1] = nodechar(stopType.bits);

		theErr = netRunCommand(cNum, &theCmd, &theResp);
	}
	else {
		// Send via a high-priority packet

		// Setup hi-prio pkt and user 0 address for now
		theCmd.Fld.SetupHdr(MN_PKT_TYPE_EXTEND_HIGH, 0);
		// Signal this is broadcast
		theCmd.Fld.Mode = 1;
		theCmd.Byte.Buffer[CMD_LOC] = MN_CTL_EXT_NODE_STOP;
		theCmd.Byte.Buffer[CMD_LOC+1] = nodechar(stopType.bits);
		// If stop type is zero, force the default stop type
		if (stopType.bits == STOP_TYPE_IGNORE) {
			theCmd.Fld.PktLen = 1;
		}
		else {
			theCmd.Fld.PktLen = 2;
		}
		// Setup for low level command
		theCmd.Byte.BufferSize = theCmd.Fld.PktLen + MN_API_PACKET_HDR_LEN;
		// Run via low level command interface to avoid changing to
		// straight command.
		theErr = infcRunCommand(cNum, &theCmd, &theResp);
		// We should get back what we sent.
		if (theErr != MN_OK ||
		    (theResp.Byte.Buffer[0] != theCmd.Byte.Buffer[0])
		||  (theResp.Byte.Buffer[1] != theCmd.Byte.Buffer[1])
		||  (theResp.Byte.Buffer[2] != theCmd.Byte.Buffer[2])
		||  (theCmd.Fld.PktLen == 2
				&& theResp.Byte.Buffer[3] != theCmd.Byte.Buffer[3]))
			return(MN_ERR_RESP_FMT);
	}
	return (theErr);
}
//																		      *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netTrigger
//
//	DESCRIPTION:
/**
	Send a <i>Trigger</i> control packet through the network.  If \a usingGroup is
	false, only the node with a matching address will interpret
	the <i>Trigger Event</i>.  Otherwise the address signifies a group trigger, and
	in this case the address field is interpreted as the group ID.

	The group ID of zero cannot be assigned to any node and therefore will not
	cause a trigger.

	\param cNum Channel number to restart. The first channel is zero.
	\param nodeOrGroupAddr Depending on the \a usingGroup setting, this is
				the group number of the node address on the channel.
	\param usingGroup Specifies the meaning of the \a nodeOrGroupAddr field.

	\return #cnErrCode; MN_OK when the network has finished triggering.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netTrigger(
		netaddr cNum,				// Net number
		nodeaddr nodeOrGroupAddr,	// Node or Group Addr
		nodebool usingGroup)		// Node(s) selection(s)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	// Build the packet
	theCmd.Fld.SetupHdr(MN_PKT_TYPE_TRIGGER, nodeOrGroupAddr);
	theCmd.Fld.PktLen = 0;
	theCmd.Byte.BufferSize = 2;
	// Pkt mode indicates group trigger
	theCmd.Fld.Mode = usingGroup;

	// Send the packet and check for correct response
	theErr = infcRunCommand(cNum, &theCmd, &theResp);
	if (theErr == MN_OK && (theResp.Fld.PktType != MN_PKT_TYPE_TRIGGER))
			return(MN_ERR_RESP_FMT);
	return (theErr);
}
//																			 *
//****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netAlertLogClear
//
//	DESCRIPTION:
///		Clear the entries out of node's alert log.
///
/// 	\param[in] theMultiAddr The address code for this node.
///		\return MN_OK if the value was successfully updated.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netAlertLogClear(
		multiaddr theMultiAddr)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	// Start with basic command
	theCmd.Fld.SetupHdr(MN_PKT_TYPE_CMD, NODE_ADDR(theMultiAddr), MN_SRC_HOST);
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_ALERT_LOG;
	theCmd.Byte.Buffer[CMD_LOC+1] = (nodechar)ALERT_ARG_CLEAR;
	theCmd.Fld.PktLen = 2;

	// Run it
	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp);
	if (theErr == MN_OK) {
		// Should return null packet
		if (theResp.Fld.PktLen)
			return(MN_ERR_RESP_FMT);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		netAlertLogGet
//
//	DESCRIPTION:
///		Get the next newest, or the oldest, if \p restart is true, Alert log
///		entry.
///
/// 	\param theMultiAddr
///				target net/node
/// 	\param restart
///				set true to get oldest item.
/// 	\param pEntryExists
///				set true if the entry has been update in pEntry.
/// 	\param pEntry
///				updated with first/next entry if pEntryExists was true.
///
///		\return MN_OK if the entry was successfully retrieved.
///
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netAlertLogGet(
		multiaddr theMultiAddr,					// Node address
		nodebool restart,						// Restart the list from oldest
		nodebool *pEntryExists,					// Updated with TRUE if <pEntry> OK
		alertLogEntry *pEntry)					// Update user's buffer
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	// No crashes by us
	if (!pEntryExists || !pEntry)
		return(MN_ERR_BADARG);
	// Assume failure until proven OK
	*pEntryExists = FALSE;

	// Start with basic command
	theCmd.Fld.SetupHdr(MN_PKT_TYPE_CMD, NODE_ADDR(theMultiAddr), MN_SRC_HOST);
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_ALERT_LOG;
	theCmd.Byte.Buffer[CMD_LOC+1] = ALERT_ARG_GET_OLDEST;
	// If they want reset, send the "arg", else send "null" arg
	theCmd.Fld.PktLen = restart ? 2 : 1;

	// Run it
	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp);
	if (theErr == MN_OK) {
		// Did the log change since last read?
		if (theResp.Fld.PktLen == 1 && theResp.Byte.Buffer[RESP_LOC] == 0)
			return(MN_ERR_LOG_CHANGED);

		// If we have a response, it must be 5 octets long (time stamp + code)
		*pEntryExists = theResp.Fld.PktLen == 5;
		if (*pEntryExists) {
			alertLogEntry *pAlert = reinterpret_cast<alertLogEntry *>
										(&theResp.Byte.Buffer[RESP_LOC]);

			// Clear MSBs to convert nodechar to enum
			theResp.Byte.Buffer[RESP_LOC+5] = 0;
			theResp.Byte.Buffer[RESP_LOC+6] = 0;
			theResp.Byte.Buffer[RESP_LOC+7] = 0;

			if (pAlert->Reason == ALERT_LOG_EPOCH_CODE){
				//TODO: Need to check MSB of timestamp to determine what
				//type it is; currently, only the restart-initiated epoch
				//type has been implemented; we still need to handle the
				//user-defined epoch type!!

				// Adjust the timestamp to clear out the user-defined epoch
				// type flag
				pAlert->Timestamp *= 2;
			}

			// Copy to user's buffer and clean up
			*pEntry = *pAlert;
		}
		// Or NULL response if we have read them all
		else if(theResp.Fld.PktLen!=0) {
			return(MN_ERR_RESP_FMT);
		}
	}
	return(theErr);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		netAlertClear
//
//	DESCRIPTION:
///		Clear and/or acknowledge items from the Alert Register. Serious
///		errors cannot be cleared via this mechanism.
///
///		\param[in] theMultiAddr The address code for this node.
///
///		\return MN_OK if the log was cleared.
///
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netAlertClear(
		multiaddr theMultiAddr)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	// Start with basic command
	theCmd.Fld.SetupHdr(MN_PKT_TYPE_CMD, NODE_ADDR(theMultiAddr), MN_SRC_HOST);
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_ALERT_CLR;
	theCmd.Fld.PktLen = 1;

	// Run it
	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp);
	if (theErr == MN_OK) {
		// Should return null packet
		if (theResp.Fld.PktLen)
			return(MN_ERR_RESP_FMT);
	}
	return(theErr);
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		netGetAlertReg
//
//	DESCRIPTION:
/**
	Get the current state of the Alert Register.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pAlertReg Ptr to result area.

	\return
		cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetAlertReg(
			multiaddr theMultiAddr,			// Node address
			alertReg *pAlertReg)			// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_ALERT_REG, NULL, &paramVal);
	*pAlertReg = *((alertReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netGetWarnReg
//
//	DESCRIPTION:
/**
	Get the current state of the Warning Register.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pWarningReg Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetWarnReg(
			multiaddr theMultiAddr,			// Node address
			alertReg *pWarningReg)			// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	// Check parameter location

	nodeIDs theNodeType = netGetDevType(theMultiAddr);
	if (theNodeType == NODEID_MD)
		theErr = netGetParameterInfo(theMultiAddr, MN_P_WARN_REG,
									NULL, &paramVal);
	else
		theErr = netGetParameterInfo(theMultiAddr, (mnParams)MN_P1_WARN_REG,
									NULL, &paramVal);

	*pWarningReg = *((alertReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netGetStatusRTReg
//
//	DESCRIPTION:
/**
	Get the current state of the <i>Status Real-Time Register</i>.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pReg Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetStatusRTReg(
			multiaddr theMultiAddr,			// Node address
			mnStatusReg *pReg)				// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_STATUS_RT_REG,
									NULL, &paramVal);
	*pReg = *((mnStatusReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netGetAttnStatusRiseReg
//
//	DESCRIPTION:
/**
	Get the current state of the <i>Status Attn/Rise Register</i>.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pReg Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetAttnStatusRiseReg(
			multiaddr theMultiAddr,			// Node address
			mnStatusReg *pReg)				// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_STATUS_ATTN_RISE_REG,
									NULL, &paramVal);
	*pReg = *((mnStatusReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netGetStatusAccumReg
//
//	DESCRIPTION:
/**
	Get the current state of the <i>Status Accumulating Register</i>.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pReg Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetStatusAccumReg(
			multiaddr theMultiAddr,			// Node address
			mnStatusReg *pReg)				// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_STATUS_ACCUM_REG,
									NULL, &paramVal);
	*pReg = *((mnStatusReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************



//******************************************************************************
// NAME																		   *
//		netGetDevType
//
//	DESCRIPTION:
/**
	Return the device type of the specified node \a theMultiAddr.

	\param[in] theMultiAddr The address code for this node.

	\return
		Specified device type
**/
//	SYNOPSIS:
MN_EXPORT nodeIDs MN_DECL netGetDevType(
		multiaddr theMultiAddr)
{
	netaddr cNum = NET_NUM(theMultiAddr);
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return NODEID_UNK;

	return((nodeIDs)SysInventory[cNum].NodeInfo
							[NODE_ADDR(theMultiAddr)].theID.fld.devType);
}
//																			   *
//******************************************************************************

//*****************************************************************************
//	NAME																	  *
//		netGetDataCollected
//
//	DESCRIPTION:
/**		
	Return consolidated motion quality data. 

	This command will return a consolidated set of data related to 
	the monitor port variable as well as motion tracking. The node collects
	mean squared low and high pass data as well as the maximum low pass value 
	encountered	since the last command request.	 The motion tracking collects
	the maximum negative and positive tracking from the command.
	
	The tracking data is collected from the last query or only collected
	during the last move depending on settings in the #iscTuneConfigReg register.

	\remark In the "by-move" mode, the values returned are always from the 
	last completed move. This will allow double-buffered collection for moves
	who's durations are longer than the request delay.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pReturnData Pointer to a \a pReturnData structure to return
		the collected data.

	\return
		#cnErrCode; MN_OK if successful and all data has been updated.
		MN_ERR_PARAM_RANGE if motion was too long of there was overflow
		detected on a parameter return. 

	\see #iscTuneConfigReg for detailed field defintions.
 
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netGetDataCollected(
			multiaddr theMultiAddr,			// Destination node
			iscDataCollect *pReturnData)	// Returned data
{
	packetbuf cmd, resp;			// Input and output buffers
	netaddr cNum;
	nodeaddr addr;
	cnErrCode theErr = MN_OK;
	paramValue sampleTimeMicroseconds;


	if (!pReturnData)
		return(MN_ERR_BADARG);

	// Build the request command
	cNum = NET_NUM(theMultiAddr);
	addr = NODE_ADDR(theMultiAddr);

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	// Insure we support this feature
	switch(SysInventory[cNum].NodeInfo[addr].theID.fld.devType ) {
	case NODEID_CS:				// ClearPath-SC
	case NODEID_MD:				// Meridian ISC FW 5.7 and beyond
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}

	// Get the sample period
	theErr = netGetParameterInfo(theMultiAddr, MN_P_SAMPLE_PERIOD,	
								  NULL, &sampleTimeMicroseconds);
	// Convert to sample-time in milliseconds
	if (theErr != MN_OK)
		return(theErr);

	// If there is no info, the node has disappeared.
	monState *pMonNow;
	if (!(pMonNow = (monState *)SysInventory[cNum].NodeInfo[addr].pNodeSpecific)) {
		return MN_ERR_OFFLINE;
	}

	// Do we need monitor state?
	if (!pMonNow->Set) {
		// Get our monitor state
		iscMonState monInfo;
		theErr = iscGetMonitor(theMultiAddr, 0, &monInfo);
		if (theErr != MN_OK) 
			return(theErr);
	}

	cmd.Fld.Addr = theMultiAddr;	
	cmd.Fld.PktLen = 1;						// No arguments
	cmd.Byte.Buffer[CMD_LOC] = ISC_CMD_DATA_COLLECT;
#define SIM_MODE 0
	// Run the command and await the response
#if !SIM_MODE
	theErr = netRunCommand(cNum, &cmd, &resp);
	if (theErr != MN_OK)
		return (theErr);
	// We have data, check for alignment/format problems
	if (resp.Fld.PktLen != ISC_CMD_DATA_COLLECT_RESP_LEN) 
		return (MN_ERR_RESP_FMT);
#endif
	// Copy response to parser to avoid alignment issues
	iscDataCollectPkt parser;
	memcpy(&parser, &resp.Byte.Buffer[RESP_LOC], sizeof(parser));
#if SIM_MODE	// TEST CASE
	pMonNow->FullScale = 1;

	parser.data.LPmeanSqLSBs = 0x1e6e;
	parser.data.LPmeanSqMSBs = 3;
	parser.data.LPmeanSqScale = 5;

	parser.data.HPmeanSqLSBs = 0x561b;
	parser.data.HPmeanSqMSBs = 2;
	parser.data.HPmeanSqScale = 12;

	const size_t TIME_SAMPLE_LP = 55555;
	parser.data.SampleTimesLSB = TIME_SAMPLE_LP&0xFFFF;
	parser.data.SampleTimesMSB = TIME_SAMPLE_LP>>16;
	parser.data.TrackingMaxNeg = -55;
	parser.data.TrackingMaxPos = 66;
	parser.data.LPmeasMaxNeg = -44;
	parser.data.LPmeasMaxPos = 33;
#endif

	// Build the responses
	pReturnData->MaxTrackingNeg = parser.data.TrackingMaxNeg;
	pReturnData->MaxTrackingPos = parser.data.TrackingMaxPos;
	pReturnData->LowPassMaxNeg = parser.data.LPmeasMaxNeg/32768. * pMonNow->FullScale;
	pReturnData->LowPassMaxPos = parser.data.LPmeasMaxPos/32768. * pMonNow->FullScale;

	// Check for saturation issues
	const int16 MAX_DATA_POS = 32767;
	const int16 MAX_DATA_NEG = -32768;

	if (parser.data.TrackingMaxNeg==MAX_DATA_NEG ||
		parser.data.TrackingMaxPos==MAX_DATA_POS ||
		parser.data.LPmeasMaxNeg==MAX_DATA_NEG ||
		parser.data.LPmeasMaxPos==MAX_DATA_POS) {
		// Setup as failure, see if we can get more data returned first
		theErr = MN_ERR_PARAM_RANGE; 
	}
	double rms;
	int64 rmsMantissa;
	// Restore 24-bit to 32-bit / sign extension
	const size_t TIMER_BITS = 32-20;	// Padding bits
	nodelong nSamples = ((parser.data.SampleTimesLSB << TIMER_BITS)
					  | (parser.data.SampleTimesMSB << (16+TIMER_BITS)))>>TIMER_BITS;
	// If negative or no samples, then move took to long or is too short
	if (nSamples <= 0) { 
		// Poison the bad return results
		pReturnData->HighPassRMS = -1;
		pReturnData->LowPassRMS = -1;
		return(MN_ERR_PARAM_RANGE);
	}
	// Return the duration in milliseconds
	pReturnData->DurationMS = nSamples * sampleTimeMicroseconds.value * .001;

	const size_t MANTISSA_BITS = 19;	// Bits in the mantissa plus sign guard
	const size_t INTEGER_BITS = 20;		// Number of integer bits
	const size_t Q_TO_SUM = 12;			// Fixed pt to float scale factor

	// Restore 19-bit mantissa (1.63) and align sign bit to machine sign-bit
	rmsMantissa = ((int64)parser.data.LPmeanSqLSBs<<(64-MANTISSA_BITS))
				| ((int64)parser.data.LPmeanSqMSBs<<(64-MANTISSA_BITS+16));
	// Move fixed-point decimal back in place/ sign extension
	rmsMantissa >>= (INTEGER_BITS+parser.data.LPmeanSqScale);
	// Convert the fixed-point mean to floating point
	rms = double(rmsMantissa)/double(1LL<<Q_TO_SUM);
	// Get root mean squared and scale by the current monitor port full-scale
	rms = sqrt(rms/double(nSamples));
	pReturnData->LowPassRMS = rms*pMonNow->FullScale;

	// Restore 19-bit mantissa (1.63) and align sign bit to machine sign-bit
	rmsMantissa = ((int64)parser.data.HPmeanSqLSBs<<(64-MANTISSA_BITS))
				| ((int64)parser.data.HPmeanSqMSBs<<(64-MANTISSA_BITS+16));
	// Move fixed-point decimal back in place/ sign extension
	rmsMantissa >>= (INTEGER_BITS+parser.data.HPmeanSqScale);
	// Calculate the mean and convert to floating point
	rms = double(rmsMantissa)/double(1LL<<Q_TO_SUM);
	// Get root mean and scale by monitor port scale
	rms = sqrt(rms/double(nSamples));
	pReturnData->HighPassRMS = rms*pMonNow->FullScale;

	return (theErr);
}
//																			  *
//*****************************************************************************

/// \cond INTERNAL_DOC

//*****************************************************************************
//	NAME																	  *
//		netGetDrvrAttnMask
//
//	DESCRIPTION:
/**
	This function get the current special driver attention register for 
	implementing Auto-Brake and Group Shutdowns via Attentions.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pAttnMask A pointer to an #attnReg structure to get the
				current value of the driver's attention mask register.
	\return MN_OK if successful.

**/
//	SYNOPSIS:
cnErrCode MN_DECL netGetDrvrAttnMask(
		multiaddr theMultiAddr,
		attnReg *pAttnMask)
{
	cnErrCode theErr;
	if (!pAttnMask)
		return MN_ERR_BADARG;
	if (theMultiAddr==MN_UNSET_ADDR)
		return MN_ERR_BADARG;

	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	// Does this node support this
	nodeparam pNum;
	switch(SysInventory[cNum].NodeInfo[theNode].theID.fld.devType) {
	case NODEID_CS:
		pNum = nodeparam(CPM_P_ATTN_DRVR_MASK);
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}
	paramValue paramVal;
	// Get the bits back
	theErr = netGetParameterInfo(theMultiAddr, pNum, NULL, &paramVal);
	if (theErr != MN_OK)
		return theErr;
	*pAttnMask = *((attnReg *)paramVal.raw.Byte.Buffer);
	return theErr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netSetDrvrAttnMask
//
//	DESCRIPTION:
/**
	This function get the current special driver attention register for 
	implementing Auto-Brake and Group Shutdowns via Attentions.

	\param[in] theMultiAddr The address code for this node.
	\param[in] pAttnMask A pointer to an #attnReg structure to set the
				driver's attention mask register at the requested node.
	\return MN_OK if successful.

**/
//	SYNOPSIS:
cnErrCode MN_DECL netSetDrvrAttnMask(
		multiaddr theMultiAddr,
		attnReg *pAttnMask)
{
	cnErrCode theErr;
	if (!pAttnMask)
		return MN_ERR_BADARG;
	if (theMultiAddr==MN_UNSET_ADDR)
		return MN_ERR_BADARG;

	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	// Does this node support this
	nodeparam pNum;
	switch(SysInventory[cNum].NodeInfo[theNode].theID.fld.devType) {
	case NODEID_CS:
		pNum = nodeparam(CPM_P_ATTN_DRVR_MASK);
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}
	theErr = netSetParameterInfo(theMultiAddr, pNum, pAttnMask->attnBits, true);
	return theErr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netGetStatusEventMask
//
//	DESCRIPTION:
/**
	This function get the current special driver attention register for 
	implementing Auto-Brake and Group Shutdowns via Attentions.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pStatusEventMask A pointer to an #mnStatusReg structure to get the
				current value of the driver's attention mask register.
	\return MN_OK if successful.

**/
//	SYNOPSIS:
cnErrCode MN_DECL netGetStatusEventMask(
		multiaddr theMultiAddr,
		mnStatusReg *pStatusEventMask)
{
	cnErrCode theErr;
	if (!pStatusEventMask)
		return MN_ERR_BADARG;
	if (theMultiAddr==MN_UNSET_ADDR)
		return MN_ERR_BADARG;

	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	// Does this node support this
	nodeparam pNum;
	switch(SysInventory[cNum].NodeInfo[theNode].theID.fld.devType) {
	case NODEID_CS:
		pNum = nodeparam(CPM_P_STATUS_EVENT_MASK);
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}
	paramValue paramVal;
	// Get the bits back
	theErr = netGetParameterInfo(theMultiAddr, pNum, NULL, &paramVal);
	if (theErr != MN_OK)
		return theErr;
	*pStatusEventMask = *((mnStatusReg *)paramVal.raw.Byte.Buffer);
	return theErr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netSetStatusEventMask
//
//	DESCRIPTION:
/**
	This function sets the current special driver attention register for 
	implementing Auto-Brake and Group Shutdowns via Attentions.

	\param[in] theMultiAddr The address code for this node.
	\param[in] pStatusEventMask A pointer to an #mnStatusReg structure to set the
				driver's attention mask register at the requested node.
	\return MN_OK if successful.

**/
//	SYNOPSIS:
cnErrCode MN_DECL netSetStatusEventMask(
		multiaddr theMultiAddr,
		const mnStatusReg *pStatusEventMask)
{
	cnErrCode theErr;
	if (!pStatusEventMask)
		return MN_ERR_BADARG;
	if (theMultiAddr==MN_UNSET_ADDR)
		return MN_ERR_BADARG;

	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);
	// Does this node support this
	nodeparam pNum;
	switch(SysInventory[cNum].NodeInfo[theNode].theID.fld.devType) {
	case NODEID_CS:
		pNum = nodeparam(CPM_P_STATUS_EVENT_MASK);
		break;
	default:
		return MN_ERR_NOT_IMPL;
	}
	packetbuf paramVal;
	// Fill the buffer with the request
	memcpy(paramVal.Byte.Buffer, pStatusEventMask, MN_STATUS_REG_OCTETS);
	paramVal.Byte.BufferSize = MN_STATUS_REG_OCTETS;
	// Send them out
	theErr = netSetParameterEx(theMultiAddr, pNum, &paramVal);
	return theErr;
}
//																			  *
//*****************************************************************************
/// \endcond

//*****************************************************************************
//	NAME																	  *
//		netPLAclear
//
//	DESCRIPTION:
/**
	This function will clear the active and non-volatile fuses for the PLA
	feature.

	\param[in] theMultiAddr The address code for this node.
	\return MN_OK if successful.

**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netPLAclear(
		multiaddr theMultiAddr) 	// node address

{
	plaFuseBuffer fuses, wrFuses;
	cnErrCode theErr;

	// Get the AND term count
	fuses.Setup(plaFuseBuffer::PLA_ARCH, plaFuseBuffer::AND_TERMS);
	theErr = netPLAget(theMultiAddr, &fuses);
	for (int i=0; i<fuses.Fuses.bits && theErr == MN_OK; i++) {
		// Clear the AND area
		wrFuses.Setup(plaFuseBuffer::PLA_AND, PARAM_OPT_MASK|i, 0);
		theErr = netPLAset(theMultiAddr, &wrFuses);
		if (theErr != MN_OK)
			return(theErr);
		// Clear the Not AND area
		wrFuses.Setup(plaFuseBuffer::PLA_NAND, PARAM_OPT_MASK|i, 0);
		theErr = netPLAset(theMultiAddr, &wrFuses);
	}
	if (theErr != MN_OK)
		return(theErr);

	// Get the OR term count
	fuses.Setup(plaFuseBuffer::PLA_ARCH, plaFuseBuffer::OR_TERMS);
	theErr = netPLAget(theMultiAddr, &fuses);
	for (int i=0; i<fuses.Fuses.bits && theErr == MN_OK; i++) {
		// Clear the OR area
		wrFuses.Setup(plaFuseBuffer::PLA_OR, PARAM_OPT_MASK|i, 0);
		theErr = netPLAset(theMultiAddr, &wrFuses);
	}
	if (theErr != MN_OK)
		return(theErr);

	// Get the Option register count
	fuses.Setup(plaFuseBuffer::PLA_ARCH, plaFuseBuffer::OPTION_REGS);
	theErr = netPLAget(theMultiAddr, &fuses);
	for (int i=0; i<fuses.Fuses.bits && theErr == MN_OK; i++) {
		// Clear the OR area
		wrFuses.Setup(plaFuseBuffer::PLA_OPTION, PARAM_OPT_MASK|i, 0);
		theErr = netPLAset(theMultiAddr, &wrFuses);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netPLAget
//
//	DESCRIPTION:
/**
	This function will get the fuse settings for the specified location within
	the PLA device.

	\param[in] theMultiAddr The address code for this node.
 	\param[in,out] pFuses Ptr to the fuse selection and result storage.
	\return MN_OK if fuses successfully updated.

**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netPLAget(
		multiaddr theMultiAddr, 	// node address
		plaFuseBuffer *pFuses)		// Ptr to fuse buffer
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	theCmd.Fld.SetupHdr(MN_PKT_TYPE_CMD, NODE_ADDR(theMultiAddr), MN_SRC_HOST);
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_PLA;
	theCmd.Byte.Buffer[CMD_LOC+1] = (nodechar)pFuses->FuseArea;
	theCmd.Byte.Buffer[CMD_LOC+2] = (nodechar)pFuses->FuseIndex.bits;
	theCmd.Fld.PktLen = 3;

	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp);
	if (theErr == MN_OK) {
		if (theResp.Fld.PktLen != 2)
			return(MN_ERR_RESP_FMT);
		pFuses->Fuses.bits = ((Uint16)theResp.Byte.Buffer[RESP_LOC] & 0xff)
						   | ((Uint16)theResp.Byte.Buffer[RESP_LOC+1]<<8);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netPLAset
//
//	DESCRIPTION:
/**
	This function will set the fuse settings for the specified location within
	the PLA device.

	\param[in] theMultiAddr The address code for this node.
 	\param[in,out] pFuses Ptr to the fuse selection and new settings storage.
	\return MN_OK if fuses successfully updated.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netPLAset(
		multiaddr theMultiAddr, 	// node address
		plaFuseBuffer *pFuses)		// Ptr to fuse buffer
{
	packetbuf theCmd, theResp;
	cnErrCode theErr;

	theCmd.Fld.SetupHdr(MN_PKT_TYPE_CMD, NODE_ADDR(theMultiAddr), MN_SRC_HOST);
	theCmd.Byte.Buffer[CMD_LOC] = MN_CMD_PLA;
	theCmd.Byte.Buffer[CMD_LOC+1] = (nodechar)pFuses->FuseArea;
	theCmd.Byte.Buffer[CMD_LOC+2] = (nodechar)pFuses->FuseIndex.bits;
	theCmd.Byte.Buffer[CMD_LOC+3] = (nodechar)pFuses->Fuses.bits;
	theCmd.Byte.Buffer[CMD_LOC+4] = (nodechar)(pFuses->Fuses.bits>>8);

	theCmd.Fld.PktLen = 5;

	theErr = netRunCommand(NET_NUM(theMultiAddr), &theCmd, &theResp);
	if (theErr == MN_OK) {
		if (theResp.Fld.PktLen != 0)
			return(MN_ERR_RESP_FMT);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************


/// \cond INTERNAL_DOC

//*****************************************************************************
//	NAME																	  *
//		extractFileNameBase
//
//	DESCRIPTION:
/**
	This function extracts the basename from a Windows or Unix type file
	path.

	\param[in] pInputName full filename path.
	\param[out] pBaseName Basename of the file path
 	\param[in] maxLen size of pBaseName array.
**/
const char *extractFileNameBase(const char *pInputName, char *pBaseName, size_t maxLen)
{
	char srcCopy[MAX_PATH];
	strncpy(srcCopy, pInputName, sizeof(srcCopy));
	// Extract last item from path
	char *pLastToken[2] = {0};
	pLastToken[0] = strtok(srcCopy, "\\/");
	while (pLastToken[0]) {
		pLastToken[1] = pLastToken[0];
		pLastToken[0] = strtok(NULL, "\\/");
	}
	// Break apart the last item at the optional "."
	pLastToken[1] = strtok(pLastToken[1], ".");
	// Send to the output string area
	strncpy(pBaseName, pLastToken[1], maxLen);
	// Return convenience pointer
	return(pBaseName);
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		setNodeNewConfig
//
//	DESCRIPTION:
/**
	These functions writes the base name of the file path and clears the
	modified indicator at the node.

	\param[in] theMultiPath The node to update.
	\param[in] pFilePath File path in UNIX or WINDOWS format.
**/
cnErrCode setNodeNewConfig(
	multiaddr theMultiAddr, 
	const char *pFilePath)
{
	cnErrCode theErr = MN_ERR_NOT_IMPL;
	char cfgFileName[MN_FILENAME_SIZE];
	// Clear 'modified' counter	and store config file name at node.
	nodeIDs theNodeType = netGetDevType(theMultiAddr);
	switch(theNodeType) {
#if 0 // Prevent usage for now
	case NODEID_MD:
		theErr = iscSetParameter(theMultiAddr, ISC_P_DRV_CONFIG_CHANGED, 0);
		break;
	case NODEID_CP:
		theErr = clearPathSetParameter(theMultiAddr, CP_P_DRV_CONFIG_CHANGED, 0);
		break;
#endif
	case NODEID_CS:
		theErr = cpmSetMotorFileName(theMultiAddr, 
									  extractFileNameBase(
											pFilePath, 
										    cfgFileName,
											sizeof(cfgFileName)));
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN,"Failed to set motor filename, err %0X\n", theErr);
			goto bailOut;
		}
		theErr = cpmSetParameter(theMultiAddr, CPM_P_DRV_CONFIG_CHANGED, 0);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN,"Failed to clear config changed indicator, err %0X\n", theErr);
			goto bailOut;
		}
		break;
	default:
		theErr = MN_ERR_NOT_IMPL;
		goto bailOut;
	}
//--------
bailOut:
//--------
	return (theErr);
}

//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		setConfigItem
//
//	DESCRIPTION:
/**
	These functions sets items in the motor file dictionary for
	subsequent writing as a file.

	\param[in] d Point to the dictionary.
	\param[in] theSection Section of file.
 	\param[in] theItem Key to store value at.
	\param[in] theVal String to store at key.
**/
//	SYNOPSIS:
cnErrCode setConfigItem(
    dictionary *d,
	const char *theSection, 
	const char *theItem, 
	const char *theVal)
{
	const size_t MAX_ITEM_CHARS = 100;
	char itemKey[MAX_ITEM_CHARS];
	snprintf(itemKey, sizeof(itemKey), "%s:%s", theSection, theItem);
	iniparser_setstring(d, itemKey, theVal);
	return MN_OK;
}
cnErrCode setConfigItem(
    dictionary *d,
	const char *theSection, 
	const char *theItem, 
	double theVal)
{
	const size_t MAX_ITEM_CHARS = 100;
	char valStr[MAX_ITEM_CHARS];
	snprintf(valStr, sizeof(valStr), "%lf", theVal);
	return setConfigItem(d, theSection, theItem, valStr);
}

cnErrCode setConfigItem(
	dictionary *d,
	const char *theSection,
	multiaddr theMultiAddr,
	nodeparam theParam,
	const paramInfo &info)
{
	const size_t MAX_ITEM_CHARS = 100;
	char valStr[MAX_ITEM_CHARS];
	const char *unitStr=NULL;
	cnErrCode theErr;
	paramValue paramVal;

	// Get the value from the node in long form, in case non-numeric
	theErr = netGetParameterInfo(theMultiAddr, theParam, NULL, &paramVal);
	if (theErr != MN_OK) {
		_RPT2(_CRT_WARN, "setConfig get param failed param=%d, err=%x\n",
			theParam, theErr);
		return theErr;
	}
	
	if (info.unitType == BIT_FIELD) {
		if (info.paramSize == 2)
			snprintf(valStr, sizeof(valStr), "&H%04X", nodeulong(paramVal.value));
		else
			snprintf(valStr, sizeof(valStr), "&H%08X", nodeulong(paramVal.value));
	}
	else if (info.paramSize > 4) {
		const size_t BYTE_REPR_WIDTH = 2;
		char byteRepr[BYTE_REPR_WIDTH + 1];

		memset(valStr, 0, sizeof(valStr));
		strncpy(valStr, "&H", 2);
		
		for (int i = info.paramSize - 1; i >= 0; i--) {
			snprintf(byteRepr, sizeof(byteRepr), "%02X", (unsigned char) paramVal.raw.Byte.Buffer[i]);
			strncat(valStr, byteRepr, BYTE_REPR_WIDTH);
		}
	}
	else {
		switch (info.unitType) {
		case TIME_USEC:
			unitStr = "usec";
			break;
		case DX_TICK:
			unitStr = "ticks";
			break;
		case VEL_TICKS_S:
			unitStr = "ticks/sec";
			break;
		case VEL_TICKS_S2:
			unitStr = "ticks/sec^2";
			break;
		case CURRENT:
			unitStr = "A";
			break;
		case VOLT:
			unitStr = "V";
			break;
		case TIME_MSEC:
			unitStr = "msec";
			break;
		case VOLTS_PER_KRPM:
			unitStr = "V/kRPM";
			break;
		case DEGREES:
			unitStr = "degrees";
			break;   
		case PERCENT_MAX:
			unitStr = "% of max.";
			break;
		case NO_UNIT:
			break;
		default:
			break;
		}
		if (unitStr) 
			snprintf(valStr, sizeof(valStr), "%lf\t;(%s)", paramVal.value, unitStr);
		else
			snprintf(valStr, sizeof(valStr), "%lf", paramVal.value);
	}
	// Check for issues
	if ((size_t)info.keyID > sizeof(ConfigKeys)/sizeof(char *))
		throw "valkeys.h is too short";
	if (info.keyID == PARAM_NULL) 
		throw "param table not setup properly";

	return setConfigItem(d, theSection, ConfigKeys[info.keyID], valStr);
}
//																			  *
//*****************************************************************************




//*****************************************************************************
//	NAME																	  *
//		getAndSetConfigItem
//
//	DESCRIPTION:
/**
	This functions retrieves the value in the configuration file and storing
	it in the drive as run-time and non-volatile locations.

	\param[in] d Point to the dictionary.
	\param[in] theSection Section of file.
	\param[in] theMultiAddr The address of the node to update.
 	\param[in] theParam The parameter number in the drive.
	\param[in] info Parameter information structure for this parameter.
**/
//	SYNOPSIS:
cnErrCode getAndSetConfigItem(
	dictionary *d,
	const char *theSection,
	multiaddr theMultiAddr,
	nodeparam theParam,
	const paramInfo &info)
{
	#define SCAN_MAX "100"
	const size_t MAX_ITEM_CHARS = 150;
	char valStr[MAX_ITEM_CHARS], keyStr[MAX_ITEM_CHARS];
	const char* DEFAULT_VAL = "__XZYZY42__", *iniItem;
	// Extract out the node address parts, the netGetFirmwareID verified the
	// multi-address is OK.
	//netaddr cNum = NET_NUM(theMultiAddr);
	//nodeaddr theNode = NODE_ADDR(theMultiAddr);
	cnErrCode theErr;
	if ((size_t)info.keyID > sizeof(ConfigKeys)/sizeof(char *))
		throw "valkeys.h is too short";
	// Build up value query
	snprintf(keyStr, sizeof(keyStr), "%s:%s",
		theSection, ConfigKeys[info.keyID]);
	iniItem = iniparser_getstring(d, keyStr, DEFAULT_VAL);
	// Missing item
	if (strcmp(iniItem, DEFAULT_VAL)==0) {
		_RPT1(_CRT_WARN, "Missing item %s\n", keyStr);
		return MN_ERR_FILE_BAD;
	}

	// Strip out comment or EOLs
	unsigned numMatches = sscanf(iniItem, "%" SCAN_MAX "s", valStr);
	if (numMatches == 0) {
		_RPT0(_CRT_WARN, "Missing item value is null\n");
		return	MN_ERR_FILE_BAD;
	}
	size_t theLength = strlen(valStr);
	if (info.paramSize > 4) {
		// Parameter too wide to read as a nodelong; do byte-by-byte hex conversion
		if (theLength % 2) {
			// We always emit 0-padded even-length strings to make reading easy
			_RPT0(_CRT_WARN, "Item length not even");
			return MN_ERR_FILE_BAD;
		}
		if (theLength > 2 + info.paramSize * 2) {
			// 2 chars for "&H" and 2 chars per byte
			_RPT0(_CRT_WARN, "Item too long");
			return MN_ERR_FILE_BAD;
		}
		if (theLength < 2) {
			// "&H" is interpreted as 0, but anything shorter is invalid;
			// we do not support decimal conversion for wide params
			_RPT0(_CRT_WARN, "Item too short");
			return MN_ERR_FILE_BAD;
		}
		// Start with a zeroed-out buffer in case the hex string is short
		paramValue paramVal;
		paramVal.raw.Byte.BufferSize = info.paramSize;
		memset(paramVal.raw.Byte.Buffer, 0, sizeof(paramVal.raw.Byte.Buffer));
		int scanSuccess; // equals 1 when hex conversion succeeds
		unsigned byteIndex; // index into the buffer
		// Endianness swap: start with the last two characters in the string
		const char *hexStrPos = valStr + theLength - 2;

		// Two chars per byte, minus the "&H"
		for (byteIndex = 0; byteIndex < theLength / 2 - 1; byteIndex++) {
			// read the characters and put the result directly into the buffer
			scanSuccess = sscanf(hexStrPos, "%2hhx", &paramVal.raw.Byte.Buffer[byteIndex]);
			if (scanSuccess < 1) {
				_RPT2(_CRT_WARN, "Failed extended hex conversion of %s, parameter %d\n",
					valStr, theParam);
				return MN_ERR_FILE_BAD;
			}
			hexStrPos -= 2;
		}
		// Run the command and update our cache
		theErr = netSetParameterEx(theMultiAddr, mnParams(theParam), &paramVal.raw);
		if (theErr != MN_OK) {
			_RPT2(_CRT_WARN, "Failed to save parameter %d in node. Err=0x%x\n",
				theParam, theErr);
			return theErr;
		}
		theErr = netSetParameterEx(theMultiAddr, mnParams(theParam + PARAM_OPT_MASK), &paramVal.raw);
		if (theErr != MN_OK) {
			_RPT2(_CRT_WARN, "Failed to save NV parameter %d in node. Err=0x%x\n",
				theParam, theErr);
			return theErr;
		}
		return theErr;
	}
	double paramVal;
	switch (info.unitType) {
	case BIT_FIELD:
		// Change from VB hex to C hex
		try {
			paramVal = hexToDec(valStr+2);
		}
		catch (...) {
			_RPT2(_CRT_WARN, "Failed hex conversion of %s, parameter %d\n", 
				valStr, theParam);
			return MN_ERR_FILE_BAD;
		}
		break;
	default:
		errno = 0;			// Zap old errors
		paramVal = atof(valStr);
		if (errno) {
			_RPT2(_CRT_WARN, "Failed numeric conversion of %s, parameter %d\n", 
				valStr, theParam);
			return MN_ERR_FILE_BAD;
		}
		break;
	}
	theErr = netSetParameterDbl(theMultiAddr, mnParams(theParam), paramVal);
	if (theErr != MN_OK) {
		_RPT2(_CRT_WARN, "Failed to save parameter %d in node. Err=0x%x\n", 
			theParam, theErr);
		return theErr;
	}
	theErr = netSetParameterDbl(theMultiAddr, mnParams(theParam+PARAM_OPT_MASK), paramVal);
	if (theErr != MN_OK) {
		_RPT2(_CRT_WARN, "Failed to save NV parameter %d in node. Err=0x%x\n", 
			theParam, theErr);
		return theErr;
	}
	return theErr;
}
//																			  *
//*****************************************************************************
 /// \endcond


//******************************************************************************
//	NAME																	   *
//		netConfigLoad
//
//	DESCRIPTION:
/**
	Query the node and build up the firmware ID used as the key for 
	a configuration file. This is in the format:
		TEK32{moniker}{model}-{amps}-{pwba}

	\param[in] theMultiAddr The address code for this node.
 	\param[in] loadFmt Expected format of the data  
	\param[in] pFilePath Pointer to configuration file.
	
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netConfigLoad(
		multiaddr theMultiAddr,
		configFmts loadFmt,
		const char *pFilePath)
{
	dictionary *d;
	char firmwareID[20];
	char keyStr[100];
	cnErrCode theErr;
	bool isAdvanced = false;
	bool hasMtrPart = false;
	bool hasNodePart = false;
 	// Extract out the node address parts, the netGetFirmwareID verified the
	// multi-address is OK.
	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theNode = NODE_ADDR(theMultiAddr);

	// TODO: add other formats
	if (loadFmt != CLASSIC && loadFmt != CLASSIC_NO_RESET)
		return MN_ERR_NOT_IMPL;

	// Make sure node is not enabled
	mnStatusReg currentStatus;
	theErr = netGetStatusRTReg(theMultiAddr, &currentStatus);
	if (theErr != MN_OK)
		return theErr;

	// Don't allow load if enabled
	if (currentStatus.cpm.Enabled) {
		return MN_ERR_FILE_ENABLED;
	}


	// Get the section name for this file
	theErr = netGetFirmwareID(theMultiAddr, sizeof(firmwareID), firmwareID);
	if (theErr != MN_OK)
		return theErr;

	// Get firmware version
	double versNum;
	theErr = netGetParameterDbl(theMultiAddr, MN_P_FW_VERSION, &versNum);
	if (theErr != MN_OK) {
		_RPT1(_CRT_WARN,"Failed getting firmware version. Err=0x%X.\n", theErr);
		return theErr;
	}
	//versID_t chipVers;
	//chipVers.verCode = Uint16(versNum);

	// Get options 
	double optionsDbl;
	optionReg options;
	theErr = netGetParameterDbl(theMultiAddr, MN_P_OPTION_REG, &optionsDbl);
	if (theErr != MN_OK) 
		return theErr;
	options.bits = nodeulong(optionsDbl);

	// Clear out any old config first to insure common start
	nodeIDs theNodeType = netGetDevType(theMultiAddr);
	switch(theNodeType) {
#if 0 // Prevent usage for now
	case NODEID_MD:
		theErr = iscFactoryDefaults(theMultiAddr);
		break;
	case NODEID_CP:
		theErr = clearPathFactoryDefaults(theMultiAddr);
		break;
#endif
	case NODEID_CS:
		isAdvanced = options.Common.Advanced;
		break;
	default:
		_RPT0(_CRT_WARN,"No config file support for this node.\n");
		theErr = MN_ERR_NOT_IMPL;
		goto bailOut;
	}

	// Load the file
	d = iniparser_load(pFilePath);
	if (d == NULL) {
		_RPT1(_CRT_WARN, "Failed to parse config file '%s'.\n", pFilePath);
		return MN_ERR_FILE_OPEN;
	}

	char *thisSect;
	const char *theItem;
	// Make sure this file matches the type of node specified.
	for (int i=0; i<iniparser_getnsec(d); i++) {
		thisSect = iniparser_getsecname(d, i);
		if (thisSect == NULL)
			return MN_ERR_CMD_INTERNAL;
		hasNodePart |= strcmp(firmwareID, thisSect) == 0;
		hasMtrPart |= strcmp(MOTOR_SECTION, thisSect) == 0;
	}
	// Missing required parts?
	if (!(hasNodePart && hasMtrPart)) {
		_RPT0(_CRT_WARN, "Missing section(s) in config file.\n");
		theErr = MN_ERR_FILE_WRONG;
		goto bailOut;
	}
	// Poison the configuration in case of failure
	theErr = netSetParameterDbl(theMultiAddr, MN_P_EE_UPD_ACK, 0);
	if (theErr != MN_OK) {
		_RPT1(_CRT_WARN,"Failed config poisoning err %0X\n", theErr);
		goto bailOut;
	}
	try {
		// Load the motor parts first
		for (size_t bank=0; bank < SysInventory[cNum].NodeInfo[theNode].bankCount; bank++) {
			for (size_t pIndx=0; 
				pIndx < SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].nParams ;
				pIndx++) {
					const paramInfo &p 
						= SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].fixedInfoDB[pIndx].info;
					if ((p.paramType & PT_IN_MTR_CFG) && !(p.paramType & PT_IN_FACT_CFG)) {
						getAndSetConfigItem(d, MOTOR_SECTION, theMultiAddr, 
											nodeparam(256*bank+pIndx), p);
					}

			}
		}
		// Load the rest of the configuration
		for (size_t bank=0; bank < SysInventory[cNum].NodeInfo[theNode].bankCount; bank++) {
			for (size_t pIndx=0; 
				pIndx < SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].nParams ;
				pIndx++) {
					const paramInfo &p 
						= SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].fixedInfoDB[pIndx].info;
					// Load if a non factory config item and skip advanced 
					// items when we are not advanced.
					if ((p.paramType & PT_IN_NODE_CFG) && !(p.paramType & PT_IN_FACT_CFG)
					&& (!(p.paramType & PT_ADV) || (isAdvanced && (p.paramType & PT_ADV)))) {
						getAndSetConfigItem(d, firmwareID, theMultiAddr, 
											nodeparam(256*bank+pIndx), p);
					}

			}
		}
		// Restore the user ID
		snprintf(keyStr, sizeof(keyStr), "%s:%s", (const char *)firmwareID, CNFG_USERID);
		theItem = iniparser_getstring(d, keyStr, "");
		theErr = netSetUserID(theMultiAddr, theItem);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN, "Failed to set user ID, err=0x%x\n", theErr);
			goto bailOut;
		}
		// Restore the user description
		snprintf(keyStr, sizeof(keyStr), "%s:%s", (const char *)firmwareID, CNFG_USER_DESC);
		theItem = iniparser_getstring(d, keyStr, "");
		theErr = netSetUserDescription(theMultiAddr, theItem);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN, "Failed to set user description, err=0x%x\n", theErr);
			goto bailOut;
		}
		double paramVal;
		// We have succeeded, update EE and ROM Ack parameters to clear any
		// errors.
		theErr = netGetParameterDbl(theMultiAddr, MN_P_EE_VER, &paramVal);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN, "Failed to get EE Ver, err=0x%x\n", theErr);
			goto bailOut;
		}
		theErr = netSetParameterDbl(theMultiAddr, MN_P_EE_UPD_ACK, paramVal);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN, "Failed to set EE Ver Ack, err=0x%x\n", theErr);
			goto bailOut;
		}
		// Update ROMSUM Ack in case this load fixed firmware updated
		theErr = netGetParameterDbl(theMultiAddr, MN_P_ROM_SUM, &paramVal);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN, "Failed to get ROM checksum, err=0x%x\n", theErr);
			goto bailOut;
		}
		theErr = netSetParameterDbl(theMultiAddr, MN_P_ROM_SUM_ACK, paramVal);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN, "Failed to set ROM update Ack, err=0x%x\n", theErr);
			goto bailOut;
		}
		// Reset modified indicator and update the filename
		theErr = setNodeNewConfig(theMultiAddr, pFilePath);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN,"Failed config finalize err %0X\n", theErr);
			goto bailOut;
		}
		if (loadFmt != CLASSIC_NO_RESET){
			// All Done, Restart node to insure its using the new settings
			theErr = mnRestartNode(theMultiAddr);
			if (theErr != MN_OK) {
				_RPT1(_CRT_WARN, "Failed to restart, err=0x%x\n", theErr);
				goto bailOut;
			}
		}
		else{
			theErr = netAlertClear(theMultiAddr);
			if (theErr != MN_OK) {
				_RPT1(_CRT_WARN, "Failed to clear shutdowns, err=0x%x\n", theErr);
				goto bailOut;
			}
			// Indicate that the params need to be refreshed
			SysInventory[cNum].pNCS->paramsHaveChanged[theNode] = TRUE;
		}
	}
	catch(...) {
		theErr = MN_ERR_FAIL;
	}
// ----------
bailOut:
// ----------
	iniparser_freedict(d);
	return theErr;
}
//																			  *
//*****************************************************************************
//*****************************************************************************
//	NAME																	  *
//		netConfigSave
//
//	DESCRIPTION:
/**
	This function with save the motor and drive configuration of a drive which
	can be loaded at a later time with #netConfigLoad function.

	\param[in] theMultiAddr The address code for this node.
 	\param[in] saveFmt Format of the data, for future use.
	\param[in] pFilePath Pointer to configuration file.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL netConfigSave(
		multiaddr theMultiAddr,
		configFmts saveFmt,
		const char *pFilePath)
{
	cnErrCode theErr = MN_OK;
	const size_t MAX_ITEM_CHARS = 150;
	// Values from the node
	double versNum, optionsDbl;
	optionReg options;
	// Place to put string items in
	char itemStr[MAX_ITEM_CHARS];
	time_t now;
	bool isAdvanced = false;

	char sectName[20];
	// TODO: add other formats
	if (saveFmt != CLASSIC)
		return MN_ERR_NOT_IMPL;

	// Get the section name for this file
	theErr = netGetFirmwareID(theMultiAddr, sizeof(sectName), sectName);
	if (theErr != MN_OK)
		return theErr;

	// Extract out the node address parts, the netGetFirmwareID verified the
	// multi-address is OK.
	netaddr cNum;
	nodeaddr theNode;
	cNum = NET_NUM(theMultiAddr);
	theNode = NODE_ADDR(theMultiAddr);

	// Get common items for the start of file
	theErr = netGetParameterDbl(theMultiAddr, MN_P_OPTION_REG, &optionsDbl);
	if (theErr != MN_OK) 
		return theErr;
	options.bits = nodeulong(optionsDbl);
	// Get firmware version
	versID_t chipVers;
	theErr = netGetParameterDbl(theMultiAddr, MN_P_FW_VERSION, &versNum);
	if (theErr != MN_OK) 
		return theErr;
	chipVers.verCode = Uint16(versNum);
	// Start building the file
	dictionary *d;
	d = dictionary_new(0);
	if (!d) 
		return MN_ERR_MEM_LOW;
	// Create the section
	try {

		// Create the section entry
		iniparser_setstring(d, sectName, "");
		// Write out chip version info
		snprintf(itemStr, sizeof(itemStr), "%d.%d.%d",
			chipVers.fld.majorVers, chipVers.fld.minorVers, chipVers.fld.buildVers);
		setConfigItem(d, sectName, CNFG_VER_KEY, itemStr);
		// Write out the user id
		theErr = netGetUserID(theMultiAddr, itemStr, sizeof(itemStr));
		if (theErr != MN_OK)
			goto bailOut;
		if (strncmp(itemStr, "Unloaded ", strlen("Unloaded ")) != 0) {
			// If it doesn't look like the default name, copy it over
			setConfigItem(d, sectName, CNFG_USERID, itemStr);
		}
		else {
			// Otherwise, save it as a blank value
			setConfigItem(d, sectName, CNFG_USERID, "");
		}
		// Write out the current time/date
		time(&now);
		struct tm *timeInfo;
		timeInfo = localtime(&now);
		snprintf(itemStr, sizeof(itemStr), "%04d-%02d-%02d %02d:%02d:%02d",
			1900+timeInfo->tm_year, timeInfo->tm_mon+1, timeInfo->tm_mday,
			timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
		setConfigItem(d, sectName, CNFG_DATE_KEY, itemStr);
		// Write out our write version
		longVers libVers;
		libVers.bits = infcVersion();
		snprintf(itemStr, sizeof(itemStr), "sFoundation %d.%d.%d",
			libVers.fld.major, libVers.fld.minor, libVers.fld.build);
		setConfigItem(d, sectName, CNFG_MOD_VER_KEY, itemStr);
		// Write out our part number
		theErr = netGetPartNumber(theMultiAddr, itemStr, sizeof(itemStr));
		if (theErr != MN_OK)
			goto bailOut;
		setConfigItem(d, sectName, CNFG_PN, itemStr);
		// Write out the user description
		theErr = netGetUserDescription(theMultiAddr, itemStr, sizeof(itemStr));
		if (theErr != MN_OK)
			goto bailOut;
		setConfigItem(d, sectName, CNFG_USER_DESC, itemStr);

		// Clear out any old config first to insure common start
		nodeIDs theNodeType = netGetDevType(theMultiAddr);
		switch(theNodeType) {
#if 0 // Prevent usage for now
		case NODEID_MD:
			theErr = iscFactoryDefaults(theMultiAddr);
			break;
		case NODEID_CP:
			theErr = clearPathFactoryDefaults(theMultiAddr);
			break;
#endif
		case NODEID_CS:
			isAdvanced = options.Common.Advanced;
			break;
		default:
			_RPT0(_CRT_WARN,"No config file support for this node.\n");
			theErr = MN_ERR_NOT_IMPL;
			goto bailOut;
		}
		// Loop through our parameter banks for configuration parameters
		for (size_t bank=0; bank < SysInventory[cNum].NodeInfo[theNode].bankCount; bank++) {
			for (size_t pIndx=0; 
				pIndx < SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].nParams ;
				pIndx++) {
					const paramInfo &p 
						= SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].fixedInfoDB[pIndx].info;
					// Grab items in our configuration and skip advanced items
					// when our node is not that type.
					bool inCfg = (p.paramType & PT_IN_NODE_CFG) != 0;
					bool advChkOK = !(p.paramType & PT_ADV)		// All non-advanced OK
							|| ((p.paramType & PT_ADV) && isAdvanced);
					if (inCfg && advChkOK) {
						setConfigItem(d, sectName, theMultiAddr, 
									  nodeparam(256*bank+pIndx), p);
					}

			}
		}

		// Create the motor section entry
		iniparser_setstring(d, MOTOR_SECTION, "");
		// Loop through the motor parameter for configuration parameters
		for (size_t bank=0; bank < SysInventory[cNum].NodeInfo[theNode].bankCount; bank++) {
			for (size_t pIndx=0; 
				pIndx < SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].nParams ;
				pIndx++) {
					const paramInfo &p 
						= SysInventory[cNum].NodeInfo[theNode].paramBankList[bank].fixedInfoDB[pIndx].info;
					bool inMtrCfg = (p.paramType & PT_IN_MTR_CFG) != 0;
					bool advMtrChkOK = !(p.paramType & PT_ADV)		// All non-advanced OK
							|| ((p.paramType & PT_ADV) && isAdvanced);
					if (inMtrCfg && advMtrChkOK) {
						setConfigItem(d, MOTOR_SECTION, theMultiAddr, 
									  nodeparam(256*bank+pIndx), p);
					}

			}
		}
		// Reset modified indicator and update the filename
		theErr = setNodeNewConfig(theMultiAddr, pFilePath);
		if (theErr != MN_OK) {
			_RPT1(_CRT_WARN,"No config file support for this node. Err=%x\n",
				theErr);
			goto bailOut;
		}
		// Write out our completed dictionary in INI format
		FILE *fo;
		fo = fopen(pFilePath, "w");
		if (fo!=NULL) {
			iniparser_dump_ini(d, fo);
			fclose(fo);
		} 
		else { 
			throw("failed to create file");
		}
	}
	catch(...) {
		dictionary_del (d);
		return MN_ERR_FAIL; 
	}
//------------
bailOut:
//------------
	dictionary_del (d);
	return theErr;
}
//																			  *
//*****************************************************************************


/// \cond	INTERNAL_DOC

//******************************************************************************
//	NAME																	   *
//		netGetFirmwareID
//
//	DESCRIPTION:
/**
	Query the node and build up the firmware ID used as the key for 
	a configuration file. This is in the format:
		TEK32 {moniker}{model}-{amps}-{pwba}

	\param[in] theMultiAddr The address code for this node.
 	\param[in] bufSizeMax Destination area of compatibility 18 octet buffer
	\param[out] theID  Buffer for c string result.
	
**/
//	SYNOPSIS:
cnErrCode netGetFirmwareID (
		multiaddr theMultiAddr,		// Node address
		size_t bufSizeMax,			// sizeof theID buffer
		char *theID)
{
	cnErrCode theErr;
	double theParam, theAmps;
	nodeulong pwbType;
	const size_t FWID_SZ = 19;
	const nodeulong PWB_TYPE_MASK = 0xc0;

	// Parameter locations that we don't want exposed publicly
	const cpmParams CPM_P_DRV_TP_IOP = (cpmParams)285;

	// Is address OK?
	netaddr cNum = NET_NUM(theMultiAddr);
	//nodeaddr addr = NODE_ADDR(theMultiAddr);
	if (cNum>NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	// The output buffer is too small?
	if (bufSizeMax<FWID_SZ) 
		return MN_ERR_BADARG;

	// Get our node ID
	devID_t theNodeType;
	theErr = netGetParameterDbl(theMultiAddr, MN_P_NODEID, &theParam); 
	if (theErr != MN_OK)
		return theErr;
	// Convert to structure format
	theNodeType.devCode = (Uint16)theParam;

	// Get the option register
	theErr = netGetParameterDbl(theMultiAddr, MN_P_OPTION_REG, &theParam);
	if (theErr != MN_OK)
		return theErr;

	switch(theNodeType.fld.devType) {
	case NODEID_MD:
		// Get PWBA type from internal register
		theErr = iscGetParameter(theMultiAddr, ISC_P_DRV_TP_IOP, &theParam);
 		if (theErr != MN_OK)
			return theErr;
		// Extract out of bits
		pwbType = (PWB_TYPE_MASK & nodeulong(theParam)) >> 6;
		theErr = iscGetParameter(theMultiAddr, ISC_P_DRV_I_MAX, &theAmps);
 		if (theErr != MN_OK)
			return theErr;
		if(snprintf(theID, bufSizeMax, "TEK32MD%02d-%d-%d", 
					 theNodeType.fld.devModel, int(theAmps), 
					 pwbType) < 0) {
			return MN_ERR_BADARG;
		}
		break;
#if defined(IMPL_NODE_CP)
	case NODEID_CP:
		// Get PWBA type from internal register (ClearPath)
		theErr = clearPathGetParameter(theMultiAddr, CP_P_DRV_TP_IOP, &theParam);
 		if (theErr != MN_OK)
			return theErr;
		// Extract out of bits
		pwbType = (PWB_TYPE_MASK & nodeulong(theParam)) >> 6;
		theErr = clearPathGetParameter(theMultiAddr, CP_P_DRV_I_MAX, &theAmps);
 		if (theErr != MN_OK)
			return theErr;
		if(snprintf(theID, bufSizeMax, "TEK32CP%02d-%d-%d", 
					 theNodeType.fld.devModel, int(theAmps), 
					 pwbType) < 0) {
			return MN_ERR_BADARG;
		}
		break;
#endif
	case NODEID_CS:
		// Get PWBA type from internal register (ClearPath-SC)
		theErr = cpmGetParameter(theMultiAddr, CPM_P_DRV_TP_IOP, &theParam);
 		if (theErr != MN_OK)
			return theErr;
		// Extract out of bits
		pwbType = (PWB_TYPE_MASK & nodeulong(theParam)) >> 6;
		theErr = cpmGetParameter(theMultiAddr, CPM_P_DRV_I_MAX, &theAmps);
 		if (theErr != MN_OK)
			return theErr;
		if(snprintf(theID, bufSizeMax, "TEK32CS%02d-%d-%d", 
					 theNodeType.fld.devModel, int(theAmps), 
					 pwbType) < 0) {
			return MN_ERR_BADARG;
		}
		break;
	default:
		return MN_ERR_UNKN_DEVICE;
	}
	return(MN_OK);
}
//																			  *
//*****************************************************************************


/*****************************************************************************
 *	!NAME!
 *		spscWatchAttnMask
 *
 *	DESCRIPTION:
 *		Record the current attention mask for ClearPath-SC nodes. This is 
 *		the parameter callback for each of these parameters.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
double MN_DECL spscWatchAttnMask(
	nodebool valIsBits, 				// TRUE=>ADC Temp, FALSE=>C Temp
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theAddr = NODE_ADDR(theMultiAddr);
	extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];
	attnReg attnreg;
	attnreg.attnBits = Uint32(convVal);
	if (valIsBits)
		return(convVal);

	switch (parameter.bits) {
	case CPM_P_ATTN_DRVR_MASK:
		SysInventory[cNum].drvrAttnMask[theAddr] = attnreg; 
		break;
	case CPM_P_ATTN_MASK:
		SysInventory[cNum].attnMask[theAddr] = attnreg; 
		break;
	default:
		_RPT0(_CRT_ASSERT, "watchAttnMask non-attention called!\n");
	}
	return(convVal);
}
/*																 !end!		*/
/****************************************************************************/
/// \endcond

// End of Group: LinkGrp
/// @}

//=============================================================================
//	END OF FILE netCmdAPI.c
//=============================================================================

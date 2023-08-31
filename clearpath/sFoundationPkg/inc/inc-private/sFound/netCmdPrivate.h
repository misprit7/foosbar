//*****************************************************************************
// $Header:: /ClearPath SC-1.0.123/User Driver/inc-private/netCmdPrivate.h 4 $
//
// NAME
//		netCmdPrivate.h
//
// DESCRIPTION:
//		netCore.c private prototypes
//
// CREATION DATE:
//		01/28/2004 16:51:59 - was netCorePrivate.h
//		06/11/2009 10:47:00 - renamed to avoid CP conflicts
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2004-2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************

#ifndef _NETCMDPRIVATE_H
#define _NETCMDPRIVATE_H

//*****************************************************************************
// NAME																          *
// 	netCmdPrivate.h headers
//
	#include "pubMnNetDef.h"
	#include "mnErrors.h"
	#include "mnParamDefs.h"
	#include "pubSysCls.h"
	#if (defined(_WIN32)||defined(_WIN64))
		#include "lnkAccessAPIwin32.h"
	#endif
	#include <stdarg.h>
	
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	netCmdPrivate.h types
//

//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	netCmdPrivate.h constants
//
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	netCmdPrivate.h function prototypes
//
//
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* 							Private functions 								  */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Helper function to fill in an error object.
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
void fillInErrs(sFnd::mnErr &eInfo, 
				cnErrCode theErr, std::string funcName,
				const char *fmtStr, ...);

void fillInErrs(sFnd::mnErr &eInfo, sFnd::INode *pNode, 
				cnErrCode theErr, std::string funcName,
				const char *fmtStr, ...);


#ifdef __cplusplus
extern "C" {
#endif

// Error code classification functions
nodebool IsNetClassErr(
cnErrCode theErr);

nodebool IsNetBreakErr(
	cnErrCode theErr);

// Return the controller number to use
netaddr coreController(multiaddr theMultiAddr);

// Get an 16-bit integer value from the node
cnErrCode coreBaseGetParameterInt(netaddr cNum, nodeaddr theNode, 
								  nodeparam paramNum, Uint16 *pVal);
// Set an 16-bit integer value to the node
cnErrCode coreBaseSetParameterInt(netaddr cNum, nodeaddr theNode, 
								  nodeparam paramNum, int pVal);

// Update the value database
cnErrCode coreUpdateParamInfo(
		multiaddr multiAddr);		// Node to update		


// Set info using enhanced value information
cnErrCode netSetParameterInfo(
		multiaddr multiAddr,		// Node address
		nodeparam theParam,		   	// Parameter number
		const double theValue,	   	// New value
		nodebool verify);			// Verify write worked

// Create basic error from the buffer contents
cnErrCode coreGenErrCode(
		netaddr cNum,				// Controller number
		packetbuf *theResp,			// Response buffer
		nodeaddr cmdNode);			// Expected node
			
// Reset the targetted node or network		
cnErrCode MN_DECL netReset(
		multiaddr multiAddr,
		nodebool broadCast); 		// node address
		
		
// Get parameter command		
cnErrCode MN_DECL netGetParameterFmt(
		packetbuf *frameBuf,		// ptr to the frame buffer to format
		nodeaddr theNode, 			// node address
		nodeparam theParamNum);		// parameter index
cnErrCode MN_DECL netGetParameterExtract(
		packetbuf *frameBuf,		// ptr to the frame buffer to format
		packetbuf *paramInfo);		// ptr to return buffer
// Set parameter command
cnErrCode MN_DECL netSetParameterFmt(
		packetbuf *frameBuf,		// ptr to the frame buffer to format
		nodeaddr theNode, 			// node address
		nodeparam theParamNum, 		// parameter index
		packetbuf *newValue);		// ptr to new parameter data

//MN_EXPORT cnErrCode MN_DECL netGetParameterDbl(
//		netaddr cNum,				// NC number
//		nodeaddr theNode, 			// device address
//		mnParams theParam, 			// parameter index
//		double *paramInfo);			// ptr for returned param
//MN_EXPORT cnErrCode MN_DECL netSetParameterDbl(
//		netaddr theNet,			// NC number
//		nodeaddr theNode, 			// device address
//		mnParams theParam, 			// parameter index
//		double newValue);			// New setting


// Internal Attention Feature for Auto-Brake and Group Shutdowns

// Get the current driver attention mask
cnErrCode MN_DECL netGetDrvrAttnMask(
		multiaddr theMultiAddr,
		attnReg *pAttnReg);

// Set the current driver attention mask
cnErrCode MN_DECL netSetDrvrAttnMask(
		multiaddr theMultiAddr,
		attnReg *pAttnReg);
		
// Get the current status event mask
cnErrCode MN_DECL netGetStatusEventMask(
		multiaddr theMultiAddr,
		mnStatusReg *pAttnReg);

// Set the current status event mask
cnErrCode MN_DECL netSetStatusEventMask(
		multiaddr theMultiAddr,
		const mnStatusReg *pAttnReg);

// Release all node parameter data for this net
void coreFreeAllNodeInfo(netaddr cNUm);
// Force flush of all cached values, next read is from the node
void coreInvalidateValCache(netaddr cNum);
void coreInvalidateValCacheByNode(netaddr cNum, nodeaddr nodeAddr);

// Update the parameter database manually from a double
cnErrCode coreSetParamFromBytes(multiaddr multiAddr, 
								 nodeparam paramNum,
								 nodeuchar *pVal,
								 unsigned sizeInBytes);

// Common initialize logic
typedef cnErrCode (*coreParamSetupFuncPtr)(multiaddr theMultiAddr);
cnErrCode MN_DECL coreInitializeEx(
		multiaddr theMultiAddr,			// Ptr to return array
		nodebool warmInitialize,
		coreParamSetupFuncPtr classSetupFunc
);

// Special parameter callback functions	that require access to net inventory
double MN_DECL spscWatchAttnMask(
	nodebool valIsBits, 				// TRUE=>ADC Temp, FALSE=>C Temp
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB);
#ifdef __cplusplus
}
#endif
 
 
#ifndef NC_DRIVER
//*****************************************************************************
// NAME																          *
//		cpymem
//		
//	DESCRIPTION:
//		This function will copy nchars of memory from <src> to <dest>.
//		
//  CREATION DATE:
//		February 13, 1998  13:24:34
//
//	RETURNS:
//		cnErrCode error code
//		
//	SYNOPSIS:
__inline void cpymem(nodeuchar *src, nodeuchar *dest, nodeushort nchars) {
	while (nchars--)
		*(dest++) = *(src++);
}
//																			  *
//*****************************************************************************
#endif


//*****************************************************************************
// NAME																          *
// 	netCmdPrivate.h constants
//
//

//																			  *
//*****************************************************************************


#endif
//============================================================================= 
//	END OF FILE netCmdPrivate.h
//=============================================================================

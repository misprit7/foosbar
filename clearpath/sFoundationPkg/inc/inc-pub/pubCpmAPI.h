//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubCpmAPI.h $
// $Revision: 18 $ $Date: 12/09/16 2:43p $
// $Workfile: pubCpmAPI.h $
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	sFoundation definitions for programming the ClearPath-SC nodes.
**/
// CREATION DATE:
//		2015-12-02 16:26:15
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2015  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
#ifndef __PUBCPM_API_H__
#define __PUBCPM_API_H__


//*****************************************************************************
// NAME																          *
// 	pubCPM_API.h headers
//
	#include "tekTypes.h"
	#include "pubNetAPI.h"
	#include "pubCpmRegs.h"
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	pubCPM_API.h constants
//
typedef enum _cpmParams cpmParams;
// 
//																			  *
//*****************************************************************************

 


//*****************************************************************************
// NAME																	      *
// 	pubCPM_API.h function prototypes
//
//
//------------------------------------------
// Parameter Group
//------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TI_COMPILER_VERSION__

MN_EXPORT cnErrCode MN_DECL cpmGetParameter(
			multiaddr theMultiAddr,			// Node address
			cpmParams theParam,			// Parameter index
			double *pParamVal);				// Ptr to value area

// Update the local parameter table and update node
MN_EXPORT cnErrCode MN_DECL cpmSetParameter(
			multiaddr theMultiAddr,	 		// Node address
			cpmParams theParam,			// Parameter index
			double paramVal);				// New value


//------------------------------------------
// Motion Group
//------------------------------------------

// Change axis to new velocity target / optional trigger
MN_EXPORT cnErrCode MN_DECL cpmForkVelMove(
			multiaddr theMultiAddr,			// Destination node
			double velTargetStepsPerSec,	// Target velocity (steps/sec)
			nodebool triggered,				// Start move with trigger
			nodelong *pBuffersRemaining=NULL);	// Ptr to move buf remaining cnt		

// Initiate positional move to target
MN_EXPORT cnErrCode MN_DECL cpmForkPosnMove( 
			multiaddr theMultiAddr,			// Destination node
			nodelong posnTarget,			// Distance (steps)
			mgPosnStyle moveType,			// Motion style
			nodelong *pBuffersRemaining=NULL);	// Ptr to move buf remaining cnt		

MN_EXPORT cnErrCode MN_DECL cpmForkProfiledMove(
			multiaddr theMultiAddr,			// Destination node
			mgMoveProfiledInfo *spec,		// Motion specification
			nodelong *pBuffersRemaining);	// Ptr to move buf remaining cnt

//------------------------------------------
// Status Group
//------------------------------------------
MN_EXPORT cnErrCode MN_DECL cpmGetStatusAccumReg(
			multiaddr theMultiAddr,			// Node address
			cpmStatusReg *pStatus);			// Current setting
MN_EXPORT cnErrCode MN_DECL cpmGetAttnStatusRiseReg(
			multiaddr theMultiAddr,			// Node address
			cpmStatusReg *pStatus);			// Current setting
MN_EXPORT cnErrCode MN_DECL cpmGetStatusFallReg(
			multiaddr theMultiAddr,			// Node address
			cpmStatusReg *pStatus);			// Current setting
MN_EXPORT cnErrCode MN_DECL cpmGetStatusRTReg(
			multiaddr theMultiAddr,			// Node address
			cpmStatusReg *pStatus);			// Current setting

// Alert and Warning Register Access
MN_EXPORT cnErrCode MN_DECL cpmGetAlertReg(
			multiaddr theMultiAddr,			// Node address
			cpmAlertReg *pAlertReg);		// Current setting
MN_EXPORT cnErrCode MN_DECL cpmGetWarningReg(
			multiaddr theMultiAddr,			// Node address
			cpmAlertReg *pWarningReg);		// Current setting

//------------------------------------------
// Homing Group
//------------------------------------------
// Initiate the homing sequencer on the specified axis
MN_EXPORT cnErrCode MN_DECL cpmSendHome(
			multiaddr theMultiAddr);		// Destination node

//------------------------------------------
// Setup Group
//------------------------------------------
MN_EXPORT cnErrCode MN_DECL cpmSetUserOutputReg (
			multiaddr theMultiAddr,			// Node address
			cpmOutReg *pOutReg);		// New setting
MN_EXPORT cnErrCode MN_DECL cpmGetUserOutputReg(
			multiaddr theMultiAddr,			// Node address
			cpmOutReg *pOutReg);		// Current setting
// Configuration Registers
MN_EXPORT cnErrCode MN_DECL cpmSetHwConfigReg(
			multiaddr theMultiAddr,			// Node address
			cpmHwConfigReg hwConfigReg);	// Current setting
MN_EXPORT cnErrCode MN_DECL cpmGetHwConfigReg(
			multiaddr theMultiAddr,			// Node address
			cpmHwConfigReg *pHwConfigReg);	// Current setting

MN_EXPORT cnErrCode MN_DECL cpmSetAppConfigReg(
			multiaddr theMultiAddr,			// Node address
			cpmAppConfigReg appConfigReg);	// Current setting
MN_EXPORT cnErrCode MN_DECL cpmGetAppConfigReg(
			multiaddr theMultiAddr,			// Node address
			cpmAppConfigReg *pAppConfigReg);	// Current setting
//																			  *
//*****************************************************************************
#endif
#ifdef __cplusplus
}
#endif

#endif

//============================================================================= 
//	END OF FILE pubCPM_API.h
//=============================================================================

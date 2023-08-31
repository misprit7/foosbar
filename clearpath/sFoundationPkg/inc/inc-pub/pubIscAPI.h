//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubIscAPI.h $
// $Revision: 26 $ $Date: 12/09/16 3:57p $
// $Workfile: pubIscAPI.h $
//
// DESCRIPTION:
/**
	\file
	\brief Function prototypes and constants related to the Meridian/ISC
	emulation API.

	\ingroup ISCgrp
	@{

**/
// CREATION DATE:
//		6/9/2009 refactored for Meridian
//		2/10/2003 originally iscCmdsAPI.h
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-2009  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//                                                                            *
//*****************************************************************************

#ifndef __PUBISCAPI_H
#define __PUBISCAPI_H

#include "pubIscRegs.h"
#include "mnParamDefs.h"
#include "pubCoreRegs.h"

 //#include "mnParamDefs.h"
 //#include "pubMotion.h"
#include "pubIex.h"

//////////////////////
// PUBLIC FUNCTIONS	//
//////////////////////

/// Helper function to convert volatile parameter access reference into
/// a non-volatile parameter access.
inline iscParams iscNonVol(iscParams volParam) {
	return((iscParams)(volParam|PARAM_OPT_MASK));
}

#ifdef __cplusplus
extern "C" {
#endif
// Restore the parameters to factory default state
MN_EXPORT cnErrCode MN_DECL iscFactoryDefaults(
			multiaddr theMultiAddr);	    // Destination node

// Get parameter value from the local parameter table or node
MN_EXPORT cnErrCode MN_DECL iscGetParameter(
			multiaddr theMultiAddr,		// Node address
			iscParams theParam,			// Parameter index
			double *pParamVal);			// Ptr to value area

MN_EXPORT cnErrCode MN_DECL iscGetParameterEx(
			multiaddr theMultiAddr,		// Node address
			iscParams theParam,			// Parameter index
			paramValue *pParamVal,		// Ptr to value area
			paramInfo *pParamInfo);		// Ptr to information area

// Update the local parameter table and update node
MN_EXPORT cnErrCode MN_DECL iscSetParameter(
			multiaddr theMultiAddr,	 	// Node address
			iscParams theParam,			// Parameter index
			double paramVal);			// New value

// Update the local parameter table and update node using a buffer
MN_EXPORT cnErrCode MN_DECL iscSetParameterEx(
			multiaddr theMultiAddr,		// Node address
			iscParams theParam,			// Parameter index
			packetbuf *pNewValue);		// New value
			

// Add to position command
MN_EXPORT cnErrCode MN_DECL iscAddToPosition(
			multiaddr theMultiAddr,		// Node address
			double theOffset);			// Offset from current location

// Add to position command, load encoder
MN_EXPORT cnErrCode MN_DECL iscAddToPositionLd(
			multiaddr theMultiAddr,		// Node address
			double theOffset);			// Offset from current location

// Synchronize position
MN_EXPORT cnErrCode MN_DECL iscSyncPosition(
			multiaddr theMultiAddr);	// Node address

// Register parameter change function
MN_EXPORT paramChangeFunc MN_DECL iscParamChangeFunc(
	  		paramChangeFunc newFunc);	// Ptr to callback function

// Macro commands - these all block until completion

MN_EXPORT cnErrCode MN_DECL iscSetUserOutputReg (
			multiaddr theMultiAddr,		// Node address
			plaOutReg newState);		// New setting
MN_EXPORT cnErrCode MN_DECL iscGetUserOutputReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetOutputReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetOutputRiseReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetOutputFallReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg);		// Current setting

// Status Register Access		
MN_EXPORT cnErrCode MN_DECL iscGetStatusAccumReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetAttnStatusRiseReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetStatusFallReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetStatusRTReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus);		// Current setting

// Attention Generation Setup
MN_EXPORT cnErrCode MN_DECL iscGetAttnMask(
			multiaddr theMultiAddr,		// Node address
			attnReg *pAttnMask);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscSetAttnMask(
			multiaddr theMultiAddr,		// Node address
			attnReg *pAttnMask);		// Current setting

// Alert and Warning Register Access with mask registers
MN_EXPORT cnErrCode MN_DECL iscGetAlertReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pAlertReg);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetAlertMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pAlertMask);		// Get current setting
MN_EXPORT cnErrCode MN_DECL iscSetAlertMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pAlertMask);		// New setting


MN_EXPORT cnErrCode MN_DECL iscGetWarningReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pWarningReg);		// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetWarningMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pWarningMask);		// Get current setting
MN_EXPORT cnErrCode MN_DECL iscSetWarningMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pWarningMask);		// New setting



// Configuration Registers
MN_EXPORT cnErrCode MN_DECL iscSetHwConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscHwConfigReg hwConfigReg);	// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetHwConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscHwConfigReg *pHwConfigReg);	// Current setting

MN_EXPORT cnErrCode MN_DECL iscSetAppConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscAppConfigReg appConfigReg);	// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetAppConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscAppConfigReg *pAppConfigReg);	// Current setting

MN_EXPORT cnErrCode MN_DECL iscSetTuneConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscTuneConfigReg tuneConfigReg);	// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetTuneConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscTuneConfigReg *pTuneConfigReg);	// Current setting

MN_EXPORT cnErrCode MN_DECL iscSetLdMtrRatio(
			multiaddr theMultiAddr,			// Node address
			nodelong loadPart,
			nodelong motorPart);			// Current setting
MN_EXPORT cnErrCode MN_DECL iscGetLdMtrRatio(
			multiaddr theMultiAddr,			// Node address
			nodelong *pLoadPart,
			nodelong *pMotorPart);			// Current setting


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//								 MOTION API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// Constraint short cuts
MN_EXPORT cnErrCode MN_DECL iscGetJerkTime(
			multiaddr theMultiAddr,
			double *pSettingValMS); 
MN_EXPORT cnErrCode MN_DECL iscSetJerkTime(
			multiaddr theMultiAddr,
			double newValMS);


//-----------------
// Velocity Group
//-----------------
// Low level command primitive
MN_EXPORT cnErrCode MN_DECL iscForkMoveVelEx(
			multiaddr theMultiAddr,		// Node address
			double velTargetStepPerSec,	// Velocity (steps/sec)
			nodelong positionTarget,	// Target Position (optional)
			mgVelStyle moveType);		// Motion style

MN_EXPORT cnErrCode MN_DECL iscForkMoveVelQueued(
			multiaddr theMultiAddr,		// Node address
			double velTargetStepPerSec,	// Velocity (steps/sec)
			nodelong positionTarget,	// Target Position (optional)
			mgVelStyle moveType,		// Motion style
			nodelong *pBuffersRemaining);	// Ptr to move buf remaining cnt

// Change axis to new velocity target / optional trigger
MN_EXPORT cnErrCode MN_DECL iscForkVelMove(
			multiaddr theMultiAddr,		// Destination node
			double velTargetStepPerSec,	// Target velocity (steps/sec)
			nodebool triggered);		// Start move with trigger

// Change axis to new velocity target with script buffer management
MN_EXPORT cnErrCode MN_DECL iscForkVelMoveBuffered(
			multiaddr theMultiAddr,		// Destination node
			double velTargetStepPerSec,	// Target velocity (steps/sec)
			nodelong positionTarget,	// Target Position (optional)
			mgVelStyle moveType);		// Motion style

MN_EXPORT cnErrCode MN_DECL iscForkSkylineMove(
			multiaddr theMultiAddr,			// The node
			double velTargetStepPerSec,		// Target velocity (steps/sec)
			nodelong positionSpec,			// Position Target [optional]
			mgVelStyle moveType,			// Enhanced style selection
			nodelong *pBuffersRemaining);	// Ptr to move buf remaining cnt

//-----------------
// Positional Group
//-----------------
// Low level command primitive 
MN_EXPORT cnErrCode MN_DECL iscForkMoveEx(
			multiaddr theMultiAddr,		// Destination node
			nodelong posnTarget,		// Distance (steps)
			mgPosnStyle moveType);		// Motion style

// Low level command primitive (>=v5.5 / buffer remaining)
MN_EXPORT cnErrCode MN_DECL iscForkMoveQueued( 
			multiaddr theMultiAddr,		// Destination node
			nodelong posnTarget,		// Distance (steps)
			mgPosnStyle moveType,		// Motion style
			nodelong *pBuffersRemaining);	// Ptr to move buf remaining cnt		

// Initiate positional motion with script buffer management.
MN_EXPORT cnErrCode MN_DECL iscForkMoveExBuffered(
			multiaddr theMultiAddr,		// Destination node
			nodelong posnTarget,			// Distance (steps)
			mgPosnStyle moveType);		// Motion style

// Compatibility with ControlPoint 
MN_EXPORT cnErrCode MN_DECL iscMoveProfiled(
			multiaddr theMultiAddr,		// Destination node
			mgMoveProfiledInfo *spec);	// Motion specification
MN_EXPORT cnErrCode MN_DECL iscMoveProfiledQueued(
			multiaddr theMultiAddr,		// Destination node
			mgMoveProfiledInfo *spec,	// Motion specification
			nodelong *pBuffersRemaining);	// Ptr to move buf remaining cnt

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//						   NODE IDENTIFICATION API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Set/Get the user ID
MN_EXPORT cnErrCode MN_DECL iscSetUserID(
			multiaddr theMultiAddr,
			const char *pNewName);

MN_EXPORT cnErrCode MN_DECL iscGetUserID(
			multiaddr theMultiAddr,
			char *pUserIdStr,
			Uint16 maxBufSize);
// Set/Get Motor File Name
MN_EXPORT cnErrCode MN_DECL iscSetMotorFileName(
			multiaddr theMultiAddr,
			const char *pNewName);

MN_EXPORT cnErrCode MN_DECL iscGetMotorFileName(
			multiaddr theMultiAddr,
			char *pMotorFileNameStr,
			Uint16 maxBufSize);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//						  MONITOR PORT/DATA ACQUISITION API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Monitor port manipulations
MN_EXPORT cnErrCode MN_DECL iscGetMonitor(
			multiaddr theMultiAddr,			// Target node addr
			nodeushort channel,				// Target monitor port if supported
			iscMonState *pState);

MN_EXPORT cnErrCode MN_DECL iscSetMonitor(
			multiaddr theMultiAddr,			// Target node addr
			nodeushort channel,				// Target monitor port if supported
			iscMonState *pNewState);

MN_EXPORT cnErrCode MN_DECL iscGetDataCollected(
			multiaddr theMultiAddr,			// Destination node
			iscDataCollect *pReturnData);	// Returned data

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//						  IEX API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
MN_EXPORT cnErrCode MN_DECL iscGetIEXStatus(
			multiaddr theMultiAddr,
			iexStatusReg *pIexStatus);

MN_EXPORT cnErrCode MN_DECL iscRestartIEX(
			multiaddr theMultiAddr);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//						     TUNING STIMULUS API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
MN_EXPORT cnErrCode MN_DECL iscGetStimulus(
			multiaddr theMultiAddr,	// Target node addr
			iscStimState *pState);

MN_EXPORT cnErrCode MN_DECL iscSetStimulus(
			multiaddr theMultiAddr,	// Target node addr
			iscStimState *pNewState);

// Reset the vector search flag
MN_EXPORT cnErrCode MN_DECL iscReVector(
			multiaddr theMultiAddr);	// Target node addr

#ifdef __cplusplus
}
#endif
/// @}

#endif // __ISCCMDSAPI_H



/*========================================================================== 
	END OF FILE iscCmdsAPI.h
  ==========================================================================*/

//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/src/iscAPI.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: iscAPI.cpp $
//
// DESCRIPTION:
/**
	\file
	\cond ISC_DOC
	\brief ISC Programming Interface Implementation.

	\defgroup ISCgrp ISC Programming Interface
	\brief Functions related to Integrated Servo Controller (ISC) functionality.

	Provides:
		- checking on the operation state of the node
		- initiating and canceling motion
		- accessing and programming the Programmable Logic Array (PLA)
		- accessing and changing I/O
		- accessing and modifying the parameter table

	\remark C and C++ Applications should include the master header file
	\c meridianHdrs.h in the source code to insure the various function
	prototypes and data types are available for your application.

	@{
**/		
//
// CREATION DATE:
//		6/6/2009 refactored from ControlPoint iscCmds.c
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-2014  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																          *
// 	 iscAPI.cpp headers
//
/// \cond	INTERNAL_DOC
 	#include <stdlib.h>
 	#include <malloc.h>
	#include <assert.h>
	#include <stdint.h>
	#include <math.h>
/// \endcond
#if 1  // TO_KILL was used to test header file issues
	#include "pubCoreRegs.h"
	#include "pubNetAPI.h"
	#include "netCmdPrivate.h"
	#include "converterLib.h"
	#include "netCmdAPI.h"
	#include "sFoundResource.h"
	#include "iscRegs.h"
	#include "pubIscAPI.h"
/// \cond	INTERNAL_DOC
	#include "lnkAccessCommon.h"
#if defined(_WIN32) || defined(_WIN64)
	#include <crtdbg.h>
#endif
#if (defined(_MSC_VER))
	#pragma warning(disable:4996)
#else
	#include <string.h>
#endif
#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
/// \endcond																  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	 iscAPI.cpp function prototypes
//
/// \cond INTERNAL_DOC
#define Sgn(x) (((x)>0) ? 1 : (((x)==0) ? 0 : -1))
#define Q15_MAX (32767./32768.)
// Module private functions

// Buffer management items
static cnErrCode MN_DECL waitForMoveBuffer(
			multiaddr theMultiAddr);			// Node to access
// "Class" constructor
cnErrCode iscClassSetup(multiaddr theMultiAddr);
// "Class" destructors
void iscClassDelete(multiaddr theMultiAddr);

EXTERN_C paramscale VelScale;
/// \endcond																  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	iscAPI.cpp imports
//
extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	iscAPI.cpp constants
//
/// \cond INTERNAL_DOC
const double ISC_MON_SCALE = 1<<16; 	// Format of scale/ampl control
const double ISC_MON_MAX_VEL = 8192;   	// Vel. full-scale (quads/sample-time)
const double ISC_MON_MIN_VEL = 4;      	// Minimum velocity to display
const double ISC_MON_MAX_POS_LEGACY = 8192;	// Position error full-scale
const double ISC_MON_MAX_POS = 32768;		// Position error full-scale
const double ISC_MON_MAX_POS_MEAS = (1<<20);
const double ISC_MON_MAX_JRK = 0.000001;
const double ISC_MON_MAX_INTG = 1UL<<31;
#define ISC_ADC_SCALE (0.8*(double)(1<<13))	// ADC measure to torque values

// RAS selector table						
typedef struct _rasTarg {
	double target;						// Optimal value
	unsigned code;						// RAS code for this
} rasTarg;

#define ISC_MV_NEG_LIMIT  0x800000	// Largest negative move
#define ISC_MV_POS_LIMIT  0x7FFFFF	// Largest positive move
/// \endcond																  *
//*****************************************************************************

//==============================================================================
/// \cond INTERNAL_DOC
//	Internally documented functions from here to "END OF UNDOCUMENTED..."
//==============================================================================

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Common unit converters
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//*****************************************************************************
//	NAME																	  *
//		convertADCmax
//
//	DESCRIPTION:
///		Conversion to and from ADC max parameter
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
static double MN_DECL convertADCmax(
	nodebool valIsBits, 				// TRUE=>drive bits, FALSE=> app bits
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// To/From value
	byNodeDB *pNodeDB)				// Parameter information
{
	double iMax;
	cnErrCode theErr;
	theErr = iscGetParameter(theMultiAddr, ISC_P_DRV_I_MAX, &iMax);
	if (theErr==MN_OK) {
		if (valIsBits) {
			return(iMax/convVal);
		}
		else {
			return(iMax/convVal);
		}
	}
	return(0);
}
//																			  *
//*****************************************************************************

/*****************************************************************************
 *	!NAME!
 *		convertAmperes
 *
 *	DESCRIPTION:
 *		Convert a 1.15 type number into a fraction of I_MAX.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertAmperes(
	nodebool valIsBits, 				// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	paramValue iMax;					// Full scale
	double newInt;						// Temp variables
	cnErrCode theErr;

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_I_MAX, NULL, &iMax);
	// The basis value is OK?
	if (theErr != MN_OK || iMax.value <= 0)
		return(0);

	if (valIsBits)   {
		// Convert to Amperes from the fraction of full scale
		return(convVal * iMax.value);
	}
	else {
		newInt = (convVal / iMax.value);
		if (newInt > Q15_MAX) newInt = Q15_MAX;
		return(newInt);
	}
}
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		convertMeasAmperes
 *
 *	DESCRIPTION:
 *		Convert a 1.15 type number into a fraction of ADC_MAX.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertMeasAmperes(
	nodebool valIsBits, 				// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,					// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	paramValue adcMax;					// Full scale
	double newInt;						// Temp variables
	cnErrCode theErr;

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ADC_MAX, NULL, &adcMax);
	// The basis value is OK?
	if (theErr != MN_OK || adcMax.value <= 0)
		return(0);

	if (valIsBits)   {
		// Convert to Amperes from the fraction of full scale
		return(convVal * adcMax.value);
	}
	else {
		newInt = (convVal / adcMax.value);
		if (newInt > Q15_MAX) newInt = Q15_MAX;
		return(newInt);
	}
}
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		convertTripAmperes
 *
 *	DESCRIPTION:
 *		Convert a 1.15 type number into a fraction of I_MAX.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
//static double MN_DECL convertTripAmperes(
//	nodebool valIsBits, 				// TRUE=>RMS, FALSE=>unscaled
//	multiaddr theMultiAddr,				// Node address
//	appNodeParam parameter,				// Target parameter
//	double convVal,						// From value
//	byNodeDB *pNodeDB)					// Parameter information
//{
//	paramValue globLim;					// Full scale
//	double newVal;						// Temp variables
//	cnErrCode theErr;
//
//	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_TRQ_LIM, NULL, &globLim);
//	// The basis value is OK?
//	if (theErr != MN_OK || globLim.value <= 0)
//		return(0);
//
//	if (valIsBits)   {
//		// Convert to Amperes from the fraction of full scale
//		return(convVal * globLim.value);
//	}
//	else {
//		newVal = (convVal / globLim.value);
//		if (newVal > 1) newVal = 1;
//		return(newVal);
//	}
//}
///*																 !end!		*/
///****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		convertFilt99pctMilliseconds
 *
 *	DESCRIPTION:
 *		Convert IIR time constants from milliseconds to and from the 99% 
 *		trip points.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertFilt99pctMilliseconds(
	nodebool scaled, 					// TRUE=>milliseconds, FALSE=>unscaled
	multiaddr theMultiAddr,					// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	paramValue sampleTime;
	cnErrCode theErr;
	nodelong intVal; double x;

	const double TripPoint = 1-0.99;	// Trip point

	// Get the sample period
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD,	
								  NULL, &sampleTime);
	// Was it OK?
	if (theErr != MN_OK && sampleTime.value <=0)
		return(0);

	if (scaled)  {
		// Convert to milliseconds from base units
		if (convVal > 32767 || convVal <=0)	
			return(0);
		return(.001*sampleTime.value * log(TripPoint)/log(convVal/32768.));
	}
	else {
		// Convert to base units from milliseconds	
		if (convVal <= 0) return(0);
		x = pow(TripPoint, .001*sampleTime.value/convVal);
		intVal = (nodelong)((32768.*x)+0.5);
		if (intVal > 32767) intVal = 32767;
		return(intVal);
	}
}
/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		convertFilt1TCMilliseconds
 *
 *	DESCRIPTION:
 *		Convert IIR time constants from milliseconds to and from the 99% 
 *		trip points.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertFilt1TCMilliseconds(
	nodebool scaled, 					// TRUE=>milliseconds, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	paramValue sampleTime;
	cnErrCode theErr;
	nodelong intVal; double x;

	const double TripPoint = 0.367879;	// Trip point 1/e

	// Get the sample period
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD,	
								  NULL, &sampleTime);
	// Was it OK?
	if (theErr != MN_OK && sampleTime.value <=0)
		return(0);

	if (scaled)  {
		// Convert to milliseconds from base units
		if (convVal > 32767 || convVal <=0)	
			return(0);
		return(.001*sampleTime.value * log(TripPoint)/log(convVal/32768.));
	}
	else {
		// Convert to base units from milliseconds	
		if (convVal <= 0) return(0);
		x = pow(TripPoint, .001*sampleTime.value/convVal);
		intVal = (nodelong)((32768.*x)+0.5);
		if (intVal > 32767) intVal = 32767;
		return(intVal);
	}
}
/*																 !end!		*/
/****************************************************************************/


///*****************************************************************************
// *	!NAME!
// *		convertVectFilt99pctMilliseconds
// *
// *	DESCRIPTION:
// *		Convert vector rate IIR time constants from milliseconds to and
// *		from the 99% trip points.
// *
// *	\return
// *		double
// *
// *	SYNOPSIS: 															    */
//static double MN_DECL convertVectFilt99pctMilliseconds(
//	nodebool scaled, 					// TRUE=>milliseconds, FALSE=>unscaled
//	multiaddr theMultiAddr,					// Node address
//	appNodeParam parameter,				// Target parameter
//	double convVal,						// From value
//	byNodeDB *pNodeDB)					// Parameter information
//{
//
//	if (scaled)  {
//		return convertFilt99pctMilliseconds(scaled, theMultiAddr, parameter, convVal, pNodeDB)/4;
//	}
//	else {
//		return convertFilt99pctMilliseconds(scaled, theMultiAddr, parameter, convVal*4, pNodeDB);
//	}
//}
///*																 !end!		*/
///****************************************************************************/



/*****************************************************************************
 *	!NAME!
 *		convertRMSlevel
 *
 *	DESCRIPTION:
 *		Convert RMS level from base units. This RMS level is a 
 *		0 TO 100 value where 100 corresponds to the RMS shutdown point.	 This
 *		converter does not need to convert to base units as it is a display
 *		value only.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertRMSlevel(
	nodebool scaled, 					// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	paramValue rmsLimit, adcMax;			// Basis information
	double finalVal;
	cnErrCode theErr;
//                             %     Trq->A                 Q Factor
//	const double CONST_FACT = 100 / (2^28) ^ 0.5
//	const double CONST_FACT = 0.006103515625;
	
	// Maximum current
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ADC_MAX, NULL, &adcMax);
	if (theErr != MN_OK || adcMax.value == 0)
		return(0);

	// RMS maximum amperes
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_RMS_LIM, NULL, &rmsLimit);
	if (theErr != MN_OK || rmsLimit.value == 0)
		return(0);

	if (convVal < 0 || convVal > 0x7FFFFFFF) 
		return(0);
		
	// Convert to integer with rounding
	//										      ADCmax
	finalVal = (nodelong)(((100 * sqrt(convVal/(1<<28)) * adcMax.value) 
				/ rmsLimit.value) + 0.5);

	// Insure the numbers don't exceed the range
	if (finalVal > 100) finalVal = 100;
	if (finalVal < 0) finalVal = 0;
	
	//_RPT2(_CRT_WARN, "%.3f  %08x\n", finalVal, lVal);

	return(finalVal);
}

/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		convertAmpsRMS
 *
 *	DESCRIPTION:
 *		Convert RMS level from base units. This RMS level is returned
 *		referenced to ADC max.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertAmpsRMS(
	nodebool valIsBits, 				// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)					// Parameter information
{
	paramValue adcMax;			// Basis information
	double finalVal;
	cnErrCode theErr;
//                             %     Trq->A                 Q Factor
//	const double CONST_FACT = 100 / (2^28) ^ 0.5
	
	// Maximum current
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ADC_MAX, NULL, &adcMax);
	if (theErr != MN_OK)
		return(0);

	if (valIsBits) {
		// Prevent errors
		if (convVal < 0) convVal = 0;
		finalVal = (sqrt(convVal) * adcMax.value);
	}
	else {
		finalVal = convVal / adcMax.value;
		finalVal *= finalVal;
	}
	return(finalVal);
}

/*																 !end!		*/
/****************************************************************************/


//****************************************************************************
//	NAME
//		convertMonGain
//
//	DESCRIPTION:
//		Convert the monitor port gain setting to/from a full-scale range
//		value.
//
//	\return
//		double
//
//  SETUPS
#if defined(__GNUC__)&&!defined(__QNX__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
//
//	SYNOPSIS:
static double MN_DECL convertMonGain(
	nodebool valIsBits, 				// TRUE=>scaled, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	iscMonVars theVar,					// The variable to adjust
	double convVal)
{
	paramValue sampleTime, iMax, adcMax;
	cnErrCode theErr;
	double baseVal, fwVer;
	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr addr = NODE_ADDR(theMultiAddr);
	assert(SysInventory[cNum].NodeInfo[addr].pNodeSpecific != NULL);
	iscState *pState = (iscState *)SysInventory[cNum].NodeInfo[addr].pNodeSpecific;
	monState &monNow = pState->MonState; 
	monNow.TestPoint = theVar;

	// Assume failure until we get to the end
	monNow.Set = false;
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_I_MAX, NULL, &iMax);
	if (theErr != MN_OK)
		return(0);
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ADC_MAX, NULL, &adcMax);
	if (theErr != MN_OK)
		return(0);
	if (iMax.value <=0 || adcMax.value <= 0)
		return(0);

	// Get the sample period
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD,	
								  NULL, &sampleTime);
	if (fabs(sampleTime.value) < 0.001) 
		// Protect from divide-by-zero
		return(0);

	if (valIsBits)  {
		// Convert from fixed point
		convVal /= ISC_MON_SCALE;
	}
	// Protect from \ 0
	if (fabs(convVal) < 0.0001)  {
		// Show as "off"
		return(0);
	}
	// return the full scale value
	switch (theVar) {
		case MON_VEL_MEAS:
		case MON_VEL_CMD:
		case MON_VEL_TRK:
		case MON_VEL_TRK_SERVO:
		case 52:
			// Return the monitor value in Hertz
			baseVal = ((1e3*ISC_MON_MAX_VEL)
			        / (sampleTime.value*convVal));
			break;

		case MON_VEL_STEP:
			baseVal = (4e3*ISC_MON_MAX_VEL)
			        / (sampleTime.value*convVal);
			break;

		case MON_JRK_CMD:
			baseVal = (1e12*ISC_MON_MAX_VEL)
					/ (pow(sampleTime.value,3) * convVal);
			break;

		case MON_ACC_CMD:
			baseVal = (1e6*ISC_MON_MAX_VEL)
					/ (sampleTime.value*sampleTime.value*convVal);
			break;

		case MON_POSN_TRK:
		case MON_POSN_DIR_TRK:
		case MON_TRK_LD:
		case MON_POSN_DIR_TRK_MTR:
		case MON_POSN_TRK_MTR:
		case MON_COUPLING:
			theErr = iscGetParameter(theMultiAddr, ISC_P_FW_VERSION, &fwVer);
			if (theErr != MN_OK || fwVer < FW_MILESTONE_IEX)
				baseVal = (ISC_MON_MAX_POS_LEGACY/convVal);
			else
				baseVal = (ISC_MON_MAX_POS/convVal);
			break;

		case MON_SINE_R:		// (2.14 format)
		case MON_COS_R:
			baseVal = (200/convVal);
			break;

		case MON_TRQ_MEAS:		// (1.15 format) scaled to trq amps
		case MON_TRQ_MEAS_PEAK:
		case MON_TRQ_D_MEAS:
		case MON_TRQ_TRK:
		case MON_TRQ_TRK_PEAK:
			baseVal = (100*adcMax.value/iMax.value)/convVal;
			break;

		case MON_POSN_MEAS:
		case 69:				// Position Cmd
			baseVal = (ISC_MON_MAX_POS_MEAS/convVal);
			break;

		case MON_INTEGRATOR:
			baseVal = (ISC_MON_MAX_INTG/convVal);
			break;

		case MON_SGN_CMD_VEL:
		case MON_SGN_CMD_STEP:
			baseVal = (100. / convVal);
			break;

		case MON_TRQ_CMD:		// (1.15 format)
		case MON_CALIBRATE:
		default:
			baseVal = (100/convVal);
			break;
	}
	if (!valIsBits) {
		baseVal *= 0x10000;				// Scale to (16.16)
		// Put at maximum number
		if (baseVal > 0x7fffffff) baseVal = 0x7fffffff;
		// Get rescaled value
		monNow.FullScale = convertMonGain(true, theMultiAddr, theVar, baseVal); 
		monNow.Set = true;
	}
	// Save our full scale value in user units
	else {
		monNow.FullScale = baseVal;
		monNow.Set = true;
	}
	return(baseVal);
}
#if defined(__GNUC__)&&!defined(__QNX__)
#pragma GCC diagnostic pop
#endif
/****************************************************************************/




//****************************************************************************
//	NAME
//		convertAngle
//
//	DESCRIPTION:
//		Convert to and from the commutation angle in encoder ticks to an
//		angle in electrical degrees.
//
//	\return
//		double
//
//	SYNOPSIS:
static double MN_DECL convertAngle(
	nodebool valIsBits, 				// TRUE=>drive bits, FALSE=>mech degrees
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *pNodeDB)
{
	paramValue poles, encDens, refOffs;
	iscHwConfigReg hwConfig;

	double ticksPerDegree, degreesPerTurn, rVal;
	cnErrCode theErr;
	
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_MTR_POLES, NULL, &poles);
	if (theErr != MN_OK)
		return(0);

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ENC_DENS, NULL, &encDens);
	if (theErr != MN_OK)
		return(0);

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_COM_RO, NULL, &refOffs);
	if (theErr != MN_OK)
		return(0);

	theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
	if (theErr != MN_OK)
		return(0);

	degreesPerTurn = poles.value * 180.;
	if (poles.value != 0) {
		ticksPerDegree = encDens.value / degreesPerTurn;
	}
	else {
		return(0);
	}

	if (valIsBits)  {
		// Convert convVal to electrical degrees
		rVal = convVal/ticksPerDegree-refOffs.value;
		if (hwConfig.Fld.VectorLock) {
			rVal += 90;
		}
		// Keep result between 0 and degreesPerTurn
		if (rVal >= degreesPerTurn)
			rVal -= degreesPerTurn;
		if (rVal < 0)
			 rVal += degreesPerTurn;
		// Return the value 
		return(rVal);
	}
	else {
		// Convert convVal to drive bits
		rVal = convVal + refOffs.value;
		if (hwConfig.Fld.VectorLock)
			rVal -= 90;
		// Scale back to ticks
		rVal *= ticksPerDegree;
		// Bounds test the results between 0 and ticksPerTurn-1
		if (rVal < 0)
			rVal += ticksPerDegree*degreesPerTurn;
		if (rVal >= ticksPerDegree*degreesPerTurn)
			rVal -= ticksPerDegree*degreesPerTurn;
		// Return the final value
		return(((nodelong)(rVal+0.5)));
	}	
}
/****************************************************************************/



//****************************************************************************
//	NAME
//		convertKe
//
//	DESCRIPTION:
//		Convert Ke to properly scaled value when using a linear motor.
//
//	\return
//		double
//
//	SYNOPSIS:
static double MN_DECL convertKe(
	nodebool valIsBits, 				// TRUE=>drive bits, FALSE=>msecs
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// To/From value
	byNodeDB *pNodeDB)				// Parameter information
{
	iscHwConfigReg hwConfig;
	paramValue encRes, encDens;
	cnErrCode theErr;
	double cVal;

	theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
	if (theErr != MN_OK)
		return(0);

	// The Encoder Density
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ENC_DENS, NULL, &encDens);
	if (theErr != MN_OK)
		return(0);
	// The Encoder Resolution
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ENC_RES, NULL, &encRes);
	if (theErr != MN_OK)
		return(0);
	cVal = ((1000.*encRes.value*encDens.value)/(60.*25400.));
	// Make 
	if (valIsBits) {
		// conVal = drive bits (V/KRPM)
		if (hwConfig.Fld.MotorType == iscHwConfigFlds::MOTOR_BRUSHLESS_LINEAR) {
			// Convert V/Inches/sec => Rotational V/KRPM
			if (cVal != 0)
				return convVal/cVal;
			else
				return 0;
		}
		else {
			return(convVal);
		}
	}
	else {
		// conVal = V/KRPM
		if (hwConfig.Fld.MotorType == iscHwConfigFlds::MOTOR_BRUSHLESS_LINEAR) {
			return convVal*cVal;
		}
		else {
			return(convVal);
		}
	}
}
/****************************************************************************/



/*****************************************************************************
 *	!NAME!
 *		convertRMSlimit
 *
 *	DESCRIPTION:
 *		Unit conversion to convert the RMS limit settings to and from the 
 *		unscaled values.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertRMSlimit(
	nodebool valIsBits, 			// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,			// Node address
	appNodeParam parameter,			// Target parameter
	double convVal,					// From value
	byNodeDB *pNodeDB)				// Parameter information
{
	paramValue adcMax;
	cnErrCode theErr;
	int rVal;

	const double CONST_FACT = 1<<12; //(4.12 basis value)
	const double TRQ_MAX_PCT = 0.9999;

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ADC_MAX, NULL, &adcMax);
	if (theErr == MN_OK) {
		// Make sure the adcMax is reasonable
		if (adcMax.value <= 0)
			return(0);
		// Make sure the value in is reasonable
		if (convVal <= 0)		   
			return(0);

		// Get the maximum current 
		if (valIsBits)
			// Convert to RMS from unscaled
			return(sqrt(convVal * adcMax.value * adcMax.value / CONST_FACT));
		else {
			// Convert RMS to unscaled value
			if (convVal > adcMax.value)	
				rVal = ((int)(CONST_FACT * 
					  ((TRQ_MAX_PCT) * (TRQ_MAX_PCT))));
			else
				rVal = ((int)(CONST_FACT * 
				           ((convVal / adcMax.value) * (convVal / adcMax.value))+0.5));
			//_RPT2(_CRT_WARN, "ISC(%d) RMS LIM = 0x%x\n", theMultiAddr, rVal);
			return(rVal);
		}
	}
	return (0);
}

/*																 !end!		*/
/****************************************************************************/





/*****************************************************************************
 *	!NAME!
 *		convertRMSlimit
 *
 *	DESCRIPTION:
 *		Unit conversion to convert the RMS limit settings to and from the 
 *		unscaled values.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertRMSlimit32(
	nodebool valIsBits, 			// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,			// Node address
	appNodeParam parameter,			// Target parameter
	double convVal,					// From value
	byNodeDB *pNodeDB)				// Parameter information
{
	paramValue adcMax;
	cnErrCode theErr;
	int rVal;
	double nodeVers;

	const double CONST_FACT = 1<<28; //(4.28 basis value)
	const double TRQ_MAX_PCT = 0.9999;

	theErr = iscGetParameter(theMultiAddr, ISC_P_FW_VERSION, &nodeVers);
	if (theErr != MN_OK)
		return(0);
	// Prior to FW_MILESTONE_RMS_LIM_32, the RMS limit was 16 bits
	if ( nodeVers < FW_MILESTONE_RMS_LIM_32 &&
		(parameter.fld.bank == 1 && parameter.fld.param == (ISC_P_DRV_RMS_LIM&0x7f)) ) {
		return convertRMSlimit(valIsBits, theMultiAddr, parameter, convVal, pNodeDB);
	}
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_DRV_ADC_MAX, NULL, &adcMax);
	if (theErr == MN_OK) {
		// Make sure the adcMax is reasonable
		if (adcMax.value <= 0)
			return(0);
		// Make sure the value in is reasonable
		if (convVal <= 0)		   
			return(0);

		// Get the maximum current 
		if (valIsBits)
			// Convert to RMS from unscaled
			return(sqrt(convVal * adcMax.value * adcMax.value / CONST_FACT));
		else {
			// Convert RMS to unscaled value
			if (convVal > adcMax.value)	
				rVal = ((int)(CONST_FACT * 
					  ((TRQ_MAX_PCT) * (TRQ_MAX_PCT))));
			else
				rVal = ((int)(CONST_FACT * 
				           ((convVal / adcMax.value) * (convVal / adcMax.value))+0.5));
			//_RPT2(_CRT_WARN, "ISC(%d) RMS LIM = 0x%x\n", theMultiAddr, rVal);
			return(rVal);
		}
	}
	return (0);
}

/*																 !end!		*/
/****************************************************************************/


/*****************************************************************************
 *	!NAME!
 *		convertRMStc
 *
 *	DESCRIPTION:
 *		Convert RMS time constants to and from seconds and base units.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertRMStc(
	nodebool valIsBits, 			// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,			// Node address
	appNodeParam parameter,			// Target parameter
	double convVal,					// From value
	byNodeDB *pNodeDB)				// Parameter information
{
	paramValue sampleTimeV;
	cnErrCode theErr;
	double b;
	double retVal;

	// Winding Amperes to torque Amperes factor
	const int SCALE_FACT_Q = 23;              // Parameter window scale 

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD,	NULL, &sampleTimeV);
	if (theErr != MN_OK)
		return(0);
	sampleTimeV.value *=  1e-6;		// Convert to seconds

	// Make sure the sampleTime is reasonable
	if (sampleTimeV.value == 0)
		return(0);

	// Insure problems return zero
	retVal = 0;

	// Convert TC to and from base units 
	if (valIsBits)	 {
		// Convert base to seconds
		b = (1. - (convVal/(1<<SCALE_FACT_Q)));
		if (b>0 && b<1)  {
			b = log(b)/sampleTimeV.value;
			if (b != 0)
				retVal = log(8./9.)/b;	
		}
		//_RPT2(_CRT_WARN, "ISC(%d) RMS TC = %.3f\n", theMultiAddr, retVal);
	}
	else {
		// Convert seconds to base units

		// Enforce minimal values
		if (convVal < 0.01) convVal = 0.01;
		b = 1 - pow(8./9., (sampleTimeV.value/convVal));
		retVal = (nodelong)((b*(1<<SCALE_FACT_Q)+.5));
		// Prevent the value from turning "negative"
		if (retVal > 32767) retVal = 32767;
		//_RPT2(_CRT_WARN, "ISC(%d) RMS TC = 0x%x\n", theMultiAddr, retVal);
	}
	return (retVal);
}


/*****************************************************************************
 *	!NAME!
 *		convertIBRMStc
 *
 *	DESCRIPTION:
 *		Convert IB RMS time constants to and from seconds and base units.
 *
 *	\return
 *		double
 *
 *	SYNOPSIS: 															    */
static double MN_DECL convertIBRMStc(
	nodebool valIsBits, 			// TRUE=>RMS, FALSE=>unscaled
	multiaddr theMultiAddr,			// Node address
	appNodeParam parameter,			// Target parameter
	double convVal,					// From value
	byNodeDB *pNodeDB)				// Parameter information
{
	paramValue sampleTimeV;
	cnErrCode theErr;
	double b;
	double retVal;

	// Winding Amperes to torque Amperes factor
	const int SCALE_FACT_Q = 23;              // Parameter window scale 

	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD,	NULL, &sampleTimeV);
	if (theErr != MN_OK)
		return(0);
	sampleTimeV.value *=  1e-6/4;		// Convert to seconds

	// Make sure the sampleTime is reasonable
	if (sampleTimeV.value == 0)
		return(0);

	// Insure problems return zero
	retVal = 0;

	// Convert TC to and from base units 
	if (valIsBits)	 {
		// Convert base to seconds
		b = (1. - (convVal/(1<<SCALE_FACT_Q)));
		if (b>0 && b<1)  {
			b = log(b)/sampleTimeV.value;
			if (b != 0)
				retVal = log(8./9.)/b;	
		}
		if (retVal < 0.01) retVal = 0.01;
		//_RPT2(_CRT_WARN, "ISC(%d) IB RMS TC = %.3f\n", theMultiAddr, retVal);
	}
	else {
		// Convert seconds to base units

		// Enforce minimal values
		if (convVal < 0.01) convVal = 0.01;
		b = 1 - pow(8./9., (sampleTimeV.value/convVal));
		retVal = (nodelong)(b*(1<<SCALE_FACT_Q));
		// Prevent the value from turning "negative"
		if (retVal > 32767) retVal = 32767;
		//_RPT2(_CRT_WARN, "ISC(%d) IB RMS TC = 0x%x\n", theMultiAddr, retVal);
	}
	return (retVal);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	iscAPI.cpp static variables
//
// The parameter handler table
#if defined (__GNUC__)
	#pragma GCC diagnostic ignored "-Wmissing-braces"
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static const paramInfoLcl iscInfoDB[] = {
// 		 1/x, signed, type,  unit,    size, scale,		config key id,			description, 			[converter]
/* 0  */ { 0, 0, PT_RO,		DEV_ID,			2, 1<<8,	PARAM_NULL,				STR_PARAM_DEVID}, 	
/* 1  */ { 0, 0, PT_RO,	 	FW_VERS,		2, 1<<12,	PARAM_NULL,				STR_PARAM_FWVERS}, 	
/* 2  */ { 0, 0, PT_RO,	 	HW_VERS,		2, 1<<8,	PARAM_NULL,				STR_PARAM_HWVERS}, 	
/* 3  */ { 0, 0, PT_RO,		BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_RESELLERID},
/* 4  */ { 0, 0, PT_RO,	 	NO_UNIT, 		4, 1,		PARAM_SERIAL_NUMBER,	STR_PARAM_SERIAL_NUM}, 	
/* 5  */ { 0, 1, PT_RO,	 	BIT_FIELD,		4, 1,		PARAM_OPTION_REG,		STR_PARAM_OPT_REG}, 	
/* 6  */ { 0, 0, PT_NV_RW,	NO_UNIT, 		2, 1,		PARAM_ROM_UPD_ACK,		STR_PARAM_ROMSUM_ACK}, 	
/* 7  */ { 0, 0, PT_RO,	 	NO_UNIT, 		2, 1,		PARAM_ROM_SUM,			STR_PARAM_FWSUM}, 	
/* 8  */ { 0, 0, PT_RO,	 	TIME_USEC, 		4, 1000,	PARAM_SAMP_RATE,		STR_PARAM_SAMP_PER}, 	
/* 9  */ { 0, 0, PT_RO_RT,	BIT_FIELD,	   12, 1,		PARAM_ALERT_REG,		STR_PARAM_ALERT_REG}, 	
/* 10 */ { 0, 0, PT_NV_RW, 	NO_UNIT,		1, 1,		PARAM_STOP_TYPE,		STR_PARAM_STOPTYPE}, 	
/* 11 */ { 0, 0, PT_VOL, 	TIME_SAMPLE,	2, 1,		PARAM_WATCHDOG_TIME,	STR_PARAM_WDTC,		    convertTimeMS}, 	
/* 12 */ { 0, 0, PT_RO, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_PARAM_NETSTAT}, 	
/* 13 */ { 0, 0, PT_ROC_RT, BIT_FIELD,		6, 1,		PARAM_NULL,				STR_PARAM_STATUS}, 	
/* 14 */ { 0, 0, PT_ROC_RT, BIT_FIELD,		6, 1,		PARAM_NULL,				STR_PARAM_STATUS_ATTN_RISE}, 	
/* 15 */ { 0, 0, PT_RAM, 	BIT_FIELD,ATTN_MASK_OCTETS, 1,PARAM_NULL,			STR_PARAM_ATTN_MASK}, 	
/* 16 */ { 0, 0, PT_RO_RT, 	BIT_FIELD,		6, 1,		PARAM_NULL,				STR_PARAM_STATUS_RT}, 	
/* 17 */ { 0, 0, PT_RO_RT, 	TIME_SAMPLE,	1, 1,		PARAM_NULL,				STR_PARAM_TIMESTAMP,	convertTimeMS}, 
/* 18 */ { 0, 0, PT_RO_RT, 	TIME_SAMPLE,	2, 1,		PARAM_NULL,				STR_PARAM_TIMESTAMP16,	convertTimeMS}, 
/* 19 */ { 0, 0, PT_RO, 	NO_UNIT,MN_PART_NUM_SIZE, 1,PARAM_PART_NUM,			STR_PARAM_PART_NUM},
/* 20 */ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_EE_UPD_ACK,		STR_PARAM_EE_ACK},
/* 21 */ { 0, 0, PT_RO,		NO_UNIT,		2, 1,		PARAM_EE_VER,			STR_PARAM_EE_VER}, 	
/* 22 */ { 0, 0, PT_ROC_RT, BIT_FIELD,		6, 1,		PARAM_NULL,				STR_PARAM_STATUS_FALL}, 	
/* 23 */ { 0, 0, PT_NV_RW, 	BIT_FIELD,		4, 1,		PARAM_CFG_HW,			STR_PARAM_CFG_HW}, 	
/* 24 */ { 0, 0, PT_NV_RW, 	BIT_FIELD,		4, 1,		PARAM_CFG_APP,			STR_PARAM_CFG_FEAT}, 	
/* 25 */ { 0, 0, PT_NONE,	NO_UNIT,		2, 1,		PARAM_NULL,				STR_UNKNOWN},	
/* 26 */ { 0, 0, PT_NONE,	NO_UNIT,		2, 1,		PARAM_NULL,				STR_UNKNOWN},	
/* 27 */ { 0, 0, PT_NONE,	NO_UNIT,		2, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 28 */ { 0, 0, PT_NONE,	NO_UNIT,		2, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 29 */ { 0, 0, PT_NONE,	NO_UNIT,		2, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 30 */ { 0, 0, PT_VOL, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_USER_IN_REG}, 	
/* 31 */ { 0, 0, PT_VOL, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_IN_SRC_REG}, 	
/* 32 */ { 0, 0, PT_RO_RT, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_OUT_REG}, 	
/* 33 */ { 0, 0, PT_ROC_RT, BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_OUT_RISE_REG}, 	
/* 34 */ { 0, 0, PT_ROC_RT, BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_OUT_FALL_REG}, 	
/* 35 */ { 0, 0, PT_NV_RW,  BIT_FIELD,		2, 1,		PARAM_NULL,				STR_CTL_STOP_OUT_REG}, 	
/* 36 */ { 0, 0, PT_VOL, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_USER_OUT_REG}, 	
/* 37 */ { 0, 0, PT_NV_RW, 	BIT_FIELD,		2, 1,		PARAM_OUT_SRC,			STR_PARAM_OUT_SRC_REG}, 	
/* 38 */ { 0, 0, PT_RW_RT, 	TIME_SAMPLE,	2, 1,		PARAM_NULL,				STR_PARAM_GP_TIMER,		convertTimeMS}, 	
/* 39 */ { 0, 0, PT_RO_RT, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_PLA_OUT_REG}, 	
/* 40 */ { 0, 0, PT_NV_RW, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_PARAM_IN_POLARITY}, 	
/* 41 */ { 0, 1, PT_NONE, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 42 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_INDEX_POSN_CAP}, 	
/* 43 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_GPI0_POSN_CAP}, 	
/* 44 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_PLA_POSN_CAP}, 	
/* 45 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_P_POSN1_CAP_INDEX}, 	
/* 46 */ { 0, 1, PT_RO_RT, 	VEL_TICK_SAMPLE,4, 1<<18,	PARAM_NULL,				STR_UNKNOWN,			convertVel},
/* 47 */ { 0, 2, PT_NV_RW, 	VEL_TICK_SAMPLE,4, 1<<17,	PARAM_VEL_LIM,			STR_PARAM_VEL_LIMIT,	convertVel}, 	
/* 48 */ { 0, 2, PT_NV_RW, 	VEL_TICK_SAMPLE2,4,1<<17,	PARAM_ACC_LIM,			STR_PARAM_ACC_LIMIT,	convertAcc},
/* 49 */ { 0, 1, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_RASCON,			STR_PARAM_RAS_CON_REG},
/* 50 */ { 0, 2, PT_RO_RT, 	VEL_TICK_SAMPLE2,4,1<<18,	PARAM_NULL,				STR_PARAM_ACC_MAX,		convertAcc},
/* 51 */ { 0, 1, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_POSN_TRIG_PT},
/* 52 */ { 0, 0, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_A_START},
/* 53 */ { 0, 0, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_B_END},
/* 54 */ { 0, 1, PT_NV_RW, 	DX_TICK,		2, 1,		PARAM_IN_RANGE_WIN,		STR_PARAM_POS_TRK_RNG},
/* 55 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_CPL_ERR}, 	
/* 56 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_PARAM_POSN_TRK},
/* 57 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 58 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 59 */ { 0, 2, PT_NV_RW, 	VEL_TICK_SAMPLE2,4,1<<17,	PARAM_NULL,				STR_PARAM_STOPACC_LIM,	convertAcc},
/* 60 */ { 0, 1, PT_RO_RT, 	DX_TICK,	    3, 1,		PARAM_NULL,				STR_PARAM_I_MEAS_POSN_MTR},
/* 61 */ { 0, 1, PT_RO_RT, 	DX_TICK,		3, 1,		PARAM_NULL,				STR_PARAM_I_MEAS_POSN},
/* 62 */ { 0, 1, PT_RO_RT, 	DX_TICK,		3, 1,		PARAM_NULL,				STR_PARAM_I_CMD_POSN},
/* 63 */ { 0, 1, PT_RO_RT, 	VEL_TICK_SAMPLE,4, 1<<18,	PARAM_NULL,				STR_PARAM_CMD_VEL,		convertVel},
/* 64 */ { 0, 1, PT_RO_RT, 	VEL_TICK_SAMPLE,4, 1<<18,	PARAM_NULL,				STR_PARAM_MEAS_VEL,		convertVel},
/* 65 */ { 0, 0, PT_NV_RW, 	VEL_TICK_SAMPLE2,4,1<<17,	PARAM_DEC_LIM,			STR_PARAM_DEC_LIMIT,	convertAcc},
/* 66 */ { 0, 0, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_HEAD_DX,			STR_PARAM_HEAD_DX},
/* 67 */ { 0, 0, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_TAIL_DX,			STR_PARAM_TAIL_DX},
/* 68 */ { 0, 0, PT_NV_RW, 	VEL_TICK_SAMPLE,4, 1<<17,	PARAM_HT_VEL_LIM,		STR_PARAM_HEADTAIL_VEL,	convertVel},	
/* 69 */ { 0, 1, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_P_POSN1_MEAS32}, 	
/* 70 */ { 0, 0, PT_ROC_RT, NO_UNIT,ALERTREG_SIZEOCTETS, 1,		PARAM_NULL,		STR_WARN_ACC}, 	
/* 71 */ { 0, 0, PT_RAM, 	NO_UNIT,ALERTREG_SIZEOCTETS, 1,		PARAM_NULL,		STR_USER_WARN_MASK_REG}, 	
/* 72 */ { 0, 0, PT_RAM, 	NO_UNIT,ALERTREG_SIZEOCTETS, 1,		PARAM_NULL,		STR_USER_ALERT_MASK_REG}, 	
/* 73 */ { 0, 0, PT_ROC_RT, NO_UNIT,ALERTREG_SIZEOCTETS, 1,		PARAM_NULL,		STR_WARN_RT}, 	
/* 74 */ { 0, 0, PT_RW_RT, 	TIME_SAMPLE,	2, 1,		PARAM_MOVE_DWELL,		STR_PARAM_DWELL,		convertTimeMS}, 	
/* 75 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 76 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 77 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 78 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 79 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 80 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 81 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 82 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 83 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 84 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 85 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 86 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 87 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 88 */ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 89 */ { 0, 0, PT_RT, 	NO_UNIT,		4, 10.*60.*60.,	PARAM_NULL,			STR_ON_TIME}, 	// .1 Sec -> Hours
/* 90 */ { 0, 1, PT_VOL, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_PARAM_USER_RAM0},
/* 91 */ { 0, 0, PT_NV_RW, 	NO_UNIT,MN_USER_NV_SIZE,1,	PARAM_NULL,				STR_PARAM_USER_EE0}, 	
/* 92 */ { 0, 0, PT_NV_RW, 	NO_UNIT,MN_USER_NV_SIZE,1,	PARAM_NULL,				STR_PARAM_USER_EE1}, 	
/* 93 */ { 0, 0, PT_NV_RW, 	NO_UNIT,MN_USER_NV_SIZE,1,	PARAM_NULL,				STR_PARAM_USER_EE2}, 	
/* 94 */ { 0, 0, PT_NV_RW, 	NO_UNIT,MN_USER_NV_SIZE,1,	PARAM_NULL,				STR_PARAM_USER_EE3},
/* 95 */ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 96 */ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 97 */ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 98 */ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 100*/ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 101*/ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 102*/ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 103*/ { 0, 0, PT_RT, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 104*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_APP_CHKSUM}, 	
/* 105*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_APP_FRAG}, 	
/* 106*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_APP_STRAY}, 	
/* 107*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_APP_OVERRUN}, 	
/* 108*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_DIAG_CHKSUM}, 	
/* 109*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_DIAG_FRAG}, 	
/* 110*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_DIAG_STRAY}, 	
/* 111*/ { 0, 0, PT_RT, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_NETERR_DIAG_OVERRUN}
};
#if defined (__GNUC__)
#pragma GCC diagnostic warning "-Wmissing-braces"
#pragma GCC diagnostic warning "-Wmissing-field-initializers"
#endif
#define PARAM_MAX (sizeof(iscInfoDB)/sizeof(paramInfoLcl))
// Node class database
static byNodeClassDB iscClassDB = {
	NULL 					// Parameter change function
};

#if defined (__GNUC__)
	#pragma GCC diagnostic ignored "-Wmissing-braces"
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static const paramInfoLcl iscDrvInfoDB[] = {
// 		 1/x, signed, type,  unit,    size, scale,		config key index,		description, 			[converter]
/* 256*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 257*/ { 0, 1, PT_RO, 	NO_UNIT,		2, 1<<9,	PARAM_I_MAX,			STR_DRV_DRV_I_MAX}, 
/* 258*/ { 0, 2, PT_RO, 	NO_UNIT,		2, 1,		PARAM_RMS_MAX,			STR_DRV_RMS_MAX,		convertRMSlimit}, 
/* 259*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1<<14,	PARAM_IR_CAL_FACTOR,	STR_DRV_IR_CAL},
/* 260*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1<<14,	PARAM_IS_CAL_FACTOR,	STR_DRV_IS_CAL},
/* 261*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},	
/* 262*/ { 0, 1, PT_RO, 	NO_UNIT,		2, 1<<14,	PARAM_ADC_MAX,			STR_DRV_ADC_MAX,		convertADCmax}, 	
/* 263*/ { 0, 0, PT_RW_RT, 	NO_UNIT,		2, 1,		PARAM_NV_MODIFIED,		STR_DRV_DIRTY}, 
/* 264*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1,		PARAM_VECT_RATE,		STR_DRV_VECTOR_RATE}, 
/* 265*/ { 0, 2, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_STEP_RES,			STR_DRV_STEP_DENS}, 	
/* 266*/ { 0, 2, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_ENC_DENS,			STR_DRV_ENC_DENS}, 	
/* 267*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 100,		PARAM_ENC_RESOL,		STR_DRV_ENC_RES}, 	
/* 268*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_POLES,			STR_DRV_POLES}, 	
/* 269*/ { 0, 2, PT_RW_RT, 	NO_UNIT,		4, 1<<7,	PARAM_MTR_KE,			STR_DRV_MTR_KE,			convertKe}, 	
/* 270*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<9,	PARAM_MTR_OHMS,			STR_DRV_MTR_OHMS}, 	
/* 271*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<9,	PARAM_MTR_ELECT_TC,		STR_DRV_MTR_ELEC_TC}, 	
/* 272*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 32768/360,PARAM_RO,				STR_DRV_RO}, 	
/* 273*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_RMS_LIM,			STR_DRV_RMS_LIM,		convertRMSlimit32}, 	
/* 274*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_RMS_TC,			STR_DRV_RMS_TC,			convertRMStc}, 	
/* 275*/ { 0, 2, PT_NV_RW, 	VEL_TICK_SAMPLE,2, 1<<2,	PARAM_SPEED_LIM,		STR_DRV_SPEED_LIM,		convertVelRound}, 	
/* 276*/ { 0, 0, PT_RO_RT, 	DX_TICK,		4, 1,		PARAM_NULL,				STR_DRV_COM_CAP,		convertAngle}, 	
/* 277*/ { 0, 0, PT_RW_RT, 	DX_TICK,		4, 1,		PARAM_MECH_ANGLE,		STR_DRV_ANGLE,			convertAngle}, 	
/* 278*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_KIP,				STR_DRV_KIP}, 
/* 279*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_KII,				STR_DRV_KII}, 
/* 280*/ { 0, 2, PT_NV_RW, 	VEL_TICK_SAMPLE,2, 1<<2,	PARAM_LD_SPEED_LIM,		STR_DRV_LD_SPEED_LIM,	convertVelRound}, 	
/* 281*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KPL,				STR_DRV_KPL}, 	
/* 282*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KV,				STR_DRV_KV}, 
/* 283*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KP,				STR_DRV_KP}, 
/* 284*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KI,				STR_DRV_KI}, 
/* 285*/ { 0, 1, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KFV,				STR_DRV_KFV}, 
/* 286*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KFA,				STR_DRV_KFA,			limit2To27}, 
/* 287*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KFJ,				STR_DRV_KFJ,			limit2To27}, 
/* 288*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 327.68,	PARAM_KFF,				STR_DRV_KFF}, 
/* 289*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_KNV,				STR_DRV_KNV}, 
/* 290*/ { 0, 1, PT_NONE, 	NO_UNIT,		2, 1,		PARAM_AHFUZZ2,			STR_DRV_ANTIHUNT2}, 
/* 291*/ { 0, 1, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_TRQ_BIAS,			STR_DRV_TRQ_BIAS,		convertAmperes}, 
/* 292*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_FUZZ_AP,			STR_DRV_FUZZ_AP}, 
/* 293*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_ANTI_HUNT_HYSTERESIS,STR_DRV_FUZZ_HYST}, 
/* 294*/ { 1, 0, PT_RO_RT, 	NO_UNIT,		2, 30,		PARAM_NULL,				STR_PARAM_FAN_SPEED}, 	
/* 295*/ { 1, 0, PT_RO, 	NO_UNIT,		2, 30,		PARAM_FAN_FAULT_SPD,	STR_PARAM_FAN_SPEED_MIN}, 	
/* 296*/ { 0, 0, PT_RO_RT, 	DX_TICK,		2, 1,		PARAM_NULL,				STR_DRV_TRK_MAX}, 	
/* 297*/ { 0, 0, PT_RO_RT, 	DX_TICK,		0, 1,		PARAM_NULL,				STR_DRV_TRK_MIN}, 	
/* 298*/ { 0, 0, PT_NV_RW, 	TIME_SAMPLE,	2, 1,		PARAM_AH_HOLDOFF,		STR_DRV_AH_HOLDOFF,		convertTimeMS}, 	
/* 299*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		8, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 300*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN},
/* 301*/ { 0, 0, PT_NV_RW, 	BIT_FIELD,		4, 1,		PARAM_CFG_TUNE,			STR_PARAM_CFG_TUNE}, 	
/* 302*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_HLESS_RAMPUP_TIME,STR_DRV_HLESS_RAMPUP_TIME}, 	
/* 303*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_HLESS_SWEEP_TIME,	STR_DRV_HLESS_SWEEP_TIME}, 	
/* 304*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_HLESS_SETTLE_TIME,STR_DRV_HLESS_SETTLE_TIME}, 	
/* 305*/ { 0, 2, PT_NV_RW,  NO_UNIT,		2, 1<<15,	PARAM_NO_COMM_START_TRQ,STR_DRV_HLESS_TRQ,		convertAmpsRMS},	
/* 306*/ { 0, 2, PT_NV_RW,  NO_UNIT,		2, 1<<2,	PARAM_HLESS_ENTRY_MOTION,STR_DRV_HS_QUAL_SPEED,	convertVelRound}, 	
/* 307*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_TRQ_LIM,			STR_DRV_TRQ_LIM,		convertAmperes}, 
/* 308*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_TRQ_FLDBACK_POS,	STR_DRV_TRQ_FLDBACK_POS,convertAmperes}, 
/* 309*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_TRQ_FLDBACK_POS_TC,STR_DRV_TRQ_FB_POS_TC, convertFilt99pctMilliseconds}, 
/* 310*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_TRQ_FLDBACK_NEG,	STR_DRV_TRQ_FLDBACK_NEG,convertAmperes}, 
/* 311*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_TRQ_FLDBACK_NEG_TC,STR_DRV_TRQ_FB_NEG_TC, convertFilt99pctMilliseconds}, 
/* 312*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_TRQ_FLDBACK_HS,	STR_DRV_HS_FLDBACK, 	convertAmperes}, 
/* 313*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_TRQ_FLDBACK_HS_TC,STR_DRV_HS_FB_TC,   	convertFilt99pctMilliseconds}, 
/* 314*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_TRQ_FLDBACK_MVDN,	STR_DRV_MVDN_FB,		convertAmperes}, 
/* 315*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_TRQ_FLDBACK_MVDN_TC,STR_DRV_MVDN_FB_TC,	convertFilt99pctMilliseconds}, 
/* 316*/ { 0, 2, PT_NV_RW_RT,NO_UNIT,		2, (1U<<15)/100.,PARAM_HS_THRESHOLD,STR_DRV_HS_TRQ_TRIP}, 
/* 317*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_HS_TC,			STR_DRV_HS_TC,			convertTimeMS},
/* 318*/ { 0, 1, PT_RO_RT, 	NO_UNIT,		2, 1<<15,	PARAM_NULL,				STR_DRV_TRQ_CMD,		convertAmperes},
/* 319*/ { 0, 1, PT_RO_RT, 	NO_UNIT,		2, 1<<13, 	PARAM_NULL,				STR_DRV_TRQ_MEAS,		convertMeasAmperes},
/* 320*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_DRV_RMS_LEVEL,		convertRMSlevel}, 
/* 321*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_TRK_ERR_LIM,		STR_DRV_TRK_ERR_LIM}, 
/* 322*/ { 0, 2, PT_NV_RW, 	TIME_SAMPLE,	2, 1,		PARAM_MV_DN_TC,			STR_DRV_MV_DN_TC,		convertTimeMS}, 
/* 323*/ { 0, 2, PT_NV_RW,  NO_UNIT,		2, 1<<15,	PARAM_STEP_RUN_TRQ,		STR_DRV_STEPPER_TRQ,	convertAmpsRMS},
/* 324*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_STEP_IDLE_TRQ,	STR_DRV_STEP_IDLE_TRQ,	convertAmpsRMS}, 
/* 325*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<15,	PARAM_STEP_ACC_TRQ,		STR_DRV_STEP_ACC_TRQ,	convertAmpsRMS}, 
/* 326*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_TRQ_FLDBACK_STEP_TC,STR_DRV_STEP_FB_TC,   convertFilt99pctMilliseconds}, 
/* 327*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1<<5,	PARAM_BUS_V,			STR_DRV_BUS_VOLTS}, 
/* 328*/ { 0, 1, PT_RO_RT, 	BIT_FIELD,		2, 1,		PARAM_NULL,				STR_DRV_TP_IOP}, 
/* 329*/ { 0, 1, PT_RO_RT, 	NO_UNIT,		2, 1U<<15,	PARAM_NULL,				STR_DRV_TP_IR,			convertMeasAmperes}, 
/* 330*/ { 0, 1, PT_RO_RT, 	NO_UNIT,		2, 1U<<15,	PARAM_NULL,				STR_DRV_TP_IS,			convertMeasAmperes}, 
/* 331*/ { 0, 1, PT_RO_RT,	NO_UNIT,		2, 1U<<15,	PARAM_NULL,				STR_DRV_TP_IR_FILT,		convertMeasAmperes}, 	
/* 332*/ { 0, 1, PT_RO_RT, 	NO_UNIT,		2, 1U<<15,	PARAM_NULL,				STR_DRV_TP_IS_FILT,		convertMeasAmperes}, 	
/* 333*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		2, 65520./300.,	PARAM_NULL,			STR_DRV_HEATSINK}, 
/* 334*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		2, 65520./300.,	PARAM_NULL,			STR_P_PWBA_TEMP}, 
/* 335*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		2, 32768./6.,	PARAM_NULL,			STR_PARAM_TP_5V},
/* 336*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		2, 32768./18,	PARAM_NULL,			STR_PARAM_TP_12V},
/* 337*/ { 0, 0, PT_VOL, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_P_IR_IS_TP_FILT_TC,	convertFilt1TCMilliseconds}, 
/* 338*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		2, 65520./300.,	PARAM_NULL,			STR_P_TEMP_SIM}, 
/* 339*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1<<4,	PARAM_NULL,				STR_P_DRV_PWR_LIM_EXP}, 	
/* 340*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1<<9,	PARAM_NULL,				STR_P_DRV_RES_MIN}, 	
/* 341*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 65520./300,	PARAM_NULL,			STR_P_FAN_ON_TEMP}, 	
/* 342*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_P_DSP_INFO}, 	
/* 343*/ { 0, 2, PT_RO, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_P_IB_FILT_TC,		convertIBRMStc}, 	
/* 344*/ { 0, 1, PT_RO, 	NO_UNIT,		2, 1U<<15,	PARAM_NULL,				STR_P_IB_TRIP,			convertAmpsRMS}, 	
/* 345*/ { 0, 0, PT_RO, 	NO_UNIT,		2, 1U<<15,	PARAM_NULL,				STR_P_PHASE_TRIP,		convertMeasAmperes}, 	
/* 346*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1U<<31,	PARAM_NULL,				STR_P_IB_RMS,			convertAmpsRMS}, 	
/* 347*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1000,	PARAM_NULL,				STR_P_TSPD}, 	
/* 348*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 349*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, (1<<17)/360., PARAM_NULL,		STR_DRV_COMM_CHK_ANGLE_LIM}, 	
/* 350*/ { 0, 2, PT_NV_RW, 	NO_UNIT,		2, 1,		PARAM_COUPLING_ERR_LIM,	STR_DRV_CPL_ERR_LIM}, 	
/* 351*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 352*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 353*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 354*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 355*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 356*/ { 0, 0, PT_NONE, 	NO_UNIT,		0, 1,		PARAM_NULL,				STR_UNKNOWN}, 	
/* 357*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		6, 1,		PARAM_NULL,				STR_P_IEX_STATE_REG}, 	
/* 358*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_IEX_USER_OUT_REG,	STR_P_IEX_USER_OUT}, 	
/* 359*/ { 0, 0, PT_NV_RW, 	NO_UNIT,		4, 1,		PARAM_IEX_STOP_OUT_REG,	STR_P_IEX_STOP_OUT}, 	
/* 360*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_OUT_REG}, 	
/* 361*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_IN_REG}, 	
/* 362*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_IN_RISE}, 	
/* 363*/ { 0, 0, PT_RO_RT, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_IN_FALL}, 	
/* 364*/ { 0, 0, PT_RAM, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_IN_ATTN_MASK}, 	
/* 365*/ { 0, 0, PT_RAM, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_RISE_ATTN_MASK}, 	
/* 366*/ { 0, 0, PT_RAM, 	NO_UNIT,		4, 1,		PARAM_NULL,				STR_P_IEX_FALL_ATTN_MASK}, 	
/* 367*/ { 0, 0, PT_RAM, 	NO_UNIT,		2, 1,		PARAM_NULL,				STR_P_IEX_GLITCH_TOLERANCE}, 	
/* 368*/ { 0, 0, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_SOFT_LIM_POSN_POS,STR_PARAM_SOFT_LIM_POS},
/* 369*/ { 0, 0, PT_NV_RW, 	DX_TICK,		4, 1,		PARAM_SOFT_LIM_POSN_NEG,STR_PARAM_SOFT_LIM_NEG}
};
#if defined (__GNUC__)
#pragma GCC diagnostic warning "-Wmissing-braces"
#pragma GCC diagnostic warning "-Wmissing-field-initializers"
#endif

#define DRV_PARAM_MAX (sizeof(iscDrvInfoDB)/sizeof(paramInfoLcl))

// Default settings
#define DFLT_STOP_ACC_LIM	100000.
#define DFLT_VEL_LIM		175000.
#define DFLT_ACC_LIM		1000000.
#define DFLT_IMAX			33.						// Current scale maximum
#define DFLT_HS_TRIP		(DFLT_IMAX*0.80)		// Hardstop Trip Point (A)
#define DFLT_FLD_BACK		(DFLT_IMAX*0.05)		// Foldback levels (A)
#define DFLT_SENSORLESS_TRQ 1.						// Sensorless startup trq (A)


#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static const factoryInitData initData[] =  { 
	// Trajectory Generator Parameters  VAL,		"Not J"
	{ NVR,	ISC_P_HW_CONFIG_REG, 		0x220 }, // comm sensors = state + thermal, encoder checking
	{ NVR,	ISC_P_APP_CONFIG_REG, 		0 },
	{ NVR,	ISC_P_DRV_TUNE_CONFIG_REG, 	0x220 }, // MtrVelocityEst=LdVelocityEst=2
	{ NVR,	ISC_P_STOP_ACC_LIM, 		DFLT_STOP_ACC_LIM },
	{ NVR,	ISC_P_ACC_LIM, 				DFLT_ACC_LIM },
	{ NVR,	ISC_P_DEC_LIM, 				DFLT_ACC_LIM },
	{ NVR,	ISC_P_VEL_LIM, 				DFLT_VEL_LIM },
	{ NVR,	ISC_P_HEAD_DISTANCE,		1000 },
	{ NVR,	ISC_P_TAIL_DISTANCE,		1000 },
	{ NVR,	ISC_P_HEADTAIL_VEL,			100000 },
	{ NVR,	ISC_P_JERK_LIM, 			3 }, //3 ms in 5.1.1 FW; 10 ms in 5.2.0+ FW
	{ NVR,	ISC_P_JERK_LIM, 			0x00000090, FW_MILESTONE_IEX }, //Attack=1, gStop=2, normal in 5.2.2+ FW
	{ NVR,	ISC_P_STOP_TYPE, 			STOP_TYPE_ABRUPT },
	{ NVR,	ISC_P_CTL_STOP_OUT_REG, 	0 },
	{ NVR,	ISC_P_USER_OUT_REG, 		0 },
	{ NV,	ISC_P_IN_POLARITY_REG, 		0 },
	{ NVR,	ISC_P_POSN_TRK_RANGE,		10 },
	// Drive Parameters
	{ NVR,	ISC_P_DRV_RMS_LIM,			1 },		// A
	{ NVR,	ISC_P_DRV_RMS_TC,			1 },		// sec
	{ NVR,	ISC_P_DRV_KIP,				0 },
	{ NVR,	ISC_P_DRV_KII,				0 },
	{ NVR,	ISC_P_DRV_KV,				0 },
	{ NVR,	ISC_P_DRV_KP,      			0 },
	{ NVR,	ISC_P_DRV_KI,      			0 },
	{ NVR,	ISC_P_DRV_KFV,     			0 },
	{ NVR,	ISC_P_DRV_KFA,     			0 },
	{ NVR,	ISC_P_DRV_KFJ,     			0 },
	{ NVR,	ISC_P_DRV_KFF,     			0 },
	{ NVR,	ISC_P_DRV_KNV,     			0 },
	{ NVR,	ISC_P_DRV_KPL,				0 },
	{ NVR,	ISC_P_DRV_AHFUZZ2,     		-1 },
	{ NVR,	ISC_P_DRV_TRQ_BIAS,			0 },
	{ NVR,	ISC_P_DRV_FUZZY_APERTURE, 	0 },
	{ NVR,	ISC_P_DRV_FUZZY_HYST, 		3 },
	{ NVR,	ISC_P_AH_HOLDOFF,			50 },
	{ NVR,	ISC_P_DRV_TRK_ERR_LIM, 		1000 },
	{ NVR,	ISC_P_DRV_CPL_ERR_LIM,		1000,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_DRV_MV_DN_TC,    		10 },			// ms
	{ NVR,	ISC_P_DRV_POS_FLDBK_TRQ, 	DFLT_FLD_BACK },
	{ NVR,	ISC_P_DRV_POS_FLDBK_TRQ_TC, 200 },
	{ NVR,	ISC_P_DRV_NEG_FLDBK_TRQ, 	DFLT_FLD_BACK },
	{ NVR,	ISC_P_DRV_NEG_FLDBK_TRQ_TC, 200 },
	{ NVR,	ISC_P_DRV_MOVEDONE_FLDBK_TRQ, DFLT_FLD_BACK },
	{ NVR,	ISC_P_DRV_MOVEDONE_FLDBK_TRQ_TC, 200 },
	{ NVR,	ISC_P_DRV_HARDSTOP_FLDBK_TRQ, DFLT_FLD_BACK },
	{ NVR,	ISC_P_DRV_HARDSTOP_FLDBK_TRQ_TC, 200 },
	{ NVR,	ISC_P_DRV_HARDSTOP_ENTRY_TRQ, 80 },
	{ NVR,	ISC_P_DRV_HARDSTOP_ENTRY_TC, 50 },	// ms
	{ NVR,	ISC_P_DRV_TRQ_LIM,			DFLT_IMAX },			// A
	{ NVR,	ISC_P_DRV_SENSORLESS_TRQ,	DFLT_SENSORLESS_TRQ },	// A
	{ NVR,	ISC_P_DRV_HARDSTOP_ENTRY_SPD, 7598.7 },	// encs/sec
	{ NVRV,	ISC_P_DRV_BUS_VOLTS,		75 },
	{ NVR,	ISC_P_DRV_TRQ_MEAS_FILT_TC,	50  },		// ms
	// Motor items last, 'cause they fail more often
	{ NVR,	ISC_P_DRV_MTR_SPEED_LIM,	200000 },	// encs/sec
	{ NVR,	ISC_P_DRV_LD_SPEED_LIM,		200000 },	// encs/sec
	{ NV,	ISC_P_DRV_ENC_RES,			1.0 },
	{ NV,	ISC_P_USER_DATA_NV0,		0 },
	{ NV,	ISC_P_USER_DATA_NV1,		0 },
	{ NV,	ISC_P_USER_DATA_NV2,		0 },
	{ NV,	ISC_P_USER_DATA_NV3,		0 },
	{ NVRV,	ISC_P_DRV_MTR_POLES,		8 },
	{ NVRV,	ISC_P_DRV_ENC_DENS,			4000 },
	{ NVRV,	ISC_P_DRV_MTR_KE,			0 },
	{ NVRV,	ISC_P_DRV_MTR_OHMS,			0 },
	{ NVRV,	ISC_P_DRV_MTR_ELEC_TC,		0 },
	{ NVRV,	ISC_P_DRV_COM_RO,			0 },
	// New Meridian Params
	{ NVRV,	ISC_P_USER_IN_REG,			0 },
	{ NVRV,	ISC_P_IN_SRC_REG,			0 },
	{ NVR,	ISC_P_OUT_PLA_SRC_REG,		0 },
	{ NVR,	ISC_P_POSN_TRIG_PT,			0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_A_START,				0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_B_END,				0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_GP_TIMER_PERIOD,		0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_DRV_STEPPER_TRQ,		0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_DRV_STEP_IDLE_TRQ,	0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_DRV_STEP_ACC_TRQ,		0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_DRV_STEP_FLDBK_TRQ_TC, 0,	FW_MILESTONE_IEX },
	{ NVRV,	ISC_P_DRV_USTEPS_PER_REV,	32000, FW_MILESTONE_IEX },
	// IEX init only power-on default to avoid errors on nodes
	// with not IEX modules attached or running.
	{ NV,	ISC_P_IEX_USER_OUT_REG,		0,	FW_MILESTONE_IEX },
	{ NV,	ISC_P_IEX_STOP_OUT_REG,		0,	FW_MILESTONE_IEX },
	{ RAM,	ISC_P_STATUS_ATTN_MASK,		0 },
	{ RAM,	ISC_P_WARN_MASK,			0 },
	{ RAM,	ISC_P_ALERT_MASK,			0 },
	{ RAM,	ISC_P_IEX_IN_REG_MASK,		0,	FW_MILESTONE_IEX },
	{ RAM,	ISC_P_IEX_IN_RISE_REG_MASK,	0,	FW_MILESTONE_IEX },
	{ RAM,	ISC_P_IEX_IN_FALL_REG_MASK,	0,	FW_MILESTONE_IEX },
	{ NVR,	ISC_P_DRV_COMM_CHK_ANGLE_LIM,20,FW_MILESTONE_VECTOR_STEEPER },
	{ NVR,	ISC_P_DRV_SENSORLESS_RAMPUP_TIME,2000,FW_MILESTONE_VECTOR_STEEPER },
	{ NVR,	ISC_P_DRV_SENSORLESS_SWEEP_TIME,1000,FW_MILESTONE_VECTOR_STEEPER },
	{ NVR,	ISC_P_DRV_SENSORLESS_SETTLE_TIME,500,FW_MILESTONE_VECTOR_STEEPER },
	{ NVR,	ISC_P_DRV_SOFT_LIM_POSN_POS,0,	FW_MILESTONE_SOFT_LIMITS },
	{ NVR,	ISC_P_DRV_SOFT_LIM_POSN_NEG,0,	FW_MILESTONE_SOFT_LIMITS },
};
#if defined (__GNUC__)
#pragma GCC diagnostic warning "-Wmissing-braces"
#pragma GCC diagnostic warning "-Wmissing-field-initializers"
#endif
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		ISC_DUMP_PKT
//
//	DESCRIPTION:							
//		Write buffer as hex debug string.
//
//	SYNOPSIS:
void ISC_DUMP_PKT(multiaddr theMultiAddr, char *msg, packetbuf *buf) {
#ifdef _DEBUG
	size_t i;
	_RPT2(_CRT_WARN, "%s: %.1f -> ", msg, infcCoreTime());
	//buf->u.fld.startOfPacket = 1;
	//buf->u.fld.zero1 = buf->u.fld.mode = 0;
	//buf->u.fld.pktType = MN_PKT_TYPE_CMD;
	for (i=0; i<buf->Fld.PktLen+MN_API_PACKET_HDR_LEN; i++) {
		_RPT1(_CRT_WARN, "%02x ", 0xFF & buf->Byte.Buffer[i]);
	}
	_RPT0(_CRT_WARN, "\n");
#endif
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscClassDelete
//
//	DESCRIPTION:
//		Delete any memory this node allocated.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
void iscClassDelete(multiaddr theMultiAddr) {
	register byNodeDB *pNodeDB;
	netaddr cNum = NET_NUM(theMultiAddr);
	pNodeDB = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
	// Delete if no one else has
	if (pNodeDB->paramBankList) {
		//_RPT1(_CRT_WARN, "iscClassDelete %d\n", theMultiAddr);
		pNodeDB->bankCount = 0;
		pNodeDB->theID.devCode = NODEID_UNK;
		free((void *)(pNodeDB->paramBankList[0]).valueDB);
		(pNodeDB->paramBankList[0]).valueDB = NULL;
		free((void *)(pNodeDB->paramBankList[1]).valueDB);
		(pNodeDB->paramBankList[1]).valueDB = NULL;
		free((void *)pNodeDB->paramBankList);
		pNodeDB->paramBankList = NULL;
		if (pNodeDB->pNodeSpecific) {
			delete (iscState *)pNodeDB->pNodeSpecific;
			pNodeDB->pNodeSpecific = NULL;
		}
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscClassSetup
//
//	DESCRIPTION:
//		Setup the parameter manager and node database for this node.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
cnErrCode iscClassSetup(
	multiaddr theMultiAddr)
{
	cnErrCode errRet;
	packetbuf resp, fwVers, dummy;
	register byNodeDB *pNodeDB;
	netaddr cNum = NET_NUM(theMultiAddr);
	//_RPT2(_CRT_WARN, "iscClassSetup net=%d, node=%d\n", cNum, theMultiAddr);
	
	// Stop using old memory
	iscClassDelete(theMultiAddr);
	// Make sure this is a Integrated Servo Controller
	errRet = netGetParameter(theMultiAddr, MN_P_NODEID, &resp);
	if (errRet == MN_OK)  {
		errRet = netGetParameter(theMultiAddr, ISC_P_FW_VERSION, &fwVers);
		if (errRet == MN_OK)  {
			// Shorthand to access the device ID field
			#define DEVID ((devID_t *)(&resp.Byte.Buffer[0]))
			#define FWVER ((versID_t *)(&fwVers.Byte.Buffer[0]))
			// Fill in the database from the node if this is a TG
			if (DEVID->fld.devType	== NODEID_MD)	 {		
				// Initialize the register/shortcut
				pNodeDB = &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)];
				// Wire in our destructor
				pNodeDB->delFunc = iscClassDelete;
				// Initialize the per node database
				pNodeDB->bankCount = 2;
				// Update the ID area
				pNodeDB->theID.fld.devType = DEVID->fld.devType;
				pNodeDB->theID.fld.devModel = DEVID->fld.devModel;
				// Create and initialize the parameter banks
				pNodeDB->paramBankList = (paramBank *)calloc(2, sizeof(paramBank));
				// Bank 0 information
				(pNodeDB->paramBankList[0]).nParams = PARAM_MAX;
				(pNodeDB->paramBankList[0]).fixedInfoDB = &iscInfoDB[0];
				// Create the value storage area together
				(pNodeDB->paramBankList[0]).valueDB 
					= (paramValue *)calloc(PARAM_MAX, sizeof(paramValue));
				// Bank 1 information
				(pNodeDB->paramBankList[1]).nParams = DRV_PARAM_MAX;
				(pNodeDB->paramBankList[1]).fixedInfoDB = &iscDrvInfoDB[0];
				// Create the value storage area together
				(pNodeDB->paramBankList[1]).valueDB 
					= (paramValue *)calloc(DRV_PARAM_MAX, sizeof(paramValue));
				// Create by node information
				pNodeDB->pNodeSpecific = new iscState;

				// Initialize the class specific items
				pNodeDB->pClassInfo = &iscClassDB; 

				// Reset all node diagnostics by reading them (clear on read)
				netGetParameter(theMultiAddr,ISC_P_NETERR_APP_CHKSUM,&dummy);
				netGetParameter(theMultiAddr,ISC_P_NETERR_APP_FRAG,&dummy);
				netGetParameter(theMultiAddr,ISC_P_NETERR_APP_STRAY,&dummy);
				netGetParameter(theMultiAddr,ISC_P_NETERR_APP_OVERRUN,&dummy);

				netGetParameter(theMultiAddr,ISC_P_NETERR_DIAG_CHKSUM,&dummy);
				netGetParameter(theMultiAddr,ISC_P_NETERR_DIAG_FRAG,&dummy);
				netGetParameter(theMultiAddr,ISC_P_NETERR_DIAG_STRAY,&dummy);
				netGetParameter(theMultiAddr,ISC_P_NETERR_DIAG_OVERRUN,&dummy);
				if (FWVER->verCode >= FW_MILESTONE_LINK_LOW) {
					netGetParameter(theMultiAddr,ISC_P_NETERR_LOW_VOLTS,&dummy);
				}

				errRet = coreUpdateParamInfo(theMultiAddr);
			}
			else
				errRet = MN_ERR_WRONG_NODE_TYPE;
		}
	}
	return(errRet);
}
//																			  *
//*****************************************************************************



//******************************************************************************
//	!NAME!
//		iscInitializeEx
//
//	DESCRIPTION:
//		This function will configure the network and download the parameters 
//		into a local copy of the parameters for the addressed node.
//
//	\return
//		MN_OK if successful
//
//	SYNOPSIS:
cnErrCode iscInitializeEx(
	multiaddr theMultiAddr,						// Ptr to return array
	nodebool warmInitialize)
{
	return(coreInitializeEx(theMultiAddr, warmInitialize, iscClassSetup));
}

/*																 !end!		*/
/****************************************************************************/



/*****************************************************************************
 *	!NAME!
 *		iscInitialize
 *
 *	DESCRIPTION:
 *		This function will configure the network and download the parameters 
 *		into a local copy of the parameters for the addressed node.
 *
 *	\return
 *		MN_OK if successful
 *
 *	SYNOPSIS: 															    */
cnErrCode iscInitialize(
	multiaddr theMultiAddr)						// Ptr to return array
{
	return(iscInitializeEx(theMultiAddr, FALSE));
}
/*																 !end!		*/
/****************************************************************************/

//******************************************************************************
//	NAME																	   *
//		iscGetStimulus
//
//	DESCRIPTION:
/**
	Get the current stimulus generator settings.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pState Ptr to the result area updated with current state 
			information.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetStimulus(
			multiaddr theMultiAddr,	// Target node addr
			iscStimState *pState)
{
	packetbuf theCmd, theResp;
	paramValue sampleTime;
	cnErrCode theErr = MN_OK;
	netaddr cNum;
	iscStimCmdPkt *pStimData = ((iscStimCmdPkt *)&theResp.Byte.Buffer[RESP_LOC]);

	cNum = coreController(theMultiAddr);
	
	// Get sample rate conversion factor	
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD, NULL, &sampleTime);
	if (theErr != MN_OK || sampleTime.value == 0)
		return(MN_ERR_RESP_FMT);
	sampleTime.value *= 0.001;			// Convert microseconds to milliseconds

	// Fill in the command head parts
	theCmd.Fld.Addr = theMultiAddr;
	theCmd.Fld.PktLen = 1;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_GET_SET_STIMULUS;
	theErr = netRunCommand(cNum, &theCmd, &theResp);
	// DEBUG PRINT
	//ISC_DUMP_PKT(theMultiAddr, "Get Stim", &theResp);
	if (theErr != MN_OK)
		return(theErr);

	// Properly formatted response?
	if (theResp.Fld.PktLen != ISC_STIM_RESP_STATUS_OCTETS) 
		return(MN_ERR_RESP_FMT);
	// Crack the return buffer into standard units
	pState->mode = (stimModes)pStimData->Mode;
	pState->period = (nodelong)(0.5+(pStimData->Period * sampleTime.value));
	pState->bits = pStimData->Status;
	switch (pState->mode) {
		case STIM_OFF:
			pState->amplitude = 0;				
			break;
		case STIM_VEL:				// Velocity	(ticks/ms)
			pState->amplitude = (ISC_MON_MAX_VEL * pStimData->Amplitude 
							   / sampleTime.value)
		             		   / 32767;
			break;
		case STIM_CAL:				// Calibrate/test (100% FS)
		case STIM_TRQ:				// Torque
			pState->amplitude = pStimData->Amplitude / 327.67;
			break;
		case STIM_POSN:				// Position	(ticks)
			pState->amplitude = pStimData->Amplitude;
			break;
		case STIM_MOVE_ONCE:
		case STIM_MOVE_RECP:
		case STIM_MOVE_REPEAT:
			pState->amplitude = pStimData->Amplitude;
			pState->slew = pStimData->Slew;
			// Convert dwell back to milliseconds
			if (sampleTime.value != 0) 
				pState->dwell = static_cast<nodelong>(pStimData->Dwell 
													 / sampleTime.value);
			else
				pState->dwell = 0;
			break;
		default:
			return(MN_ERR_BADARG);
	}
	//_RPT3(_CRT_WARN, "iscGetStimulus: mode %d ampl=%f per=%f\n", pState->mode, 
	//									pState->amplitude, pState->period);
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetStimulus
//
//	DESCRIPTION:
/**
	Change the stimulus generator setting.

	\param[in] theMultiAddr The address code for this node.
	\param[in] pNewState Ptr to the new state desired.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetStimulus(
			multiaddr theMultiAddr,	// Target node addr
			iscStimState *pNewState)
{
	packetbuf theCmd, theResp;
	paramValue sampleTime;
	nodelong lVal, tVal;
	cnErrCode theErr = MN_OK;
	netaddr cNum;

	cNum = coreController(theMultiAddr);

	//_RPT3(_CRT_WARN, "iscSetStimulus: mode %d ampl=%f per=%f\n", pNewState->mode, pNewState->amplitude, pNewState->period);
	// Get sample rate conversion factor	
	theErr = netGetParameterInfo(theMultiAddr, ISC_P_SAMPLE_PERIOD,	
								NULL, &sampleTime);
	if (theErr != MN_OK || sampleTime.value == 0)
		return(MN_ERR_BADARG);
	sampleTime.value *= 0.001;			// Convert microseconds to milliseconds

	// Fill in the command head parts
	theCmd.Fld.Addr = theMultiAddr;
	switch(pNewState->mode){
		case STIM_MOVE_ONCE:
		case STIM_MOVE_RECP:
		case STIM_MOVE_REPEAT:
			theCmd.Fld.PktLen = ISC_STIM_CMD_PKT_PROF_SIZE;
			break;
		default:
			theCmd.Fld.PktLen = ISC_STIM_CMD_PKT_SIZE;
			break;
	}
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_GET_SET_STIMULUS;
	// Shortcut to the command buffer
	iscStimCmdPkt *pSimCmd = ((iscStimCmdPkt *)&theCmd.Byte.Buffer[CMD_LOC+1]);

	// Set the mode
	pSimCmd->Mode = (nodechar)pNewState->mode;
	// Saturate @ largest value
	// Set the 0.5*period(ms) in sample-time(us) counts
	lVal = (nodelong)(0.5+(pNewState->period/sampleTime.value));
	// Saturate @ largest value
	if (lVal > 32767) lVal = 32767;
	pSimCmd->Period = (nodeshort)(lVal);
	// Zero out potentially unused fields
	pSimCmd->Dwell = 0;
	pSimCmd->Status = 0;
	pSimCmd->Slew = 0;

	// Figure out the scaling for the amplitude based on the mode
	switch(pNewState->mode)  {
		case STIM_OFF:
			lVal = 0;				
			break;
		case STIM_VEL:				// Velocity	(ticks/ms)
			lVal = (nodelong)(32767. * pNewState->amplitude * sampleTime.value 
			             / ISC_MON_MAX_VEL + ((pNewState->amplitude<0)?(-.5):(.5)));
			if (lVal > 32767) lVal = 32767;
			break;
		case STIM_CAL:				// Calibrate/test (100% FS)
		case STIM_TRQ:				// Torque
			lVal = (nodelong)(327.67*pNewState->amplitude);
			if (lVal > 32767) lVal = 32767;
			break;
		case STIM_POSN:				// Position	(ticks)
			if (pNewState->amplitude < 32767)
				lVal = (nodelong)pNewState->amplitude;
			else
				lVal = 32767;
			break;
		case STIM_MOVE_ONCE:
		case STIM_MOVE_RECP:
		case STIM_MOVE_REPEAT:
			pSimCmd->Period = static_cast<nodeshort>(pNewState->period);
			lVal = (nodelong)pNewState->amplitude;
			// Saturate @ largest value
			if (pNewState->amplitude > INT32_MAX) lVal = INT32_MAX;
			pSimCmd->Slew = (nodeshort)pNewState->slew;
			// Convert request milliseconds to sample-counts
			tVal = (nodelong)(0.5+(pNewState->dwell/sampleTime.value));
			if (tVal > INT16_MAX) tVal = INT16_MAX;
			pSimCmd->Dwell = (nodeshort)tVal;
			break;
		default:
			return(MN_ERR_BADARG);
	}
	
	pSimCmd->Amplitude = lVal;

	// Make it so, wait for return
	//ISC_DUMP_PKT(theMultiAddr, "Set Stim", &theCmd);

	theErr = netRunCommand(cNum, &theCmd, &theResp);
	if (theErr == MN_OK && theResp.Fld.PktLen != 0)
		return(MN_ERR_RESP_FMT);
	// Test we get back our settings 
	//{
	//	 iscStimState x;
	//	 theErr = iscGetStimulus(theMultiAddr, &x); 
	//}
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetMonitor
//
//	DESCRIPTION:
/**
	Get the monitor state for the selected node. 

	\param[in] theMultiAddr The address code for this node.
	\param[in] channel Targets which monitor port channel. Set this to zero.
	\param[out] pState Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetMonitor(
			multiaddr theMultiAddr,	// Target node addr
			nodeushort channel,		// Target monitor port if supported
			iscMonState *pState)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr = MN_OK;
	appNodeParam param;
	netaddr cNum;

	// Pointer to data overlay in packet
	iscMonNodeState *MON_AREA_GET 
		= ((iscMonNodeState *)&theResp.Byte.Buffer[RESP_LOC]);

	cNum = coreController(theMultiAddr);

	// We only support one channel
	if (channel != 0)
		return(MN_ERR_BADARG);

	theCmd.Fld.Addr = theMultiAddr;
	theCmd.Fld.PktLen = 1;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_GET_SET_MONITOR;
	// Get the raw data
	theErr = netRunCommand(cNum, &theCmd, &theResp);
	//ISC_DUMP_PKT(theMultiAddr, "Get Mon", &theResp);
	if (theErr!=MN_OK)
		return(theErr);
	// Properly formatted response?
	if (theResp.Fld.PktLen < 9 || theResp.Fld.PktLen > 10) 
		return(MN_ERR_RESP_FMT);
	// Fill in the return buffer
	if (pState)  {
		// Convert buffer to VB friendly structure
		pState->var = (iscMonVars)MON_AREA_GET->var;
		pState->tuneSync = (iscTuneSyncs)MON_AREA_GET->tuneSync;

		// Convert gain number to full scale equivalent
		pState->gain = convertMonGain(TRUE, theMultiAddr, 
											   pState->var, 
											   MON_AREA_GET->gain);

		// Convert filter TC bits to milliseconds
		param.bits = 0;					// Not used by converter anyways
		pState->filterTC = convertFilt1TCMilliseconds(TRUE, theMultiAddr, param, 
									MON_AREA_GET->filter,
									&SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)]);

		// Copy tune syncs
	}
	return(theErr);	
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetMonitor
//
//	DESCRIPTION:
/**
	Set the monitor state for the selected node. This function will
	also update the parameter values which mirror the individual states.

	\param[in] theMultiAddr The address code for this node.
	\param[in] channel Targets which monitor port channel. Set this to zero.
	\param[out] pNewState Ptr to desired new state.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetMonitor(
			multiaddr theMultiAddr,	// Target node addr
			nodeushort channel,		// Target monitor port if supported
			iscMonState *pNewState)
{

	cnErrCode theErr = MN_OK;
	packetbuf theCmd, theResp;
	nodelong paramLng;
	appNodeParam param;
	netaddr cNum;

	// Shortcut to the command buffer
	iscMonNodeState *MON_AREA_SET 
		= ((iscMonNodeState *)&theCmd.Byte.Buffer[CMD_LOC+1]);

	// We only support one channel
	if (channel != 0) 
		return(MN_ERR_BADARG);

	// Get controller context
	cNum = coreController(theMultiAddr);
	if (cNum > NET_CONTROLLER_MAX)
			return(MN_ERR_BADARG);

	if (theErr != MN_OK) {
		return(theErr);
	}

	// Fill in the command head parts
	theCmd.Fld.Addr = NODE_ADDR(theMultiAddr);
	theCmd.Fld.PktLen = 10;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_GET_SET_MONITOR;

	// Set the variable member
	MON_AREA_SET->var = (nodeshort)pNewState->var;
	
	// Convert the full scale value to a gain number
	paramLng = (nodelong)convertMonGain(FALSE, theMultiAddr, 
						  (iscMonVars)(pNewState->var & ~(MON_OPTION_MASKS)), 
						  pNewState->gain);
	MON_AREA_SET->gain = paramLng;

	// Convert the filter constant to the internal representation
	param.bits = 0;					// This is not used by the converter
	paramLng = (nodelong)convertFilt1TCMilliseconds(FALSE, theMultiAddr, param, 
						  pNewState->filterTC, 
						  &SysInventory[cNum].NodeInfo[NODE_ADDR(theMultiAddr)]);
	MON_AREA_SET->filter = (nodeshort)paramLng;

	// tuneSync
	MON_AREA_SET->tuneSync = (nodeshort)pNewState->tuneSync;
	theErr = netRunCommand(cNum, &theCmd, &theResp);
	//ISC_DUMP_PKT(theMultiAddr, "Set Mon", &theCmd);
	if (theErr == MN_OK && theResp.Fld.PktLen != 0)
		return(MN_ERR_RESP_FMT);
	// Test that we get our setting
	//iscMonState x;
	//theErr = iscGetMonitor(theMultiAddr, 0, &x);

	return(theErr);
}
//																			   *
//******************************************************************************

/*****************************************************************************
 *	!NAME!
 *		iscParamChangeFunc
 *
 *	DESCRIPTION:
 *		Set the parameter change callback function.
 *
 *	\return
 *		Old handler
 *
 *	SYNOPSIS: 															    */
MN_EXPORT paramChangeFunc MN_DECL iscParamChangeFunc(
	  	paramChangeFunc newFunc)		// Node number of changed
{
	paramChangeFunc oldFunc = iscClassDB.paramChngFunc;
	iscClassDB.paramChngFunc = newFunc;
	return(oldFunc);
}
/*																 !end!		*/
/****************************************************************************/

//==============================================================================
// END OF UNDOCUMENTED API FUNCTIONS TO THIS LINE
/// \endcond
//==============================================================================



/*****************************************************************************
 *	!NAME!
 *		iscSyncPosition
 *
 *	DESCRIPTION:
 *		Run the Synchronize Position command.
 *
 *	\return
 *		#cnErrCode; MN_OK if successful
 *
 *	SYNOPSIS: 															    */
MN_EXPORT cnErrCode MN_DECL iscSyncPosition(
			multiaddr theMultiAddr)			// Node address
{
	packetbuf cmd, theResp;					// Input and output buffers
	cnErrCode theErr;
	netaddr cNum;

	cNum = coreController(theMultiAddr);
	cmd.Fld.Addr = theMultiAddr;	
	cmd.Fld.PktLen = ISC_CMD_SYNC_POSN_LEN;
	cmd.Byte.Buffer[CMD_LOC] = ISC_CMD_SYNC_POSN;
	theErr = netRunCommand(cNum, &cmd, &theResp);
	return theErr;
}

/*																 !end!		*/
/****************************************************************************/








////****************************************************************************
////	NAME
////		iscReEnable
////
////	DESCRIPTION:
////		Reset the vector search flag
////
////	\return
////		#cnErrCode; MN_OK if successful
////
////	SYNOPSIS:
//MN_EXPORT cnErrCode MN_DECL iscReEnable(
//			multiaddr theMultiAddr)	// Target node addr
//{
//	packetbuf theCmd;
//	cnErrCode theErr = MN_OK;
//	netaddr cNum;
//
//	cNum = coreController(theMultiAddr);
//	// Fill in the command head parts
//	theCmd.Fld.Addr = theMultiAddr;
//	theCmd.Fld.PktLen = 1;
//	theCmd.Byte.Buffer[CMD_LOC] = MD_CMD_RE_ENABLE;
//	theErr = netRunCommand(cNum, &theCmd, NULL);
//	return(theErr);
//}
///****************************************************************************/

//******************************************************************************
//	NAME																	   *
//		iscReVector
//
//	DESCRIPTION:
/**
	Reset the vector search flag. This function is mostly diagnostic in nature.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscReVector(
			multiaddr theMultiAddr)	// Target node addr
{
	packetbuf theCmd, theResp;
	cnErrCode theErr = MN_OK;
	netaddr cNum;

	cNum = coreController(theMultiAddr);
	// Fill in the command head parts
	theCmd.Fld.Addr = theMultiAddr;
	theCmd.Fld.PktLen = 1;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_RE_VECTOR;
	theErr = netRunCommand(cNum, &theCmd, &theResp);
	return(theErr);
}
//																			   *
//******************************************************************************

//****************************************************************************
//	NAME
//		iscGet/SetJerkTime
//
//	DESCRIPTION:
//		Manipulate jerk limit in terms of time. This is a TG porting function.
//
//	\return
//		double
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetJerkTime(
			multiaddr theMultiAddr,
			double *settingValMS) 
{
	return(iscGetParameter(theMultiAddr, ISC_P_JERK_LIM, settingValMS));
}

MN_EXPORT cnErrCode MN_DECL iscSetJerkTime(
			multiaddr theMultiAddr,
			double newValMS)
{
	return(iscSetParameter(theMultiAddr, ISC_P_JERK_LIM, newValMS));
}


//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//-----------------------------------------------------------------------------
//			         MOTION INITIATION AND CONTROL API
//-----------------------------------------------------------------------------
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


////*****************************************************************************
////	NAME																	  *
////		waitForMoveIdle
////
////	DESCRIPTION:
////      This function spin waits for the move subsystem to become idle.
////		TODO: 01/6/09 DS Add timeout to protect scripts.
////
////	\return
////		#cnErrCode; MN_OK if successful
////
////	SYNOPSIS:
//static cnErrCode MN_DECL waitForMoveIdle(
//			multiaddr theMultiAddr)				// Node to access
//{
//	mnStatusReg rtStatus;
//	cnErrCode theErr;
//
//    do {
//        theErr  = iscGetStatusRTReg(theMultiAddr, &rtStatus);
//		infcSleep(0);
//    } while (theErr == MN_OK
//			 && (!rtStatus.Fld.PLAin.DrvFld.MoveDone
//			  || !rtStatus.Fld.LowAttn.DrvFld.MoveBufAvail));
//	return (theErr);
//}
////																			  *
////*****************************************************************************


/// \cond INTERNAL_DOC
//*****************************************************************************
//	NAME																	  *
//		waitForMoveBuffer
//
//	DESCRIPTION:
//		This function spin waits until the move buffer is available.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
cnErrCode MN_DECL waitForMoveBuffer(
			multiaddr theMultiAddr)			// Node to access
{
	return(MN_OK);
 //   mnStatusReg rtStatus;				// RVW? TODO...
 //   cnErrCode theErr;
	//double timeOut;
 //   
 //   // Wait for room in the buffer
	//timeOut = infcCoreTime() + 10000;
 //   do {
 //   	theErr = iscGetStatusRTReg(theMultiAddr, &rtStatus);
	//	infcSleep(0);
	//	if (infcCoreTime() > timeOut)
	//		return(MN_ERR_TIMEOUT);
 //   } while (theErr == MN_OK 
 //   		 && !rtStatus.bit.moveBufEmpty);
	//return(theErr);
}	
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscForkMoveVelQueued
//
//	DESCRIPTION:
//      Sends a move that ramps up or down in speed to Velocity (using the
//      previously set limits for acceleration and jerk [RAS]).
//
//      Program execution does not suspend.  Once Velocity is achieved, the
//      move stays at Velocity until explicitly changed. Velocity is specified
//      as steps/second.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkMoveVelQueued(
			multiaddr theMultiAddr,		// Node address
			double velTargetStepPerSec,	// Velocity (steps/sec)
			nodelong positionTarget,	// Target Position (optional)
			mgVelStyle moveType,		// Motion style
			nodelong *pBuffersRemaining)	// Ptr to move buf remaining cnt
{
	packetbuf cmd, resp;					// Input and output buffers
	netaddr cNum;
	double velScale, velTarg;
	nodelong iVelTarg;
	cnErrCode theErr = MN_OK;

	// Get sample rate as basis for ticks/sample
    theErr = iscGetParameter(theMultiAddr, ISC_P_SAMPLE_PERIOD, &velScale);
	if (theErr == MN_OK) {
		cNum = NET_NUM(theMultiAddr);
		cmd.Fld.Addr = NODE_ADDR(theMultiAddr);	
		// Len=cmd byte, 4 bytes target vel, mode byte
		cmd.Fld.PktLen = 1 + sizeof(iVelTarg) + 1;
		cmd.Byte.Buffer[CMD_LOC] = ISC_CMD_MOVE_VEL_EX;
		velTarg = (nodelong)(0.5 + (velTargetStepPerSec * ISC_VEL_MOVE_SCALE * velScale 
									   / 1000000.));
		// Saturate test
		/// \cond INTERNAL_DOC
		#define MAX_VEL_Q 31
		/// \endcond
		if (velTarg > (1LL<<MAX_VEL_Q)-1)
			velTarg = (1LL<<MAX_VEL_Q)-1;
		else if(velTarg < (-1LL<<MAX_VEL_Q))
			velTarg  = -1LL<<MAX_VEL_Q;
		// Save as long integer bits
		iVelTarg = (nodelong)velTarg;
		// copy the position target into the command buffer
		memcpy(&cmd.Byte.Buffer[CMD_LOC+1], &iVelTarg, sizeof(iVelTarg));
		// Copy move type over
		cmd.Byte.Buffer[CMD_LOC+5] = (nodechar)moveType.styleCode;
		// If positional involved
		if (positionTarget) {
			memcpy(&cmd.Byte.Buffer[CMD_LOC+6], &positionTarget, 
											    sizeof(positionTarget));
			cmd.Fld.PktLen += sizeof(positionTarget);
		}
		// Run the command and await the response
		theErr = netRunCommand(cNum, &cmd, &resp);
		if (pBuffersRemaining && theErr == MN_OK)
			*pBuffersRemaining = resp.Byte.Buffer[RESP_LOC];
	}
	return(theErr);
}
//																			  *
//*****************************************************************************
/// \endcond

//*****************************************************************************
//	NAME																	  *
//		iscForkMoveVelEx
//
//	DESCRIPTION:
//      Sends a move that ramps up or down in speed to Velocity (using the
//      previously set limits for acceleration and jerk [RAS]).
//
//      Program execution does not suspend.  Once Velocity is achieved, the
//      move stays at Velocity until explicitly changed. Velocity is specified
//      as steps/second.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkMoveVelEx(
			multiaddr theMultiAddr,			// The node
			double velTargetStepPerSec,		// Target velocity (steps/sec)
			nodelong positionTarget,		// Position Target [optional]
			mgVelStyle moveType)			// Enhanced style selection
{
	return iscForkMoveVelQueued(theMultiAddr, velTargetStepPerSec,
							    positionTarget, moveType,
								NULL);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscForkVelMoveBuffered
//
//	DESCRIPTION:
//      Sends a move that ramps up or down in speed to Velocity (using the
//      previously set limits for acceleration and jerk [RAS]).
//
//      Program execution does not suspend.  Once Velocity is achieved, the
//      move stays at Velocity until explicitly changed. Velocity is specified
//      as steps/second.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkVelMoveBuffered(
			multiaddr theMultiAddr,			// The node
			double velTargetStepPerSec,		// Target velocity (steps/sec)
			nodelong positionTarget,			// Position Target [optional]
			mgVelStyle moveType)			// Enhanced style selection
{
	// Wait for buffer to free up
	cnErrCode theErr = waitForMoveBuffer(theMultiAddr);
	// If OK, run the command, it shouldn't fail due to no buffer space.
	if (theErr == MN_OK) {
		theErr = iscForkMoveVelEx(theMultiAddr, velTargetStepPerSec, 
								  positionTarget, moveType);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		iscMoveProfiled
//
//	DESCRIPTION:
//		This function performs a move using an information buffer
//		for motion specification.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscMoveProfiled(
			multiaddr theMultiAddr,		// Destination node
			mgMoveProfiledInfo *spec)	// Target (steps)
{
	packetbuf cmd, resp;					// Input and output buffers
	netaddr cNum;
	cnErrCode theErr = MN_OK;

	//_RPT3(_CRT_WARN, "%.1f move(%d)=>%d\n", infcCoreTime(), theMultiAddr, spec->value);

	// TODO convert to use most net efficient form of command
	cNum = coreController(theMultiAddr);
	cmd.Fld.Addr = theMultiAddr;	
	cmd.Fld.PktLen = 1 + 1 + sizeof(spec->value);
	cmd.Byte.Buffer[CMD_LOC] = ISC_CMD_MOVE_POSN_EX;
	iscMovePosnExPacket *MOVE_DATAP = 
		((iscMovePosnExPacket *)&cmd.Byte.Buffer[CMD_LOC+1]);
	MOVE_DATAP->style = spec->type;
	// copy the position target into the command buffer
	memcpy(&cmd.Byte.Buffer[CMD_LOC+2], &spec->value, 
										  sizeof(spec->value));
	// Run the command and await the response
	theErr = netRunCommand(cNum, &cmd, &resp);
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscMoveProfiledQueued
//
//	DESCRIPTION:
//		This function performs a move using an information buffer
//		for motion specification.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscMoveProfiledQueued(
			multiaddr theMultiAddr,		// Destination node
			mgMoveProfiledInfo *spec,	// Target (steps)
			nodelong *pBuffersRemaining)
{
	packetbuf cmd, resp;					// Input and output buffers
	netaddr cNum;
	cnErrCode theErr = MN_OK;

	//_RPT3(_CRT_WARN, "%.1f move(%d)=>%d\n", infcCoreTime(), theMultiAddr, spec->value);
	// TODO convert to use most net efficient form of command
	cNum = coreController(theMultiAddr);
	cmd.Fld.Addr = theMultiAddr;	
	cmd.Fld.PktLen = 1 + 1 + sizeof(spec->value);
	cmd.Byte.Buffer[CMD_LOC] = ISC_CMD_MOVE_POSN_EX;
	iscMovePosnExPacket *MOVE_DATAP = 
		((iscMovePosnExPacket *)&cmd.Byte.Buffer[CMD_LOC+1]);
	MOVE_DATAP->style = spec->type;
	// copy the position target into the command buffer
	memcpy(&cmd.Byte.Buffer[CMD_LOC+2], &spec->value, 
										  sizeof(spec->value));
	// Run the command and await the response
	theErr = netRunCommand(cNum, &cmd, &resp);

	// If run on older node, assume nothing left
	if (pBuffersRemaining) {
		*pBuffersRemaining = 0;
		if (resp.Fld.PktLen)
			*pBuffersRemaining = resp.Byte.Buffer[RESP_LOC];
	}
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscForkMoveExBuffered
//
//	DESCRIPTION:
//		This function performs initiates a move with buffer management.
//
//	\return
//		#cnErrCode; MN_OK if successful
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkMoveExBuffered(
			multiaddr theMultiAddr,		// Destination node
			nodelong posnTarget,			// Target (steps)
			mgPosnStyle moveType)
{
	// Wait for buffer to free up
	cnErrCode theErr = waitForMoveBuffer(theMultiAddr);
	// If OK, run the command, it shouldn't fail due to no buffer space.
	if (theErr == MN_OK) {
		theErr = iscForkMoveEx(theMultiAddr, posnTarget, moveType);
	}
	return(theErr);
}
//																			  *
//*****************************************************************************

#ifdef BONE_YARD
//Boneyard: Some features haven't been implemented yet, but we want to save
//the original functions and declarations for when they are.

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ISC Drive IO sample register
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
typedef union _iscDrvIOsampleReg  {
	nodeushort bits;
	struct _iscDrvIOsampleRegBits  {
		unsigned					: 4;	// 0-3
		unsigned nPosLimit			: 1;	// 4  ~Positive limit (in)
		unsigned nNegLimit			: 1;	// 5  ~Negative limit (in)
		unsigned Shutdown			: 1;	// 6  Shutdown (out)
		unsigned Commutation		: 3;	// 7-9  Commutation TSR code (in)
		unsigned EnableIn			: 1;	// 10 Enable (in) emulated
		unsigned AmpDisable			: 1;	// 11 Amplifier stage disable (out)
		unsigned DriveLED			: 1;	// 12 Drive LED (out)
		unsigned OverCurrent		: 1;	// 13 Overcurrent (in)
		unsigned Thermostat			: 1;	// 14 Thermostat input
		unsigned ModeIn				: 1;	// 15 Mode line (in) <unused>
	} bit;
} iscDrvIOsampleReg;

/*																 !end!		*/
/****************************************************************************/

#endif	 // BONE_YARD
/// \endcond

//==============================================================================
// DOCUMENTED API FUNCTIONS GO BELOW THIS LINE
//==============================================================================

//******************************************************************************
//	NAME																	   *
//		iscGetParameter
//
//  DESCRIPTION:
/**
	Get parameter value with no extra information.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theParam The ISC parameter code
	\param[out] pParamVal Ptr to result

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetParameter(
			multiaddr theMultiAddr,		// Node address
			iscParams theParam,			// Parameter index
			double *pParamVal)			// Ptr to value area
{
	paramValue pVal;
	cnErrCode theErr;

	theErr = iscGetParameterEx(theMultiAddr, theParam, &pVal, NULL);
//	if (theParam == (ISC_P_DRV_I_MAX)) {
//		_RPT3(_CRT_WARN, "ding p#%d(%d)=>%f\n", theParam, theMultiAddr, pVal.value );
//	}
//	else {
//		_RPT2(_CRT_WARN, "isc param %d=%f\n", theParam, pVal.value);
//	}
	if (pParamVal != NULL) 
		*pParamVal = pVal.value;
	return(theErr);
}
//																               *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		iscSetParameter
//
//  DESCRIPTION:
/**
	This function will update the local parameter value for the selected
	node and then change the value at the node if possible. If the
	parameter is inaccessible the appropriate error is returned.
		
	\param[in] theMultiAddr The address code for this node.
	\param[in] theParam The ISC parameter code
	\param[out] paramVal New value to set at node

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetParameter(
			multiaddr theMultiAddr,		// Node address
			iscParams theParam,			// Parameter index
			double paramVal)			// New value
{
	cnErrCode theErr = MN_OK;
	theErr = netSetParameterInfo(theMultiAddr,
								  theParam,
								  paramVal, FALSE);
	#ifdef _DEBUG
		if(theErr != MN_OK)
			_RPT4(_CRT_WARN,"netSetParameterInfo(%d, %d, %f) failed, err=0x%x", 
			theMultiAddr, theParam, paramVal, theErr);
	#endif
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetParameterEx
//
//  DESCRIPTION:
/**
	This function will retreive the local parameter value for the
	selected node and return its value and information pertaining to it.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theParam The ISC parameter code
	\param[out] pTheValue Ptr to the parameter value / raw bits.
	\param[out] pTheInfo Ptr to optional information if non-NULL

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetParameterEx(
			multiaddr theMultiAddr, 		// Node address
			iscParams theParam,			// Parameter index
			paramValue *pTheValue,		// Ptr to returning value area
			paramInfo *pTheInfo) 		// Ptr to returning optional information
{

	cnErrCode theErr=MN_OK;
	theErr = netGetParameterInfo(theMultiAddr,
								  theParam, 
								  pTheInfo, 
								  pTheValue);
	return(theErr);
}

//																               *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		iscSetParameterEx
//
//  DESCRIPTION:
/**
	Set the parameter using the bytes described in the \a paramVal
	buffer.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theParam The ISC parameter code
	\param[out] pNewValue Ptr to bit-oriented packet buffer. The buffer size
				will determine the amount written to the node.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetParameterEx(
			multiaddr theMultiAddr,		// Node address
			iscParams theParam,			// Parameter index
			packetbuf *pNewValue)		// New value
{
	cnErrCode theErr = MN_OK;
	// Set the raw value at the node
	theErr = netSetParameterEx
		(theMultiAddr, theParam, pNewValue);
	return(theErr);
}
//																               *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		iscAddToPosition
//
//  DESCRIPTION:
/**
	Change the node's measured and commanded positions by the \p theOffset 
	amount.	All position capture sources are adjusted to reflect the offset
	in number space. The change is atomically applied to all values. 

	This function will not cause any motion to occur and provides a mechanism
	for aligning the application's number space with the node's.
		
	\remark Incoming capture sources properly reflect the offset in 
	number space.

	\param[in] theMultiAddr The address code for this node.
	\param[in] theOffset Offset amount in encoder units.

	\return
		#cnErrCode; MN_OK if successful

	\see ISC_P_POSN_CAP_GPI1
	\see ISC_P_POSN_CAP_INDEX
	\see ISC_P_POSN_CAP_INDEX_LD
	\see ISC_P_POSN_CAP_PLA
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscAddToPosition(
			multiaddr theMultiAddr,		// Node address
			double theOffset)			// Offset amount
{
	packetbuf theCmd, theResp;					// Input and output buffers
	cnErrCode theErr;
	netaddr cNum;

	cNum = coreController(theMultiAddr);
	// Format the command
	theCmd.Fld.Addr = theMultiAddr;	
	theCmd.Fld.PktLen = ISC_CMD_ADD_POSN_LEN;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_ADD_POSN;
	*((nodelong *)&theCmd.Byte.Buffer[CMD_LOC+1]) 
		= (nodelong)(theOffset);
	// Run it and wait for response
	theErr = netRunCommand(cNum, &theCmd, &theResp);
	return theErr;
}
//																               *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		iscAddToPositionLd
//
//  DESCRIPTION:
/**
	Change the load's measured position by the \p theOffset 
	amount.
		
	\param[in] theMultiAddr The address code for this node.
	\param[in] theOffset Offset amount in encoder units.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscAddToPositionLd(
			multiaddr theMultiAddr,		// Node address
			double theOffset)			// Ptr to parameter list definitions
{
	packetbuf theCmd, theResp;					// Input and output buffers
	cnErrCode theErr;
	netaddr cNum;

	theCmd.Fld.Addr = NODE_ADDR(theMultiAddr);	
	theCmd.Fld.PktLen = ISC_CMD_ADD_POSN_LEN;
	theCmd.Byte.Buffer[CMD_LOC] = ISC_CMD_ADD_POSN_LD;
    *((nodelong *)&theCmd.Byte.Buffer[CMD_LOC+1]) 
		= (nodelong)((theOffset));

	cNum = coreController(theMultiAddr);
	theErr = netRunCommand(cNum, &theCmd, &theResp);
	return theErr;
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetUserOutputReg
//
//  DESCRIPTION:
/**
	Set the state of the User Output Register. When the output register is
	controlled by bits in this register, changes here are reflected in the 
	output register. This register typically is used to enable the drive and to 
	set the General Purpose Outputs.

	\param[in] theMultiAddr The address code for this node.
	\param[in] newState New state for User Output Register.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetUserOutputReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg newState)			// New setting
{
	// Update the double value
	return(iscSetParameter(theMultiAddr, ISC_P_USER_OUT_REG, newState.bits));
}

//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetUserOutputReg
//
//  DESCRIPTION:
/**
	Get the current setting of the User Output Register.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pGPOreg Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetUserOutputReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg)			// Current setting
{
	double pVal;
	cnErrCode theErr;
	
	theErr = iscGetParameter(theMultiAddr, ISC_P_USER_OUT_REG, &pVal);
	// Update the raw buffer
	pGPOreg->bits = (nodeushort)pVal;
	return(theErr);
}

//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetOutputReg
//
//  DESCRIPTION:
/**
	Get the current setting of the Output Register.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pGPOreg Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetOutputReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg)			// Current setting
{
	double pVal;
	cnErrCode theErr;
	
	theErr = iscGetParameter(theMultiAddr, ISC_P_OUT_REG, &pVal);
	// Update the raw buffer
	pGPOreg->bits = (nodeushort)pVal;
	return(theErr);
}

//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetOutputRiseReg
//
//  DESCRIPTION:
/**
	Get the current setting of the Output Rise Register

	\param[in] theMultiAddr The address code for this node.
	\param[out] pGPOreg Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetOutputRiseReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg)			// Current setting
{
	double pVal;
	cnErrCode theErr;
	
	theErr = iscGetParameter(theMultiAddr, ISC_P_OUT_RISE_REG, &pVal);
	// Update the raw buffer
	pGPOreg->bits = (nodeushort)pVal;
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetOutputFallReg
//
//  DESCRIPTION:
/**
	Get the current setting of the Output Fall Register

	\param[in] theMultiAddr The address code for this node.
	\param[out] pGPOreg Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetOutputFallReg(
			multiaddr theMultiAddr,		// Node address
			plaOutReg *pGPOreg)			// Current setting
{
	double pVal;
	cnErrCode theErr;
	
	theErr = iscGetParameter(theMultiAddr, ISC_P_OUT_FALL_REG, &pVal);
	// Update the raw buffer
	pGPOreg->bits = (nodeushort)pVal;
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetStatusAccumReg
//
//	DESCRIPTION:
/**
	Get and clear the current Status Accumulation Register value.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pStatus Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS: 
MN_EXPORT cnErrCode MN_DECL iscGetStatusAccumReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_STATUS_ACCUM_REG, 
							   &paramVal, NULL);
	pStatus->clear();					// Insure MSBs cleared
	*pStatus = *((mnStatusReg *)&(paramVal.raw.Byte.Buffer[0]));
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetAttnStatusRiseReg
//
//	DESCRIPTION:
/**
	Get and clear the current accumulation of rising status register states.

	\note Any bits that are unmasked for Attention Generation always return
	zero.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pStatus Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful

	\deprecated To future-proof for future devices use the generic
	read function #netGetAttnStatusRiseReg.
**/
//	SYNOPSIS: 
MN_EXPORT cnErrCode MN_DECL iscGetAttnStatusRiseReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_STATUS_ATTN_RISE_REG, 
							  &paramVal, NULL);
	pStatus->clear();					// Insure MSBs cleared
	*pStatus = *((mnStatusReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetStatusFallReg
//
//	DESCRIPTION:
/**
	Get and clear the current accumulation of the falling status register 
	states.

	\note Any bits that are unmasked for Attention Generation always return
	zero.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pStatus Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful
**/		
//	SYNOPSIS: 
MN_EXPORT cnErrCode MN_DECL iscGetStatusFallReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_STATUS_FALL_REG, 
							  &paramVal, NULL);
	pStatus->clear();					// Insure MSBs cleared
	*pStatus = *((mnStatusReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetStatusRTReg
//
//	DESCRIPTION:
/**
	Get a snapshot of the current status register states.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pStatus Ptr to current state.

	\return
		#cnErrCode; MN_OK if successful

	\deprecated To future-proof for future devices use the generic
	read function #netGetStatusRTReg.
	\todo add other common register accesors for mask, etc.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetStatusRTReg(
			multiaddr theMultiAddr,		// Node address
			mnStatusReg *pStatus)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_STATUS_RT_REG, 
							   &paramVal, NULL);
	pStatus->clear();					// Insure MSBs cleared
	*pStatus = *((mnStatusReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetAlertReg
//
//	DESCRIPTION:
/**
	Get the current state of the Alert Register.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pAlertReg Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetAlertReg(
			multiaddr theMultiAddr,			// Node address
			alertReg *pAlertReg)			// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_ALERT_REG, &paramVal, NULL);
	*pAlertReg = *((alertReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetWarningReg
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
MN_EXPORT cnErrCode MN_DECL iscGetWarningReg(
			multiaddr theMultiAddr,			// Node address
			alertReg *pWarningReg)			// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_WARN_REG, &paramVal, NULL);
	*pWarningReg = *((alertReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetAttnMask
//
//	DESCRIPTION:
/**
	Get the current state of the Attention Mask Register. Bits set in this
	register will allow automatic generation of Attentions (Network Interrupts)
	to the host.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pAttnMask Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetAttnMask(
			multiaddr theMultiAddr,		// Node address
			attnReg *pAttnMask)			// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_STATUS_ATTN_MASK, 
							   &paramVal, NULL);
	*pAttnMask = *((attnReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetAttnMask
//
//	DESCRIPTION:
/**
	Set the Attention Mask Register. Bits set in this
	register will allow automatic generation of Attentions (Network Interrupts)
	to the host.

	\param[in] theMultiAddr The address code for this node.
	\param[in] pAttnMask Ptr to new setting.

	\return		
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetAttnMask(
			multiaddr theMultiAddr,		// Node address
			attnReg *pAttnMask)			// Current setting
{
	cnErrCode theErr;
	packetbuf paramVal;
	memcpy(paramVal.Byte.Buffer, pAttnMask, ATTN_MASK_OCTETS);
	paramVal.Byte.BufferSize = ATTN_MASK_OCTETS;
	theErr = iscSetParameterEx(theMultiAddr, ISC_P_STATUS_ATTN_MASK, &paramVal);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetAlertMaskReg
//
//	DESCRIPTION:
/**
	Get the current state of the <i>Alert Mask Register</i> (#ISC_P_ALERT_MASK). 
	Bits set in this register could allow the automatic generation of
	Attentions (Network Interrupts) when specifically selected items become 
	active. The #lowAttnReg::UserAlert bit is set in the Status Register when 
	the Alert Register bit-wised ANDed with this register is non-zero.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pAlertMask Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful

	\see \ref iscSetAlertMaskReg
	\see \ref mnStatusReg
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetAlertMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pAlertMask)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_ALERT_MASK, 
							   &paramVal, NULL);
	*pAlertMask = *((alertReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetAlertMaskReg
//
//	DESCRIPTION:
/**
	Set the current state of the <i>Alert Mask Register</i> (#ISC_P_ALERT_MASK). 
	Bits set in this register could allow the automatic generation of 
	Attentions (Network Interrupts) when specifically selected items become 
	active. The #lowAttnReg::UserAlert bit is set in the Status Register when 
	the Alert Register bit-wised ANDed with this register is non-zero. 
	The alert mask for #lowAttnReg::UserAlert, when set, will generate an 
	automatic attention.

	\param[in] theMultiAddr The address code for this node.
	\param[in] pAlertMask Ptr to new setting.

	\return		
		#cnErrCode; MN_OK if successful

	\see \ref iscGetAlertMaskReg
	\see \ref mnStatusReg
	\see \ref iscSetAttnMask
	\see \ref mnAttnsPage
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetAlertMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pAlertMask)		// Current setting
{
	cnErrCode theErr;
	packetbuf paramVal;
	memcpy(paramVal.Byte.Buffer, pAlertMask, ALERTREG_SIZEOCTETS);
	paramVal.Byte.BufferSize = ALERTREG_SIZEOCTETS;
	theErr = iscSetParameterEx(theMultiAddr, ISC_P_ALERT_MASK, &paramVal);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetWarningMaskReg
//
//	DESCRIPTION:
/**
	Get the current state of the <i>Warning Mask Register</i> (#ISC_P_WARN_REG). 
	Bits set in this register could allow the automatic generation of Attentions
	(Network Interrupts) when specifically selected items become active. The 
	#lowAttnReg::Warning bit is set in the Status Register when the
	Warning Register bit-wised ANDed with this register is non-zero.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pWarnMask Ptr to result area.

	\return
		#cnErrCode; MN_OK if successful

	\see \ref iscSetWarningMaskReg
	\see \ref mnStatusReg
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetWarningMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pWarnMask)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_WARN_MASK, 
							   &paramVal, NULL);
	*pWarnMask = *((alertReg *)&paramVal.raw.Byte.Buffer[0]);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetWarningMaskReg
//
//	DESCRIPTION:
/**
	Set the current state of the <i>Warning Mask Register</i> (#ISC_P_WARN_REG). 
	Bits set in this register could allow the automatic generation of Attentions
	(Network Interrupts) when specifically selected items become active. The 
	#lowAttnReg::Warning bit is set in the Status Register when the
	Warning Register bit-wised ANDed with this register is non-zero.
	The alert mask for #lowAttnReg::Warning, when set, will generate an 
	automatic attention.

	\param[in] theMultiAddr The address code for this node.
	\param[in] pWarnMask Ptr to new setting.

	\return		
		#cnErrCode; MN_OK if successful

	\see \ref iscGetWarningMaskReg
	\see \ref mnStatusReg
	\see \ref iscSetAttnMask
	\see \ref mnAttnsPage
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetWarningMaskReg(
			multiaddr theMultiAddr,		// Node address
			alertReg *pWarnMask)		// Current setting
{
	cnErrCode theErr;
	packetbuf paramVal;
	memcpy(paramVal.Byte.Buffer, pWarnMask, ALERTREG_SIZEOCTETS);
	paramVal.Byte.BufferSize = ALERTREG_SIZEOCTETS;
	theErr = iscSetParameterEx(theMultiAddr, ISC_P_WARN_MASK, &paramVal);
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetHwConfigReg
//
//	DESCRIPTION:
/**
	Get the current state of the Hardware Configuration Register.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pHwConfigReg Ptr to the result.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetHwConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscHwConfigReg *pHwConfigReg)	// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_HW_CONFIG_REG, 
							&paramVal, NULL);
	*pHwConfigReg = *((iscHwConfigReg *)&paramVal.raw.Byte.Buffer[0]);

	pHwConfigReg->bits &= 0xffffffff;			// 32 bit register
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetHwConfigReg
//		As of 01/11/10, this is a 32 bit register
//
//	DESCRIPTION:
/**
	Set the current state of the Hardware Configuration Register. This register
	controls many the hardware related options and features.  For example 
	the IEX control is setup here.

	\param[in] theMultiAddr The address code for this node.
	\param[in] hwConfigReg New state.

	\return	
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetHwConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscHwConfigReg hwConfigReg)		// New setting
{
	hwConfigReg.bits &= 0xffffffff;
	// Update the double value
	return(iscSetParameter(theMultiAddr, ISC_P_HW_CONFIG_REG, hwConfigReg.bits));
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetAppConfigReg
//
//	DESCRIPTION:
/**
	Get the current state of the Application Configuration Register. This 
	register controls application and drive specific features such as control
	of the hard-stop feature.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pAppConfigReg Ptr to the result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetAppConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscAppConfigReg *pAppConfigReg)	// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_APP_CONFIG_REG, 
							   &paramVal, NULL);
	*pAppConfigReg = *((iscAppConfigReg *)&paramVal.raw.Byte.Buffer[0]);

	pAppConfigReg->bits &= 0xffffffff;			// 32 bit register
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetAppConfigReg
//		As of 01/11/10, this is a 32 bit register
//
//	DESCRIPTION:
/**
	Set the state of the Application Configuration Register. This 
	register controls application and drive specific features such as control
	of the hard-stop feature.

	\param[in] theMultiAddr The address code for this node.
	\param[in] appConfigReg New state for the register.

	\return		
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetAppConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscAppConfigReg appConfigReg)	// New setting
{
	appConfigReg.bits &= 0xffffffff;
	// Update the double value
	return(iscSetParameter(theMultiAddr, ISC_P_APP_CONFIG_REG, 
						   appConfigReg.bits));
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetTuneConfigReg
//
//	DESCRIPTION:
/**
	Get the state of the Tuning Configuration Register. This 
	register controls items related to servo tuning.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pTuneConfigReg Ptr to the result area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetTuneConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscTuneConfigReg *pTuneConfigReg)	// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_DRV_TUNE_CONFIG_REG, 
							   &paramVal, NULL);
	*pTuneConfigReg = *((iscTuneConfigReg *)&paramVal.raw.Byte.Buffer[0]);

	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetTuneConfigReg
//		As of 01/11/10, this is a 32 bit register
//
//	DESCRIPTION:
/**
	Set the state of the Tuning Configuration Register. This 
	register controls tuning related features such as anti-hunt
	and the velocity estimators.

	\param[in] theMultiAddr The address code for this node.
	\param[in] tuneConfigReg New state to set.

	\return	
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetTuneConfigReg(
			multiaddr theMultiAddr,			// Node address
			iscTuneConfigReg tuneConfigReg)	// New setting
{
	// Update the double value
	return(iscSetParameter(theMultiAddr, ISC_P_DRV_TUNE_CONFIG_REG, 
							tuneConfigReg.bits));
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetLdMtrRatio
//
//	DESCRIPTION:
/**
	Get the load to motor encoder ratio. This setting specifies
	the coupling that exists between the motor and load encoders.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pLoadPart Ptr to the load portion of the load/mtr ratio.
	\param[out] pMotorPart Ptr to the motor portion of the load/mtr ratio.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetLdMtrRatio(
			multiaddr theMultiAddr,			// Node address
			nodelong *pLoadPart,
			nodelong *pMotorPart)	// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;
	
	theErr = iscGetParameterEx(theMultiAddr, ISC_P_DRV_LD_MTR_RATIO, 
							   &paramVal, NULL);
	if (theErr == MN_OK){
		*pLoadPart = *((nodelong *)&paramVal.raw.Byte.Buffer[0]);
		*pMotorPart = *((nodelong *)&paramVal.raw.Byte.Buffer[4]);
	}
	return(theErr);
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetLdMtrRatio
//
//	DESCRIPTION:
/**
	Set the load to motor encoder ratio. This setting specifies
	the coupling that exists between the motor and load encoders.

	\param[in] theMultiAddr The address code for this node.
	\param[in] loadPart The load portion of the load/mtr ratio to set.
	\param[in] motorPart The motor portion of the load/mtr ratio to set.

	\return		
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetLdMtrRatio(
			multiaddr theMultiAddr,			// Node address
			nodelong loadPart,
			nodelong motorPart)	// New setting
{
	packetbuf buf;
	// Set the Ld to Mtr ratio to loadPart:motorPart
	buf.Byte.BufferSize = sizeof(nodelong)*2;
	*(nodelong*)(&buf.Byte.Buffer[0]) = loadPart;
	*(nodelong*)(&buf.Byte.Buffer[4]) = motorPart;
	// Always write the ld/mtr ratio to EE
	iscSetParameterEx(theMultiAddr, 
							 (iscParams)(ISC_P_DRV_LD_MTR_RATIO+128), 
							 &buf);
	// Always write the ld/mtr ratio to EE
	return(iscSetParameterEx(theMultiAddr, 
							 (iscParams)(ISC_P_DRV_LD_MTR_RATIO), 
							 &buf));
}
//																               *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscFactoryDefaults
//
//	DESCRIPTION:
/**
	Initialize the node to factory defaults. All tuning and features are
	restored to factory ship out state.

	\param[in] theMultiAddr The address code for this node.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscFactoryDefaults(
			multiaddr theMultiAddr)			// Destination node
{
	double romSum, eeVer;
	packetbuf buf;
	cnErrCode theErr;
	unsigned i, maxItems;
	double nodeVers, pVal;
	optionReg opts;

	maxItems = sizeof(initData)/sizeof(factoryInitData);

	// Dis-Acknowledge EE update
	if ((theErr = iscGetParameter(theMultiAddr, ISC_P_EE_VER, &eeVer)) != MN_OK)
		return(theErr);
	if ((theErr = iscGetParameter(theMultiAddr, ISC_P_ROM_SUM, &romSum)) != MN_OK)
		return(theErr);
    if ((theErr = netSetParameterInfo(theMultiAddr, ISC_P_EE_UPD_ACK , 
										  -eeVer, TRUE)) != MN_OK)
		return(theErr);

	if ((theErr = iscGetParameter(theMultiAddr, ISC_P_FW_VERSION, &nodeVers)) != MN_OK)
		return(theErr);
    
	if ((theErr = iscGetParameter(theMultiAddr, ISC_P_OPTION_REG, &pVal)) != MN_OK)
		return(theErr);
    opts.bits = (unsigned)pVal;

	for (i=0 ; i<maxItems ; i++) {
		if ((nodeVers >= initData[i].minVersion) 
		&& ((initData[i].fwOption==0)		// Cover invariant cases
		|| (opts.Common.Product==3 && initData[i].fwOption==2)) ) {  // Cover K case(s)
			switch (initData[i].type) {
				case RAM:
					// There is no EE version; just set the RAM value
					theErr = netSetParameterInfo(theMultiAddr, 
											 initData[i].paramNum, 
						                     initData[i].value, TRUE);
					break;
				case NV:
					// There is no RAM shadow; just set the EE value
					theErr = netSetParameterInfo(theMultiAddr, 
											 initData[i].paramNum|PARAM_OPT_MASK, 
						                     initData[i].value, TRUE);
					break;
				case NVRV:
					// Set the RAM version and the EE version will be updated
					theErr = netSetParameterInfo(theMultiAddr, 
											 initData[i].paramNum, 
						                     initData[i].value, TRUE);
					break;
				case NVR:
					// Set non-volatile
					theErr = netSetParameterInfo(theMultiAddr, 
											 initData[i].paramNum|PARAM_OPT_MASK, 
						                     initData[i].value, TRUE);
					//If the EE write failed, don't bother trying the RAM value
					if (theErr == MN_OK){
						// Set RAM shadow
						theErr = netSetParameterInfo(theMultiAddr, 
												 initData[i].paramNum, 
												 initData[i].value, TRUE);
						//In this case, we don't really care if the RAM value took
						theErr = MN_OK;
					}
					break;
				default:
					// We don't do anything with items not defined yet
					_RPT1(_CRT_WARN, "Factory default item not processed: type %d\n",
								initData[i].type);
					break;
			}
#if 0
			{
				iscOpStateReg ops; cnErrCode e;
				// TODO debug
				e = iscGetAlertReg (theMultiAddr, &ops); 
				if(e==MN_OK && ops.bit.runtimeErr)  {
					double pVal;
					e = iscGetParameter(theMultiAddr, ISC_P_LAST_RUNTIME_ERR, &pVal);
					_RPT2(_CRT_WARN, "ISC factory defaults +> runtime error @ %d Code=%d\n",
									 initData[i].paramNum|PARAM_OPT_MASK,(nodelong)pVal);				
				}
			}
#endif
		}
		if (theErr != MN_OK && theErr != MN_ERR_CMD_ARGS) {
			_RPT2(_CRT_WARN, "ISC factory defaults: failed param#%d, err=0x%x\n",
							 initData[i].paramNum, theErr);				
			return(theErr);
		}
	}

	// Clear out the user ID
	theErr = iscSetUserID(theMultiAddr, "");
	if (theErr != MN_OK) {
		return(theErr);
	}
    // Clear out the motor filename
	theErr = iscSetMotorFileName(theMultiAddr, "");
	if (theErr != MN_OK) {
		return(theErr);
	}
    
	// Clear the user managed storage
	buf.Byte.BufferSize = 13;
	for (i=0; i<4; i++) {
		if ((theErr = iscSetParameterEx(theMultiAddr, 
									   (iscParams)(ISC_P_USER_DATA_NV0+i), 
									   &buf)) != MN_OK)
			return(theErr);
	}

	// Setup the monitor port
	iscMonState newMonState;
	newMonState.var = static_cast<iscMonVars>(MON_TRQ_CMD | MON_SAVE_NV_MASK);
	newMonState.gain = 100;
	newMonState.filterTC = 0;
	newMonState.tuneSync = ISC_MON_SYNC_POS_START;
    if ((theErr = iscSetMonitor(theMultiAddr, 0, &newMonState)) != MN_OK)
		return(theErr);
		
	// Clear the PLA if we have it
	if (nodeVers >= FW_MILESTONE_IEX) {
		if ((theErr = netPLAclear(theMultiAddr)) != MN_OK) {
			return(theErr);
		}
		// Initialize Ld to Mtr ratio to 1:1
		if ((theErr = iscSetLdMtrRatio(theMultiAddr, 1, 1)) != MN_OK){
			return(theErr);
		}
	}

	if (opts.DrvFld.HwPlatform == ISC9x_300V_DC_Large 
		|| opts.DrvFld.HwPlatform == ISC9x_300V_DC_Mid) {
		
		if ((theErr = netSetParameterInfo(theMultiAddr, ISC_P_DRV_BUS_VOLTS, 
										  300, TRUE)) != MN_OK)
			return(theErr);
	}
	
	// Acknowledge firmware & configuration update
    if ((theErr = netSetParameterInfo(theMultiAddr, ISC_P_EE_UPD_ACK, 
										  eeVer, TRUE)) != MN_OK)
		return(theErr);
    if ((theErr = netSetParameterInfo(theMultiAddr, ISC_P_ROM_SUM_ACK, 
										  romSum, TRUE)) != MN_OK)
		return(theErr);
    
	// Force node to reset and re-init parameter DB
	{
		double pVal2;
		iscGetParameter(theMultiAddr, ISC_P_TIMESTAMP, &pVal2);
	}
	theErr=iscInitialize(theMultiAddr);
	// Got through here OK
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscGetUserID
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
MN_EXPORT cnErrCode MN_DECL iscGetUserID(
		multiaddr theMultiAddr,
		char *pUserIdStr,
		Uint16 maxBufSize)
{
	return netGetUserID(theMultiAddr, pUserIdStr, maxBufSize);
}
//																 			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		iscSetUserID
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
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetUserID(
		multiaddr theMultiAddr,
		const char *pNewName)
{
	return netSetUserID(theMultiAddr, pNewName);
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		iscGetMotorFileName
//
//	DESCRIPTION:
/**
	Get the motor file name for this node. This information is normally
	maintained by the motor file loading API.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pMotorFileNameStr Ptr to result area.
	\param[in] maxBufSize Maximum size of the \a pMotorFileNameStr buffer.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetMotorFileName(
		multiaddr theMultiAddr,
		char *pMotorFileNameStr,
		Uint16 maxBufSize)
{
	packetbuf nameBuf, cmdBuf;
	cnErrCode theErr;
	size_t i;
	
	cmdBuf.Byte.Buffer[CMD_LOC] = ISC_CMD_MOTOR_FILE;
	for(i=CMD_LOC+1; i<(MN_FILENAME_SIZE+CMD_LOC+1); i++)
		cmdBuf.Byte.Buffer[i] = 0;
	cmdBuf.Fld.Addr = theMultiAddr;
	cmdBuf.Fld.PktLen = 1;

	theErr = netRunCommand(NET_NUM(theMultiAddr), &cmdBuf, &nameBuf);
	
	if (theErr == MN_OK) {
		// Limit return size to buffer size
		i = (nameBuf.Fld.PktLen < maxBufSize) 
			? nameBuf.Fld.PktLen : maxBufSize;
		strncpy((char *)&pMotorFileNameStr[0], 
				(const char *)&nameBuf.Byte.Buffer[CMD_LOC], i);
		pMotorFileNameStr[i] = 0;				// Insure term'd
	}
	else {
		pMotorFileNameStr[0] = 0;
	}
	return(theErr);
}
//																			   *
//******************************************************************************


/// \cond INTERNAL_DOC
//******************************************************************************
//	NAME																	   *
//		iscSetMotorFileName
//
//	DESCRIPTION:
/**
	Set the motor file name for this node. 

	\param[in] theMultiAddr The address code for this node.
	\param[out] pNewName Ptr null terminated ANSI string up to 25 characters in
				length. 

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscSetMotorFileName(
		multiaddr theMultiAddr,
		const char *pNewName)
{
	packetbuf nameBuf, respBuf;
	size_t i;
	size_t nSize = strlen(pNewName);
	cnErrCode theErr = MN_OK;
	
	if (pNewName == NULL) 
		return(MN_ERR_BADARG);
		
	if (nSize > MN_FILENAME_SIZE) nSize = MN_FILENAME_SIZE;

	// Clear out buffer with 0 padding
	for(i=nSize; i<MN_FILENAME_SIZE; i++)
		nameBuf.Byte.Buffer[i+CMD_LOC+1] = 0;

	nameBuf.Byte.Buffer[CMD_LOC] = ISC_CMD_MOTOR_FILE;
	// Cmd Byte + Motor File Octets
	nameBuf.Fld.PktLen = 1+MN_FILENAME_SIZE;
	nameBuf.Fld.Addr = theMultiAddr;
	strncpy((char *)&nameBuf.Byte.Buffer[CMD_LOC+1], 
			(char *)&pNewName[0], nSize);

	theErr = netRunCommand(NET_NUM(theMultiAddr), &nameBuf, &respBuf); 

	return(theErr);
}
//																			   *
//******************************************************************************
/// \endcond


//******************************************************************************
//	NAME																	   *
//		iscGetIEXStatus
//
//	DESCRIPTION:
/**
	Get the current IEX status info.  This includes its operation state as well
	as diagnostic information on the health of the IEX link.

	\param[in] theMultiAddr The address code for this node.
	\param[out] pIexStatus	Ptr to the returned status area.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscGetIEXStatus(
			multiaddr theMultiAddr,			// Node address
			iexStatusReg *pIexStatus)		// Current setting
{
	cnErrCode theErr;
	paramValue paramVal;

	theErr = iscGetParameterEx(theMultiAddr, ISC_P_IEX_STATUS, &paramVal, NULL);
	// Overlay buffer with struct
	register iexStatusReg *pBuf 
		= reinterpret_cast<iexStatusReg *>(&paramVal.raw.Byte.Buffer[0]);
	*pIexStatus = *pBuf;
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscRestartIEX
//
/**
	Restart the IEX feature. This typically required after an IEX link 
	disruption.

	\param[in] theMultiAddr The address code for this node.

	\return
		#cnErrCode; MN_OK if successful
**/
MN_EXPORT cnErrCode MN_DECL iscRestartIEX(
			multiaddr theMultiAddr)	// Target node addr
{
	cnErrCode theErr = MN_OK;
	iscHwConfigReg hwConfig;
	int readCnt;

	//Get the current HW Config Register
	theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
	if (theErr != MN_OK)
		return(theErr);

	//Tell the IEX to stop
	hwConfig.Fld.IEXstop = 1;
	theErr = iscSetHwConfigReg(theMultiAddr, hwConfig);
	if (theErr != MN_OK)
		return(theErr);

	//Make sure it took
	readCnt = 0;
	theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
	if (theErr != MN_OK)
		return(theErr);
	while (hwConfig.Fld.IEXstop == 0){
		//only try for a few times
		if (++readCnt > 3)
			return MN_ERR_CMD_CMD_UNK;
		theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
		if (theErr != MN_OK)
			return(theErr);
	}

	//Tell the IEX to get going again
	hwConfig.Fld.IEXstop = 0;

	//Write the new config to the drive
	theErr = iscSetHwConfigReg(theMultiAddr, hwConfig);
	if (theErr != MN_OK)
		return(theErr);

	//make sure it took
	readCnt = 0;
	theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
	if (theErr != MN_OK)
		return(theErr);
	while (hwConfig.Fld.IEXstop == 1){
		//only try for a few times
		if (++readCnt > 3)
			return MN_ERR_CMD_CMD_UNK;
		theErr = iscGetHwConfigReg(theMultiAddr, &hwConfig);
		if (theErr != MN_OK)
			return(theErr);
	}

	return theErr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscForkVelMove
//
//	HISTORY
//      Created 11/18/1998 10:17:56 AM
//		adopted VB on 08/16/1999 14:04:40
//
//	DESCRIPTION:
/**
	Sends a move that ramps up or down in speed to \a velTargetStepPerSec 
	(using the previously set limits for acceleration and jerk [RAS]).

	Program execution does not suspend.  Once target velocity is achieved, the
	move stays there until explicitly changed. The velocity is specified
	as steps/second.

	Setting the \a triggered parameter true will cause the move to await
	a trigger event before starting.

	\see \ref ISCtriggerRefPage Starting Triggered Moves

	\param[in] theMultiAddr The address code for this node.
	\param[in] velTargetStepPerSec Target velocity in encoder steps per second
		units.
	\param[in] triggered Set this parameter to download the move and await a 
		trigger event.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkVelMove(
			multiaddr theMultiAddr,		// Destination node
			double velTargetStepPerSec,	// Target velocity (steps/sec)
			nodebool triggered)			// Start move with trigger
{

	return(iscForkMoveVelEx(theMultiAddr, velTargetStepPerSec, 0,
					triggered ? MG_MOVE_VEL_STYLE_TRIG : MG_MOVE_VEL_STYLE));
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		iscForkSkylineMove
//
//	DESCRIPTION:
/**
	This function allows access to the skyline move command. This function
	does not block host execution.

    \copydetails ISCmoveSkylinePage

	\see \ref ISCmoveDonePage
	\see #mnStatusReg Node Status Register

	\param[in] theMultiAddr The address code for this node.
	\param[in] velTargetStepPerSec Target velocity magnitude in encoder 
		steps per second units.
	\param[in] positionSpec End of the velocity segment position target. This
		is either an offset from the current commanded position at the start 
		of the move or an absolute position depending on the \a moveType.
	\param[in] moveType This parameter selects the move style and optional
		modes, such as triggering and positional value processing.
	\param[out] pBuffersRemaining Pointer to location updated with the move
		buffer count remaining.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkSkylineMove(
			multiaddr theMultiAddr,			// The node
			double velTargetStepPerSec,		// Target velocity (steps/sec)
			nodelong positionSpec,				// Position Target
			mgVelStyle moveType,			// Enhanced style selection
			nodelong *pBuffersRemaining)		// Ptr to move buf remaining cnt
{
	packetbuf cmd, resp;			// Input and output buffers
	netaddr cNum;
	double velScale, velTarg;
	nodelong iVelTarg;
	cnErrCode theErr = MN_OK;

	if (!pBuffersRemaining)
		return(MN_ERR_BADARG);

	// Get sample rate as basis for ticks/sample
    theErr = iscGetParameter(theMultiAddr, ISC_P_SAMPLE_PERIOD, &velScale);
	if (theErr == MN_OK) {
		// TODO convert to use most net efficient form of command
		cNum = coreController(theMultiAddr);
		cmd.Fld.Addr = theMultiAddr;	
		// Len=cmd byte, 4 bytes target vel, mode byte
		cmd.Fld.PktLen = 1 + sizeof(iVelTarg) + 1;
		cmd.Byte.Buffer[CMD_LOC] = ISC_CMD_MOVE_VEL_EX;
		velTarg = (nodelong)(0.5 + (velTargetStepPerSec * ISC_VEL_MOVE_SCALE * velScale 
									   / 1000000.));
		// Saturate test
		/// \cond INTERNAL_DOC
		#define MAX_VEL_Q 31
		/// \endcond
		if (velTarg > (1LL<<MAX_VEL_Q)-1)
			velTarg = (1LL<<MAX_VEL_Q)-1;
		else if(velTarg < (-1LL<<MAX_VEL_Q))
			velTarg  = -1LL<<MAX_VEL_Q;
		// Save as long integer bits
		iVelTarg = (nodelong)velTarg;
		// copy the position target into the command buffer
		memcpy(&cmd.Byte.Buffer[CMD_LOC+1], &iVelTarg, sizeof(iVelTarg));
		// Copy move type over
		cmd.Byte.Buffer[CMD_LOC+5] = (nodechar)moveType.styleCode;
		// If positional involved
		if (positionSpec) {
			memcpy(&cmd.Byte.Buffer[CMD_LOC+6], &positionSpec, 
											    sizeof(positionSpec));
			cmd.Fld.PktLen += sizeof(positionSpec);
		}
		// Run the command and await the response
		theErr = netRunCommand(cNum, &cmd, &resp);
		// Extract off the remaining buffers
		*pBuffersRemaining = resp.Byte.Buffer[RESP_LOC];
	}
	return(theErr);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscForkMoveEx
//
//	DESCRIPTION:
/**
	This is the main function to initiate a positional move with 
	no buffer management. The motion is constrained to the settings of
	the acceleration, velocity and jerk limits.  For the more
	complex head and tail moves the additional constraints of head and
	tail distances as well as head/tail velocity limits apply. Once this
	move has been accepted, any constraints values changed will
	apply only to the next move segment.

	The \a moveType field is most easily supplied a #mgPosnStyles enumeration
	type. This #mgPosnStyle provides a copy constructor to allow direct
	use of this enumeration.

	A C example for issuing a head/tail absolute position move.
	\code
	theErr = iscForkMoveEx(m_nodeAddr, target, MG_MOVE_STYLE_HEADTAIL_ABS);
	\endcode 

	\see \ref mgPosnStyles for information on the various profiles.
	\see \ref ISCtriggerRefPage Starting Triggered Moves

	\param[in] theMultiAddr The address code for this node.
	\param[in] posnTarget Target position. If this is a relative type move 
		then this is the increment from the initial position. If this is 
		and absolute move, then this is the final location.
	\param[in] moveType The move style. This is a bit oriented value that
		specifies the profile, triggering event handling and the handling 
		of the target.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkMoveEx(	// Move with no buffer management
			multiaddr theMultiAddr,			// Destination node
			nodelong posnTarget,			// Target (steps)
			mgPosnStyle moveType)
{
	mgMoveProfiledInfo spec;
	spec.value = posnTarget;
	spec.type = moveType.styleCode;
	return(iscMoveProfiled(theMultiAddr, &spec));
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		iscForkMoveQueued
//
//	DESCRIPTION:
/**
	This is the main function to initiate a positional moves and inquire
	on how many more pending moves the node will accept. 

	The motion is constrained to the settings of the acceleration, velocity
	and jerk limits.  For the more complex head and tail moves the additional
	constraints of head and tail distances as well as head/tail velocity
	limits apply. Once this move has been accepted, any constraints values
	changed will apply only to the next move segment.

	This function is similar to the #iscForkMoveEx function and only
	differs from it in that the remaining buffer level is returned on
	each call. This facilitates the queuing of additional moves.

	The \a moveType field is most easily supplied a #mgPosnStyles enumeration
	type. This #mgPosnStyle provides a copy constructor to allow direct
	use of this enumeration.

	A C example for issuing a head/tail absolute position move.
	\code
	theErr = iscForkMoveQueued(m_nodeAddr, target, MG_MOVE_STYLE_HEADTAIL_ABS, &nMovesLeft);
	\endcode 

	\see \ref mgPosnStyles for information on the various profiles.
	\see \ref ISCtriggerRefPage Starting Triggered Moves

	\remark If a triggered move is accepted in a "stream" of other non-triggered
	moves, it will block upon its execution until a new trigger event
	occurs.

    \note This feature is only available on firmware version 5.5 and later.

	\param[in] theMultiAddr The address code for this node.
	\param[in] posnTarget Target position. If this is a relative type move 
		then this is the increment from the initial position. If this is 
		and absolute move, then this is the final location.
	\param[in] moveType The move style. This is a bit oriented value that
		specifies the profile, triggering event handling and the handling 
		of the target.
	\param[out] pBuffersRemaining Pointer to buffers remaining count.
		Upon return the node will accept up to \a pBuffersRemaining more
		calls to this function.

	\return
		#cnErrCode; MN_OK if successful
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL iscForkMoveQueued(
			multiaddr theMultiAddr,			// Destination node
			nodelong posnTarget,				// Target (steps)
			mgPosnStyle moveType,			// Style code
			nodelong *pBuffersRemaining)		// Ptr to move buf remaining cnt
{
	mgMoveProfiledInfo spec;
	spec.value = posnTarget;
	spec.type = moveType.styleCode;
	return(iscMoveProfiledQueued(theMultiAddr, &spec, pBuffersRemaining));
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		iscGetDataCollected
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
MN_EXPORT cnErrCode MN_DECL iscGetDataCollected(
			multiaddr theMultiAddr,			// Destination node
			iscDataCollect *pReturnData)	// Returned data
{
	return (netGetDataCollected(theMultiAddr, pReturnData));
}
//																			  *
//*****************************************************************************


/// @}
// if ISC_DOC
/// \endcond
//==============================================================================
//	END OF FILE iscAPI.cpp
//==============================================================================
#endif

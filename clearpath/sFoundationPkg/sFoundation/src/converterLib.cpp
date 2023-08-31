//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/src/converterLib.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: converterLib.cpp $
/// \cond INTERNAL_DOC
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	\brief Useful functions to convert node's native units to standard
	values and back.

	These functions must utilize only common MN_P style parameters and
	the incoming referenced parameter for their function. This allows
	them to be used on any node class.
**/
//
// CREATION DATE:
//		2012-03-14 14:26:32
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 3-14  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	converterLib.cpp headers
//
	#include "converterLib.h"
	#include "netCmdPrivate.h"
	#include "netCmdAPI.h"
	#include <math.h>
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	converterLib.cpp function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	converterLib.cpp constants
//
//
#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	converterLib.cpp static variables
//
// 

//																			  *
//*****************************************************************************


//****************************************************************************
//	NAME
//		convertAcc
//
//	DESCRIPTION:
//		This function converts to and from ticks/second and ticks/sample-time.
//
//	RETURNS:
//		double
//
//	SYNOPSIS:
double MN_DECL convertAcc(				// ticks/sample-time <=> ticks/sec
	nodebool valIsBits, 				// TRUE=>ticks/sample-time, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData)
{
	paramValue sampleTime;				
	cnErrCode theErr;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_SAMPLE_PERIOD,
							      NULL, &sampleTime);
	// Avoid divide-by-0 problems and other errors
	if (theErr != MN_OK || sampleTime.value == 0)
		return(0);

	if (valIsBits)  {
		return(1e12 * convVal / (sampleTime.value * sampleTime.value));
	}
	else {
		// convVal in ms => us / # us/sample-time
		return(1e-12 * convVal * sampleTime.value * sampleTime.value);
	}
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		convertSquared
//
//	DESCRIPTION:
//		This function converts values represented as the square of their
//		actual value to and from its normal state.
//
//	RETURNS:
//		double
//
//	SYNOPSIS:
double MN_DECL convertSquared(			// sample-times <=> milliseconds
	nodebool valIsBits, 				// TRUE=>sample-times, FALSE=>ms
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData)
{
	if (valIsBits)  {
		if (convVal<0)
			return(0);
		// Return x^.5
		return(sqrt(convVal));
	}
	else {
		// Return x^2
		return(convVal*convVal);
	}
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		convertTimeMS
//
//	DESCRIPTION:
//		This function converts sample-times to milliseconds based on the
//		MN_SAMPLE_PERIOD parameter.
//
//	RETURNS:
//		double
//
//	SYNOPSIS:
double MN_DECL convertTimeMS(			// sample-times <=> milliseconds
	nodebool valIsBits, 				// TRUE=>sample-times, FALSE=>ms
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData)
{
	paramValue sampleTime;				
	cnErrCode theErr;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_SAMPLE_PERIOD,
							      NULL, &sampleTime);
	// Avoid divide-by-0 problems and other errors
	if (theErr != MN_OK || sampleTime.value == 0)
		return(0);

	if (valIsBits)  {
		// Return sample-times in milliseconds
		return(convVal * .001 * sampleTime.value);
	}
	else {
		// convVal in ms => us / # us/sample-time
		return((long)((1000 * convVal / sampleTime.value)+0.5));
	}
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		convertVel
//
//	DESCRIPTION:
//		This function converts to and from ticks/second and ticks/sample-time.
//
//	RETURNS:
//		double
//
//	SYNOPSIS:
double MN_DECL convertVel(				// ticks/sample-time <=> ticks/sec
	nodebool valIsBits, 				// TRUE=>ticks/sample-time, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData)
{
	paramValue sampleTime;				
	cnErrCode theErr;

	theErr = netGetParameterInfo(theMultiAddr, MN_P_SAMPLE_PERIOD,
							      NULL, &sampleTime);
	// Avoid divide-by-0 problems and other errors
	if (theErr != MN_OK || sampleTime.value == 0)
		return(0);

	if (valIsBits)  {
		return(1e6 * convVal / sampleTime.value);
	}
	else {
		// convVal in ms => us / # us/sample-time
		return(1e-6 * convVal * sampleTime.value);
	}
}
/****************************************************************************/



//****************************************************************************
//	NAME
//		convertVelRound
//
//	DESCRIPTION:
//		This function converts to and from ticks/second and ticks/sample-time.
//		Results are rounded with no decimals.
//		This is needed for 16 bit velocity parameters that has no bits for decimals
//
//	RETURNS:
//		double
//
//	SYNOPSIS:
double MN_DECL convertVelRound(				// ticks/sample-time <=> ticks/sec
	nodebool valIsBits, 				// TRUE=>ticks/sample-time, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData)
{
	double ReturnDouble;

	// Call the original convertVel function
	ReturnDouble = convertVel(valIsBits,
		theMultiAddr,
		parameter,
		convVal,
		nodeData);
	
	// Round the value
	if(ReturnDouble-floor(ReturnDouble)>=0.5)
		ReturnDouble = ceil(ReturnDouble);
	else
		ReturnDouble=floor(ReturnDouble);

	return ReturnDouble;
}
/****************************************************************************/


//****************************************************************************
//	NAME
//		convert2To27
//
//	DESCRIPTION:
//		Convert Ke to properly scaled value when using a linear motor.
//
//	\return
//		double
//
//	SYNOPSIS:
double MN_DECL limit2To27(
	nodebool valIsBits, 				// TRUE=>drive bits, FALSE=> app bits
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// To/From value
	byNodeDB *pNodeDB)					// Parameter information
{
	if (valIsBits) {
		return(convVal);
	}
	else {
		//  Limit to 2^26 range
		if (convVal > ((1<<26)-1))
			return ((1<<26)-1);
		else if (convVal < (-1<<26))
			return (-1<<26);
	}
	return(convVal);
}
/****************************************************************************/


//*****************************************************************************
//	NAME																	  *
//		cleanForXML
//
//	DESCRIPTION:
/**
	This function will replace any troublesome XML characters with "_"
	characters.

	\param[in,out] Pointer to original string.
**/
void cleanForXML(char *pInputStr) {
	char *pch;
	pch = pInputStr;
	// Kill control characters
	while (*pch) {
		if (*pch < ' ')	{
			*pch = ' ';
		}
		else if(*pch >= 0x7f) {
			*pch = '_';
		}
		else {
			switch (*pch) {
			case '<':
			case '>':
			case '&':
			case '\'':
			case '\"':
				*pch = '_';
				break;
			default:
				// Is OK as-is
				break;
			}
		}
		pch++;
	}
}
//																			  *
//*****************************************************************************

/// \endcond INTERNAL_DOC

//============================================================================= 
//	END OF FILE converterLib.cpp
//=============================================================================

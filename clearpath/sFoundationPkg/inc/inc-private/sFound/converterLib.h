//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc-private/converterLib.h $
// $Date: 01/19/2017 17:39 $
// $Workfile: converterLib.h $
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	\brief Brief Module Description

	
**/
// CREATION DATE:
//		2012-03-14 14:27:44
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
#ifndef __CONVERTERLIB_H__
#define __CONVERTERLIB_H__


//*****************************************************************************
// NAME																          *
// 	converterLib.h headers
//
	#include "tekTypes.h"
	#include "pubMnNetDef.h"
	#include "mnParamDefs.h"
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	converterLib.h function prototypes
//
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Common unit converters
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MN_DECL convertAcc(				// ticks/sample-time^2 <=> ticks/sec^2
	nodebool valIsBits, 				// TRUE=>scaled, FALSE=>unscaled
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData);

double MN_DECL convertSquared(			// x^2 <=> x
	nodebool valIsBits, 				// TRUE=>scaled, FALSE=>unscaled
	multiaddr multiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData);

double MN_DECL convertTimeMS(			// sample-times <=> milliseconds
	nodebool valIsBits, 				// TRUE=>sample-times, FALSE=>ms
	multiaddr multiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData);

double MN_DECL convertVel(				// ticks/sample-time <=> ticks/sec
	nodebool valIsBits, 				// TRUE=>ticks/sample-time, FALSE=>unscaled
	multiaddr multiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData);

// rounded for vel with no decimal places
double MN_DECL convertVelRound(			// ticks/sample-time <=> ticks/sec 
	nodebool valIsBits, 				// TRUE=>ticks/sample-time, FALSE=>unscaled
	multiaddr multiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// From value
	byNodeDB *nodeData);

double MN_DECL limit2To27(
	nodebool valIsBits, 				// TRUE=>drive bits, FALSE=> app bits
	multiaddr theMultiAddr,				// Node address
	appNodeParam parameter,				// Target parameter
	double convVal,						// To/From value
	byNodeDB *pNodeDB);

// String utility functions
void cleanForXML(char *pInputStr);
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	converterLib.h constants
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	converterLib.h static variables
//
// 

//																			  *
//*****************************************************************************



#endif
//============================================================================= 
//	END OF FILE converterLib.h
//=============================================================================

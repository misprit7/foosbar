//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubIex.h $
// $Revision: 16 $ $Date: 12/09/16 3:53p $
// $Workfile: pubIex.h $
//
// DESCRIPTION:
/**
	\file
	\brief IEX constants and data types.

	The IEX programming interface consists of an <i>IEX Status Register</i>
	and a number of enumerated constants. The status register allows
	the user application to evaluate the status and reliability of the 
	IEX modules and their wiring.
**/
// CREATION DATE:
//		9/13/2010 08:16:15 AM
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2010 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//
//*****************************************************************************
#ifndef __PUBIEX_H__
#define __PUBIEX_H__


//*****************************************************************************
// NAME																	      *
// 	pubIex.h headers
//
	#include "tekTypes.h"
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	pubIex.h function prototypes
//

//																			  *
//*****************************************************************************




//*****************************************************************************
/**
	\brief IEX Link Operational State

	The status register maintains the IEX subsystem's operational state
	via these enumerated constants.
**/
typedef enum _iexLinkState {
	/// Searching for IEX module(s)
	IEX_LINK_SEARCH,				// 0	
	/// Power stability wait
	IEX_LINK_WAIT,					// 1 
	/// Testing IEX modules
	IEX_LINK_TEST,					// 2 
	/// Modules online working
	IEX_LINK_ONLINE,				// 3 
	/// Module link broken
	IEX_LINK_BROKEN,				// 4 
	/// IEX modules stopped
	IEX_LINK_STOPPED				// 5 
} iexLinkState;	///< A typename for enum _iexLinkState


//																			  *
//*****************************************************************************


//*****************************************************************************
/**
	\brief IEX Status Register. 

	This information is returned by the device's IEX status parameter. It
	contains the operational status and quality of the IEX subsytem. The
	glitch registers will be non-zero when there is poor cabling or extreme
	levels of electromagnetic interference on the IEX link.
**/
#ifdef DOXYGEN_SCAN
typedef union _iexStatusReg {
#else
typedef PACK_BYTES union _iexStatusReg {
#endif
	/// This is the bit-wise interface
	Uint16 bits[4];					
	/// This is the field access to the status information
	struct {
		/** \brief Count of IEX devices found.

		This field contains the number of detected IEX modules attached
		to this node.
		**/
		unsigned deviceCnt			: 3;	// 0-2
		/** \brief IEX not configured properly.
		
		The detected count of IEX modules does not match the expected count
		or too many IEX modules were detected on the expansion link.
		**/
		unsigned configWarning		: 1;	// 3
		/** \brief IEX Operational state.
		
		This field contains the operational state of the IEX system. 
		Interactions when the link is not in the IEX_LINK_ONLINE state
		will return an error.
		**/
		iexLinkState state			: 4;	// 4-7
		// reserved
		unsigned na8_15				: 8;	// 8-15
		/** \brief Count of glitches detected.

		This field will contain the count of link glitches detected since
		the last read. It clears on read. Excessive errors will cause this
		field to saturate at 32767.
		**/
		unsigned glitchCount		: 16;	// 16-31
		/** \brief Maximum count of consecutive glitches.
		
		This field will contain the highest count of sequential link glitches
		detected since the last read. It clears on read. Excessive errors will
		cause this field to saturate at 32767.
		**/
		unsigned glitchPeak			: 16;	// 32-47
		// reserved
		Uint16	 spare;						// 48-63
	} Fld;									///< Field access to IEX status register
} iexStatusReg;								///< IEX Status Register Type
//																			  *
//*****************************************************************************

#endif //__PUBIEX_H__
// =============================================================================
//	END OF FILE pubIex.h
// =============================================================================


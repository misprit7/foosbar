//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc-private/iscRegs.h $
// $Date: 01/19/2017 17:39 $
// $Workfile: iscRegs.h $
//
/// \file
/// \brief C API: Private Meridian data structures
//
// NAME
//		iscRegs.h
//
// DESCRIPTION:
///		This header file defines the private SST node registers.
///		They handle all the drive and power supply products.
//
// CREATION DATE:
//		12/23/2010 11:46:39
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2010 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			   *
//******************************************************************************
#ifndef __ISCREGS__H__
#define	__ISCREGS__H__

//******************************************************************************
// NAME																		   *
// 	iscRegs.h constants

// Introduction of stepper, iex, enhanced RAS, PLA
#define FW_MILESTONE_IEX 0x5200		
// Had link voltage low counter
#define FW_MILESTONE_LINK_LOW 0x5203
// Had vector stepper
#define FW_MILESTONE_VECTOR_STEEPER 0x5300
// Had Software limits
#define FW_MILESTONE_SOFT_LIMITS 0x5400
// Had 32-bit RMS limit
#define FW_MILESTONE_RMS_LIM_32 0x5403
// Had skyline moves
#define FW_MILESTONE_SKYLINE_MOVE 0x5500
//																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
//  HwPlatform enum for the optionReg union
//
// DESCRIPTION
/**
	\brief Hardware platform enum for the option control register field.

	Identifies the hardware configuration of the drive.
**/
//
typedef enum _HwPlatform {
	ISC9x_75V_DC,			//4xx and 5xx
	ISC9x_300V_DC_Large,	//75x and 76x
	ISC9x_300V_DC_Mid,		//60x, 62x, 72x, and 73x
	ISC9x_75V_DC_Backplane	//2xx and 3xx
} HwPlatform;
//																			   *
//******************************************************************************

//******************************************************************************
// NAME																		   *
//  Structure for maintaining the monitor port's state.
//
// DESCRIPTION
/**
	\brief Hardware platform enum for the option control register field.

	Identifies the hardware configuration of the drive.
**/
//
typedef struct _monState {
	bool		Set;					// Set if we have current data
	iscMonVars	TestPoint;				// Current test point
	double		FullScale;				// Full scale value for this TestPoint
	_monState() {
		Set=false;
		TestPoint = MON_TRQ_CMD;
		FullScale = 0;
	}
} monState;
//																			   *
//******************************************************************************

// Place to put any by-node state in
typedef struct _iscState {
	monState	MonState;				// Monitor state information
} iscState;


//******************************************************************************
// NAME																		   *
// 	iscRegs.h function prototypes
//
// DESCRIPTION
/**
	\brief Internal prototypes.

	Identifies the hardware configuration of the drive.
**/
//
// Initialize the parameter local copy of the parameter table
cnErrCode iscInitialize(
			multiaddr theMultiAddr);

cnErrCode iscInitializeEx(
			multiaddr theMultiAddr,
			nodebool warmInitialize);
//																			 *
//****************************************************************************

#endif
//============================================================================
//	END OF FILE iscRegs.h
//============================================================================

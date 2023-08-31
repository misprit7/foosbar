//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubMonPort.h $
// $Revision: 12 $ $Date: 12/09/16 4:02p $
// $Workfile: pubMonPort.h $
//
/// \file
/// \brief Monitor Port types and constants
//
// NAME
//		pubMonPort.h
//
// DESCRIPTION:
///		Public monitor port definitions.
//
// CREATION DATE:
//		10/08/2009 16:24:52
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2009 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//
//*****************************************************************************
#ifndef __PUBMONPORT_H__
#define	__PUBMONPORT_H__


//*****************************************************************************
// NAME																	      *
// 		pubMonPort.h channel definitions
//
// DESCRIPTION
// 		Supported channel for data acquisition and the monitor port
typedef enum _monTestPoints {
    MON_INTERNAL0 = 0,              // 0 : internal mode
    MON_INTERNAL1 = 1,              // 1 : internal mode
    MON_VEL_MEAS,                   // 2 : Velocity, measured (steps/ms)
    MON_VEL_CMD,                    // 3 : Velocity, commanded (steps/ms)
    MON_VEL_STEP,                   // 4 : Velocity, input (steps/ms)
    MON_VEL_TRK,                    // 5 : Velocity, tracking (steps/ms)
    MON_POSN_TRK,                   // 6 : Position, tracking (quad counts)
    MON_TRQ_MEAS,                   // 7 : Torque, measured (% of max)
    MON_TRQ_CMD,                    // 8 : Torque, commanded (% of max)
    MON_CALIBRATE,                  // 9 : Stimulus generator (% of max)
    MON_ANALOG_IN,                  // 10: Analog input (% of max)
    MON_SINE_R,                     // 11: Vector R sin (% of max)
    MON_COS_R,                      // 12: Vector R cos (% of max)
    MON_VOLT_D,                     // 13: Volts D (% of max)
    MON_VOLT_Q,                     // 14: Volts Q (% of max)
    MON_SGN_CMD_VEL,                // 15: SGN(cmd vel)
    MON_SGN_CMD_STEP,               // 16: SGN(step vel)
    MON_INTEGRATOR,                 // 17: Integrator
    MON_POSN_MEAS,                  // 18: Position, measured (quad counts)
    MON_VEL_TRK_SERVO,              // 19: Servo velocity tracking (steps/ms)
    MON_JRK_CMD,                    // 20: Jerk, commanded (Mquads/sec^3)
    MON_ACC_CMD,                    // 21: Acceleration, commanded (quads/sec^2)
    MON_TRQ_D_MEAS,                 // 22: Torque, measured D (% of max)
    MON_VOLTS_MAX,                  // 23: Maximum phase voltage (% of max)
    MON_NOT_INRANGE,                // 24: Not In-Range
    MON_NOT_MOVEDONE,               // 25: Not Move Done
    MON_NOT_HLFB,                   // 26: Not HLFB
    MON_POSN_DIR_TRK,				// 27: Directional Tracking, Load
    MON_TRQ_TRK,                    // 28: Torque tracking
    MON_COUPLING,   	            // 29: Motor-Load coupling
    MON_POSN_DIR_TRK_MTR,			// 30: Directional Tracking, Motor
    MON_POSN_TRK_MTR,				// 31: Tracking, Motor

    MON_TRK_LD=33,					// 33: Tracking, Load
    MON_TRQ_TRK_PEAK,				// 34: Peak Torque Tracking
    MON_TRQ_MEAS_PEAK,				// 35: Peak Torque Measured
    MON_BUS_VOLTS,					// 36: Bus Voltage
    MON_SHOW_HIGH_PASS = 8192,		// Show high pass monitor port data
	MON_SAVE_NV_MASK = 16384,		// OR with testpoint to save in Non-Volatile
	MON_OPTION_MASKS = MON_SHOW_HIGH_PASS|MON_SAVE_NV_MASK
} monTestPoints;

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 		pubMonPort.h tune sync sources
//
// DESCRIPTION
// 		Supported channel for data acquisition and the monitor port
typedef union _tuneSyncs {
	unsigned bits;
	struct {
		int PosStart	: 1;				// Pulse at start of pos cmd
		int NegStart	: 1;				// Pulse at start of neg cmd
		int PosEnd		: 1;				// Pulse at end of pos cmd
		int NegEnd		: 1;				// Pulse at end of neg cmd
		int OnShutdown	: 1;				// Pulse at shutdown	
	} Fld;
} tuneSyncs;
//																			  *
//*****************************************************************************

#endif
//=============================================================================
//	END OF FILE pubMonPort.h
//=============================================================================

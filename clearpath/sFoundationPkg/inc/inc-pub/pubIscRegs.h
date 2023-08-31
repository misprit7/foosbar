//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/inc/pubIscRegs.h $
// $Revision: 104 $ $Date: 12/13/16 12:15p $
// $Workfile: pubIscRegs.h $
//
// DESCRIPTION:
//
/**
    \file
    \brief Register definitions and constants related to the Meridian ISC.

    This header file describes the register set and parameters used by the ISC.


    \ingroup ISCgrp
    @{
**/
//		The ISC registers are defined with an extra layer for backwards compatibility
//		with the existing code base.
//
// CREATION DATE:
//      03/13/1998 13:29:19
//
// COPYRIGHT NOTICE:
//      (C)Copyright 1998-2010  Teknic, Inc.  All rights reserved.
//
//      This copyright notice must be reproduced in any copy, modification,
//      or portion thereof merged into another program. A copy of the
//      copyright notice must be included in the object library of a user
//      program.
//                                                                            *
//*****************************************************************************


#ifndef _PUBISCREGS_H_
#define _PUBISCREGS_H_

//*****************************************************************************
// NAME                                                                       *
//  pubIscRegs.h headers
//
#include "tekTypes.h"
#include "pubMnNetDef.h"
#include "pubRegFldCommon.h"
#ifndef __TI_COMPILER_VERSION__     // Not needed on dsp
#include "pubMotion.h"

#include "pubMonPort.h"
#endif


//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  pubIscRegs.h constants
//

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
    \brief ISC Parameter Numbers.
    \public

    This enumeration defines the set of internal ISC registers accessible by
    the #iscGetParameter and #iscSetParameter functions.
**/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
enum _iscParams {
    ISC_P_NODEID,                   // 0  00 Device ID
    ISC_P_FW_VERSION,               // 1  01 FW version
    ISC_P_HW_VERSION,               // 2  02 HW version
    ISC_P_RESELLER_ID,              // 3  03 Reseller ID
    ISC_P_SER_NUM,                  // 4  04 Unit serial number
    ISC_P_OPTION_REG,               // 5  05 Unit options
    ISC_P_ROM_SUM_ACK,              // 6  06 Firmware update ack
    ISC_P_ROM_SUM,                  // 7  07 Firmware checksum
    ISC_P_SAMPLE_PERIOD,            // 8  08 Sample period (nsec)
    ISC_P_ALERT_REG,                // 9  09 Shutdown register
    ISC_P_STOP_TYPE,                // 10 0a Node Stop Type
    ISC_P_WATCHDOG_TIME,            // 11 0b Watchdog time constant
    ISC_P_NET_STAT,                 // 12 0c Network Status
    ISC_P_STATUS_ACCUM_REG,         // 13 0d Status accum register
    ISC_P_STATUS_ATTN_RISE_REG,     // 14 0e Status attn/rise register
    ISC_P_STATUS_ATTN_MASK,         // 15 0f Status mask register
    ISC_P_STATUS_RT_REG,            // 16 10 Status reg (realtime)
    ISC_P_TIMESTAMP,                // 17 11 8-bit timestamp
    ISC_P_TIMESTAMP16,              // 18 12 16-bit timestamp
    ISC_P_PART_NUM,                 // 19 13 Part Number String
    ISC_P_EE_UPD_ACK,               // 20 14 EE Update Acknowlegde
    ISC_P_EE_VER,                   // 21 15 EE Version Number
    ISC_P_STATUS_FALL_REG,          // 22 16 Status fall register
    ISC_P_HW_CONFIG_REG,            // 23 17 Hardware config/setup reg
    ISC_P_APP_CONFIG_REG,           // 24 18 Feature config/setup reg
                                    // 25 19
                                    // 26 1a
                                    // 27 1b
                                    // 28 1c
                                    // 29 1d
    ISC_P_USER_IN_REG=30,           // 30 1e User input value
    ISC_P_IN_SRC_REG,               // 31 1f Input source
    ISC_P_OUT_REG,                  // 32 20 Output register
    ISC_P_OUT_RISE_REG,             // 33 21 Output rise edge
    ISC_P_OUT_FALL_REG,             // 34 22 Output fall edge
    ISC_P_CTL_STOP_OUT_REG,         // 35 24 Controlled output register
    ISC_P_USER_OUT_REG,             // 36 23 User output register
    ISC_P_OUT_PLA_SRC_REG,          // 37 25 Output source definition
    ISC_P_GP_TIMER_PERIOD,          // 38 26 Gen. purpose timer period
    ISC_P_PLA_OUT_REG,              // 39 27 PLA output
    ISC_P_IN_POLARITY_REG,          // 40 28 Input polarity register
                                    // 41 29
    ISC_P_POSN_CAP_INDEX=42,        // 42 2a Encoder index position
    ISC_P_POSN_CAP_GPI1,            // 43 2b Encoder capture with GPI1
    ISC_P_POSN_CAP_PLA,             // 44 2c Encoder capture with PLA
    ISC_P_POSN_CAP_INDEX_LD,        // 45 2d Load Encoder index position
    ISC_P_RAS_MAX_VELOCIY,          // 46 2e Maximum velocity for current RAS
    ISC_P_VEL_LIM=47,               // 47 2f Trapezoidal velocity limit
    ISC_P_ACC_LIM,                  // 48 30 Trapezoidal acceleration limit
    ISC_P_JERK_LIM,                 // 49 31 Jerk limit
    ISC_P_ACC_MAX,                  // 50 32 Test Point: Max Acc.
    ISC_P_POSN_TRIG_PT,             // 51 33 At Position location
    ISC_P_A_START,                  // 52 34 A after start point
    ISC_P_B_END,                    // 53 35 B before end point
    ISC_P_POSN_TRK_RANGE,           // 54 36 Tracking in range window
                                    // 55 37
    ISC_P_POSN_TRK=56,              // 56 38 Position tracking error
                                    // 57 39
                                    // 58 3a
    ISC_P_STOP_ACC_LIM = 59,        // 59 3b Stop acceleration limit
    ISC_P_POSN_MEAS_MTR,			// 60 3c Encoder 0 (motor) measured position
    ISC_P_POSN_MEAS = 61,           // 61 3d Measured position
    ISC_P_POSN_CMD,                 // 62 3e Commanded position
    ISC_P_VEL_CMD,                  // 63 3f Commanded velocity
    ISC_P_VEL_MEAS,                 // 64 40 Measured velocity
    ISC_P_DEC_LIM,                  // 65 41 Deceleration Limit
    ISC_P_HEAD_DISTANCE,            // 66 42 Head distance
    ISC_P_TAIL_DISTANCE,            // 67 43 Tail distance
    ISC_P_HEADTAIL_VEL,             // 68 44 Head/Tail velocity
    ISC_P_POSN_MEAS_LD,             // 69 45 Encoder 1 measured position
    ISC_P_WARN_REG,                 // 70 46 Warnings pending
    ISC_P_WARN_MASK,                // 71 47 Warnings for attention
    ISC_P_ALERT_MASK,               // 72 48 Alert selection
									// 73 49
	ISC_P_MOVE_DWELL = 74,			// 74 4a Post move dwell time (sec)

    ISC_P_ON_TIME = 89,             // 89 59 Unit On Time
    ISC_P_USER_RAM0,                // 90 5a User RAM parameter
    ISC_P_USER_DATA_NV0,            // 91 5b User NV (13-bytes)
    ISC_P_USER_DATA_NV1,            // 92 5c User NV (13-bytes)
    ISC_P_USER_DATA_NV2,            // 93 5d User NV (13-bytes)
    ISC_P_USER_DATA_NV3,            // 94 5e User NV (13-bytes)

    ISC_P_NETERR_APP_CHKSUM=104,    // 104 68 Application Net Checksum counter
    ISC_P_NETERR_APP_FRAG,          // 105 69 Application Net Fragment counter
    ISC_P_NETERR_APP_STRAY,         // 106 6a Application Net Stray data counter
    ISC_P_NETERR_APP_OVERRUN,       // 107 6b Application Net Overrun counter
    ISC_P_NETERR_DIAG_CHKSUM,       // 108 6c Diagnostic Net Checksum counter
    ISC_P_NETERR_DIAG_FRAG,         // 109 6d Diagnostic Net Fragment counter
    ISC_P_NETERR_DIAG_STRAY,        // 110 6e Diagnostic Net Stray data counter
    ISC_P_NETERR_DIAG_OVERRUN,      // 111 6f Diagnostic Net Overrun counter
    ISC_P_NETERR_LOW_VOLTS=116,     // 116 74 Link Voltage Low Event counter

    // BANK 1 items (Drive/Motor Configuration)
                                    // 256 00
    ISC_P_DRV_I_MAX=257,            // 257 01 Current Full Scale (A)
    ISC_P_DRV_RMS_MAX,              // 258 02 Factory RMS limit
    ISC_P_DRV_FACT_IR_CAL,          // 259 03 Ir Sensor Calibration
    ISC_P_DRV_FACT_IS_CAL,          // 260 04 Is Sensor Calibration
                                    // 261 05
    ISC_P_DRV_ADC_MAX=262,          // 262 06 ADC full scale (A)
    ISC_P_DRV_CONFIG_CHANGED,       // 263 07 Configuration changed
    ISC_P_DRV_VECTOR_RATE,          // 264 08 Encoder Dens. (/[turn/pole-pair])
    ISC_P_DRV_USTEPS_PER_REV,       // 265 09 Stepper uSteps (/[turn/pole-pair])
    ISC_P_DRV_ENC_DENS,             // 266 0a Encoder Dens. (/[turn/pole-pair])
    ISC_P_DRV_ENC_RES,              // 267 0b Encoder step distance (microns/.01)
    ISC_P_DRV_MTR_POLES,            // 268 0c Motor Pole Count
    ISC_P_DRV_MTR_KE,               // 269 0d Motor: Ke (V/KRPM)
    ISC_P_DRV_MTR_OHMS,             // 270 0e Motor: Resistance (ohm)
    ISC_P_DRV_MTR_ELEC_TC,          // 271 0f Motor: Elect Time Constant (ms)
    ISC_P_DRV_COM_RO,               // 272 10 Commutation Reference Offset
    ISC_P_DRV_RMS_LIM,              // 273 11 RMS shutdown limit (A)
    ISC_P_DRV_RMS_TC,               // 274 12 RMS time constant (sec) @ 100% trq
    ISC_P_DRV_MTR_SPEED_LIM,        // 275 13 Motor Speed Limit (quads/msec)
    ISC_P_DRV_COM_CAP,              // 276 14 Commutation Edge Capture
    ISC_P_DRV_COM_ANGLE,            // 277 15 Commutation Angle Reference (quads)
    ISC_P_DRV_KIP,                  // 278 16 Vector Torque: Kip
    ISC_P_DRV_KII,                  // 279 17 Vector Torque: Kii
    ISC_P_DRV_LD_SPEED_LIM,         // 280 18 Load Speed Limit (quads/msec)
    ISC_P_DRV_KPL,                  // 281 19 Compensator: Kpl
    ISC_P_DRV_KV,                   // 282 1a Compensator: Kv
    ISC_P_DRV_KP,                   // 283 1b Compensator: Kp
    ISC_P_DRV_KI,                   // 284 1c Compensator: Ki
    ISC_P_DRV_KFV,                  // 285 1d Compensator: Kfv
    ISC_P_DRV_KFA,                  // 286 1e Compensator: Kfa
    ISC_P_DRV_KFJ,                  // 287 1f Compensator: Kfj (jerk ff)
    ISC_P_DRV_KFF,                  // 288 20 Compensator: Kff (fric. ff)
    ISC_P_DRV_KNV,                  // 289 21 Compensator: Knv
    ISC_P_DRV_AHFUZZ2,              // 290 22 Compensator: Antihunt 2
    ISC_P_DRV_TRQ_BIAS,             // 291 23 Compensator: Torque Bias (A)
    ISC_P_DRV_FUZZY_APERTURE,       // 292 24 Fuzzy Aperture
    ISC_P_DRV_FUZZY_HYST,           // 293 25 Fuzzy Hystersis
    ISC_P_DRV_FAN_SPEED,            // 294 26 Drive Fan Speed (RPM)
    ISC_P_DRV_FAN_WARN_SPEED,       // 295 27 Drive Fan Warning Min Speed (RPM)
    ISC_P_TRK_ERR_MAX=296,          // 296 28 Accumulated maximum error
    ISC_P_TRK_ERR_MIN,              // 297 29 Accumulated minimum error
    ISC_P_AH_HOLDOFF,               // 298 2A Anti-hunt holdoff
    ISC_P_DRV_LD_MTR_RATIO,         // 299 2B Load to motor encoder ratio
                                    // 300 2C
    ISC_P_DRV_TUNE_CONFIG_REG=301,  // 301 2D
    ISC_P_DRV_SENSORLESS_RAMPUP_TIME,//302 2E Sensorless start torque rampup time (ms)
    ISC_P_DRV_SENSORLESS_SWEEP_TIME,// 303 2F Sensorless start angle sweep time (ms)
    ISC_P_DRV_SENSORLESS_SETTLE_TIME,//304 30 Sensorless start settle time (ms)
    ISC_P_DRV_SENSORLESS_TRQ,       // 305 31 Sensorless start torque
    ISC_P_DRV_HARDSTOP_ENTRY_SPD,   // 306 32 Hard-stop speed qualifier
    ISC_P_DRV_TRQ_LIM,              // 307 33 Torque Limit  (A)
    ISC_P_DRV_POS_FLDBK_TRQ,        // 308 34 + Torque foldback limit (A)
    ISC_P_DRV_POS_FLDBK_TRQ_TC,     // 309 35 + Torque foldback time const (ms)
    ISC_P_DRV_NEG_FLDBK_TRQ,        // 310 36 - Torque foldback limit (A)
    ISC_P_DRV_NEG_FLDBK_TRQ_TC,     // 311 37 - Torque foldback time const (ms)
    ISC_P_DRV_HARDSTOP_FLDBK_TRQ,   // 312 38 Hardstop foldback limit (A)
    ISC_P_DRV_HARDSTOP_FLDBK_TRQ_TC,// 313 39 Hardstop foldback time const (ms)
    ISC_P_DRV_MOVEDONE_FLDBK_TRQ,   // 314 3a Move Done Foldback Limit (A)
    ISC_P_DRV_MOVEDONE_FLDBK_TRQ_TC,// 315 3b Move Done Foldback time const (ms)
    ISC_P_DRV_HARDSTOP_ENTRY_TRQ,   // 316 3c Foldback Torque Trip Level (A)
    ISC_P_DRV_HARDSTOP_ENTRY_TC,    // 317 3d Hard Stop: entry delay (ms)
    ISC_P_DRV_TRQ_CMD,              // 318 3e Torque/Force Commanded
    ISC_P_DRV_TRQ_MEAS,             // 319 3f Torque/Force Measured
    ISC_P_DRV_RMS_LVL,              // 320 40 RMS level
    ISC_P_DRV_TRK_ERR_LIM,          // 321 41 Tracking Error Limit (quad cnts)
    ISC_P_DRV_MV_DN_TC,             // 322 42 Move Done: Time Constant (ms)
    ISC_P_DRV_STEPPER_TRQ,          // 323 43 Recovery window maximum
    ISC_P_DRV_STEP_IDLE_TRQ,        // 324 44 Stepper idle torque (A)
    ISC_P_DRV_STEP_ACC_TRQ,         // 325 45 Stepper accel torque (A)
    ISC_P_DRV_STEP_FLDBK_TRQ_TC,    // 326 46 Stepper trq foldback time const (ms)
    ISC_P_DRV_BUS_VOLTS,            // 327 47 Bus Voltage (V)
    ISC_P_DRV_TP_IOP,               // 328 48 I/O testpoint
    ISC_P_DRV_TP_IR,                // 329 49 R Channel testpoint
    ISC_P_DRV_TP_IS,                // 330 4a S Channel testpoint
    ISC_P_DRV_TP_IR_FILT,           // 331 4b R Channel testpoint/filter
    ISC_P_DRV_TP_IS_FILT,           // 332 4c S Channel testpoint/filter
    ISC_P_HEATSINK_TEMP,            // 333 4d Heatsink temperature
    ISC_P_AMBIENT_TEMP,             // 334 4e Internal temperature
    ISC_P_DRV_TP_5V,                // 335 4f 5V testpoint
    ISC_P_DRV_TP_12V,               // 336 50 12V testpoint
    ISC_P_DRV_TRQ_MEAS_FILT_TC,     // 337 51 Trq Measured Filter Time Const
    ISC_P_DRV_TEMP_SIM,             // 338 52 Temp Simulator
    ISC_P_DRV_FACT_KM_FACTOR,       // 339 53 Km Factor
    ISC_P_DRV_FACT_MTR_MIN_RES,     // 340 54 Mtr Min Res
    ISC_P_FACT_FAN_ON_TEMP,         // 341 55 Fan On Temp
    ISC_P_DRV_PARTID,               // 342 56 Part ID
    ISC_P_DRV_FACT_IB_TC,           // 343 57 IB TC
    ISC_P_DRV_FACT_IB_TRIP,         // 344 58 IB Trip Point (A)
    ISC_P_DRV_FACT_PHASE_TRIP,      // 345 59 Phase Trip Point (A)
    ISC_P_DRV_IB_LVL,               // 346 5a IB Level
    ISC_P_DRV_TSPD,                 // 347 5b Tspd phase delay
    ISC_P_DRV_COMM_CHK_ANGLE_LIM = 349, // 349 5d Comm check angle limit
    ISC_P_DRV_CPL_ERR_LIM = 350,    // 350 5e Coupling Error Limit (quad cnts)

    ISC_P_IEX_STATUS=357,           // 357 6f Current IEX status info
    ISC_P_IEX_USER_OUT_REG,         // 358 66 IEX output register
    ISC_P_IEX_STOP_OUT_REG,         // 359 67 IEX output register when stopped
    ISC_P_IEX_OUT_REG,              // 360 68 IEX last output register
    ISC_P_IEX_IN_REG,               // 361 69 IEX input register
    ISC_P_IEX_IN_RISE_REG,          // 362 6a IEX input rise register
    ISC_P_IEX_IN_FALL_REG,          // 363 6b IEX input fall register
    ISC_P_IEX_IN_REG_MASK,          // 364 6c IEX input register
    ISC_P_IEX_IN_RISE_REG_MASK,     // 365 6d IEX input rise register
    ISC_P_IEX_IN_FALL_REG_MASK,     // 366 6e IEX input fall register
	ISC_P_IEX_GLITCH_LIM,           // 367 6f IEX glitch limit
	ISC_P_DRV_SOFT_LIM_POSN_POS,    // 368 70 Software limit positive position
	ISC_P_DRV_SOFT_LIM_POSN_NEG     // 369 71 Software limit negative position
};

/// Typedef for ISC parameter enum
typedef enum _iscParams iscParams;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ISC PLA OR term indices for its outputs.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/**
	\brief OR fuse indices for selecting PLA outputs.

	These constants are used as the index of an OR term to select an
	output.
**/
typedef enum _iscPLAoutIndex {
	/**
	Index for <i>General Purpose Output 0</i>.

	\see \ref plaOutReg::Gpo0
	**/
    ISC_PLAOUT_GPO0_BIT,                  	// 0 Gen. purpose output 0
	/**
	Index for <i>General Purpose Output 1</i>.

	\see \ref plaOutReg::Gpo1
	**/
    ISC_PLAOUT_GPO1_BIT,                  	// 1 Gen. purpose output 1
	/**
	Index for <i>Drive Enable Request</i>.

	\see \ref plaOutReg::Enable
	**/
    ISC_PLAOUT_ENABLE_BIT,                	// 2 Drive enable request
	/**
	Index for <i>Position Capture</i>.

	\see \ref plaOutReg::Capture
	**/
    ISC_PLAOUT_CAPTURE_BIT,               	// 3 Position capture trigger
	/**
	Index for <i>Move Trigger</i>.	

	\see \ref plaOutReg::Trigger
	**/
    ISC_PLAOUT_TRIGGER_BIT,               	// 4 Move trigger
	/**
	Index for <i>Node Stop Initiator</i>.	

	\see \ref plaOutReg::NodeStop
	**/
    ISC_PLAOUT_NODE_STOP_BIT,             	// 5 Trigger node stop
	/**
	Index for <i>Reset Timer</i>.	

	\see \ref plaOutReg::ResetTimer
	**/
    ISC_PLAOUT_RESET_TIMER_BIT,				// 6 Reset the gen purpose timer
	/**
	Index for <i>PLA Feedback Term 0</i>.	

	\see \ref plaOutReg::PLAattn0
	**/
    ISC_PLAOUT_PLA_ATTN0_BIT,             	// 7 PLA output 0
	/**
	Index for <i>PLA Feedback Term 1</i>.	

	\see \ref plaOutReg::PLAattn1
	**/
    ISC_PLAOUT_PLA_ATTN1_BIT,             	// 8 PLA output 1
											// 9
											// 10
											// 11

	/**
	Index for <i>Positive Torque/Force Foldback</i>.	

	\see \ref plaOutReg::PosTrqFldBk
	**/
	ISC_PLAOUT_POS_TRQ_FLBK_BIT = 12,		// 12 Positive torque foldback
	/**
	Index for <i>Negative Torque/Force Foldback</i>.	

	\see \ref plaOutReg::NegTrqFldBk
	**/
	ISC_PLAOUT_NEG_TRQ_FLBK_BIT				// 13 Negative torque foldback
} iscPLAoutIndex;

//****************************************************************************
// Insure structs are byte packed on target
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
//****************************************************************************


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ISC CONFIGURATION REGISTER DEFINITIONS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/**
	\brief Meridian-ISC hardware configuration register fields.
**/
typedef struct PACK_BYTES _iscHwConfigFlds
{
	/// Motor Type Selection
	typedef enum _motorTypes {
		MOTOR_BRUSHLESS_ROTARY,         ///<  0: Rotary, brushless
		MOTOR_BRUSH_ROTARY,             ///<  1: Rotary, brush
		MOTOR_BRUSHLESS_LINEAR,         ///<  2: Linear, brushless
		MOTOR_UNKNOWN,                  ///<  3: Unknown motor
		MOTOR_STEPPER = -4,             ///<  4/-4: Stepper motor
		MOTOR_RESERVED5 = -3,           ///<  5/-3: Reserved
		MOTOR_STEPPER_CLOSED_LOOP = -2  ///<  6/-2: Closed loop stepper
	} motorTypes;                       ///< Motor Types

										/// Commutation Sensor Mode Selection
	typedef enum _commSensorModes {
		CSM_STATE_SEQ_THERMAL,          ///<  0 Chk bad state & seq.,000=thermal fault
		CSM_STATE_SEQ,                  ///<  1 Chk bad state & seq.,000!=thermal fault
		CSM_STATE_THERMAL,              ///<  2 Chk bad state only, 000=thermal fault
		CSM_STATE,                      ///<  3 Chk bad state only, 000!=thermal fault
		CSM_NO_STATE_CHECK = 4,           ///<  4 Ignore the states
		CSM_SENSORLESS_WITH_DAMPING = 6,  ///<  6 Sensorless start (no checks)
		CSM_SENSORLESS_NO_DAMPING = 7,    ///<  7 Sensorless start, A-Style (DA4x)
		CSM_SENSORLESS_DAMPING_DIST_CHK = -2, ///<  14 Sensorless start (distance check, damping)
		CSM_SENSORLESS_NO_DAMP_DIST_CHK = -1  ///<  15 Sensorless start (distance check, A-Style)
	} commSensorModes;                  ///< Commutation Sensor Handling

										/// Servo Mode Selection.
	typedef enum _servoModes {
		/*!
			\brief Single-Loop

			Closed loop servo with feedback from the motor's encoder.
		**/
		SINGLE_LOOP,                    //  0: Single Loop from motor encoder
		MODE_NA1,                       //  1: Illegal mode
		SINGLE_LOAD,                    //  2: Single Loop/load encoder units, read 2nd enc.
		/*!
			\brief Dual-Loop

			Dual loop servo. The measured position is from the load encoder and
			the motor's encoder is used for inner velocity control.
		**/
		DUAL_LOOP,                      //  3: Dual Loop, read 2nd encoder
		/*!
			\brief Open-Loop

			Open loop servo for stepper motors. The motor's position
			is optionally allowed to be read but never acted on for position
			control. When the motor's encoder is enabled, the position
			tracking is calculated for display and safety shutdown purposes.
		**/
		OPEN_LOOP = -4                  //  4: Open loop
	} servoModes;                       // Servo Modes

	/**
		\brief Motor type selection.

		Attached motor type.
	**/
	motorTypes MotorType : 3;					//  0-3
	/**
		\brief Vector Lock Diagnostic Enable

		If the motor is vector oriented, such as a brushless motor
		this will force the vector to freeze.
	**/
	BFld VectorLock : 1;						//   3 
	/**
		\brief Commutation handling for vector type motors.

		The commutation handling for vector type motors.
	**/
	commSensorModes CommMode : 4;				//  4-7
	/**
		\brief Commutation sensor inverter.

		Invert the commutation sensors.
	**/
	BFld CommutationInvert : 1;					//   8 
	/**
		\brief Encoder sequence checking.

		If set, the encoders will be tested for bad sequences. This will
		generate warnings and/or shutdowns when encountered.
	**/
	BFld EncCheck : 1;							//   9 
	/**
		\brief Disable phase overload checking.

		If set, this bit will disable the phase overload checking. This
		test should be disabled only under extreme conditions as real
		problems could lead to drive or motor destruction.
	**/
	BFld DisPhaseOver : 1;						//  10
	/**
		\cond FW_MILESTONE_5R4
		\brief Dynamic Brake Disable (v5.4)

		If not set, this bit will provide motor braking by connecting the
		windings to the motor power bus's return. The drive will modulate
		the braking to prevent damage to the power stage during this time.


		When this feature is enabled, the brake effect is engaged with
		any or all of these conditions:
		- Motor power bus low conditions that assert the
		#alertReg::BusVoltageLow bit.
		- Disabling of the enable.
		- Shutdown condition that disables the drive
		\endcond

		\note This feature requires ISC firmware version 5.4 and above.
	**/
	BFld DisDynamicBrake : 1;					//  11 
	BFld: 4;									//  12-15
	/**
		\brief Expected IEX count/plug-and-play mode.

		This field, if non-zero will cause the IEX module to check for
		the presence of the specified IEX modules on the IEX link. This
		provides a mechanism to insure the setup of each node is correctly
		implemented. If there is a mismatch the IEX status register will
		reflect this. This register is accessed via the #iscGetIEXStatus
		function.
	**/
	BFld IEXcount : 3;							//  16-18
	/**
		\brief IEX system stop.

		This field controls the IEX modules. Setting this bit to one
		will stop the IEX subsystem and hold the outputs at the last
		updated value. If the inputs are read or outputs attempting to
		change will return an error.

		When this bit is cleared, the IEX subsystem is restarted. If
		the modules are connected and powered properly the subsystem
		will go online and interaction with the IEX modules may be
		performed.

		Watch the #iscGetIEXStatus results to find out when you can
		interact with the modules.
	**/
	BFld IEXstop : 1;							//  19 
	/**
		\brief Servo operation mode.

		Change the servo loop mode. This can provide single-loop,
		dual-loop or open-loop modes.

		The single-loop uses the motor's encoder as the feedback device
		for positioning.  The dual-loop mode uses the load encoder for
		the position feedback device and the motor as as stablilizing
		feedback device. In open-loop mode, the measured position is not
		used.
	**/
	servoModes ServoMode : 3;					//  20-22
	/**
		\brief Ignore open-loop encoder.

		Setting this field will cause the open-loop mode to ignore the
		motor's encoder and cause the measured position to equal the
		commanded position.
	**/
	BFld IgnoreEnc : 1;							//  23
	/**
		\brief Load encoder sequence check.

		If set, the load encoder will be tested for bad sequences
		when operating in single loop mode. This will
		generate warnings and/or shutdowns when encountered.
	**/
	/**
		 Detect ld encoder bad seqs in single loop mode
	**/
	BFld LdEncCheck : 1;						//  24
} iscHwConfigFlds;


/**
    \brief Hardware Configuration Register.

	\copydetails ISC_P_HW_CONFIG_REG

	\see \ref iscGetHwConfigReg
	\see \ref iscSetHwConfigReg
**/
typedef union _iscHwConfigReg {
	/// Settings as an integer
	Uint32 bits;
	///< Field based access for <i>Hardware Configuration Register</i>.
	iscHwConfigFlds Fld;
    /// \cond INTERNAL_DOC
    // Indicate commSensorMode state chk enabled
    tBOOL UsingCommStateChk() {
        // Non-state chk entries of commSensorModes should have MSb of 1.
        STATIC_ASSERT((iscHwConfigFlds::CSM_NO_STATE_CHECK & iscHwConfigFlds::CSM_SENSORLESS_WITH_DAMPING
                       & iscHwConfigFlds::CSM_SENSORLESS_NO_DAMPING & iscHwConfigFlds::CSM_SENSORLESS_DAMPING_DIST_CHK
                       & iscHwConfigFlds::CSM_SENSORLESS_NO_DAMP_DIST_CHK & 0x4) != 0);
        return ((Fld.CommMode & 0x4) == 0);
    }

    // Indicate commSensorMode 000 == thermal fault
    tBOOL UsingCommThermalChk() {
        // 000 == thermal fault entries of commSensorModes should have LSb of 0.
        STATIC_ASSERT(((iscHwConfigFlds::CSM_STATE_SEQ_THERMAL | iscHwConfigFlds::CSM_STATE_THERMAL) & 0x1) == 0);
        return (UsingCommStateChk() && (Fld.CommMode & 0x1) == 0);
    }
    //! \endcond

} iscHwConfigReg;
//                                                                            *
//*****************************************************************************



//*****************************************************************************
/**																			  *
\brief Meridian ISC Application Configuration Register fields.

\copydetails ISC_P_APP_CONFIG_REG

\see \ref iscGetAppConfigReg
\see \ref iscSetAppConfigReg
**/
// Note: the numbers in the field descriptions denote their bit
// range in the register.
/// Field of the <i>Application Configuration Register</i>.
typedef struct _iscAppConfigFlds {

	/// General Purpose Input/Limit Switch handling
	typedef enum _limitModes {
		/*!
			Ignores General Purpose inputs for travel limits.
		**/
		LIMIT_MODE_OFF,                     //  0: ignore limits
		/*!
			General Purpose Inputs 2 and 3 are travel limits.
		**/
		LIMIT_MODE_POSITION,                //  1: Limit position
		/*!
			Not implemented
		**/
		LIMIT_MODE_TORQUE,                  //  2: Limit torque
		/*!
			\cond FW_MILESTONE_5R4
			Travel limits set using drive parameters.

			\note Soft position limit mode is only available on firmware
			version 5.4 and later.
		**/
		LIMIT_MODE_SOFT_POSN = -4			//  4: Software position limit
	} limitModes;                           ///< GPI/Limit Switch Modes
	/*!
		\brief Trigger group.

		Trigger group ID for the trigger command. The Trigger Group
		of zero is always ignored.
	**/
	BFld TriggerGroup : 2;				//   0 ID for group trigger event
	BFld na2 : 1;						//     2
	/*!
		\brief GPI-0 Global Node Stop Enable

		If asserted the de-assertion of the GPI-0 causes this node
		to enter its Node Stop defined by the Stop Type Parameter
		(#ISC_P_STOP_TYPE). This node will also generate a node stop
		to all of it's upstream nodes.
	**/
	BFld GPI0nodeStop : 1;				//   3 Gen Node Stop on GPI-0 Rise
	/*!
		\brief GPI-0 <i>A After Start</i> comparison source.

		For the <i>A After Start</i> feature, this field will select if the
		position is based on the commanded or the measured values.
		<table class="doxtable">
		<tr> <th>Code</th>  <th>Description             </th> </tr>
		<tr> <td>0</td>     <td>Commanded Position      </td> </tr>
		<tr> <td>1</td>     <td>Measured Position       </td> </tr>
		</table>
		Set the A After Start Parameter (#ISC_P_A_START) to setup
		this feature.
	**/
	BFld AAfterStartMeas : 1;			//   4 0=Commanded,1=Measured
	/*!
		\brief GPI-0 <i>B Before End</i> comparison source.

		For the <i>B Before End</i> feature, this field will select if the
		position is based on the commanded or the measured values.
		<table class="doxtable">
		<tr> <th>Code</th>  <th>Description             </th> </tr>
		<tr> <td>0</td>     <td>Commanded Position      </td> </tr>
		<tr> <td>1</td>     <td>Measured Position       </td> </tr>
		</table>
		Set the B Before Start Parameter (#ISC_P_B_END) to setup
		this feature.
	**/
	BFld BBeforeEndMeas : 1;			//   5 0=Commanded,1=Measured
	/*!
		\brief GPI-0 <i>At Position</i> comparison source.

		For the <i>At Position</i> feature, this field will select if the
		position is based on the commanded or the measured values.
		<table class="doxtable">
		<tr> <th>Code</th>  <th>Description             </th> </tr>
		<tr> <td>0</td>     <td>Commanded Position      </td> </tr>
		<tr> <td>1</td>     <td>Measured Position       </td> </tr>
		</table>
		When the #plaOutReg::Capture output is asserted the
		specified position is captured to the PLA Capture register.
		(#ISC_P_POSN_CAP_PLA)
	**/
	BFld AtPositionMeas : 1;			//   6 0=Commanded,1=Measured
	BFld na7 : 1;						//     7
	/*!
		\brief <i>Hard Stop</i> Foldback Enable

		This field enables the <i>Hard Stop</i> feature. This feature
		will allow the application to utilize a rigid end stop
		as a home position. This feature allows the deletion of
		a home sensor from a design. Setting this bit enables
		this feature.
		The torque/force is
		specified by the <i>Hard Stop Foldback</i> parameter
		(#ISC_P_DRV_HARDSTOP_FLDBK_TRQ). If torque/force
		relaxation is required from the initial torque/force to
		the end torque/force use the <i>Hard Stop Relax
		Time Constant</i> (#ISC_P_DRV_HARDSTOP_FLDBK_TRQ_TC).

		/todo HardStop Feature
	**/
	BFld FoldbackHardStop : 1;			//   8 Foldback on hard stop
	/*!
		\brief <i>Move Done</i> Foldback Enable

		This field enables the <i>Move Done Foldback</i> feature.
		This feature will allow the application to utilize
		a rigid end stop as a home position. This feature
		allows the deletion of a home sensor from a design.

		Asserting this bit will cause a torque/force foldback
		to assert at the end of move. The torque/force is
		specified by the <i>Move Done Foldback</i> parameter
		(#ISC_P_DRV_MOVEDONE_FLDBK_TRQ). If torque/force
		relaxation is required from the initial torque/force to
		the end torque/force, use the <i>Move Done Relax
		Time Constant</i> (#ISC_P_DRV_MOVEDONE_FLDBK_TRQ_TC).

		\see \ref ISCmoveDonePage
	**/
	BFld FoldbackMoveDone : 1;			//   9 Foldback on move done
	BFld na10_11 : 2;					//    10
	/*!
		\brief <i>Limit Switch/GPI</i> Enable

		This field controls the processing of the GPI-2 and GPI-3
		as well as the limit mode enabled. Three limit modes are
		available:
		- Ignore Mode (\ref LIMIT_MODE_OFF):
		The GPI-2 and GPI-3 inputs are used as general purpose inputs.
		- Position Mode (\ref LIMIT_MODE_POSITION):
		GPI-2 and GPI-3 are treated as limit switches
		and the node will cancel movement when one of these limits is
		asserted.
		- Soft Position Mode (\ref LIMIT_MODE_SOFT_POSN):
		The GPI-2 and GPI-3 inputs are used
		as general purpose inputs and the travel limits are set using
		parameters to specify the upper and lower positional limits for travel.

		\note The Soft Position Mode is only available on firmware
		version 5.4 and later.
	**/
	limitModes LimitMode : 3;			//  12-14 Limit switch mode
	/*!
		\brief <i>Limit Ramp Stop</i>

		This field controls the type of stop that is used when
		a limit is encountered. If asserted, a ramp-style stop
		will be used; otherwise, an abrupt stop will be used
		for stopping at a limit.

		\note A ramp-type stop is only
		available on firmware version 5.4 and later.
	**/
	BFld LimitRampStop : 1;				//  15 Limit stop type
} iscAppConfigFlds;
//                                                                            *
//*****************************************************************************


//*****************************************************************************
/**																			  *
    \brief Application Configuration Register.

    \copydetails ISC_P_APP_CONFIG_REG

	\see \ref iscGetAppConfigReg
	\see \ref iscSetAppConfigReg
**/
typedef union _iscAppConfigReg {
    /// Register state as an integer.
    Uint32 bits;
	/**
	Application Configuration field access
	**/
	iscAppConfigFlds Fld;                                 
    // For C++ non-firmware usage, include a constructor.
    #ifdef __cplusplus
        #ifndef __TI_COMPILER_VERSION__
        _iscAppConfigReg() {                // Clear bits on construction
            bits = 0;
        }
        #endif
    #endif
} iscAppConfigReg;
//																			  *
//*****************************************************************************

//*****************************************************************************
/**																			  *
    \brief Tuning Configuration Register.

    This register controls various servo performance settings. For example:
    anti-hunt mode settings are here.

	\see \ref ISC_P_APP_CONFIG_REG
	\see \ref iscGetAppConfigReg
	\see \ref iscSetAppConfigReg
**/
typedef union _iscTuneConfigReg {
    /// Antihunt selections
    typedef enum _antihuntModes {
        ANTIHUNT_OFF,                       ///<  0 Off
        ANTIHUNT_STD                        ///<  1 Standard Mode
    } antihuntModes;                        ///< Anti-hunt modes
    /// Register value as an integer
    Uint32 bits;
    // Note: the numbers in the field descriptions denote their bit
    // range in the register.
    struct _iscTuneConfigFlds {
        /**
			\brief Anti-Hunt Mode selection.

            Anti-Hunt mode select from the #antihuntModes enumeration.
        **/
        antihuntModes AntiHunt      : 2;    // 0-1 Anti-hunt modes
		/// \cond INTERNAL_DOC
        BFld na2_3              : 2;    // 2
		/// \endcond
        /**
			\brief Motor's velocity estimator selection.

            Motor's velocity estimator selection.
        **/
        BFld MtrVelocityEst     : 3;    // 4-6 Motor velocity estimator
		/// \cond INTERNAL_DOC
        BFld na7                : 1;    // 7
		/// \endcond
        /**
			\brief Load's velocity estimator selection.

            Load's velocity estimator selection.
        **/
        BFld LdVelocityEst      : 3;    // 8-10 Load velocity estimator
		/// \cond INTERNAL_DOC
		BFld na11				: 1;
		/// \endcond
		/**
			\brief Data Collection control (v5.6)

			Clearing these bits will collect data during each move command.
			This is the typical setting for this feature. 

			Setting this bit will cause the <i>Data Collection</i>
			feature to free-run the collection between command calls. This
			is a special testing mode and generally not of much value
			for applications.

			\note This feature appears on ISC firmware version 5.6 and later.
		**/
		BFld DataCollectFreeRunApp	: 1;	// 12 Data collector free runs on App
		BFld DataCollectFreeRunDiag	: 1;	// 13 Data collector free runs on Diag
    } Fld;                                  ///< Field Access to Tuning Register
    // For C++ non-firmware usage, include a constructor.
    #ifdef __cplusplus
		#ifndef __TI_COMPILER_VERSION__
        _iscTuneConfigReg() {               // Clear bits on construction
            bits = 0;
        }
        #endif
    #endif
} iscTuneConfigReg;
//																			  *
//*****************************************************************************

//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//                          ISC NODE STATUS REGISTER
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//      This register is made up of three 16-bit sections for a total length
//      of 48 bits. The lower 24-bits can be used to generate spontaneous
//      attention packets to the host as a form of interrupt to the host.
//
//      The three sections are:
//          - Attentionable state information
//          - PLA input register (first 8-bits attentional)
//          - Non-attentionable state
//
//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


//*****************************************************************************
// NAME                                                                       *
//  iscLowAttnFlds union
//
// DESCRIPTION
/**
 	\cond ISC_DOC
    \brief <i>Lower Attentionable</i> portion of the <i>Status Register</i>.

    This portion of the Status Register (#mnStatusReg) can be used to generate
    attention packets.

	\see \ref mnStatusReg for details about accessing this type of register.
**/

// Use typedef for bitfields so that the size of the field, and apparently
// therefore the size of the struct, will be the same on host and node.
// Note that just "unsigned" causes a 32 vs 16-bit mismatch on host and node.


/**
	LSBs of the attentionable part of the status register.	
**/
typedef struct _iscLowAttnFlds {
    //                              Bit #     Describe
    /**
		\brief Warning is present.

		\copydetails attnReg::DrvFld

    **/
    BFld Warning        : 1;    // 0  00 Warning is present
    /**
		\brief User alert is present.

		\copydetails attnReg::DrvFld

    **/
    BFld UserAlert      : 1;    // 1  01 User Alert is present
    /**
		\brief Node not ready for operation.

		\copydetails attnReg::DrvFld

    **/
    BFld NotReady       : 1;    // 2  02 Not Rdy for Operation
    /**
		\brief Move buffer is available.

		\copydetails attnReg::MoveBufAvail

        \remark
        This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld MoveBufAvail   : 1;    // 3  03 Move buffer is available
    /**
		\brief IEX input rise detected.

		\copydetails attnReg::IEXrise

        \remark
        This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld IEXrise        : 1;    // 4  04 IEXrise & riseMask is nonzero
    /**
		\brief IEX input fall detected.

		\copydetails attnReg::IEXfall

        \remark
        This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld IEXfall        : 1;    // 5  05 IEXfall & fallMask is nonzero
    /**
		\brief IEX status and statistics changed.

		\copydetails attnReg::IEXstatus

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld IEXstatus      : 1;    // 6  06 IEX status reg changed
    /**
		\brief Motion will occur on enable

		Most of this logic will be handled by the UI. However, there are
		a few command controllers that have to supply some additional
		information for this logic. Instead of creating separate bits for
		each command controller, they can all use this one.

        \remark This bit can generate an attention request to the host for
        autonomous handling. (NOTE: This bit probably does not need to be
        in the attentionable part of the status register. When/If we free
        up more bits in the unattentionable part for clearPath, this could
        be moved.)
    **/
    BFld MotionPending            : 1;    // 7
    /**
		\brief Positive limit asserted (FW v5.4).

		\copydetails attnReg::InPosLimit

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld InPosLimit     : 1;    // 8 Positive limit asserted
    /**
		\brief Negative limit asserted (FW v5.4).

		\copydetails attnReg::InNegLimit
		
        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld InNegLimit     : 1;    // 9 Negative limit asserted
    /**
		\brief Motion Blocked (FW v5.4).

		\copydetails attnReg::MotionBlocked
		
        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld MotionBlocked     : 1; //10 Motion blocked
	///\cond INTERNAL_DOC
	/**
     	\brief Trigger Pulse Detected

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld TriggerPulse		: 1; //11 Trigger pulse detected
    /**
     	\brief In Homing Sequence

        \remark This bit can generate an attention request to the host for
        autonomous handling. Indicates that a homing sequence is being
        executed
    **/
    BFld Homing				: 1; //12 In Homing Sequence
	///\endcond
	/**
		\brief Going To Disable Soon

		\remark This bit can generate an attention request to the host for
		autonomous handling. Indicates that the drive is about to go
		disabled.
	**/
	BFld GoingDisabled		: 1; //13 Heads-up going to disable
	/**
		\brief Status Rise Event detected

		\remark This bit is set when the StatusRT register ANDed 
		with the Group Status Mask Register is non-zero.
	**/
	BFld StatusEvent		: 1; //14 Heads-up going to disable
	/**
		\brief Drive Enabled

		\remark This bit is set when the drive is enabled. This
		can be used to release a brake.
	**/
	BFld Enabled			: 1; //15 Drive is enabled
} iscLowAttnFlds;
/// \endcond                                                                  *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  _iscPlaInFlds struct
//
// DESCRIPTION
/**
	\cond ISC_DOC
    \brief <i>PLA Inputs</i> portion of the <i>Status Register</i>.

    The middle portion of the Status Register (#mnStatusReg) that feeds the PLA
    function. The lower eight bits can be used to generate attention
    packets.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct _iscPlaInFlds {
    // ---------------- These fields are attentionable ----------------
    //                                    Status #
    /**
		\brief Node stopped.

		\copydetails attnReg::DrvFld

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld NodeStopped    : 1;    // 0  16: node stop
    /**
		\brief Move completed and settled.

		\copydetails attnReg::MoveDone

        \remark This bit can generate an attention request to the host for
        autonomous handling.
   **/
    BFld MoveDone       : 1;    // 1  17: move done
    /**
		\brief Tracking out of range.

		\copydetails attnReg::OutOfRange

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld OutOfRange     : 1;    // 2  18: not in range
    /**
		\brief <i>B Counts from End</i> of Move.

		\copydetails attnReg::BFromEnd

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld BFromEnd       : 1;    // 3  19: n counts from end
    /**
		\brief PLA Feedback output 0.

		\copydetails attnReg::PlaOut0

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld PlaOut0        : 1;    // 4  20: PLA output 0
    /**
		\brief PLA Feedback output 1.

		\copydetails attnReg::PlaOut1

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld PlaOut1        : 1;    // 5  21: PLA output 1
    /**
		\brief General Purpose Input 0.

		\copydetails attnReg::Gpi0

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld Gpi0           : 1;    // 6  22: general purpose input 0
    /**
		\brief General Purpose Input 1.

		\copydetails attnReg::Gpi1

        \remark This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld Gpi1           : 1;    // 7  23: general purpose input 1
    // ------------ Remaining fields are not attentionable ------------
    /**
		\brief General Purpose Input 2 / Positive Limit.

        This bit asserts when the General Purpose Input 2 is asserted
        or, when in position limit mode, the node is in the positive limit
        switch.

        The operational mode is set by the
        #iscAppConfigReg::LimitMode field.
    **/
    BFld Gpi2_posLim    : 1;    // 8  24: GPI2 / positive limit asserted
    /**
		\brief General Purpose Input 3 / Negative Limit.

        This bit asserts when the General Purpose Input 3 is asserted
        or, when in position limit mode, the node is in the negative limit
        switch.

        The operational mode is set by the
        #iscAppConfigReg::LimitMode field.
    **/
    BFld Gpi3_negLim    : 1;    // 9  25: GPI3 / negative limit asserted
    /**
		\brief Crossed At Position location.

        This bit asserts when the measured position crosses the
        location specified in the <i>At Position Location</i> 
        (#ISC_P_POSN_TRIG_PT).
    **/
    BFld AtPosition     : 1;    // 10 26: at position
    /**
		\brief Masked IEX active.

        This bit asserts when the <i>IEX Input Register</i>
        (#ISC_P_IEX_IN_REG), bitwise ANDed with the
        <i>IEX Input Mask Register</i> (#ISC_P_IEX_IN_REG_MASK),
        produces a non-zero result.
    **/
    BFld MaskedIEX      : 1;    // 11 27: IEX & IEX mask = nonzero
    /**
		\brief <i>A From Start</i> of move.

        This bit asserts when the move is <i>A From Start</i>
        (#ISC_P_A_START) of the move. The basis of the move is
        selected by the
        #iscAppConfigReg::AAfterStartMeas bit in
        the <i>Application Configuration Register</i>
        (#ISC_P_APP_CONFIG_REG).
		\note Behavior is undefined for moves that cross a 32-bit
		signed position boundary
    **/
    BFld AFromStart     : 1;    // 12 28: n counts from start
    /**
		\brief Commanded move is negative.

        This bit asserts when the current command resulted in the
        position changing in a negative direction. This bit changes
        at the start of a move and persists until the next move.
        A zero step move is considered positive.

        This feature is useful for controlling foldback modes based
        on the motion issued using the PLA feature.
        \see \ref ISCplaPage
		\see \ref ISCfoldbackPage
    **/
    BFld MoveCmdNeg     : 1;    // 13 29: move direction is negative
    /**
		\brief Drive is disabled.

        This bit asserts when the ISC's power amplifier is
        disabled.
    **/
    BFld Disabled       : 1;    // 14 30: Drive disabled
    /**
		\brief <i>General Purpose Timer</i> has expired.

        This bit asserts when the <i>General Purpose Timer</i>
        timer has expired. This timer is reset by the
        #plaOutReg::ResetTimer bit in the <i>Output
        Register.</i> The duration of this timer is set by the
        <i>General Purpose Timer Time Constant</i>
        (#ISC_P_GP_TIMER_PERIOD).

		\see \ref ISCgpTimerPage
    **/
    BFld TimerExpired   : 1;    // 15 31: timer expired
	
} iscPlaInFlds;
/// \endcond                                                                  *
//*****************************************************************************

//*****************************************************************************
// NAME                                                                       *
//  iscOutFlds structure
//
// DESCRIPTION

/**
	\cond ISC_DOC
    \brief Meridian ISC output register definition. It allows access to
	actuation of various features within and external via electrical outputs
	from the node.
**/
typedef struct PACK_BYTES _iscOutFlds
{
    /*                   -----------  --------------------------------- *
     *                         bit #  description                       *
     *                   -----------  --------------------------------- */
    /**
		\brief General purpose output 0 assert.

        This bit controls the <i>General Purpose Output 0</i> 
        pin on the node's I/O connector.

		\see \ref ISCoutputsPage
    **/
    BFld Gpo0        : 1;    // 0 : general purpose output 0
    /**
		\brief General purpose output 1 assert.

        This bit controls the <i>General Purpose Output 1</i> 
        pin on the node's I/O connector.

		\see \ref ISCoutputsPage
    **/
    BFld Gpo1        : 1;    /* 1 */ // general purpose output 1
    /**
		\brief Enable drive's power stage.

        This bit controls the enabling of the drive's power stage and
        prepares the node to accept motion commands, which will be
        accepted when the <i>Status Register's</i> 
        #lowAttnReg::NotReady bit has cleared.
    **/
    BFld Enable      : 1;    /* 2 */ // enable request
    /**
		\brief Capture measured position on asserting edge.

        The asserting edge (0->1) of this bit will capture the measured
        position to the PLA capture register (#ISC_P_POSN_CAP_PLA).
    **/
    BFld Capture     : 1;    /* 3 */ // capture
    /**
		\brief Generate trigger event on asserting edge.

        The asserting edge (0->1) of this bit will generate a <i>Trigger
        Event</i> which allows a waiting move  to begin.
    **/
    BFld Trigger     : 1;    /* 4 */ // a.k.a. go
    /**
		\brief Generate node stop on asserting edge.

        The asserting edge (0->1) of this bit will initiate the node stop
        programmed in the Stop Type register (#ISC_P_STOP_TYPE).
    **/
    BFld NodeStop    : 1;    /* 5 */ // enter node stop
    /**
		\brief Reset general purpose timer.

        The asserting edge (0->1) of this bit will reset the <i>General
        Purpose Timer</i> with duration (#ISC_P_GP_TIMER_PERIOD) and
        clear the timer expired bit in the Status Register.

		\see \ref ISCgpTimerPage
    **/
    BFld ResetTimer  : 1;    /* 6 */ // reset timer
    /**
		\brief PLA Feedback output 0.

        This output is fed back to a similarly named input in the
        <i>Status Register</i>. It is used to make simple state machines
        or to map inputs available to the PLA system to an Attentionable
        location.

		\see \ref ISCplaPage
		\see \ref ISCinputsPage
    **/
    BFld PLAattn0    : 1;    /* 7 */ // PLA output 0
    /**
		\brief PLA Feedback output 1.

        This output is fed back to a similarly named input in the
        <i>Status Register</i>. It is used to make simple state machines or
        to map inputs available to the PLA system to an Attentionable
        location.

		\see \ref ISCplaPage
		\see \ref ISCinputsPage
    **/
    BFld PLAattn1    : 1;    /* 8 */ // PLA output 1
    BFld             : 1;    /* 9 */ // unused
    BFld             : 1;    /* 10 */ // unused
    BFld             : 1;    /* 11 */ // unused
    /**
		\brief Positive torque/force foldback enable.

        Setting this bit can control the amount of positive torque/force
        available. The Positive Foldback Torque (#ISC_P_DRV_POS_FLDBK_TRQ) 
        parameter controls the amount.

		\see \ref ISCfoldbackPage
    **/
    BFld PosTrqFldBk : 1;    /* 12 */ // positive torque foldback enable
    /**
		\brief Negative torque/force foldback enable.

        Setting this bit can control the amount of negative torque/force
        available. The Negative Foldback Torque (#ISC_P_DRV_NEG_FLDBK_TRQ) 
        parameter controls the amount.

		\see \ref ISCfoldbackPage
    **/
    BFld NegTrqFldBk : 1;    /* 13 */ // negative torque foldback enable
    BFld             : 1;    /* 14 */ // unused
    BFld             : 1;    /* 15 */ // unused
} iscOutFlds;
/// \endcond                                                                  *
//*****************************************************************************

/**
	\cond INTERNAL_DOC
    \brief Shutdown Field States
    \public

    These elements describe the state of the node shutdown system.
**/
typedef enum _shutdownStates {
    SHDN_STATE_IDLE         = 0,    ///< All is well
    SHDN_STATE_DELAY        = 1,    ///< Shutdown is imminent
    SHDN_STATE_RAMP         = -2,   ///< Axis is ramping down to zero speed
    SHDN_STATE_STOPPED      = -1    ///< Axis is shutdown
} shutdownStates;


//*****************************************************************************
// NAME                                                                       *
//  iscNonAttnFlds union
//
// DESCRIPTION

/**
	\cond ISC_DOC
    \brief Non-Attentional portion of the ISC <i>Status Register</i>.

    The most significant sixteen bit portion of the <i>Status Register</i>
    (#mnStatusReg).
    This portion of the register does not generate any attention packets.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct PACK_BYTES _drvNonAttnFlds
{
    //                                Status #
    /**
		\brief Motion Indicator.

        This field represents the load's velocity status. The
        table describes the states:
		- 0: Near zero speed
		- 1: Moving in a positive direction
		- 2: Moving in a negative direction
		- 3: Moved in both directions since the last read

		\note This feature requires ISC firmware version 5.4 and above.
    **/
	BFld InMotion		: 2;	//0-1 32-33: Motion Signum
    /**
		\brief The Output Register bits are set using the
		<i>Controlled Output Register</i> (#ISC_P_CTL_STOP_OUT_REG).

        The Node Stop feature has forced the outputs to the
        <i>Controlled</i> state. In this state the <i>Output
        Register</i> is set to the state stored in the
        <i>Controlled Output Register</i> (#ISC_P_CTL_STOP_OUT_REG).

		\see \ref ISCnodeStopPage
    **/
	/**
		\brief For drives with step+direction inputs and this feature.
		This field is set true when the step+direction inputs are
		active.
	**/
    BFld StepsActive    : 1;  // 0  34: True when step+dir active
	/**
		The Node Stop feature has forced the node into its 
		<i>Controlled state</i>. The output register is now set to the
		#ISC_P_CTL_STOP_OUT_REG register.

	**/
    BFld InCtrlStop     : 1;    // 3  35: Controlled Stop OutputReg
    /**
		\brief Fan running.

        For units with a fan, this field is asserted when the
        fan is on. This is a warning that the ambient or internal
        temperature is approaching the shutdown level.
    **/
    BFld FanOn          : 1;    // 4  36: Fan is on
    /**
		\brief Vector searching.

        For vector based motors, this bit asserted when the
        vector has not been initialized.
    **/
    BFld VectorSearch   : 1;    // 5  37: Searching for commutation edges
    /**
		\brief Motion command has completed.

        This bit is asserted after the motion command has been completed.
        The move is considered totally complete when the <i>Move Done</i>
        #plaInReg::MoveDone has been been asserted.
    **/
    BFld MoveCmdComplete: 1;    // 6  38: Move command completed
    /**
		\brief In hard-stop mode.

        This bit asserts when the ISC has entered the <i>
        Hard Stopped</i> mode and the node is applying clamping
        torque/force. The feature is enabled with the
        #iscAppConfigReg::FoldbackHardStop bit in the 
		<i>Application Configuration Register</i> (#ISC_P_APP_CONFIG_REG).
        \see \ref ISCfoldbackHardStop
    **/
    BFld InHardStop     : 1;    // 7  39: In hard-stop mode
    /**
		\brief Shutdown state.

        This field describes the shutdown condition of the node.
    **/
#if defined(_WIN32)||defined(_WIN64)
	// Avoid compiler padding bug
    BFld ShutdownState	: 2;	// 8-9:40-41 Shutdown state
#else
    shutdownStates ShutdownState : 2;// 8-9:40-41 Shutdown state
#endif
    /**
		\brief Hardware failure detected.

        The unit's shutdown is caused by a hardware failure. This
        unit should be replaced.
    **/
    BFld HwFailure      : 1;    // 10 42: Replace the node
    /**
		\brief Move awaiting trigger event.

        The node has a triggered move awaiting release.

		\see \ref ISCnodeStopPage
    **/
    BFld TriggerArmed   : 1;    // 11 43: Triggered move loaded on drive
	/// \cond INTERNAL_DOC
    BFld		        : 1;    // 12 44: Velocity Type
	/// \endcond
    /**
		\brief Index detected on motor encoder.

        The node has detected an index pulse on the motor's encoder. Read
        the <i>Motor's Encoder Index</i> (#ISC_P_POSN_CAP_INDEX) parameter
        to get its location.
    **/
    BFld IndexMtr       : 1;    // 13 45: Index detected on motor encoder
    /**
		\brief Index detected on load encoder.

        The node has detected an index pulse on the load's encoder. Read
        the <i>Load's Encoder Index</i> (#ISC_P_POSN_CAP_INDEX_LD) parameter
        to get its location.
    **/
    BFld IndexLd        : 1;    // 14 46: Index detected on load encoder
    /**
		\brief Software inputs are being used.

        Indicates when inputs that have the ability to be controlled either
        by software or hardware have been switched to software control.
    **/
    BFld SoftwareInputs  : 1;    // 15 47: Using Software Inputs
} iscNonAttnFlds;
/// \endcond                                                                  *
//*****************************************************************************

//*****************************************************************************
// Initial release of sFoundation compatible structs.
/**
	S-Foundation original release PLA input register fields for drives.
**/
typedef union _iscPlaInOldFlds {
	plaInCommonFlds Common;
	iscPlaInFlds DrvFld;
} iscPlaInOldFlds;

/**
	S-Foundation original release low order attentionable fields for drives.
**/
typedef union _iscLowAttnOldFlds {
	lowAttnCommonFlds Common;
	iscLowAttnFlds DrvFld;
} iscLowAttnOldFlds;

/**
	S-Foundation original release non-attentionable fields for drives.
**/
typedef union _iscNonAttnOldFlds {
	nonAttnCommonFlds Common;
	iscNonAttnFlds DrvFld;
} iscNonAttnOldFlds;

/**
	\cond ISC_DOC
    \brief <i>PLA Inputs</i> portion of the <i>Status Register</i>.

    This is status register field definitions for the Meridian ISC.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct _iscStatusRegOldFlds {
    iscLowAttnOldFlds  LowAttn; 	//!< Low part, all attentionable bits
    iscPlaInOldFlds    PLAin;		//!< PLA input,  partially attentionable bits
    iscNonAttnOldFlds  NonAttn;		//!< High part, non-attentionable
} iscStatusRegOldFlds;
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME                                                                       *
//  iscStatusRegFlds struct
//
// DESCRIPTION
/**
	\cond ISC_DOC
    \brief <i>PLA Inputs</i> portion of the <i>Status Register</i>.

    This is status register field definitions for the Meridian ISC.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct PACK_BYTES _iscStatusRegFlds {
    iscLowAttnFlds  LowAttn; 	 //!< Low part, all attentionable bits
    iscPlaInFlds    PLAin;		 //!< PLA input,  partially attentionable bits
    iscNonAttnFlds  NonAttn;	 //!< High part, non-attentionable
} iscStatusRegFlds; 
//                                                                           *
//****************************************************************************

//******************************************************************************
// NAME	
//	iscStatusReg union
/**
	\cond ISC_DOC
	\brief The Status Register definition for Meridian ISC.
**/
typedef union _iscStatusReg { 
	/**
		Bit-wise access to the status register contents.
	**/
	Uint16 bits[3];
	iscStatusRegFlds Fld;
} iscStatusReg;
//                                                                           *
//****************************************************************************


//******************************************************************************
// NAME	
//	iscAttnFlds structure
/**
	\cond ISC_DOC
	\brief Attention Register definition for Meridian Nodes.

	ISC Drive Attention fields.  \b NOTE: the numbers in
	description refer to the bit number in the register.
**/
typedef struct _iscAttnFlds {                // Attentionable 24-bits of status
    /**
		\brief Warning asserted.

        This bit asserts when the <i>Warning Register</i> contents,
        (#ISC_P_WARN_REG) ANDed with the user's <i>Warning Mask
        Register</i> contents (#ISC_P_WARN_MASK), is non-zero.
    **/
    BFld Warning        : 1;    /* 0 */ // Warning asserted
    /**
		\brief 	User alert asserted.

        This bit asserts when the <i>Alert Register</i> contents,
        (#ISC_P_ALERT_REG) ANDed with the user's <i>Alert Mask
        Register</i> contents (#ISC_P_ALERT_MASK), is non-zero.
    **/
    BFld UserAlert      : 1;    /* 1 */ // User alert asserted
    /**
		\brief Not ready for operation.

        This bit asserts when the node is not ready. Consult the
        <i>Status Register</i> (#mnStatusReg), as well as the other
        bits in this register, for the reasons why the node is not
        ready.

        If the shutdown is clearable, the user application can try to 
        clear the alerts with the IStatus::AlertsClear function. If the enable
        request (#plaOutReg::Enable) for the drive is still asserted and
        there are no other shutdown reasons, the node will
        automatically become ready.
		**/
    BFld NotReady       : 1;    /* 2 */ // Not ready for operation
    /**
		\brief Move buffer is available.

        This bit asserts when the move buffer is ready to
        accept another move command. Moves sent while this
        bit is de-asserted will be rejected by the node. Various
        alert states may prevent the node from executing motion.

        \see \ref ISCshutdownsPage
		\see \ref MoveAnatomy 
    **/
    BFld MoveBufAvail   : 1;    /* 3 */ // Move buffer is available
    /**
		\brief IEX input rise detected.

        This bit asserts when the <i>IEX Input Risen Register</i>
        (#ISC_P_IEX_IN_RISE_REG), ANDed with the user's <i>IEX Risen
        Mask Register</i> (#ISC_P_IEX_IN_RISE_REG_MASK), is non-zero.
        To clear this bit, the application must read the <i>IEX Inputs
        Risen Register</i> (#ISC_P_IEX_IN_RISE_REG).

    **/
    BFld IEXrise        : 1;    /* 4 */ // IEX input rise detected
    /**
		\brief IEX input fall detected.

        This bit asserts when the <i>IEX Input Fallen Register</i>
        (#ISC_P_IEX_IN_FALL_REG), ANDed with the user's <i>IEX Fallen
        Mask Register</i> (#ISC_P_IEX_IN_FALL_REG_MASK), is non-zero.
        To clear this bit, the application must read the <i>IEX
        Inputs Fallen Register</i> (#ISC_P_IEX_IN_FALL_REG).
    **/
    BFld IEXfall        : 1;    /* 5 */ // IEX input fall detected
    /**
		\brief IEX status and statistics changed.

        This bit asserts when the <i>IEX Status Register</i> (#ISC_P_IEX_STATUS) changes.
        It can be used to detect problems with the IEX subsystem
        due to IEX link issues or power problems with the IEX modules
        themselves. Use the #iscGetIEXStatus function to read the
        IEX system status.
    **/
    BFld IEXstatus      : 1;    /* 6 */ // IEX status and statistics
	/// \cond INTERNAL_DOC
    BFld na7            : 1;    // 7
	/// \endcond
    /**
		\brief Positive limit asserted (FW v5.4).

        This bit asserts when the positive position limit switch is
        activated.  The conditions for when this bit is set depend on the
        value of the #iscAppConfigReg::LimitMode setting in the
        <i>Application Configuration Register</i> (#ISC_P_APP_CONFIG_REG).
        Use the #iscSetAppConfigReg function to set up this mode.
        Commands in the positive direction  will be flushed when this bit
        is asserted.

		\note This feature requires ISC firmware version 5.4 and above.
		\see #plaInReg::Gpi2_posLim for this feature in older firmware.
    **/
    BFld InPosLimit     : 1;    /* 8 */ // Positive limit asserted
    /**
		\brief Negative limit asserted (FW v5.4).

        This bit asserts when the negative position limit switch
		is activated.  The conditions for when this bit is set depend
		on the value of the #iscAppConfigReg::LimitMode setting
        in the <i>Application Configuration Register</i>.
        (#ISC_P_APP_CONFIG_REG) Use the #iscSetAppConfigReg function
        to set up this mode.  Commands in the negative direction
		will be flushed when this bit is asserted.

		\note This feature requires ISC firmware version 5.4 and above.
		\see #plaInReg::Gpi3_negLim for this feature in older firmware.
	**/
    BFld InNegLimit     : 1;    /* 9 */ // Negative limit asserted
    /**
		\brief Motion Blocked (FW v5.4).

        This bit asserts when motion commands are blocked due 
        a motion related condition. Some of these include:
		- E-Stop
		- Motion command under-run
		- Repeated attempt to move into a position limit either
		via the hardware limit switch or the soft limits.

		\see #ISC_P_DRV_SOFT_LIM_POSN_POS
		\see #ISC_P_DRV_SOFT_LIM_POSN_NEG
		\see #iscAppConfigReg::LimitMode
		\see #alertReg::FwMoveBufUnderrun
		\see #alertReg::FwBadRASVelrequest
		\see #alertReg::FwMoveGenRange
		\see #alertReg::FwMoveGenHwLim

		\note This feature requires ISC firmware version 5.4 and above.
    **/
    BFld MotionBlocked     : 1; /* 10 */ // Motion Blocked
    /**
     	\brief Trigger Pulse Detected

		This bit indicates when a trigger pulse occurs. The action taken
		by the drive when a trigger occurs is dependent on the
		configuration and state of the drive at that time.
    **/
    BFld TriggerPulse		: 1; /* 11 */ // Trigger pulse detected
	/// \cond INTERNAL_DOC
    BFld na12_15	        : 4;    /* 12-15 */
	/// \endcond
    /**
		\brief Node Stopped.

        This bit represents the receipt of a node stop event. Depending
        on the method the node stop was initiated with, the node may
        or may not accept motion commands and may not allow changes to
        the output register. If the node stop involved an E-Stop request,
        this bit will remain on until the E-Stop state is cleared.

        Node stops with the <i>Quiet</i> bit set will not assert
        this bit.

        \see \ref ISCnodeStopPage
    **/
    BFld NodeStopped    : 1;    /* 16 */ // node stopped
    /**
		\brief Move Done and Settled.

        This bit is asserted at the end of every settled move command.
        Once the  #nonAttnReg::MoveCmdComplete state has been reached the
		node will check compliance with the <i>Move Done</i> criteria to
		be met. Once this occurs this bit will be asserted and remain so
		until the beginning of the next move command.

        For velocity type moves, this bit will be set once the
        node has attained the commanded velocity.

        \see \ref ISCmoveDonePage
    **/
    BFld MoveDone       : 1;    /* 17 */ // move done and settled
    /**
		\brief Tracking out of range.

        This bit asserts when the current <i>Tracking Error</i>
        (#ISC_P_POSN_TRK) has exceeded the <i>Tracking Range</i>
        (#ISC_P_POSN_TRK_RANGE) parameter.
    **/
    BFld OutOfRange     : 1;    /* 18 */ // not in range
    /**
		\brief <i>B Counts from End</i> of Move.

        This bit asserts when the move is <i>B From End</i>
        (#ISC_P_B_END) from the end of a positional move. 
        The basis of the move is selected by the
        #iscAppConfigReg::BBeforeEndMeas setting
        in the <i>Application Configuration Register</i>
        (#ISC_P_APP_CONFIG_REG). Use the #iscSetAppConfigReg function
        to set up this mode.

		\note Behavior is undefined for moves that cross a 32-bit
		signed position boundary

    **/
    BFld BFromEnd       : 1;    /* 19 */ // B counts from end
    /**
		\brief PLA Feedback output 0.

        This is a feedback term from the plaOutReg.

        \see \ref ISCplaPage
    **/
    BFld PlaOut0        : 1;    /* 20 */ // PLA output 0
    /**
		\brief PLA Feedback output 1.

        This is a feedback term from the plaOutReg.

        \see \ref ISCplaPage
    **/
    BFld PlaOut1        : 1;    /* 21 */ // PLA output 1
    /**
		\brief General Purpose Input 0.

        This bit asserts when the General Purpose Input 0 is asserted.
    **/
    BFld Gpi0           : 1;    /* 22 */ // general purpose input 0
    /**
		\brief General Purpose Input 1.

        This bit asserts when the General Purpose Input 1 is asserted.
    **/
    BFld Gpi1           : 1;    /* 23 */ // general purpose input 1
} iscAttnFlds; 
/// \endcond																  *
//*****************************************************************************


//****************************************************************************
// NAME                                                                      *
//  iscAlertFlds struct
//
// DESCRIPTION
/**
	\cond ISC_DOC
	\brief ISC Alert Register Fields
	
	This structure describes the ISC specific fields within the Alert 
	Register.
**/
typedef struct PACK_BYTES _iscAlertFlds
{
	/// Common shutdown events. Least significant bits 0-31.
	commonAlertFlds Common;
	//                              ----------------------------------------
	//                              |len|bit #|description                 |
	//                              ----------------------------------------
	/**
		\brief Bad Data Acquisition State

		The data acquisition system is in a bad state due to unsupported mode
		or option(s).

		Remedies:
		- Stop and restart the data acquisition system.
	**/
	BFld FwBadDataAcqState      : 1; /* 32 */ // Bad Data Acquisiton State
	/**
		\brief Internal error in Move Generator

		Move Generator Miscellaneous shutdown.

		Remedies:
		- Verify the move parameter ranges and options. There maybe
		a conflicting request.
	**/
	BFld FwMoveGenMisc          : 1; /* 33 */ // Internal error in Move Gen
	/**
		\brief Move Generator Off Target

		Move Generator was found off target. This is most likely a firmware
		error detected by an internal inconsistency.

		Remedies:
		- Update firmware
		- Try different acceleration and/or velocity limits.
	**/
	BFld FwMoveGenOffTarget     : 1; /* 34 */ // Move Gen off Target
	/**
		\brief Move > 8M Incremental

		Attempting to move too far. Although the command and measurement
		number space is 32 bits wide, this firmware has an internal limit
		of motion of about 8 million steps (2^31-1).

		Remedies:
		- Break up move into smaller pieces.
	**/
	BFld FwMoveGenRange         : 1; /* 35 */ // Move > 8M incremental
	/**
		\brief Jerk Limit Bad

		Move started with bad Jerk Limit request. An illegal or malformed
		Jerk Limit was specified for a move starting. This move is
		canceled.

		Remedies:
		- Check the Jerk Limit (#ISC_P_JERK_LIM) setting before
		issuing the move.
	**/
	BFld FwBadRASrequest        : 1; /* 36 */ // Jerk Lim bad
	/**
		\brief Move Buffer Underran

		Move Generator ran out of pending moves before terminal move.  This
		would occur with spline type moves when the host cannot keep up
		with the moves required to follow a path.

		Remedies:
		- Create moves with longer time intervals
		- Use a more efficient mechanism to keep move buffer busy. Sometimes
		an attention based mechanism will work.
	**/
	BFld FwMoveBufUnderrun      : 1; /* 37 */ // Move Buffer Underran
	/**
		\brief Velocity Limit Too High for Jerk Limit

		The move was canceled due to velocity limit exceeding the encoder's
		or Jerk Limit maximum allowed speed.

		Remedies:
		- Reduce the Velocity Limit (#ISC_P_VEL_LIM). Consult the
		Jerk Limit Maximum Velocity parameter
		(#ISC_P_RAS_MAX_VELOCIY) for the maximum allowable speed
		at the current Jerk Limit setting.
		- Change the Jerk Limit (#ISC_P_JERK_LIM) to one with
		less speed limiting.
	**/
	BFld FwBadRASVelrequest     : 1; /* 38 */ // Vel. Limit too high for RAS
	/**
		\brief Invalid move spec was altered

		The move could not meet both the distance and acceleration
		parameters of the move with the given initial velocity.
		The acceleration limit was increased to allow the
		move to complete without overshooting.

		Remedies:
		- Allow a longer distance for the terminal move
		- Increase the acceleration limit
		- Limit the velocity prior to the terminal move
	**/
	BFld FwMoveSpecAltered      : 1; /* 39 */ // Move spec altered
	/**
		\brief Phase Sensor Bad

		Phase Sensor failed Power-On Self Test.

		Remedies:
		- Replace drive.
	**/
	BFld HwPhaseSensor          : 1; /* 40 */ // Phase Sensor bad
	/**
		\brief Limit switch activated

		Attempting to move in a direction in which the limit switch
		is activated.

		Remedies:
		- Move away from the limit or clear the limit activation.
	**/
	BFld FwMoveGenHwLim         : 1; /* 41 */ // Limit switch activated
	/**
		\brief Soft Limit prevents move

		Attempting to move outside the range specified by the software
		limits.

		Remedies:
		- Move away from the limit or adjust the limit settings.
	**/
	BFld FwMoveGenSwLim         : 1; /* 42 */ // SoftLim range exceeded
	/**
		\brief Move Buffer Overrun

		Attempting to buffer up more move than the move buffer can hold.

		Remedies:
		- Slow down rate at which moves are being sent to the ClearPath to
		  allow moves to execute
	**/
	BFld FwMoveBufOverrunLockdown	: 1; /* 43 */ // Move Buffer Overrun
	/**
		\brief Courtesy 5V Overloaded

		Courtesy 5 Volt Power Supply is overloaded.

		Remedies:
		- Check for shorts on Motor or Load Encoder.
		- Check for shorts on General Purpose I/O connections.
		- If this persists with nothing attached to the ISC, the unit is
		defective and should be replaced.
	**/
	BFld Io5VSupplyOverload     : 1;  /* 44 */ // Courtesy 5V overloaded
	/**
		\brief Brake Overloaded

		Brake output is overloaded (for models that include brake output).

		Remedies:
		- Check for shorts in wiring harnesses.
		- Use a brake that requires less current.
	**/
	BFld IoBrakeOverload        : 1;  /* 45 */ // Brake output overloaded
	/**
		\brief GPO-0 Overloaded

		General Purpose Output 0 is overloaded (for models that support
		individual output overload detection).

		Remedies:
		- Check for shorts in wiring harnesses.
		- Reduce load on output (i.e., use LED instead of
		incandescent lights).
	**/
	BFld IoGPO0overload         : 1;  /* 46 */ // GPO-0 overloaded
	/**
		\brief GPO-1 Overloaded

		General Purpose Output 1 is overloaded (for models that support
		individual output overload detection).

		Remedies:
		- Check for shorts in wiring harnesses.
		- Reduce load on output (i.e., use LED instead of
		incandescent lights).
	**/
	BFld IoGPO1overload         : 1;  /* 47 */ // GPO-1 overloaded

	/**
		\brief GPO Overload Detector Bad

		General Purpose Overload Detector defective.

		Remedies:
		- Replace drive.
	**/
	BFld IoGPOdetector          : 1; /* 48 */ // GPO overload detector bad
	/**
		\brief GPO Overloaded

		A General Purpose Output is overloaded.

		Remedies:
		- Check for shorts in wiring harnesses.
		- Reduce load on output (i.e., use LED instead of
		incandescent lights).
	**/
	BFld IoGPOoverload          : 1; /* 49 */ // GPO overloaded
	/**
		\brief Invalid Inputs

		Inputs are in an invalid combination; HW enable
		occurred while ClearPath was in motion

		Remedies:
		- Check that HW inputs match the current SW inputs
		when switching from SW to HW enabled
	**/
	BFld IoInvalidInputs        : 1; /* 50 */ // IO invalid
	/**
		\brief Invalid Inputs - Lockdown

		Inputs are in an invalid combination; HW enable
		occurred while ClearPath was stopped

		Remedies:
		- Check that HW inputs match the current SW inputs
		when switching from SW to HW enabled
	**/
	BFld IoInvalidInputsLkdn    : 1; /* 51 */ // IO invalid
	/**
		\brief AC Power Lost

		Motor was running with AC power and AC power has been removed.

		Remedies:
		- Check AC power wiring and power quality to attached modules.
	**/
	BFld ACPowerLost		    : 1; /* 52 */ // AC Power lost
	/**
		\brief IEX Glitch Event

		IEX Module is detecting link communications troubles.

		Remedies:
		- Check wiring and power quality to attached modules.
		- Use higher quality wiring and/or shielding on link cables.
	**/
	BFld IEXlinkGlitch          : 1; /* 53 */ // IEX glitched detected
	/**
		\brief IEX Multiple Glitch Events

		IEX Module is detecting excessive link communications troubles
		and went offline.

		Remedies:
		- Check wiring and power quality to attached modules.
		- Use higher quality wiring and/or shielding on link cables.
	**/
	BFld IEXlinkMultiGlitch     : 1; /* 54 */ // IEX multiple glitch event
	BFld na55                   : 1; // 55

	/**
		\brief Commutation Bad Sequence

		A bad sequence was detected from the commutation sensors.

		Remedies:
		- Check wiring and cable shield to motor.
		- Replace motor.
		- Check for excessive electrical noise in system.
	**/
	BFld MtrCommBadSeq          : 1; /* 56 */ // Commutation Bad Sequence
	/**
		\brief Commutation Bad State Code

		An illegal '111' or '000' commutation signal pattern was detected.
		This may occur with Hudson-style motors that are over-temp and
		the ISC is not set up to distinguish this operational mode.

		Remedies:
		- Check wiring and cable shield to motor.
		- Replace motor.
		- Check for excessive electrical noise in system.
		- Check commutation handling settings for use with Hudson motors.
	**/
	BFld MtrCommBadState        : 1; /* 57 */ // Commutation Bad State code
	/**
		\brief Vector Problem

		The motor's commutation vector has drifted. This will lead to poor
		performance. This is normally a warning indication.

		Remedies:
		- Check wiring and shield to motor.
		- Fix defective motor
		- Check for excessive electrical noise in system.
		- Prevent encoder from slipping with respect to motor rotor/forcer.
	**/
	BFld MtrVector              : 1; /* 58 */ // Vector Problem
	/**
		\brief Critical Vector Problem

		The motor's commutation vector has drifted beyond tolerance. This will
		lead to poor performance. This is the shutdown due to the
		MtrCommBadState persisting or getting worse.

		Remedies:
		- Check wiring and shield to motor.
		- Defective motor
		- Check for excessive electrical noise in system.
		- Encoder slipping with respect to motor rotor/forcer.
	**/
	BFld MtrVectorCrit          : 1; /* 59 */ // Critical Vector Problem
	/**
		\brief Motor Encoder Glitch

		A glitch was detected on the motor encoder signals. This glitch occurs
		when the normal two phase sequence has transitioned through an
		illegal state.

		Remedies:
		- Check wiring and shield to motor.
		- Check for excessive electrical noise in system.
		- Defective motor.
	**/
	BFld MtrEncGlitch           : 1; /* 60 */ // Mtr Encoder glitch
	/**
		\brief Motor Encoder Overspeed

		The motor's encoder has exceeded the speed limit set up by the
		Motor Speed Limit (#ISC_P_DRV_MTR_SPEED_LIM) parameter.

		Remedies:
		- Reduce command speed.
		- Check wiring and shield to motor.
		- Check for excessive electrical noise in system.
		- Replace the defective encoder.
	**/
	BFld MtrEncOverspeed        : 1; /* 61 */ // Mtr Encoder overspeed
	/**
		\brief Load Encoder Glitch

		 A glitch was detected on the load encoder signals. This glitch occurs
		when the normal two phase sequence has transitioned through an
		illegal state.

		Remedies:
		- Check wiring and shield to motor.
		- Check for excessive electrical noise in system.
		- Defective encoder.
	**/
	BFld LdEncGlitch            : 1; /* 62 */ // Ld Encoder glitch
	/**
		\brief Load Encoder Overspeed

		The load encoder has exceeded the speed limit set up by the
		Load Speed Limit (#ISC_P_DRV_LD_SPEED_LIM) parameter.

		Remedies:
		- Reduce command speed.
		- Check wiring and shield to motor.
		- Check for excessive electrical noise in system.
		- Replace the defective encoder.
	**/
	BFld LdEncOverspeed         : 1; /* 63 */ // Ld Encoder overspeed
	/**
		\brief Motor Phase Overload

		A motor phase has overloaded.

		Remedies:
		- Check wiring for shorts.
		- Check motor tuning files.
		- Retune the motor torque parameters.
		- Defective motor.
	**/
	BFld MtrPhaseOverload       : 1; /* 64 */ // Phase overload
	/**
		\brief Motor Hot

		A motor too hot (for models that support a motor thermostat input).

		Remedies:
		- Reduce motion stress.
		- Add external motor cooling.
	**/
	BFld MtrThermalOverload     : 1; /* 65 */ // Thermal overload
	/**
		\brief Onboard Parameters Badly Set up

		A required parameter has an illegal value.

		Remedies:
		- Reset node to factory defaults and reload a valid configuration
		file.
	**/
	BFld MtrBadSetupParams      : 1; /* 66 */ // Bad Setup Parameter(s)
	/**
		\brief Hard Stop Broke Free

		The motor has broken free during hard stop clamping. Once the motor
		has entered the hard stopped mode, it must stay reasonably
		still. If the coupling or end stop should break free, the axis may
		unexpectedly accelerate.

		Remedies:
		- If this is due to clamping drift, adjust the <i> Hard Stop
		Entry Speed</i> (#ISC_P_DRV_HARDSTOP_ENTRY_SPD) parameter upwards.
	**/
	BFld SvoHardStopRelease     : 1; /* 67 */ // Hardstop released
	/**
		\brief Tracking Problem

		The position tracking error is about to exceed the critical tracking limit.
		This usually appears in the Warning Register first.

		Remedies:
		- Reduce feed rate.
		- Improve drive tuning.
	**/
	BFld SvoTracking            : 1; /* 68 */ // Tracking Problem
	/**
		\brief Critical Tracking Problem

		The position tracking error has exceeded the critical tracking limit.
		This is a resettable shutdown condition.

		Remedies:
		- Reduce feed rate.
		- Improve drive tuning.
		- Increase <i>Tracking Limit</i> (#ISC_P_DRV_TRK_ERR_LIM) to a more
		reasonable value.
	**/
	BFld SvoTrackingCrit        : 1; /* 69 */ // Critical Tracking Problem
	/**
		\brief RMS Overload

		The RMS load is approaching the critical limit.  This usually
		appears in the Warning Register first.

		Remedies:
		- Reduce feed rate.
		- Improve drive tuning.
		- Use a larger motor or drive.
	**/
	BFld SvoRMSoverload         : 1; /* 70 */ // RMS overload
	/**
		\brief Critical RMS Overload

		The RMS load has exceeded the critical limit. This is a resettable shutdown
		condition.

		Remedies:
		- Reduce feed rate.
		- Improve drive tuning.
		- Use a larger motor or drive.
	**/
	BFld SvoRMScritOverload     : 1; /* 71 */ // Critical Tracking Problem
	/**
		\brief Dual-Loop Coupling Error Exceeded

		The dual-loop coupling error between the motor and the load has
		exceeded the <i>Coupling Limit</i> (#ISC_P_DRV_CPL_ERR_LIM) parameter.
		The most common reason for this would involve the transmission
		between the motor and the load. A belt break or slip of the coupler
		would be a common problem leading to this condition.

		This is a resettable shutdown condition.

		Remedies:
		- Fix the broken or slipping coupling between the motor and the
		load.
		- Reduce feed rate.
		- Improve drive tuning.
		- Increase <i>Coupling Limit</i> (#ISC_P_DRV_CPL_ERR_LIM) to a more
		reasonable value.
	**/
	BFld SvoCoupling            : 1; /* 72 */ // Dual-Loop Coupling Problem
	/**
		\brief Step+Direction Input Noise

		For nodes that support the step and direction input, this warning
		occurs when there are detected glitches on the step and direction
		inputs.

		This is a resettable shutdown condition.

		Remedies:
		- Check wiring and shield to node.
		- Check for excessive electrical noise in system.
		- Defective motion controller.
	**/
	BFld SvoStepNoise           : 1; /* 73 */ // Step+Dir Noise Problem
	/**
		\brief Bus Voltage Saturation

		This warning occurs when the node required full bus voltage to
		drive the motor.

		Remedies:
		- Check power taps to bus supply.
		- Check torque/force loop tunings.
		- Check for mechanical issues such as binding.
		- Check gearing and motor sizing for this application.
		- See system configuration manual for more tips on this condition.
	**/
	BFld SvoVoltSat             : 1; /* 74 */ // Output Voltage Saturation
	/**
		\brief Torque/Force Saturation

		This warning occurs when the node commanded full torque/force to
		drive the motor.

		Remedies:
		- Check power taps to bus supply.
		- Check torque loop tunings.
		- Check for mechanical issues such as binding slides.
		- Check gearing and motor sizing for this application.
		- Check for the misapplication of any Torque/Force limiters such as
		the Global Torque/Force limit (#ISC_P_DRV_TRQ_LIM).
		- See system configuration manual for more tips on this condition.
	**/
	BFld SvoTrqSat              : 1; /* 75 */ // Torque/Force Saturated
	/**
		\brief Insufficient Sensorless Sweep Distance

		This shutdown occurs when a motor's sensorless start fails to
		sweep the appropriate amount.

		Remedies:
		- Check for excessive friction on axis.
		- Check for excessive external forces acting on the axis.
		- Check that motor wiring is correct. Phase or encoder miswiring
		can lead to this shutdown.
		- Verify that the configuration file is correct for the drive;
		reload the configuration file.
		- Check torque loop tunings.
		- Check for the misapplication of any Torque/Force limiters such as
		the Global Torque/Force limit (#ISC_P_DRV_TRQ_LIM).
	**/
	BFld NoCommSweepFailed      : 1; /* 76 */ // Sensorless sweep distance problem
	/**
		\brief Sensorless Motion in Wrong Direction

		This shutdown occurs when a motor's sensorless start detects
		the motor moving in the wrong direction.

		Remedies:
		- Check that motor wiring is correct. Phase or encoder miswiring
		can lead to this shutdown.
		- Verify that the configuration file is correct for the drive;
		reload the configuration file.
		- Check for excessive external forces acting on the axis.
	**/
	BFld NoCommSweepReversed    : 1; /* 77 */ // Sensorless sweep direction problem
	BFld NoCommFailed    		: 1; /* 78 */ // Sensorless failed after too many retries
	/**
		 \brief Index Count Zero Warning

		 No index pulses have been detected

		 Remedies:
		 - Return to Teknic
	 **/
	BFld IndexCountZeroWarn		: 1; /* 79 */ // No index detected
	/**
		\brief High Ambient Temperature

		The node has detected an excessive ambient operating
		temperature.  This is condition is recoverable when the
		temperature has been sufficiently reduced.

		Remedies:
		- Verify node mounting orientation is correct.
		- Provide more ventilation to drive.
		- See system configuration manual for more tips on this condition.
	**/
	BFld TempAmbient            : 1; /* 80 */ // High Ambient Temperature
	/**
		\brief Heatsink Hot

		The node has detected an excessive heatsink temperature. This is
		condition is recoverable when the temperature has been sufficiently
		reduced.

		Remedies:
		- Check mounting for lack of heat transfer to system heatsink.
		- Verify mounting orientation is correct.
		- Provide more ventilation to drive.
		- See system configuration manual for more tips on this condition.
	**/
	BFld TempHeatsink           : 1; /* 81 */ // High Heat Sink Temp.
	/**
		\brief Motor Supply Bus Short

		The node's bus (motor) current has exceeded maximum thresholds.

		Remedies:
		- Check phase wiring for short circuits.
		- Check for defective motor.
	**/
	BFld BusOverCurrent         : 1; /* 82 */ // Power Bus Short
	/**
		\brief Motor Supply Bus Voltage High

	   The node's bus (motor) voltage has exceeded maximum thresholds.
This is recoverable when the bus voltage has returned to a safe level.

		Remedies:
		- Check power supply taps for proper settings.
		- Check and/or install bus regenerated voltage bleed system.
	**/
	BFld BusOverVoltage         : 1; /* 83 */ // Power Bus Over-voltage
	/**
		\brief Motor Supply Bus Voltage Low

		The node has detected a loss or brown-out of the node's bus (motor)
		voltage. This condition will automatically clear upon restoration
		of the bus voltage without host intervention.

		Remedies:
		- Check that power supply taps have proper settings for locale.
		- Check and/or install bus regeneration bleed system.
	**/
	BFld BusVoltageLow          : 1; /* 84 */ // Power Bus Low
	/**
		\brief Motor Supply Bus Overloaded

		The node's bus (motor) current has exceeded maximum continuous thresholds.

		Remedies:
		- Check that power supply taps have proper settings for locale.
		- Verify the configuration file is correct for this node.
		- Reduce motion duty cycle and/or aggressiveness.
		- Check phase wiring for short circuits.
	**/
	BFld BusRMSoverload         : 1; /* 85 */ // Power Bus Overload
	/**
		\brief Fan Speed Low

		For models that include a fan, this warning condition occurs
		when the fan appears jammed.

		Remedies:
		- Replace drive
	**/
	BFld FanSpeedLow            : 1; /* 86 */ // Fan on but not up to speed
	/**
		\brief MtrEncIndexMissing

		The motor encoder index seen when it was not expected
		or not seen when it was expected

		Remedies:
		- Replace drive
	**/
	BFld MtrEncIndexMissing     : 1; /* 87 */ // Mtr index is missing
	BFld NoCommHwErr         	: 1; /* 88 */ // Hall-less failed due to HW problem
	BFld NoCommBusLow         	: 1; /* 89 */ // Hall-less failed due low bus voltage
	BFld NoCommSetupErr         : 1; /* 90 */ // Hall-less failed due incorrect settings
	/**
		\brief Home Limit Lockdown

		Attempting to move past the home position
	**/
	BFld HomeLimLockdown         : 1; /* 91 */ // Home Limit
	/**
		\brief Travel Limit Lockdown

		Attempting to move past the travel limits; clears on re-enable
	**/
	BFld TravelLimLockdown		: 1; /* 92 */ // Travel Limit
	BFld MtrEncIndexMisplaced	: 1; /* 93 */ //  Mtr index seen at an incorrect location
} iscAlertFlds;
/// \endcond																  *
//*****************************************************************************




//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//                      NOT DOCUMENTED ITEMS - BELOW
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//****************************************************************************
// NAME                                                                      *
//  iscOptionsFlds struct
//
// DESCRIPTION
/**
	\cond INTERNAL_DOC
	\brief ISC Option Register Fields
	
	This structure describes the ISC specific fields within the Option 
	Register.
**/
typedef struct PACK_BYTES _iscOptionsFlds
{
    /*                   -----------  ----------------------------------- *
     *                         bit #  description                         *
     *                   -----------  ----------------------------------- */
    BFld VoltMode        : 1; // 0:  Volt mode setup
    BFld RuntimeCal      : 1; // 1:  1=Enable run time calibration
    BFld nDualLoop       : 1; // 2:  1=does not support dual-loop
    BFld nClosedStepper  : 1; // 3:	1=does not support closed-loop stepper

    BFld Product    : 3; // 4:  3-bit product number, 0=U/R, 1=J/K, 2=T
                             // 5:  ""
                             // 6:	""
    BFld LPB             : 1; // 7:  LPB mode, 0=disable, 1=enable

    BFld OptionBrd  : 3; // 8:  Installed option board (LSB)
                             // 9:  ""
                             // 10: Installed option board (MSB)
    BFld VoltTable  : 1; // 11 0=S,P volt table, 1=G

    BFld HwPlatform : 4; // 12: 4-bit hardware platform code (LSB)
    						 // 13: ""
                             // 14: ""
                             // 15: "" (MSB)

    BFld CaseType   : 3; // 16: Case type (LSB)
                             // 17:
                             // 18: Case type (MSB)
    BFld FWverMask       : 1; // 19: Inhibit display of minor FW version

    BFld ExtWty     : 1; // 20: Extended Wty option
    BFld Vendor     : 4; // 21: Vendor code (LSB)
                             // 22:  0=TEKNIC
                             // 23:

                             // 24: Vendor code (MSB)
    BFld Debug      : 3; // 25: Debug Mode (LSB)
                             // 26:  0,7=off
                             // 27: Debug Mode (MSB)
    BFld na28_31	: 4; // 28-31
} iscOptionsFlds;
/// \endcond																  *
//*****************************************************************************

//****************************************************************************
// Restore byte packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif
//****************************************************************************

/// \cond INTERNAL_DOC


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * MONITOR PORT SETTINGS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifndef __TI_COMPILER_VERSION__             // Not needed on dsp

typedef  monTestPoints iscMonVars;

typedef enum _iscTuneSyncs {
    ISC_MON_SYNC_OFF,           //  0: None
    ISC_MON_SYNC_POS_START = 1, //  1: At Start of Positive Move
    ISC_MON_SYNC_NEG_START = 2, //  2: At Start of Negative Move
    ISC_MON_SYNC_POS_END = 4,   //  4: At End of Positive Move
    ISC_MON_SYNC_NEG_END = 8,   //  8: At End of Negative Move
    ISC_MON_SYNC_SHUTDOWN = 16  // 16: At Shutdown
} iscTuneSyncs;

// VB Friendly structure
/**
    \brief Monitor Port Settings structure.

    This structure controls the selection of the diagnostic test points within
    the drive.
**/
typedef struct _iscMonState {
    double          gain;       ///<  Gain setting
    double          filterTC;   ///<  Output filter time constant (ms)
    monTestPoints   var;        ///<  Test point to display
    iscTuneSyncs    tuneSync;   ///<  Is a bit field
} iscMonState;

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef PACK_BYTES struct _iscMonNodeState {
    nodeushort      var;        // 16-bit variable selector
    nodelong        gain;       // 32-bit gain
    nodeshort       filter;     // 16-bit filter TC
    nodeshort       tuneSync;   // 16-bit (should be bitfield)
} iscMonNodeState;
#endif // __TI_COMPILER_VERSION__
/// \endcond
///**
//    \brief Data Collection Return structure (v5.6)
//
//    This structure is updated with the VB friendly results of the
//	data collection command.
//	
//	\note This feature appears on ISC firmware version 5.6 and later.
//**/
//typedef PACK_BYTES struct _iscDataCollect {
//	double			LowPassRMS;		///< Low Pass RMS of monitor port value
//	double			LowPassMaxPos;	///< Maximum positive of monitor port value
//	double			LowPassMaxNeg;	///< Maximum negative of monitor port value
//	double			HighPassRMS;	///< High Pass RMS of monitor port value
//	double			DurationMS;		///< Duration (msec)
//	nodelong		MaxTrackingPos;	///< Maximum positive tracking
//	nodelong		MaxTrackingNeg;	///< Maximum negative tracking
//} iscDataCollect;
/// \cond INTERNAL_DOC
/**
    \brief Data Collection Return Payload defintion (v5.6).

    This structure overlays the data returned to extract data collection
	information to return via iscDataCollect.

	\note This feature appears on ISC firmware version 5.6 and later.
**/
#define ISC_CMD_DATA_COLLECT_RESP_LEN 17
typedef PACK_BYTES union _iscDataCollectPayload {
	Uint16 buffer[(ISC_CMD_DATA_COLLECT_RESP_LEN+1)/2];
	struct   {	 
		// Mean^2 filtered sums during motion generation
		nodeushort	LPmeanSqLSBs;		///< Lower 16-bit sum
		nodeushort	HPmeanSqLSBs;		///< Lower 16-bit sum
		// Maximums detected of monitor port channel. Cleared on read or
		// at start of move depending on tune register setting. 
		_iq15_s		LPmeasMaxPos;		///< Maximum LP measured (positive)
		_iq15_s		LPmeasMaxNeg;		///< Maximum LP measured (negative)
		// Maximums of position tracking. Cleared on read or
		// at start of move depending on tune register setting.
		nodeshort	TrackingMaxPos;		///< Maximum tracking (positive)
		nodeshort	TrackingMaxNeg;		///< Maximum tracking (negative)
		// Remainder of mean^2 data with scaling factors. These are
		// here to preserve memory alignment and packing considerations.
		unsigned	LPmeanSqScale:5;	///< 2^n scaling factor for HP sum
		signed		LPmeanSqMSBs:3;		///< Upper MSBs of LP mean^2 sum
		unsigned	HPmeanSqScale:5;	///< 2^n scaling factor for HP sum
		signed		HPmeanSqMSBs:3;		///< Upper MSBs of HP mean^2 sum
		unsigned	SampleTimesLSB:16;	///< Sample-times of measurement
		signed		SampleTimesMSB:5;	///< MSBs of sample-times
	} data;
} iscDataCollectPkt;

const int32 MAX_DATA_COLLECT_SAMPLES = (1L<<20);
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * STIMULUS GENERATOR SETTINGS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// Stimulus mode types
typedef enum _stimModes {
    STIM_OFF    = 0,            ///<  Off
    STIM_VEL    = 1,            ///<  Velocity toggle
    STIM_TRQ    = 2,            ///<  Torque toggle
    STIM_POSN   = 3,            ///<  Position toggle
    STIM_CAL    = 4,            ///<  Calibrate/test value
    STIM_MOVE_ONCE= 5,          ///<  Profiled single move
    STIM_MOVE_RECP= 6,          ///<  Profiled reciprocating moves
    STIM_MOVE_REPEAT= 7,        ///<  Repeated unidirection moves
    STIM_REF_MAX
} stimModes;

typedef enum _iscStimStatus {
    ISC_STIM_STATUS_SST,
    ISC_STIM_STATUS_EXTENDED
} iscStimStatus;

// Stimulus Request Packet Format used to create stimulus settings
// and to extract stimulus state.

typedef PACK_BYTES struct _iscStimCmdPkt {
    nodeshort       Mode;       // Operational mode
    nodeshort       Period;     // Period in sample-times
    nodelong        Amplitude;  // Amplitude in mode units
    // - short form "classic" request end
    nodeshort       Slew;       // Profiled slew sample-times
    nodeshort       Dwell;      // Dwell sample-times
    // - short form "profiled" request end
    nodeshort       Status;     // Status bits (status request only)
} iscStimCmdPkt;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
// Command sizes
#define ISC_STIM_CMD_PKT_PROF_SIZE (1+4*OCTET_SIZE(nodeshort)+OCTET_SIZE(nodelong))
#define ISC_STIM_CMD_PKT_SIZE (1+2*OCTET_SIZE(nodeshort)+OCTET_SIZE(nodelong))
#define ISC_STIM_CMD_STATUS_SIZE 1
#define ISC_STIM_RESP_STATUS_OCTETS 14

// VB/C friendly stimulus request structure
/**
    \brief Stimulus Move Generator Settings structure.

    This structure controls the internal tuning stimulus move generator.
**/
typedef struct _iscStimState {
    double          amplitude;  ///< Amplitude mode specific units
    double          period;     ///< Stimulus period for "toggle" types
    long            slew;
    long            dwell;
    long            bits;
    stimModes       mode;       // Operational mode
} iscStimState;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * DRIVE COMMAND NUMBERS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
typedef enum _iscCmds {
    ISC_CMD_GET_PARAM = MN_CMD_GET_PARAM0,  // 0 Get parameter (bank 0)
    ISC_CMD_SET_PARAM = MN_CMD_SET_PARAM0,  // 1 Set parameter (bank 0)
    ISC_CMD_GET_PARAM1 = MN_CMD_GET_PARAM1, // 2 Get parameter (bank 1)
    ISC_CMD_SET_PARAM1 = MN_CMD_SET_PARAM1, // 3 Set parameter (bank 1)
    ISC_CMD_NODE_STOP  = MN_CMD_NODE_STOP,  // 4 E-stop (Node Stop, Stim Cancel)
    ISC_CMD_NET_ACCESS = MN_CMD_NET_ACCESS, // 5 Set net access level
    ISC_CMD_USER_ID = MN_CMD_USER_ID,       // 6 Get/Set the user ID
    ISC_CMD_CHK_BAUD_RATE = MN_CMD_CHK_BAUD_RATE, // 7 Get OK for proposed baud rate
    ISC_CMD_ALERT_CLR = MN_CMD_ALERT_CLR,   // 8 Clear non-serious Alert reg bits
    ISC_CMD_ALERT_LOG = MN_CMD_ALERT_LOG,   // 9 Read, clear, mark epoch of Alert log

    // Start of node specific commands
    ISC_CMD_MOTOR_FILE=MN_CMD_COMMON_END,   // 16 Motor file name
    ISC_CMD_ADD_POSN=19,                    // 19 Adjust servo position
    ISC_CMD_ADD_POSN_LD,                    // 20 Adjust open loop load encoder
    ISC_CMD_SYNC_POSN,                      // 21 Synchronize position
    ISC_CMD_GET_SET_MONITOR,                // 22 Monitor port setting
    ISC_CMD_GET_SET_STIMULUS,               // 23 Stimulus settings
    ISC_CMD_RE_VECTOR,                      // 24 Reset the vector search
    ISC_CMD_DATA_ACQ,                       // 25 Data acquisition/scope feature
    ISC_CMD_TEST_GEN,                       // 26 Test Generator

	ISC_CMD_DATA_COLLECT = 28,				// 28 Special data collection command (v5.6)

    // Short form position motion initiators
    ISC_CMD_MOVE_POSN_ABS=64,//+MG_MOVE_STYLE_NORM_ABS,	// 64 Mv posn: abs
    ISC_CMD_MOVE_POSN_REL,//64+MG_MOVE_STYLE_NORM,		// 65 Mv posn: relative
    ISC_CMD_MOVE_POSN_ABS_TRIG,//=64+MG_MOVE_STYLE_NORM_TRIG_ABS,// 66 Mv posn: abs
    ISC_CMD_MOVE_POSN_REL_TRIG,//=64+MG_MOVE_STYLE_NORM_TRIG,	 // 67 Mv posn: relative
    // Short form velocity motion initiator
    ISC_CMD_MOVE_VEL=68,                    // 68 Move velocity
    ISC_CMD_MOVE_VEL_TRIG,                  // 69 Move velocity

    // Full feature motion initiator
    ISC_CMD_MOVE_POSN_EX,                   // 70 Move Posn: expanded
    ISC_CMD_MOVE_VEL_EX                     // 71 Move Velocity: expanded
} iscCmds;


#ifndef __TI_COMPILER_VERSION__             // Skip DSP C problematic defs
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// POSITIONAL MOVE COMMANDS AND DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Expanded Profile move command format
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef PACK_BYTES struct _iscMoveProfiledExPacket {
    int style                 : 8;  // Format of move (mgPosnStyle)
    int value                 :32;  // Distance of the motion
} iscMovePosnExPacket;
#ifdef _MSC_VER
#pragma pack(pop)                   // Align to bytes boundaries
#endif

#endif // __TI_COMPILER_VERSION__

//                                                                            *
//*****************************************************************************

enum ISC_RAS_CODES {
    ISC_RAS_OFF = 0,                    // 0  OFF
    ISC_RAS_3MS,                        // 1  3 ms
    ISC_RAS_5MS,                        // 2  5 ms
    ISC_RAS_9MS,                        // 3  9 ms
    ISC_RAS_15MS,                       // 4  15 ms
    ISC_RAS_24MS,                       // 5  24 ms
    ISC_RAS_44MS                        // 6  44 ms
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TEST POINT I/O REGISTER
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  I/O test point register compatible with all drives.

// BIT NUMBERS and MASKS
typedef enum _TPIObits {
    TPIO_OVERV_BIT      =0,             // Over-voltage
    TPIO_MTR_INDX_BIT   =1,             // Motor index bit logic level
//                      =2,
//                      =3,

    TPIO_NOT_POS_LIM_BIT=4,             // +Limit switch hit
    TPIO_NOT_NEG_LIM_BIT=5,             // -Limit switch hit
    TPIO_READY_OUT_BIT  =6,             // Ready (not, out)
    TPIO_COMM_1_BIT     =7,             // Hall effect feedback 1

    TPIO_COMM_2_BIT     =8,             // Hall effect feedback 2
    TPIO_COMM_3_BIT     =9,             // Hall effect feedback 3
    TPIO_ENABLE_IN_BIT  =10,            // Enable drive in
    TPIO_AMP_DIS_BIT    =11,            // Amplifier disable bit

    TPIO_MTR_THERM_BIT  =12,            // Motor Over Temp
    TPIO_NOT_OVERI_BIT=13,              // Hardware fault detected (not)
    TPIO_TEMP_FAULT_BIT =14,            // Temperature fault detected
    TPIO_MODE_IN_BIT    =15             // Enable torque drive
} TPIObits;

// MASKS
typedef enum {
    TPIO_NOT_POS_LIM=(1<<TPIO_NOT_POS_LIM_BIT), // +Limit switch hit
    TPIO_NOT_NEG_LIM=(1<<TPIO_NOT_NEG_LIM_BIT), // -Limit switch hit
    TPIO_READY_OUT  =(1<<TPIO_READY_OUT_BIT),   // Ready output (not)
    TPIO_MTR_THERM  =(1<<TPIO_MTR_THERM_BIT),   // Motor Over Temperature
    TPIO_COMM_1     =(1<<TPIO_COMM_1_BIT),      // Hall effect feedback 1
    TPIO_COMM_2     =(1<<TPIO_COMM_2_BIT),      // Hall effect feedback 2
    TPIO_COMM_3     =(1<<TPIO_COMM_3_BIT),      // Hall effect feedback 3
    TPIO_ENABLE_IN  =(1<<TPIO_ENABLE_IN_BIT),   // Enable drive in
    TPIO_AMP_DIS_OUT=(1<<TPIO_AMP_DIS_BIT),     // Amp Disabled (out)
    TPIO_NOT_OVERI  =(1<<TPIO_NOT_OVERI_BIT),   // Fault in (not)
    TPIO_TEMP_FAULT =(1<<TPIO_TEMP_FAULT_BIT),  // Temperature fault (not)
    TPIO_MODE_IN    =(1<<TPIO_MODE_IN_BIT),     // Torque mode enable
    TPIO_MTR_INDX   =(1<<TPIO_MTR_INDX_BIT)     // Motor Index logic level
} TPIOmasks;

typedef union _tpIOreg {
#ifdef __cplusplus
public:
#endif
    int16 bits;
        /*                   -----------  ----------------------------------- *
         *                         bit #  description                         *
         *                   -----------  ----------------------------------- */
    struct _tpIOfld {
        BFld OverVoltage        : 1;    // 0  Over-Voltage detected
        BFld MtrIndex           : 1;    // 1  Motor Index track logic level
        BFld na23               : 2;    // 2-3
        BFld PosLimit           : 1;    // 4  Positive limit
        BFld NegLimit           : 1;    // 5  Negative limit
        BFld nReadyOut          : 1;    // 6  Shutdown (not, out)
        BFld Commutation        : 3;    // 7-9  Commutation TSR code
        BFld EnableIn           : 1;    // 10 Enable request
        BFld AmpDisable         : 1;    // 11 Amplifier stage disable (out)
        BFld MtrOverTemp        : 1;    // 12 Motor Over Temperature
        BFld nOverCurrent       : 1;    // 13 Overcurrent (not)
        BFld Thermostat         : 1;    // 14 Thermostat input
        BFld ModeIn             : 1;    // 15 Mode line
    } Fld;
    #ifdef __cplusplus
    _tpIOreg() { bits = 0; };
    _tpIOreg(int16 initial) { bits = initial; };
    #endif
} tpIOreg;
//                                                                            *
//*****************************************************************************


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Number of bytes in each of these commands
#define ISC_CMD_MOVE_PROF_LEN       5
#define ISC_CMD_MOVE_SPLINE_LEN     10
#define ISC_CMD_MOVE_PROF_EX_LEN    5
#define ISC_CMD_MOVE_FLUSH_LEN      1
#define ISC_CMD_MOVE_VEL_EX_LEN     8
#define ISC_CMD_GET_PLA_LEN         2
#define ISC_CMD_SET_PLA_BASE_LEN        2       // + fuses
#define ISC_CMD_ADD_POSN_LEN            5
#define ISC_CMD_SYNC_POSN_LEN       1
#define ISC_CMD_GET_ACQUISITION_LEN 1
#define ISC_CMD_ACQUIRE_CONTROL_LEN 5

// Velocity scaling factor
#define ISC_VEL_MOVE_SCALE (1LL<<MV_VEL_Q)  // Velocity scaling factor
/// \endcond

/// @}

#endif // _PUBISCREGS_H_
//=============================================================================
//  END OF FILE pubIscRegs.h
//=============================================================================

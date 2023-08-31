//*****************************************************************************
// DESCRIPTION:
/**
	\file
	\brief S-Foundation register definitions for programming ClearPath-SC nodes.

	This header defines the node specific registers of the ClearPath-SC node.
	These include:
	- Status Register
	- Application Configuration
	- Hardware Configurations
	- Alert Register fields
	- Output Register fields
**/
// PRINCIPLE AUTHORS:
//		DWS
//
// CREATION DATE:
//		2015-12-23 16:26:15
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2015-2017  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
#ifndef __PUBCPM_REGS_H__
#define __PUBCPM_REGS_H__

//*****************************************************************************
// NAME																          *
// 	pubCPM_API.h headers
//
	#include "tekTypes.h"
	#include "pubMotion.h"
	#include "pubRegFldCommon.h"
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME                                                                       *
//  pubCpmRegs.h constants
//
													/** \cond INTERNAL_DOC **/
#define ALERTREG_SIZEOCTETS		12
#define SC_USER_DESCR_SPAN		5		// Number parameters in
#define SC_USER_DESCR_CHUNK 	25		// # chars per parameter
// Max description size
#define SC_USER_DESCR_SZ 		(CP_USER_DESCR_SPAN*CP_USER_DESCR_CHUNK)
													/** \endcond **/

													/** \cond INTERNAL_DOC **/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
    ClearPath-SC Parameter Numbers.
    \public
    This enumeration defines the set of ClearPath-SC registers accessible using 
	the sFoundation #sFnd::IInfoEx class.
**/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef enum _cpmParams {
// Bank 0 - Core Parameters
    CPM_P_NODEID,                        // 0  00 Device ID
    CPM_P_FW_VERS,                       // 1  01 FW version
    CPM_P_HW_VERS,                       // 2  02 HW version
    CPM_P_RESELLER_ID,                   // 3  03 Reseller ID
    CPM_P_SER_NUM,                       // 4  04 Unit serial number
    CPM_P_OPTION_REG,                    // 5  05 Unit options
    CPM_P_ROM_SUM_ACK,                   // 6  06 Firmware update ack
    CPM_P_ROM_SUM,                       // 7  07 Firmware checksum
    CPM_P_SAMPLE_PERIOD,                 // 8  08 Sample period (nsec)
    CPM_P_ALERT_REG,                     // 9  09 Shutdown register
    CPM_P_STOP_TYPE,                     // 10 0a Node Stop Type
    CPM_P_WATCHDOG_TIME,                 // 11 0b Watchdog time constant

	CPM_P_STATUS_ACCUM_REG=13,           // 13 0d Status accum register
    CPM_P_STATUS_RISE_REG,               // 14 0e Status rise register
    CPM_P_STATUS_RT_REG=16,              // 16 10 Status reg (realtime)
    CPM_P_TIMESTAMP,                     // 17 11 8-bit timestamp
    CPM_P_TIMESTAMP16,                   // 18 12 16-bit timestamp
    CPM_P_PART_NUM,                      // 19 13 Part Number String
    CPM_P_EE_UPD_ACK,                    // 20 14 EE Update Acknowlegde
    CPM_P_EE_VER,                        // 21 15 EE Version Number
    CPM_P_STATUS_FALL_REG,               // 22 16 Status fall register
    CPM_P_HW_CONFIG_REG,                 // 23 17 Hardware config/setup reg
    CPM_P_APP_CONFIG_REG,                // 24 18 Feature config/setup reg
    CPM_P_WARN_REG,                      // 25 19 Warnings pending
    CPM_P_WARN_RT_REG,                   // 26 1a Warnings (realtime)
    CPM_P_WARN_MASK,                     // 27 1b Warnings for attention
    CPM_P_ALERT_MASK,                    // 28 1c Alert selection
    CPM_P_ON_TIME,                       // 29 1d Unit On Time
	CPM_P_POSN_CAP_INA_HI_SPD,			 // 30 1e Captured position (Input A, high-speed capture)
    CPM_P_USER_RAM0,                     // 31 1f User RAM parameter
    CPM_P_USER_DATA_NV0,                 // 32 20 User NV (13-bytes)
    CPM_P_USER_DATA_NV1,                 // 33 21 User NV (13-bytes)
    CPM_P_USER_DATA_NV2,                 // 34 22 User NV (13-bytes)
    CPM_P_USER_DATA_NV3,                 // 35 23 User NV (13-bytes)
    CPM_P_NETERR_APP_CHKSUM,             // 36 24 Application Net Checksum counter
    CPM_P_NETERR_APP_FRAG,               // 37 25 Application Net Fragment counter
    CPM_P_NETERR_APP_STRAY,              // 38 26 Application Net Stray data counter
    CPM_P_NETERR_APP_OVERRUN,            // 39 27 Application Net Overrun counter
    CPM_P_NETERR_DIAG_CHKSUM,            // 40 28 Diagnostic Net Checksum counter
    CPM_P_NETERR_DIAG_FRAG,              // 41 29 Diagnostic Net Fragment counter
    CPM_P_NETERR_DIAG_STRAY,             // 42 2a Diagnostic Net Stray data counter
    CPM_P_NETERR_DIAG_OVERRUN,           // 43 2b Diagnostic Net Overrun counter

	CPM_P_ATTN_MASK=45,					 // 45 2d Attention mask register
	CPM_P_GP_TIMER,						 // 46 2e General-purpose timer period
	CPM_P_AT_POSN_LOC,					 // 47 2f At Position location
	CPM_P_A_START,						 // 48 30 A after start location
	CPM_P_B_END,						 // 49 31 B before end location
	CPM_P_USER_OUT_REG,					 // 50 32 User output settings

	CPM_P_OUT_REG = 52,					 // 52 34 Output register
    CPM_P_DRV_MTR_SPEED_LIM,             // 53 35 Motor Speed Limit (quads/msec)

	CPM_P_ATTN_DRVR_MASK=55,			 // 55 37 Driver's attention register
	CPM_P_STATUS_EVENT_MASK,			 // 56 38 Status Event Mask
// Motion Constraint Group
	CPM_P_APP_RAS_DELAY,				 // 57 39 Stopping delay caused by RAS
    CPM_P_RAS_MAX_VELOCITY=60,	 		 // 60 3c Maximum velocity for current RAS
    CPM_P_VEL_LIM,                       // 61 3d Trapezoidal velocity limit
    CPM_P_ACC_LIM,                       // 62 3e Trapezoidal acceleration limit
    CPM_P_JERK_LIM,                      // 63 3f Jerk limit
    CPM_P_DEC_LIM,                       // 64 40 Deceleration Limit
    CPM_P_STOP_ACC_LIM,                  // 65 41 Stop acceleration limit
	CPM_P_HEAD_DISTANCE, 				 // 66 42 Head distance
	CPM_P_TAIL_DISTANCE,				 // 67 43 Tail distance
	CPM_P_HEADTAIL_VEL,					 // 68 44 Head/Tail velocity
	CPM_P_MOVE_DWELL,                    // 69 45 Post-move dwell time (sec)

	CPM_P_DRV_STOP_QUALIFY_TC=72,		 // 72 48 Stopped qualification time (ms)

	CPM_P_DRV_STOP_QUALIFY_VEL=74,		 // 74 4a Stopped qualify velocity
// Motion Status Group
    CPM_P_DRV_TRQ_CMD=80,                // 80 50 Commanded torque
    CPM_P_DRV_TRQ_MEAS,                  // 81 51 Measured torque 
    CPM_P_DRV_RMS_LVL,                   // 82 52 RMS level
    CPM_P_POSN_MEAS,                     // 83 53 Measured position
    CPM_P_POSN_CMD,                      // 84 54 Commanded position
    CPM_P_VEL_MEAS,                      // 85 55 Measured velocity
    CPM_P_VEL_CMD,                       // 86 56 Commanded velocity

    CPM_P_DRV_RMS_SLOW_LVL=88,           // 88 58 RMS level (slow acting)

    CPM_P_POSN_TRK=90,                   // 90 5a Position tracking error
    CPM_P_POSN_TRK_RANGE,                // 91 5b Tracking in-range window
    CPM_P_DRV_MV_DN_TC,                  // 92 5c Move Done: Time Constant (ms)

	CPM_P_VEL_TRK_RANGE = 97,			 // 97 61 Vel tracking pct (1.15)

    CPM_P_INA_TC = 99,					 // 99 Input A filter (ms)
	CPM_P_INB_TC = 102,	                 // 102 Input B filter (ms)
    
    // BANK 1 items (Drive/Motor Configuration)
// Factory Settings
    CPM_P_DRV_ADC_MAX=256,               // 256 0  ADC full scale (A)
    CPM_P_DRV_I_MAX,                     // 257 1  Current Full Scale (A)
    CPM_P_DRV_RMS_MAX,                   // 258 2  Factory RMS limit
    CPM_P_DRV_FACT_IR_CAL,               // 259 3  Ir Sensor Calibration
    CPM_P_DRV_FACT_IS_CAL,               // 260 4  Is Sensor Calibration
    CPM_P_DRV_FACT_KM_FACTOR,            // 261 5  Km Factor
    CPM_P_DRV_FACT_MTR_MIN_RES,          // 262 6  Mtr Min Res
    CPM_P_DRV_FACT_IB_TC,                // 263 7  IB time constant
    CPM_P_DRV_FACT_IB_TRIP,              // 264 8  IB Trip Point (A)
    CPM_P_DRV_FACT_PHASE_TRIP,           // 265 9  Phase Trip Point (A)
    CPM_P_DRV_FACT_FS_BUSV,              // 266 A Full scale bus volts
    CPM_P_FACT_OVER_V_TRIP=268,			 // 268 0c Over-voltage shutdown point
    CPM_P_FACT_VBUS_CAL,				 // 269 0d Bus voltage calibration factor
// Safety Items
    CPM_P_DRV_RMS_LIM,					 // 270 0e RMS shutdown limit (A)
    CPM_P_DRV_RMS_TC,                    // 271 0f RMS time constant (sec) @ 100% trq
    CPM_P_POSN_TRK_ERR_LIM,              // 272 10 Position tracking rrror limit
	CPM_P_DRV_RMS_INIT,					 // 273 11 RMS initial value on reset (%)
	CPM_P_DRV_IB_PEAK,					 // 274 12 IB Level Peak
    CPM_P_DRV_RMS_SLOW_LIM,				 // 275 13 RMS shutdown limit (A) (Slow acting)
	CPM_P_TP_PHASE_BD = 276,			 // 276 14 Phase board test point

	CPM_P_POSN_CAP_INB=277,				 // 277 15 Captured position (Input B)
// Drive State Items   
    CPM_P_DRV_BUS_VOLTS=280,             // 280 18 Real-time Bus Voltage (V)
    CPM_P_DRV_CONFIG_CHANGED,            // 281 19 Configuration changed
    CPM_P_DRIVE_TEMP=291,              	 // 291 23 Drive temperature (deg C)
    CPM_P_DRV_IB_LVL=294,                // 294 26 IB Level

	CPM_P_OVER_TEMP_TRIP=308,			 // 308 34 Over-Temp Trip Point
// Motor Parameters                     
    CPM_P_DRV_KIP_ADJ,		             // 309 35 Vector Torque: Kip Adjust
    CPM_P_DRV_KIP=318,                   // 318 3e Vector Torque: Kip
    CPM_P_DRV_KII,                       // 319 3f Vector Torque: Kii
    CPM_P_DRV_STEP_DENS,                 // 320 40 Step+Dir Resolution (counts/rev)

	CPM_P_CMD_CNTS_PER_REV=322,			 // 322 42 Command encoder density (counts/rev)
    CPM_P_DRV_KR,                        // 323 43 Compensator: Kr
    CPM_P_DRV_KII_ADJ,                   // 324 44 Vector Torque: Kii Adjust
// Tuning parameters                    
    CPM_P_DRV_TUNE_CONFIG_REG,		     // 325 45 Tuning Configuration Register
    CPM_P_DRV_KV,                        // 326 46 Compensator: Kv
    CPM_P_DRV_KP,                        // 327 47 Compensator: Kp
    CPM_P_DRV_KI,                        // 328 48 Compensator: Ki
    CPM_P_DRV_KFV,                       // 329 49 Compensator: Kfv
    CPM_P_DRV_KFA,                       // 330 4a Compensator: Kfa
    CPM_P_DRV_KFJ,                       // 331 4b Compensator: Kfj (jerk ff)

    CPM_P_DRV_KNV=333,                   // 333 4d Compensator: Knv
	CPM_P_DRV_AH_FILT_TC,				 // 334 4e Anti-hunt filter time constant
    CPM_P_DRV_TRQ_BIAS,		             // 335 4f Compensator: Torque Bias (A)
    CPM_P_DRV_FUZZY_APERTURE,            // 336 50 Fuzzy Aperture
    CPM_P_DRV_FUZZY_HYST,                // 337 51 Fuzzy Hystersis
    CPM_P_AH_HOLDOFF,                    // 338 52 Anti-hunt holdoff
    CPM_P_DRV_KZERO,                     // 339 53 Compensator: Kzero
    CPM_P_DRV_IZERO,                     // 340 54 Compensator: Izero
    CPM_P_DRV_TARGET_WIN,                // 341 55 Compensator: Target Window
    CPM_P_DRV_STAB_WIN,                  // 342 56 Compensator: Stability Window

	CPM_P_DRV_KP_FACTOR=346,			 // 346 5a KpOut Factor
	CPM_P_DRV_KPZ_FACTOR,				 // 347 5b KpZero Factor
	CPM_P_DRV_KI_FACTOR,				 // 348 5c Ki Factor
	CPM_P_DRV_FINE_TUNE,				 // 349 5d Fine Tuning
// Foldback Settings                    
    CPM_P_DRV_TRQ_LIM ,					 // 350 5e Torque Limit  (A)
    CPM_P_DRV_POS_FLDBK_TRQ,             // 351 5f + Torque foldback limit (A)
    CPM_P_DRV_POS_FLDBK_TRQ_TC,          // 352 60 + Torque foldback time const (ms)
    CPM_P_DRV_NEG_FLDBK_TRQ,             // 353 61 - Torque foldback limit (A)
    CPM_P_DRV_NEG_FLDBK_TRQ_TC,          // 354 62 - Torque foldback time const (ms)

    CPM_P_DRV_HARDSTOP_FLDBK_TRQ=357,    // 357 65 Hardstop foldback limit (A)
    CPM_P_DRV_HARDSTOP_FLDBK_TRQ_TC,     // 358 66 Hardstop foldback time const (ms)
    CPM_P_DRV_HARDSTOP_ENTRY_SPD,        // 359 67 Hard-stop speed qualifier
    CPM_P_DRV_HARDSTOP_ENTRY_TRQ,        // 360 68 Foldback Torque Trip Level (A)
    CPM_P_DRV_HARDSTOP_ENTRY_TC,         // 361 69 Hard Stop: entry delay (ms)

	CPM_P_DRV_HARDSTOP_TRQ_LIM=363,		 // 363 6b Alt Trq Lim for Hardstop homing (A)
    CPM_P_DRV_RMS_SLOW_TC,               // 364 6c RMS time constant (sec) @ 100% trq (slow acting)
		
	CPM_P_DLY_TO_DISABLE=368,			 // 368 70 Delay to Disable for brakes (ms)
	CPM_P_FACT_SOFT_START_FREQ,			 // 369 71 Soft start PWM carrier frequency (Hz)
// VRS settings
	CPM_P_DRV_VRS_TRQ_FILT=370,			 // 370 72 VRS Filtering (ms)
	CPM_P_DRV_VRS_FULL_ON=371,			 // 371 73 VRS High Limit (V)
	CPM_P_DRV_VRS_FULL_OFF=362,			 // 362 6a VRS Low Limit (V)
	CPM_P_DRV_BUSV_ADJ_RATE=372,		 // 372 74 VBus runtime adjust rate (ms)

// Drive Set Flags Register
	CPM_P_DRV_SET_FLAGS=375,			 // 375 77 Drive SetFlags Register
	CPM_P_FACT_UNDER_TEMP_TRIP=379,		 //	379 7b Under-Temp Trip (deg C 10.6)
	CPM_P_DRV_ENCODER_TEMP,     		 // 380 7c Internal temp of encoder (deg C 10.6)
	CPM_P_DRV_STATOR_TEMP,				 // 381 7d Internal temp of stator (deg C 10.6)
	CPM_P_FACT_OVER_TEMP_TRIP_STATOR,	 // 382 7e Over-Temp Trip Stator (deg. C)
	CPM_P_FACT_OVER_TEMP_WARN_STATOR,	 // 383 7f Over-Temp Warn Stator (deg. C)

// Bank 2 Items - Application Specific Settings
	// Homing Settings
    CPM_P_APP_HOMING_VEL=512,    		 // 512 00 Homing Velocity
    CPM_P_APP_HOMING_OFFSET,             // 513 01 Initial offset index after homing
    CPM_P_APP_HOMING_ACCEL,				 // 514 02 Homing acceleration
    CPM_P_APP_HOMING_DELAY=516,          // 516 04 Homing delay after hardstop
	CPM_P_EVENT_SHUTDOWN_MASK,           // 517 05 Power event shutdown/warning selector (0=warning, 1=shutdown)

	CPM_P_PWR_AC_LOSS_TC,				 // 518 06 AC Loss shutdown/warning time constant
	CPM_P_PWR_AC_WIRING_ERROR_TC,		 // 519 07 AC Wiring Error shutdown/warning time constant

	CPM_P_FACT_LPB_RESTORE_V = 520,		 // 520 08 Bus V restoration voltage
	CPM_P_PWRUP_HOLDOFF_TIME = 521,		 // 521 09 Sensorless startup holdoff after powerup
	CPM_P_FACT_LPB_ENTRY_V = 522,		 // 522 0a Bus V minimum operation voltage

	CPM_P_XPS_OUT_SRC_REG = 545,		 // 545 21 XPS output register MUX
	CPM_P_XPS_IN_SRC_REG,				 // 546 22 XPS input register MUX

	CPM_P_XPS_USER_IN_REG=548,			 // 548 24 User driven input register
	CPM_P_XPS_STATUS_IN_REG,			 // 549 25 Drive controlled input register
	CPM_P_XPS_INVERT_INPUT,				 // 550 26 XPS bits driving pos limit feature
	CPM_P_XPS_INVERT_OUTPUT,			 // 551 27 XPS bits driving pos limit feature
	CPM_P_XPS_FEAT_ENABLE=554,			 // 554 2a XPS bits driving enable feature
	CPM_P_XPS_FEAT_TRIGGER=556,			 // 556 2c XPS bits driving trigger feature
	CPM_P_XPS_FEAT_NODE_STOP,			 // 557 2d XPS bits driving node stop feature
	CPM_P_XPS_FEAT_RESET_TIMER,			 // 558 2e XPS bits driving timer reset feature
	CPM_P_XPS_FEAT_ATTN0,				 // 559 2f XPS bits driving attn 0 feature
	CPM_P_XPS_FEAT_ATTN1,				 // 560 30 XPS bits driving attn 1 feature
	CPM_P_XPS_FEAT_AT_HOME=563,			 // 563 33 XPS bits driving homing feature
	CPM_P_XPS_FEAT_POS_TRQ_FLDBK,		 // 564 34 XPS bits driving pos torque limit feature
	CPM_P_XPS_FEAT_NEG_TRQ_FLDBK,		 // 565 35 XPS bits driving neg torque limit feature
	CPM_P_XPS_FEAT_IN_POS_LIM,			 // 566 36 XPS bits driving pos limit feature
	CPM_P_XPS_FEAT_IN_NEG_LIM,			 // 567 37 XPS bits driving neg limit feature

	CPM_P_USER_SOFT_LIM_POSN_1 = 568,	 // 568 38 User defined software limit 1
	CPM_P_USER_SOFT_LIM_POSN_2,			 // 569 39 User defined software limit 2
	CPM_P_POWER_REG = 572,				 // 572 3c Power Status Register
	CPM_P_FACT_PHASE_BD_SN = 574,		 // 574 3e Phase board serial num
	CPM_P_FACT_PHASE_BD_REV,			 // 575 3f Phase board rev
	CPM_P_DRV_MIN_OPER_VOLTS = 578,		 // 578 42 Minimum operating voltage
	CPM_P_OVER_TEMP_TRIP_USER,			 // 579 43 User Over-Temp Trip
} cpmParams;
													/** \endcond **/

//*****************************************************************************
// Insure structs are byte packed on target
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
//*****************************************************************************






//*****************************************************************************
// NAME																	      *
// 	cpmHwConfigReg union and cpmHwConfigFlds
												/** \cond ExpertDoc **/
/**
	\brief ClearPath-SC Hardware Configuration Field Definitions.

	These field control the interactions of the node with the electrical
	or other hardware based interfaces. 
	
	These include:
	- homing setup
	- A after start source
	- B before end source
	- various input polarity settings
**/
struct _cpmHwConfigFlds {
	/// Homing styles for the HomingStyle field.
	enum _homingStyles {
		HOME_TO_SWITCH = 0,			///<  0 Homing to switch input
		HOME_TO_HARDSTOP = 1		///<  1 Homing to a hardstop
	};
	/// Homing styles for the HomingStyle field.
	typedef enum _homingStyles homingStyles;
													/**  INTERNAL_DOC **/
	BFld: 4;									//	0-3 
													/**  **/
	/**
		\brief <i>A After Start</i> reference.
		
		Use measured position in determining A After Start, else use
		commanded.
	**/
	BFld AAfterStartMeas : 1;					// 4
	/**
		\brief <i>B Before End</i> reference.

		Use measured position in determining B Before End, else use
		commanded.
	**/
	BFld BBeforeEndMeas : 1;					// 5
	/**
		\brief <i>At position</i> reference.
		 
		Use measured position in determining At Position, else use
		commanded.
	**/
	BFld AtPositionMeas : 1;					// 6
	/**
		\brief Vector Regen Shunt Enable.

		If set, allow excess bus voltage to be dissipated in the windings.
	**/
	BFld VRSEnable : 1;							// 7
	/**
		\brief Vector Validation.

		If set, compare the sensorless start vector with the RO value.
		If outside the acceptable range, cause a shutdown.
	**/
	BFld VectorValidation : 1;					// 8
													/**  INTERNAL_DOC **/
	BFld 	: 1;    							// 9
													/**  **/
													/** \endcond  **/
	/**
		\brief High-Speed Position Capture control

		When should the position be captured by the high-speed input switch?
		0 means to capture position when Input A turns off, 1 means to 
		capture position when Input A turns on.
	**/
	BFld CapturePolarityHiSpd : 1;				// 10
	/**
		\brief Position Capture control.

		When should the position be captured by the input switch?
		0 means to capture position when Input B turns off, 1 means to 
		capture position when Input B turns on.
	**/
	BFld CapturePolarity : 1;					// 11
													/**  INTERNAL_DOC **/
	/**
		\brief Data acquisition over-sampling (for internal use).

		Determines the rate that data acquisition points are sent
		0 = every other sample
		1 = every sample
	**/
	BFld DataAcqOversampling : 1;				// 12
													/**  **/
	/**
		\brief Sensorless Start Mode

		Determines when the initial vector is established.
		0 = At Power-On
		1 = At First Enable

		First enable is the recommended method if multiple nodes share
		a power supply. This minimizes a power supply overload condition
		when a system is first powered up.

		\note The power-on-delay parameter may be used to stagger the vector 
		initialize for power-on mode.
	**/
	BFld SensorlessOnEnable : 1;				// 13
	/**
		\brief Homing style

		What type of homing mode is being used? 0 is homing
		to a switch input, 1 is homing to a hardstop
	**/
	BFld HomingStyle : 1;						// 14

	/*!
		\brief <i>Step & Direction Command Input Format</i>

		If asserted, the input format will use the quadrature format. If
		not asserted, the input format is step & direction.

		\note This feature was introduced with firmware version 1.6

		\note Used by ClearView
	**/
	BFld StepDirAsQuadrature: 1;				// 15

    /**
     * \brief Expected Power Source
     *
     *  On AC capable motors, this field sets the expected power connection.
     *
     *  Set to 0 for all DC powered motors and 1 for AC Powered.
     *
	 *	\note This feature was introduced with firmware version 1.6
	 *
	 *	\note Used by ClearView
     **/
    unsigned UsingAC : 1;						// 16 0=DC, 1=AC
    /**
     * \brief Expected AC Power Type
     *
     *  On AC capable motors, this field sets the expected AC power source.
     *
     *  Set to 0 for all 3phase powered motors and 1 for single phase.
     *
	 *	\note This feature was introduced with firmware version 1.6.10
	 *
	 *	\note Used by ClearView
     **/
    unsigned Using1PhAC : 1;					// 17 0=3ph, 1=1ph
													/**  INTERNAL_DOC **/
	BFld: 2;									// 18-19
													/**  **/
	/**
		\brief Sensorless start algorithm(s) to use

		When SensorlessMode is not set, this value determines the sensorless 
		start method.
	**/
	BFld ValidStartMethods		 : 4;			// 20-23
};
/// ClearPath-SC Hardware Configuration Field Definitions.
typedef struct _cpmHwConfigFlds cpmHwConfigFlds;
/** \endcond  **/
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  cpmHwConfigReg union
//
// DESCRIPTION
															/** \cond ExpertDoc **/
/**
    \brief ClearPath-SC Hardware Configuration Register Type

	This container type holds a copy of the <i>Hardware Configuration 
	Register</i>. The contents can be accessed via \a bits field or via
	the individual fields via the \a Fld field.

	\copydetails _cpmHwConfigFlds

	\see sFnd::ISetupEx::HW
**/
union PACK_BYTES _cpmHwConfigReg {
    /// \brief Settings as an integer
    Uint32 bits;
	/// \brief Field access for hardware configuration.
	cpmHwConfigFlds Fld;

#ifdef __cplusplus
#ifndef __TI_COMPILER_VERSION__

	/// Clear bits on construction
	_cpmHwConfigReg() {
		bits = 0;
	}

#endif
#endif
};
/// \copybrief _cpmHwConfigReg
typedef union _cpmHwConfigReg  cpmHwConfigReg;

																/** \endcond  **/
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  cpmAppConfigFlds union
//
// DESCRIPTION
												/** \cond ExpertDoc **/
/**
	\brief ClearPath-SC Application Configuration Register Field Definitions.

	The fields in this register control the various operational modes and 
	specifications for the features in this node. 

	It controls the following aspects of the node:
	- Travel Limit processing
	- Operational modes: Step+Direction vs Host Controlled
	- Trigger Group assignment
	- The reversal of the direction input for step+direction
	- How homing is processed
	- Control of disabling the node
**/
struct PACK_BYTES _cpmAppConfigFlds {
	/**
		\brief Limit Switch handling for the limit mode field.
	**/
	typedef enum _limitModes {
		/*!
		Ignores General Purpose inputs for travel limits.
		**/
		LIMIT_MODE_OFF,                     //  0: ignore limits
		/*!
		Inputs A and B can be travel limits depending on
		the crosspoint switch settings.
		**/
		LIMIT_MODE_POSITION,                //  1: Limit position
		/*!
		Travel limits set using drive parameters.
		**/
		LIMIT_MODE_SOFT_POSN = 4,            //  4: Software position limit
		/*!
		Travel is limited by soft limits and hardware inputs based on
		the crosspoint switch settings.
		**/
		LIMIT_MODE_SOFT_AND_INPS = 5
	} limitModes; 

	/**
		\brief Operational Modes for the Mode field.
	**/
	typedef enum _operationalMode {
		/*!
		11: Step+Direction is followed
		**/
		MOVE_STEP_DIR = 11,
		/*!
		20: Idle mode that has no built in commands
		**/
		IDLE_MODE = 20
	} operationalMode;

	// Note: the numbers in the field descriptions denote their bit
	// range in the register.
	/*!
		\brief Operational Mode

		Which operational mode is the drive in

		\note Used by ClearView
		**/
	BFld Mode : 6;							// 0-5 Operational Mode
	/**
		\brief Trigger Group
		
		For advanced models, this field defines the trigger group we will
		respond to for initiating waiting moves. A zero value removes this
		node from all groups.
	**/
	BFld TriggerGroup : 2;					// 6-7 
	/*!
		\brief <i>Limit Switch</i> Enable

		Controls processing of limit switches \todo

		- Ignore Mode (\ref LIMIT_MODE_OFF):
	**/
	BFld LimitMode : 3;						// 8-10 Limit switch mode
	/*!
		\brief <i>Limit Ramp Stop</i>

		This field controls the type of stop that is used when
		a limit is encountered. If asserted, a ramp-style stop
		will be used; otherwise, an abrupt stop will be used
		for stopping at a limit.

		\note Used by ClearView
	**/
	BFld LimitRampStop : 1;					// 11 [TODO] Limit stop type
													/**  INTERNAL_DOC **/
	BFld: 2;								// 12-13 
													/**  **/
	/*!
		\brief <i>Step & Direction Command Modes</i> reverses
		the direction of motion.

		\note Used by ClearView
	**/
	BFld DirInputReverse : 1;				// 14

	/**  INTERNAL_DOC **/
	BFld: 1;								// 15

	/*!
		\brief <i>Offset Move Performed in Homing Direction</i>

		If asserted, this indicates that the post-homing offset move should be
		in the same direction as the specified homing.

		\note Used by ClearView
	**/
	BFld OffsetMoveInHomeDir : 1;			// 16
													/**  INTERNAL_DOC **/
	BFld: 1;								// 17 
	BFld: 1;								// 18 
												/**  **/
	/*!
		\brief <i>Homing Direction</i>

		Sets the direction for rotation for the homing sequence (asserted
		means reverse or CW)

		\note Used by ClearView
	**/
	BFld HomingDirection : 1;				// 19

	/**  **/
	/*!
		\brief <i>On Disable Action</i>

		Sets the action the motor will take upon disabling and for AC
		units upon loss of AC power.

		\note Used by ClearView and introduced with firmware version 1.6.
	**/
	mnDisableActions DisableAction : 3;		// 20-22

													/**  INTERNAL_DOC **/
	BFld: 7;								// 23-29
													/**  **/
	/*!
		\brief <i>Manual Homing</i>

		Allows a user-driven hardstop homing to be performed in S&D modes

		\note Used by ClearView
	**/
	BFld ManualHoming : 1;    				// 30 Manual Homing
};
/// \copybrief _cpmAppConfigFlds
typedef struct  _cpmAppConfigFlds cpmAppConfigFlds;
												/** \endcond  **/

//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  cpmAppConfigReg union
//
// DESCRIPTION
														/** \cond ExpertDoc **/
/**
    \brief ClearPath-SC Application Configuration Register Type.

	\copydetails _cpmAppConfigFlds

	\note Care must be taken when changing these to not have them be
	overridden by ClearView or a <i>Configuration File</i> load.

	\see sFnd::ISetupEx::App For information on changing via the host.
	\see _cpmAppConfigFlds For Specific field information.
**/
union PACK_BYTES _cpmAppConfigReg {
			
    /**
		\brief Register state as an integer.

		Register state as an integer.
	**/
    Uint32 bits;
	/**
		\brief ClearPath-SC Field view of the <i>Application 
		Configuration Register</i> .
	**/
	cpmAppConfigFlds Fld;                                  
    // For C++ non-firmware usage, include a constructor.
    #ifdef __cplusplus
        #ifndef __TI_COMPILER_VERSION__
													/**  INTERNAL_DOC **/
		/// Clear bits on construction
		_cpmAppConfigReg() {                
            bits = 0;
        }
													/**  **/
        #endif
    #endif
};
/// \brief ClearPath-SC Application Configuration Register Type.
typedef union _cpmAppConfigReg cpmAppConfigReg;
														/** \endcond  **/
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      cpmOutFlds structure
//
//  DESCRIPTION:
/**
    \cond SC_EXPERT
    \brief ClearPath-SC Output Field Definitions.

    This is a container that defines the output register that is used
	to control various features of the ClearPath-SC. The actual output
	register can defined from a combination of the application requests or
	from the Crosspoint Switch (XPS). The XPS feature allows the mapping
	of internal state or electrical based inputs to control the state
	of the operating output register.

    To check on the operations of the Output Register there are a suite of
    accumulating registers based on the sample-by-sample basis. These read-only
    functions can access what happened to the output register since the last
    request
        - CPMOuts::OutReg
        - \todo add rise/fall set

    \warning
    For multi-threaded applications care must be taken to provide
    serialization for the read-modify-write actions as well as accumulating
    the Rise and Fall transition registers for use by your application.

    The following diagram shows the layout of this register.
        \image html "SC_Out_Reg.png"
        \image latex "SC_Out_Reg.eps"
**/
struct PACK_BYTES _cpmOutFlds {
	/*                            ------
	 *                             bit #
	 *                            ------ */
													
	BFld		        : 2;		// 1
													
	/**
		\brief Enable drive's power stage.

		This field controls the enabling of the drive's power stage and
		prepares the node to accept motion commands, which will be
		accepted when the <i>Status Register's</i>
		cpmStatusReg::NotReady bit has cleared.
	**/
	BFld EnableReq   : 1;			// 2
	/**
		
		\brief Capture measured position on asserting edge.

		The asserting edge (0->1) of This field will capture the measured
		position to the PLA capture register.
	**/
	BFld PosnCapture  : 1;			// 3
	/**
		\brief Generate trigger event on asserting edge.

		The asserting edge (0->1) of This field will generate a <i>Trigger
		Event</i> which allows a waiting move  to begin.
	**/
	BFld Trigger     : 1;			// 4
	/**
		
		\brief Generate node stop on asserting edge.

		The asserting edge (0->1) of This field will initiate the node stop
		programmed in the Stop Type register.
	**/
	BFld NodeStop    : 1;			// 5
	/**
		\brief Reset general purpose timer.

		The asserting edge (0->1) of This field will reset the <i>General
		Purpose Timer</i> with duration and
		clear the timer expired bit in the <i>Status Register</i>.

	**/
	BFld ResetTimer  : 1;			// 6
	/**
		
		\brief XPS Feedback output 0.

		This output is fed back to a similarly named input in the
		<i>Status Register</i>. It is used to make simple state machines
		or to map inputs available to the XPS system to an Attentionable
		location.
	**/
	BFld XPSattn0    : 1;			// 7
	/**
		\brief XPS Feedback output 1.

		This output is fed back to a similarly named input in the
		<i>Status Register</i>. It is used to make simple state machines or
		to map inputs available to the XPS system to an Attentionable
		location.
		\todo Doc details?
	**/
	BFld XPSattn1    : 1;			// 8
													
	BFld             : 1;			// 9
	BFld             : 1;			// 10
													
	/**
		\brief Signal the end of homing move.

		If homing move in progress, terminate it and start any offset
		motion and mark this position as the zero point.
	**/
	BFld TerminateHomeMove	: 1;	// 11
	/**
		\brief Start Positive torque foldback

		Setting This field can control the amount of positive torque
		available. The Positive Foldback Torque (IFoldbacksAdv::PositiveTrq)
		parameter controls the amount.

		\todo ISCfoldbackPage
	**/
	BFld StartPosTrqFldBk : 1;		// 12
	/**
		\brief Start Negative torque foldback

		Setting This field can control the amount of negative torque
		available. The Negative Foldback Torque (IFoldbacksAdv::NegativeTrq)
		parameter controls the amount.

		\todo ISCfoldbackPage
	**/
	BFld StartNegTrqFldBk : 1;		// 13
	/**
		\brief Positive Limit detected.

		This will cancel any positive motion and prevent additional
		travel in the positive direction if the limit mode is set
		appropriately.
	**/

	BFld PosLimitHit  : 1;			// 14 
	/**
		\brief Negative Limit detected.

		This will cancel any negative motion and prevent additional
		travel in the negative direction if the limit mode is set
		appropriately.
	**/
	BFld NegLimitHit  : 1;			// 15 
};
/// ClearPath-SC Output Register Field Definitions
typedef struct _cpmOutFlds cpmOutFlds;
//              \endcond                                                              *
//*****************************************************************************



//*****************************************************************************
// NAME                                                                       *
//  cpmOutReg union
//
// DESCRIPTION
/**
	 \cond INTERNAL_DOC
	\brief ClearPath-SC Output Register Type.

	This union is used to hold the contents of a ClearPath-SC output register.
**/
union PACK_BYTES _cpmOutReg
{
	/**
		\brief Bit-wise access to the ClearPath-SC output register.
	**/
	Uint16 bits;
	/**
		\brief Field access to the ClearPath-SC output register.
	**/
	cpmOutFlds Fld;

#ifdef __cplusplus
#ifndef __TI_COMPILER_VERSION__

	/// Clear bits on construction
	_cpmOutReg() {
		bits = 0;
	}

#endif
#endif
};

/// ClearPath-SC Output Register Type
typedef union _cpmOutReg cpmOutReg;
//       \endcond                                                                     *
//*****************************************************************************



//*****************************************************************************
// NAME                                                                       *
//  cpmStatusRegFlds structure
//
// DESCRIPTION

/**
	\brief ClearPath-SC Node Status Register Fields.

	This page provides the detailed list of fields in the ClearPath SC status register.  
	The lower 32-bits of this register are attentionable.


	\see sFnd::ValueStatus for information on accessing different status registers within the Node.
	\see \ref AttentionsPage for information regarding attentions.

	\image html "SC_Status_Reg.png"  "Status Register Overview"
	\image latex "SC_Status_Reg.eps" "Status Register Overview"	height=5in
**/
struct PACK_BYTES _cpmStatusRegFlds {
	/**
		\brief Motion Activity Indicator

		This is a bit-mapped field definition that describes 
		the current motion detected on the motor. The VEL_BOTH
		state will occur with accumulating status when the motor
		has moved in both directions since the last status query.
	**/
													/** \cond INTERNAL_DOC **/
	typedef enum _velStates {
		VEL_STOPPED,					//< 0: Stopped
		VEL_POSITIVE,					//< 1: Positive velocity detected
		VEL_NEGATIVE,					//< 2: Negative velocity detected
		/** 3: For accumulating registers, this represents the detection of 
			velocity in both directions **/
		VEL_BOTH						
	} velStates;
	/**
		\brief Shutdown States

		This represents the current shutdown state. SHDN_STATE_IDLE means
		the node is running with no current or pending shutdowns.
	**/
	typedef enum _shutdownStates {
		SHDN_STATE_IDLE         = 0,    ///< All is well
														
		SHDN_STATE_DELAY        = 1,    ///< Shutdown is imminent
		SHDN_STATE_RAMP         = 2,   ///< Axis is ramping down to zero speed
														
		SHDN_STATE_STOPPED      = 3    ///< Axis is shutdown
	} shutdownStates;
															/** \endcond  **/
	/*  bits 00-15 - - - - - - - - - - - - - - - - - - - - - - ------ */
	// Attentionable bits start here
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	//										Bit #
											/** \cond INTERNAL_DOC **/
	/**
		\brief Warning is present.

		This field is set when the bit-wise AND of the User Warning register
		and the Warning Register is non-zero.

		\see sFnd::IAttnNode::WarnMask

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
		**/
	BFld Warning					: 1;	// 00 
											/** \endcond  **/
	/**
		\brief User alert is present.

		This field is set when the bit-wise AND of the [User Alert register](@ref sFnd::IAttnNode::AlertMask)
		and the Alert Register is non-zero.

		\see sFnd::IAttnNode::AlertMask

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
		**/
	BFld UserAlert					: 1;    // 01
	/**
		\brief Node not ready for operation.

		If this field is true, a Node is not ready to execute motion.  You must make sure to [clear shutdowns](@ref sFnd::IStatus::AlertsClear), ensure the
		node is [Enabled](@ref sFnd::INode::EnableReq), and [clear any NodeStops.](@ref sFnd::IMotion::NodeStopClear)

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld NotReady					: 1;    // 02
	/**
		\brief Move buffer is available.

		This field indicates the Node can receive additional move commands which will be stored in the Node's move buffer.
		If this bit is deasserted, any move command sent to the Node will be ignored.

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld MoveBufAvail				: 1;	// 03
	/**
		\brief Node ready for operation.

		This field is asserted when a Node is ready to receive motion commands.

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld Ready						: 1;    // 04

	/**
		\brief Power Event Occurred.

		This field is asserted when a power event occurs. The power status
		register can be used to determine the specifics.

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld PowerEvent					: 1;	// 05

	/**
		\brief Alert Present.

		This field is set when the <i>Alert Register</i> is non-zero.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld AlertPresent				: 1;    // 06
													/** \cond INTERNAL_DOC **/
	BFld 							: 1;	// 07
													/** \endcond **/
	/**
		\brief Positive limit asserted.

		When This field is asserted, the Node is unable to execute moves in the positive direction.
		The input configured through Clearview to limit motion in the positive direction is currently limiting motion.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld InPosLimit					: 1;    // 08 
	/**
		\brief Negative limit asserted.

		When This field is asserted, the Node is unable to execute moves in the negative direction.
		The input configured through Clearview to limit motion in the negative direction is currently limiting motion.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld InNegLimit					: 1;    // 09
	/**
		\brief Motion Blocked.

		This field is asserted when motion is blocked by an E-Stop Nodestop Type.

		\see CPMnodeStopPage

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld MotionBlocked				: 1;	// 10 
	/**
		\brief Was Homed

		Indicates that a homing sequence has been
		executed.

		\remark This field can generate an attention request to the host for
		autonomous handling. 
	**/
	BFld WasHomed					: 1;	// 11
	///\endcond
	/**
		\brief In Homing Sequence

		Indicates that a homing sequence is being
		executed.	

		\see [Homing Page](@ref HomingPage) for more information on homing

		\remark This field can generate an attention request to the host for
		autonomous handling. 
	**/
	BFld Homing						: 1;	// 12
	/**
		\brief Going To Disable Soon

		Indicates that the drive is about to go
		disabled.

		\remark This field can generate an attention request to the host for
		autonomous handling. 
	**/
	BFld GoingDisabled				: 1;	// 13
	/**
		\brief Status Rise Event detected

		This field is set when the bit-wise AND of the [Status Mask](@ref sFnd::IAttnNode::StatusMask)
		and the Status Register is non-zero.

		\see sFnd::IAttnNode::StatusMask

		\remark
		This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld StatusEvent				: 1;	// 14
	/**
		\brief Drive Enabled

		This field is set when the drive is enabled. 

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld Enabled					: 1;	// 15
	/*  bits 16-31 - - - - - - - - - - - - - - - - - - - - - - ------ */
	/**
		\brief Motion Canceled.

		This condition will occur when a queued or executing move
		is canceled en-route by a NodeStop.

		\see CPMnodeStopPage

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld MoveCanceled		: 1;	// 16
	/**
		\brief Move completed and settled.

		This field asserts when a Move is complete.

		\see \ref CPMmoveDonePage

		\remark This field can generate an attention request to the host for
		autonomous handling.
   **/
	BFld MoveDone			: 1;    // 17
	/**
		\brief Tracking out of range.

		This field indicates the Node is currently not tracking within the In-Range window.  This is especially common during moves.

		\see \ref CPMmoveDonePage for more information regarding setting up the In-Range window in ClearView

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld OutOfRange			: 1;    // 18
	/**
		\brief <i>B Counts from End</i> of move.

		This field asserts when the move is [B From End](@ref sFnd::IMotionAdv::BeforeEndDistance) of the move.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld BFromEnd			: 1;    // 19
	/**
		\brief Greater than position location.

		This field asserts when the measured position is greater than
		the  Absolute Position location specified in the torque limiting dialog window of ClearView.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld AbovePosn			: 1;    // 20
	/**
		\brief At Target Velocity.

		This field asserts when velocity commands have reached their
		target velocity.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld AtTargetVel		: 1;	// 21
	/**
		\brief Input-A state.

		This field asserts when the Node's input A asserts.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld InA				: 1;    // 22
	/**
		\brief Input-B state.

		This field asserts when the Node's input B asserts.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld InB				: 1;    // 23
	/**
		\brief Inverted Input-A state.

		This field asserts when the Input-A is de-asserted.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld InvInA				: 1;    // 24
	/**
		\brief Inverted Input-B state.

		This field asserts when the Input-B is de-asserted.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld InvInB				: 1;    // 25
													/** \cond INTERNAL_DOC **/
	BFld					: 2;    // 26-27
													/** \endcond **/

	/**
		\brief <i>A From Start</i> of move.

		This field asserts when the move is [A From Start](@ref sFnd::IMotionAdv::AfterStartDistance) of the move. 
		
		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld AFromStart			: 1;    // 28
	/**
		\brief Commanded move is negative.

		This field asserts when the current command resulted in the
		position changing in a negative direction. This field changes
		at the start of a move and persists until the next move.
		A zero step move is considered positive.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld MoveCmdNeg			: 1;    // 29
	/**
		\brief Drive is disabled.

		This field asserts when the drive is disabled.

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld Disabled			: 1;	// 30
												/** \cond INTERNAL_DOC **/
	/**
		\brief <i>General Purpose Timer</i> has expired.

		This field asserts when the <i>General Purpose Timer</i>
		timer has expired. This timer is reset by the
		cpmOutReg::ResetTimer bit in the <i>Output
		Register.</i> The duration of this timer is set by the
		<i>General Purpose Timer Time Constant</i>
		(CPM_P_GP_TIMER_PERIOD).

		\remark This field can generate an attention request to the host for
		autonomous handling.
	**/
	BFld TimerExpired		: 1;	// 31
												/** \endcond  **/

	/*  bits 32-47 - - - - - - - - - - - - - - - - - - - - - - ------ */
	// ------------ Remaining fields are not attentionable ------------
	/**
		\brief Motion Indicator.

		This field represents the load's velocity status. The
		table describes the states:
		- 0: Near zero speed
		- 1: Moving in a positive direction
		- 2: Moving in a negative direction
		- 3: Moved in both directions since the last read (for Accumulating Registers)
	**/
	// Avoid VS compiler padding bug, don't use enum
	BFld InMotion					: 2;	// 32-33 
	/**
		\brief The Output Register disable bit is set due to a NodeStop.

		The Node Stop feature has forced the enable output to the
		disabled state.  [Clear Nodestops](@ref sFnd::IMotion::NodeStopClear) to return the Node to it's previous enable state.

		\see \ref CPMnodeStopPage
	**/
	BFld InDisableStop				: 1;	// 34
											/** \cond INTERNAL_DOC **/
	/**
		\brief Controlled Stop in affect.
	**/
	BFld InCtrlStop      			: 1;	// 35 
	/**
		\brief Fan running.

		For units with a fan, this field is asserted when the
		fan is on. This is a warning that the drive
		temperature is approaching the shutdown level.
	**/
	BFld FanOn           			: 1;	// 36 
											/** \endcond  **/
	/**
		\brief Vector searching.

		This field asserted when the vector has not been finalized.  
		Typically, this happens on the first enable after power-up.
		Once the vector is finalized normal operation can commence.
	**/
	BFld VectorSearch    			: 1;	// 37 
	/**
		\brief Motion command has completed.

		This field is asserted after the motion command has been completed.
		The move is considered totally complete when the [Move Done](@ref _cpmStatusRegFlds::MoveDone) has been asserted.
	**/
	BFld MoveCmdComplete			: 1;    // 38
	/**
		\brief In hard-stop mode.

		This field asserts when the ClearPath-SC has entered the <i>
		Hard Stopped</i> mode during homing and the node is applying clamping
		torque. 

		\see HomingPage
	**/
	BFld InHardStop     			: 1;    // 39 
	/**
		\brief Shutdown state.

		These fields describes the shutdown condition of the node:
		- 0: Idle: no shutdowns are present
		- 1: Delay: A shutdown is imminent 
		- 2: Ramp: Node is ramping down to zero speed
		- 3: Stopped: Node is currently shutdown
	**/
	// Avoid VS compiler padding bug, don't use enum
	BFld ShutdownState				: 2;	// 40-41

	/**
		\brief Hardware failure detected.

		The unit's shutdown is caused by a hardware failure. This
		unit should be replaced.
	**/
	BFld HwFailure      			: 1;	// 42
	/**
		\brief Move awaiting trigger event.

		The node has a triggered move awaiting release.

		\see \ref CPMtriggerPage
	**/
	BFld TriggerArmed   			: 1;	// 43
	/**
		\brief This field is set true when the step+direction inputs are
		active.

		This field is set true when the step and direction inputs are
		active.
	**/
	BFld StepsActive     			: 1;	// 44
											/** \cond INTERNAL_DOC **/
	/**
		\brief Index detected on motor encoder.

		The node has detected an index pulse on the motor's encoder.
	**/
	BFld IndexMtr       			: 1;	// 45
											/** \endcond **/
											/** \cond INTERNAL_DOC **/
    /**
		\brief Software inputs are being used.

        Indicates when inputs that have the ability to be controlled either
        by software or hardware have been switched to software control.
    **/
    BFld SoftwareInputs  			: 1;    // 46	
											/** \endcond  **/
	// ------------------------------------------------------------------------
																/** \cond INTERNAL_DOC **/
	/** 
		\brief Update a string with the set fields in the Status Register.

		\param[in,out] buffer  Point to string buffer to update
		\param[in] size  The size of the buffer in characters including the null
		character.
		\return the parameter \a buffer.
		<b>Example Code</b>
		\code
			char myBuffer[500];
			cout << myStatusRef.cpm.StateStr(myBuffer, sizeof(myBuffer));
		\endcode
	**/
	MN_EXPORT char * MN_DECL StateStr(char *buffer, size_t size) const;
																/** \endcond  **/
};
/// ClearPath-SC Status Register Field definitions
typedef struct _cpmStatusRegFlds cpmStatusRegFlds;
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  cpmStatusReg union
//
// DESCRIPTION
													/** \cond INTERNAL_DOC **/
/**
	\brief ClearPath-SC Status Register Type.

	The status register type defines the very large register which maintains 
	the status and state of the node. The node maintains a suite of these
	registers that have various behaviors. 

	There are accumulating types that clear upon reading and one snap shot
	of the current state.

	Details of this register:
	\copydetails _cpmStatusRegFlds

	\see sFnd::IStatus::RT	Real-time snapshot.
	\see sFnd::IStatus::Rise	Items that have asserted since last access.
	\see sFnd::IStatus::Fall	Items that have deasserted since last access.
	\see sFnd::IStatus::Accum Items that have asserted, and accumulate. This is 
	is similar to the sFnd::IStatus:Rise, but currently asserted status is not
	cleared upon reading.
**/
union PACK_BYTES _cpmStatusReg {
	/**
		\brief Bit-wise integer access to the status register.
	**/
	Uint16 bits[3];
	/**
		\brief Access to the fields of the ClearPath-SC <i>Status Register</i>.
	**/
	cpmStatusRegFlds Fld;

#ifdef __cplusplus

	// Constructor, create with bits cleared
	_cpmStatusReg() {                   // Clear bits on construction
		bits[0] = 0;
		bits[1] = 0;
		bits[2] = 0;
	}
#endif


};
/// ClearPath-SC Status Register Type
typedef union _cpmStatusReg cpmStatusReg;
														/** \endcond  **/
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  cpmAlertFlds struct
//
// DESCRIPTION
/**
	\brief ClearPath-SC Alert and Warning Register Field Definitions.

	This page provides the detailed list of the Clearpath SC alerts. These fields describe the reason a
	shutdown has occurred, and potential solutions.

	\see sFnd::ValueAlert For information on accessing alert registers within the Node

	\image html "cpmAlertReg.png"

**/
struct PACK_BYTES _cpmAlertFlds {
    /// \brief Common shutdown events. Least significant bits 0-31.
	commonAlertFlds Common;
    //                              ----------------------------------------
    //                              |len|bit #|description                 |
    //                              ----------------------------------------
									/** \cond INTERNAL_DOC **/
	BFld : 4; /* 32-35 */
									 /** \endcond **/
											   /** \cond INTERNAL_DOC **/
	/**
	\brief Jerk Limit Bad.

	A move was started with bad Jerk Limit request. An illegal or
	malformed <i>Jerk Limit</i> was specified for a move starting.
	This move canceled.

	Remedies:
	- Check the Jerk Limit setting before issuing the move.
	**/
	BFld JrkLimRequestBad : 1; /* 36 */
											   /** \endcond  **/
											   /** \cond INTERNAL_DOC **/
	/**
	\brief Move Buffer Under-run.

	Move Generator ran out of pending moves before the terminal move.
	This would occur with move sequences where the host cannot keep
	up with the moves required to follow a path.

	Remedies:
	- Create moves with longer time intervals.
	- Use a more efficient mechanism to keep the move buffer busy.
	Sometimes an attention-based mechanism will work.
	**/
	BFld MoveBufUnderrun : 1; /* 37 */
													  /** \endcond  **/
	/**
	\brief Velocity limit too high for jerk limit.

	The move was canceled due to the [Velocity Limit](@ref sFnd::IMotion::VelLimit) of the attempted move exceeding
	the encoder's or RAS/G-Stop's maximum allowed speed.

	Remedies:
	- Reduce the [Velocity Limit](@ref sFnd::IMotion::VelLimit) of the move. 
	- Decrease the RAS/G-Stop to one with less speed limiting(a lower time).
	
	**/
	BFld JrkLimVelRequestBad : 1; /* 38 */
								  /** \cond INTERNAL_DOC **/
	/**
	\brief Invalid move spec was altered.

	The move could not meet both the distance and acceleration
	parameters of the move with the given initial velocity.
	The acceleration limit was increased to allow the
	move to complete without overshooting.

	Remedies:
	- Allow a longer distance for the terminal move.
	- Increase the [Acceleration Limit](@ref sFnd::IMotion::AccLimit).
	- Reduce the velocity prior to the terminal move.
	**/
	BFld MoveSpecAltered : 1; /* 39 */
									/** \endcond  **/
	/**
	\brief Phase sensor bad.

	Phase sensor failed power-on self test.

	Remedies:
	- Replace motor.
	**/
	BFld PhaseSensorFailed : 1; /* 40 */
	/**
	\brief Limit switch activated.

	Attempting to move in a direction in which the limit switch
	is activated. Motion in the direction of the limit is prevented.  The current move, and any queued moves, are
	canceled.

	Remedies:
	- Move away from the limit switch or clear the limit switch activation.
	**/
	BFld LimSwitchActivated : 1; /* 41 */
	/**
	\brief Soft limit prevents move.

	A move attempted to exceed the bounds set by the software limits.
	The current move, and any queued moves, are canceled.

	Remedies:
	- Decrease the distance of the commanded move such that it ends within the bounds of the current soft limit settings.
	- Move away from the limit
	- Adjust the soft limit settings.

	**/
	BFld SoftLimitExceeded : 1; /* 42 */
								/** \cond INTERNAL_DOC **/
								// Note: Must be separate around word boundary
	BFld : 5; /* 43-47 */
	BFld : 4; /* 48-51 */
								/** \endcond **/
	/**
	\brief AC Power Lost

	Motor was running with AC power and AC power has been removed.

	Remedies:
	- Check AC power wiring and power quality to attached modules.
	**/
	BFld ACPowerLost		    : 1; /* 52 */ // AC Power lost
	/**
	\brief AC Phase Lost

	Motor was running with AC power and one or more of the AC phases have been removed.

	Remedies:
	- Check AC power wiring and power quality to attached modules.
	**/
	BFld ACPhaseLost		    : 1; /* 53 */ // AC Phase lost
								/** \cond INTERNAL_DOC **/
	BFld : 1; /* 54 */
								/** \endcond **/
	/**
	\brief Low motor temperature.

	The motor has detected an excessive operating
	temperature. This condition is recoverable when the
	temperature has been sufficiently reduced.

	Remedies:
	- Move the motor to a warmer environment.
	**/
	BFld MtrTempLow				: 1; /* 55 */
								/** \cond INTERNAL_DOC **/
	BFld : 2; /* 56-57 */
								/** \endcond **/

	/**
	\brief Vector Problem.

	The motor's commutation vector has drifted. This will lead to poor
	performance. This is normally a warning indication.

	Appears in: WarnReg

	Remedies:
	- Replace defective motor.
	**/
	BFld MtrVectorBad : 1; /* 58 */
	/** \cond INTERNAL_DOC **/
							
	BFld : 1; /* 59 */
							/** \endcond **/
	/**
	\brief Motor encoder glitch.

	A glitch was detected on the motor encoder signals.

	Remedies:
	- Replace motor.
	**/
	BFld MtrEncGlitch : 1; /* 60 */
						   /** \cond INTERNAL_DOC **/
	/**
	\brief Motor Encoder Overspeed.

	The motor's encoder has exceeded the speed limit of the Motor.

	Appears in: WarnReg

	Remedies:
	- Reduce commanded speed.
	**/
	BFld MtrEncOverspeed : 1; /* 61 */
						
	BFld : 2; /* 62-63 */
							/** \endcond **/
	/**
	\brief Motor phase overload.

	A motor phase has overloaded.

	Remedies:
	- Restore motor to factory defaults and reload a valid configuration
	file.
	- Check the mechanics for abnormally high friction/binding. Grease if necessary.
	- If problem persists under no-load conditions, replace motor.
	**/
	BFld MtrPhaseOverload : 1; /* 64 */
								/** \cond INTERNAL_DOC **/
	BFld : 1; /* 65 */
								/** \endcond **/
	/**
	\brief Bad motor parameters.

	A motor-specific parameter has an illegal value.

	Remedies:
	- Restore motor to factory defaults and reload a valid configuration
	file.
	- If restoring to factory defaults doesn't clear the problem, replace
	 motor.
	**/
	BFld MtrBadSetupParams : 1; /* 66 */
	/**
	\brief Hardstop broke free.

	The hardstop moved when pressure was applied by the motor. Once the motor
	has qualified for the hardstop condition, the mechanics must stay reasonably still while motor torque is applied at the foldback level.
	If the coupling or hardstop should break free, the axis will move in the direction of the hardstop and this alert will be issued

	Remedies:
	- Checking homing parameters in ClearView
	- Ensure that the hardstop for this axis is solid and unmovable.
	- For High friction applications, or machines subject to binding issues, ensure that the hardstop entry torque defined through ClearView
		is high enough that the motor will not qualify for the hardstop condition except when the axes is against the hardstop on the machine.
	**/
	BFld HardStopBrokeFree : 1; /* 67 */
								/** \cond INTERNAL_DOC **/
	BFld : 1; /* 68 */
								/** \endcond **/
	/**
	\brief Tracking shutdown.

	The position tracking error has exceeded the user specified critical tracking limit.
	This is a resettable shutdown condition.

	Remedies:
	- Check the mechanics for abnormally high friction/binding. Grease if necessary.
	- Reduce velocity, acceleration, and/or duty cycle.
	- Check that the configuration file is appropriate for this motor.
	- Increase tracking error limit to a higher value through the ClearView application.

	**/
	BFld TrackingShutdown : 1; /* 69 */
							   /** \cond INTERNAL_DOC **/
	/**
	\brief RMS Overload Warning.

	This warning occurs when the RMS load is approaching the critical limit.


	Remedies:
	- Check the mechanics for abnormally high friction/binding.
	- Reduce feed rate.
	- Improve tuning.
	- Use a larger motor.
	**/
	BFld RMSOverload : 1; /* 70 */
								/** \endcond  **/
	/**
	\brief RMS torque overload shutdown.

	The RMS torque of the motor has exceeded the critical limit. This is a resettable shutdown
	condition.

	Remedies:
	- Check the mechanics for abnormally high friction/binding.
	- Reduce velocity, acceleration, and/or duty cycle.
	- Check that the configuration file is appropriate for this motor.
	- If the motor is actively cooled with a fan raise the continuous current limit of the motor appropriately
	- Use a larger motor.
	**/
	BFld RMSOverloadShutdown : 1; /* 71 */
								/** \cond INTERNAL_DOC **/
	BFld : 2; /* 72-73 */
	/**
	\brief Bus Voltage Saturation.

	This warning occurs when full bus voltage was required to
	drive the motor.

	Appears in: WarnReg

	Remedies:
	- Check power taps to bus supply.
	- Check torque loop tunings.
	- Check for mechanical issues such as binding.
	- Check gearing and motor sizing for this application.
	- See system configuration manual for more tips on this condition.
	**/
	BFld BusVoltSat : 1; /* 74 */
	/**
	\brief Torque Saturation.

	This warning occurs when full torque was commanded to
	drive the motor.

	Appears in: WarnReg

	Remedies:
	- Check power taps to bus supply.
	- Check torque loop tunings.
	- Check for mechanical issues such as binding slides.
	- Check gearing and motor sizing for this application.
	- Check for the misapplication of any torque limiters such as
	the Global Torque Limit.
	- See system configuration manual for more tips on this condition.
	**/
	BFld TrqSat : 1; /* 75 */
					 /** \endcond  **/
	/**
	\brief Insufficient sensorless sweep distance.

	This shutdown occurs when a motor's sensorless start fails to
	move the appropriate distance.

	Remedies:
	- Check the mechanics for abnormally high friction/binding. Grease if necessary.
	- Check for excessive external forces acting on the axis.
	- Verify that the configuration file is correct for the motor;
	reload the configuration file.
	- Check that the power supply has been correctly configured
	for your AC voltage, and is producing the expected voltage level.
	- Check for the misapplication of any torque limiters such as
	the Global Torque Limit that might be limiting torque in the motor.
	**/
	BFld NoCommSweepFailed : 1; /* 76 */
	/**
	\brief Sensorless motion in wrong direction.

	This shutdown occurs when a motor's sensorless start detects
	the motor moving in the wrong direction.

	Remedies:
	- Verify that the configuration file is correct for the motor;
	reload the configuration file.
	- Check for excessive external forces acting on the axis.
	- Check that the power supply has been correctly configured
	for your AC voltage, and is producing the expected voltage level.
	- Check the mechanics for abnormally high friction/binding.
	**/
	BFld NoCommSweepReversed : 1; /* 77 */
	/**
	\brief Sensorless startup failed.

	This shutdown occurs when the sensorless startup failed
	after too many retries.

	Remedies:
	- Verify that the configuration file is correct for the motor;
	reload the configuration file.
	- Check that the power supply has been correctly configured
	for your AC voltage, and is producing the expected voltage level.
	- Check the mechanics for abnormally high friction/binding.
	- Check for excessive external forces acting on the axis.
	**/
	BFld NoCommFailed : 1; /* 78 */
						   /** \cond INTERNAL_DOC **/
	/**
	\brief Index Count Zero Warning.

	This warning occurs when no index pulses have been detected.

	Remedies:
	- Replace defective motor.
	**/
	BFld IndexCountZeroWarn : 1; /* 79 */
								 /** \endcond  **/
	/**
	\brief High Drive temperature.

	The motor has detected an excessive operating
	temperature. This condition is recoverable when the
	temperature has been sufficiently reduced.

	Remedies:
	- Provide more ventilation/fan cooling to motor.
	**/
	BFld DriveTempHigh : 1; 	/* 80 */

	/**
	\brief Stator temperature.

	The motor has detected an excessive motor stator operating
	temperature. This condition is recoverable when the
	temperature has been sufficiently reduced.

	Remedies:
	- Provide more ventilation/fan cooling to motor.
	**/
	BFld StatorTempHigh	: 1; /* 81 */

	/**
	\brief Motor's bus power supply overloaded.

	The motor's bus current has exceeded maximum thresholds.

	Remedies:
	- Increase the bus voltage.
	- If condition still persists under non-stressful no-load conditions,
	replace the damaged motor.
	- Check the mechanics for abnormally high friction/binding. Grease if necessary.
	**/
	BFld BusOverCurrent : 1; /* 82 */
	/**
	\brief Motor's bus power supply voltage too high.

	The motor's bus voltage has exceeded maximum thresholds.
	This is recoverable when the bus voltage return to the operating range of the motor.

	Remedies:
	- Check that the power supply has been correctly configured
	for your AC voltage and is producing the expected voltage level.
	- Check and/or install regenerated bus voltage management system.
	**/
	BFld BusOverVoltage : 1; /* 83 */
	/**
	\brief Motor's bus power supply voltage too low.

	The motor has detected a loss or brown-out of the motor's bus
	voltage. This condition will automatically clear upon restoration
	of the bus voltage without host intervention.

	Remedies:
	- Check that the power supply has been correctly configured
	for your AC voltage and is producing the expected voltage level.
	- Check the mechanics for abnormally high friction/binding. Grease if necessary.
	**/
	BFld BusVoltageLow : 1; /* 84 */
	/**
	\brief Motor's bus power supply overloaded.

	The motor's bus current has exceeded maximum continuous thresholds.

	Remedies:
	- Check that the power supply has been correctly configured
	for your AC voltage and is producing the expected voltage level.
	- Check the mechanics for abnormally high friction/binding.
	- Verify the configuration file in the motor is appropriate.
	- Reduce motion duty cycle, acceleration, or load.
	**/
	BFld BusRMSOverload : 1; /* 85 */
													/** \cond INTERNAL_DOC **/
	BFld : 1; /* 86 */
	/**
	\brief MtrEncIndexMissing

	The motor encoder index was not seen when it was expected.

	Appears in: AlertReg, WarnReg

	Remedies:
	- Replace defective motor
	**/
	BFld MtrEncIndexMissing : 1; /* 87 */
	
	/**
	 * \brief Bus Voltage under Minimum Operating Voltage
	 *
	 * The motor's bus voltage has drooped below the user-defined operating voltage.
	 *
	 * Remedies:
	 *  - Check that the Minimum Operating Voltage parameter is set correctly
	 * 	- Check that the power supply has been correctly configured
	 *  for your voltage and is producing the expected voltage level.
	 *  - Check the mechanics for abnormally high friction/binding. Grease if necessary.
	 */
	BFld BusVoltageUnderOperatingV : 1; /* 88 */

	BFld : 1; /* 89 */
													/** \endcond **/
													/** \cond INTERNAL_DOC **/
	BFld : 3; /* 90-92 */
													/** \endcond **/
	/**
	\brief MtrEncIndexMisplaced

	The motor encoder index was seen when it was not expected.

	Appears in: AlertReg, WarnReg

	Remedies:
	- Replace defective motor
	**/
	BFld MtrEncIndexMisplaced : 1; /* 93 */
	/**
	\brief StepsDuringPosnRecovery

	Step input commands were seen during the position recovery operation.

	Appears in: AlertReg

	Remedies:
	- Check that power supply is producing the expected voltage level.
	- Do not send steps until the motor is ready
	**/
	BFld StepsDuringPosnRecovery : 1; /* 94 */

	/**
	\brief Create a new line delimited string of the alerts set.

	Create a new line delimited string of the alerts set

	\param[in,out] buffer Pointer to a string buffer to replace with the
	alert fields that are non-zero.
	\param[in] size Count of characters in \a buffer.
	\return The original pointer the buffer \a buffer. This allows this function
	to be the argument of printing statement or other formatting function.
	**/
	MN_EXPORT char * MN_DECL StateStr(char *buffer, size_t size) const;
													/** \endcond  **/
};

/// ClearPath-SC Alert and Warning Register Field Definitions
typedef struct _cpmAlertFlds  cpmAlertFlds;
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  cpmAlertReg union
//
// DESCRIPTION
// Alert Register Size in octets: 96 bits = 3 32-bit entities = 12 octets
														/** \cond SC_EXPERT **/
/**
	\brief ClearPath-SC Alert and Warning Register Type.

	The alert and its related warning registers define the shutdown and
	transient operational state of the node. When a node enters a shutdown
	or potentially non-operable condition, the node will set bits in this
	register to tell the application what went wrong. The bits are classified
	by the following criteria:
	- \b Serious-fatal. This means the node has some condition that will
	prevent its operation. This is usually due to some setup issue or a
	hardware failure detected during the power-on self check. See the specific
	bit for potential recovery actions to allow node operations to continue.
	If the <i>Status Register's</i> [HwFailure](@ref _cpmStatusRegFlds::HwFailure) bit is asserted, this is a
	hardware failure and recovery is probably not possible.
	- \b Serious. This means some node self-preservation shutdown occurred.
	This can occur for various reasons, such as an excessive amount of torque
	was being utilized by the application.
	- \b Warning. These bits are usually set in the Warning register first
	to provide a "heads up" indication of looming troubles. Sometimes they
	will propagate to the Alert register and cause a shutdown.

	Many shutdowns can be cleared via the sFnd::IStatus::AlertsClear function to allow
	continued node operations. Most fatal errors cannot be cleared by this
	mechanism and require a power-cycle or node restart to clear them.

	The following diagram illustrates big picture view of the register.

	\image html "cpmAlertReg.png"
**/
union _cpmAlertReg {
	/// \brief Bit-wide view of register as three unsigned 32-bit integers.
	Uint32 bits[3];
	/// \brief Field access to the ClearPath-SC status register
	cpmAlertFlds Fld;
	// If we have C++, include these useful member functions.
																/// cond INTERNAL_DOC
	#ifdef __cplusplus
	#ifdef __TI_COMPILER_VERSION__
	bool InShutdown();
	bool InLockdown();
	bool InBlockMotionShutdown();
	bool InBlockMotionSelfClearShutdown();
	bool InNodeNeedsRepairShutdown();
	bool InSeriousShutdown();
	bool InKillOutputsShutdown();
	bool InAlert();
	#endif
	// Constructor, create with bits cleared
	_cpmAlertReg() {                   // Clear bits on construction
		STATIC_ASSERT(ALERTREG_SIZEOCTETS == 12);
		bits[0] = 0;
		bits[1] = 0;
		bits[2] = 0;
	}
	#endif
																	/// endcond
};
/// ClearPath-SC Alert and Warning Register Type.
typedef union _cpmAlertReg cpmAlertReg;
															/** \endcond **/
//                                                                            *
//*****************************************************************************



//****************************************************************************
// NAME                                                                      *
//  cpmTuneConfigReg union
//
// DESCRIPTION
/**
    \brief ClearPath-SC Tuning Configuration Register Type.

    This register controls various servo performance settings. For example:
    anti-hunt mode settings are here.

	\note These should be set from ClearView and not programmatically 
	changed unless directed by Teknic support staff.
**/
union PACK_BYTES _cpmTuneConfigReg {
    /// \brief Antihunt selections
    typedef enum _antihuntModes {
        ANTIHUNT_OFF,                       ///<  0 Off
        ANTIHUNT_STD                        ///<  1 Standard Mode
    } antihuntModes;                        ///< Anti-hunt modes

    /**
		\brief Register value as an integer

		Bit-wise representation of the <i>Tuning Configuration Register</i>.
	**/
    Uint32 bits;

    // Note: the numbers in the field descriptions denote their bit
    // range in the register.
	/** \cond INTERNAL_DOC **/
	/**
		\brief ClearPath-SC Tuning Configuration Fields.

		This register controls esoteric features of the various
		compensators and is generally maintained by ClearView. Programmatic
		changes should only be made under the advisement of Teknic 
		support.
	**/
    struct _cpmTuneConfigFlds {
        /**
			\brief Anti-Hunt Mode selection.

            Anti-Hunt mode select from the #antihuntModes enumeration.
        **/
        BFld AntiHunt			: 2;    // 0-1
													
        BFld					: 2;    // 2
													
        /**
			\brief Motor's velocity estimator selection.

            Motor's velocity estimator selection.
        **/
        BFld MtrVelocityEst     : 3;    // 4-6
 													
        /**
         	 \brief Don't ask if the load has changed

         	 If not set, prompt the user to verify that the load connected to
         	 this motor has not changed each time the motor is loaded into MSP
 
			 \note Used by ClearView
        **/
        BFld DontAskAboutLoad	: 1;	// 7 
        /**
         	 \brief Optimizing is Active
         	 Motor tuning gains are being modified by the auto-optimize process
         	 If communication is lost during the process, reset the node.

			 \note Used by ClearView
         **/
        BFld AutoTuning			: 1;	// 8
													
        /**
         	 \brief Force Velocity Compensator for stimulus/movegen moves
         	 Motor tuning gains are being modified by the auto-optimize process
         	 If communication is lost during the process, reset the node
         **/
        BFld UseVelCompInStim	: 1;	// 9
        /**
			\brief Track Kp0, Ki0 Gains
			As Kp-Out and Ki-Out are changed,
			Kp0 and Ki0 should be changed to match

			\note Used by ClearView
       **/
        BFld TrackGains			: 1;	// 10
        /**
        	\brief EnableFineTuning
        	Apply fine tuning scale factors to Kp/Ki and set the target window size

			\note Used by ClearView
        **/
        BFld EnableFineTuning	: 1;	// 11
													
		BFld 					: 1;	// 12
													
        /**
         	\brief DisableNonLinearGains
         	Turns off dynamically adjusted position gains based on tracking error

			\note Used by ClearView
         **/
        BFld DisableNonLinearGains	: 1;// 13
		/**
			\brief Data Collection control

			Clearing these bits will collect data during each move command.
			This is the typical setting for this feature.

			Setting This field will cause the <i>Data Collection</i>
			feature to free-run the collection between command calls. This
			is a special testing mode and generally not of much value
			for applications.
		**/
		BFld DataCollectFreeRunApp	: 1;	// 14 Data collector free runs on App
													
		BFld DataCollectFreeRunDiag	: 1;	// 15 Data collector free runs on Diag

		BFld VelocityCompensatorOverride : 1; // 16 Velocity Compensator Override
													
    };                               
	/**
		\brief ClearPath-SC Field Access for the Tuning Register
	**/
	struct _cpmTuneConfigFlds Fld;
    // For C++ non-firmware usage, include a constructor.
    #ifdef __cplusplus
		#ifndef __TI_COMPILER_VERSION__
													
		/// Clear bits on construction
		_cpmTuneConfigReg() {               
            bits = 0;
        }
													
        #endif
    #endif
};
/// \copybrief _cpmTuneConfigReg
typedef union _cpmTuneConfigReg cpmTuneConfigReg;

//                                                                            *
//*****************************************************************************

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//                      NOT DOCUMENTED ITEMS - BELOW
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

													/** \cond INTERNAL_DOC **/



//****************************************************************************
// NAME                                                                      *
//  cpmOptionRegFlds union
//
// DESCRIPTION
/**
	\brief ClearPath-SC Option Register Field Definitions.

	This is a factory set register that is used for testing and commissioning
	ClearPath-SC nodes.
**/
struct _cpmOptionRegFlds {
	/*                   -----------  ----------------------------------- *
	*                          bit #  description                         *
	*                    -----------  ----------------------------------- */
	BFld VoltMode : 1;			// 0:  Volt mode setup
	BFld RuntimeCal : 1;		// 1:  1=Enable run time calibration
	BFld ClockCal : 1;			// 2:  1=clock calibration
	BFld DirCwPositive : 1;		// 3:  1=CW direction is positive motion

	BFld Product : 2;			// 4:  2-bit product number
								// 5:  ""
	BFld Advanced : 1;			// 6:  Advanced features available
	BFld VectorLock : 1;		// 7   Lock Vector Mode

	BFld IndexTolerance : 2;	// 8-9
	BFld IgnoreIndx : 1;		// 10 Prevent Ref Offset on index
	BFld NoVectUpdate : 1;		// 11 do not apply the angle from the enhanced 
								//    sensorless algorithm

	BFld: 4;					// 12-15 <-DO NOT combine these BFld entries!
	BFld: 3;					// 16-18 <-DO NOT combine these BFld entries!
	BFld FWverMask : 1;			// 19: Inhibit display of minor FW version

	BFld: 1;					// 20:
	BFld Vendor : 4;			// 21: Vendor code (LSB)
								// 22:  0=TEKNIC
								// 23:
								// 24: Vendor code (MSB)
	BFld Debug : 3;				// 25: Debug Mode (LSB)
								// 26:  0,7=off
								// 27: Debug Mode (MSB)
	BFld SensorlessDelaySeed : 3;//  28-30
};
/// ClearPath-SC Option Register Field Definitions
typedef struct _cpmOptionRegFlds cpmOptionsFlds;
//                                                                            *
//*****************************************************************************


typedef struct _cpmSetFlagsFlds {
	BFld:1;
	BFld HardStop : 1;					// 1  Simulate hitting a hard stop
	BFld HomingReq : 1;					// 2  Request to initiate homing
	BFld HomingComplete : 1;			// 3  Homing completed manually
	BFld HomingInvalid : 1;				// 4  Force homing to be invalid
} cpmSetFlagsFlds;

typedef union _cpmSetFlagsReg {
	Uint16 bits;
	cpmSetFlagsFlds fld;

	/// Clear bits on construction
#ifdef __cplusplus
#ifndef __TI_COMPILER_VERSION__

	/// Clear bits on construction
	_cpmSetFlagsReg() {
		bits = 0;
	}

#endif
#endif
} cpmSetFlagsReg;

												/** \endcond **/

//*****************************************************************************
// Restore byte packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif
//*****************************************************************************
#endif // __PUBCPM_REGS_H__
//============================================================================= 
//	END OF FILE pubCpmRegs.h
//=============================================================================

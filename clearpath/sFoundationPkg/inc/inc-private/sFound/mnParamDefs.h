//******************************************************************************
// $Archive: /ClearPath SC/User Driver/inc/mnParamDefs.h $
// $Date: 01/19/2017 17:39 $
// $Workfile: mnParamDefs.h $ 
//
// DESCRIPTION:
/**
	\file
	\cond INTERNAL_DOC
	\brief
		Parameter managment API definitions. This includes common "unit" 
		and parameter type definitions.

	\ingroup ISCgrp
	@{
**/ 		
//
// CREATION DATE:
//		02/13/1998 12:46:56 - as netCore.h
//		06/11/2009 10:22:00 - renamed for Meridian family
//																	  	 !-8!
// COPYRIGHT NOTICE:
//		(C)Copyright 1998  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//
//																			   *
////****************************************************************************

#ifndef __MNPARAMDEFS_H__
#define __MNPARAMDEFS_H__

/**************************************************************************** 
 * !NAME!																    *
 * 	mnParamDefs.h headers
 *																		!0! */
#include "lnkAccessAPI.h"
#include <assert.h>
/* 																	   !end!*
 ****************************************************************************/




/****************************************************************************
 * !NAME!																	*
 * 	mnParamDefs.h types
 * 																		    */
/* Command Set Constants */
#define RESP_STATUS_LEN 2			// Length of common response items (status)


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\brief Unit Type codes.

	These code provide access to the desired units for a value.
**/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef enum _unitTypes {
	NO_UNIT,						// No intrinsic unit
	BIT_FIELD,						// Value is a bit field
	DX_TICK,						// Distance: the encoder tick
	DX_INCH,						// Distance: inches
	DX_MM,							// Distance: millimeters
	DX_REV,							// Distance: revolution
	VOLT,							// Voltage in volts
	CURRENT,						// Current in amperes
	TIME_S,							// Time in seconds
	TIME_MSEC,						// Time in milliseconds
	TIME_USEC,						// Time in microseconds
	TIME_SAMPLE,					// Time in sample times
	VEL_TICK_MSEC,					// Velocity is encoder counts / ms
	VEL_TICK_MSEC2,					// Acceleration is encoder counts / ms^2
	VEL_TICK_SAMPLE,				// Velocity is encoder counts / sample time
	VEL_TICK_SAMPLE2,				// Acceleration is encoder counts / sample time^2
	VEL_STEPS_S,					// Velocity is steps / second
	VEL_STEPS_MS,					// Velocity is steps / millisecond
	VEL_RPM,						// Velocity is revolutions per minute
	VEL_RPS,						// Velocity is revolutions per second
	VEL_IPS,						// Velocity is inches per second
	VEL_MMPS,						// Velocity is mm per second
	STRING8,						// String value	(fixed 8 bytes)
	OSTRING,						// OLESTR
	FW_VERS,						// Firmware version structure
	HW_VERS,						// Hardware version structure	
	DEV_ID,
	IOC_HZ,							// IOC filter corner frequency
	UNIT_HZ,						// Frequency in Hz
	DEG_C,
	VOLTS_PER_KRPM,
	HOURS,
	STRING,
	DEGREES,
	VEL_TICKS_S,					// Velocity in ticks/sec
	VEL_TICKS_S2,					// Acceleration in ticks/sec^2
	PERCENT_MAX,
	TIME_MIN,						// Time in minutes
	} unitTypes;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\brief Parameter types codes

	These codes describe the parameter type from the host's perspective.
	The type is a bit oriented value made up of basic types. They tell
	the driver and user how access to the parameter will be treated. For
	example PT_RT will ignore the cached indicator and cause another
	transfer from the node upon request. Non PT_RT will normal only read
	the node's value once and return the cached copy.
**/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef enum _paramTypes {
	// Base type bits
	PT_NONE,						///<  Parameter is not present
	PT_RO=1,						///<  Read only parameter
	PT_VOL=2,						///<  Volatile R/W parameter
	PT_NV=4,						///<  Non-volatile R/W parameter
	PT_RT=8,						///<  Real-time non-cached value
	PT_CLR=16,						///<  Clear on read type
	PT_RAM=32,						///<  RAM R/W type
	PT_ADV=64,						///<  Exists in advanced models
	PT_IN_NODE_CFG=128,				///<  In node section of cfg file
	PT_IN_MTR_CFG=256,				///<  In motor section of cfg file
	PT_IN_ALL_CFG=512,				///<  Extended config in drive section
	PT_IN_FACT_CFG=1024,			///<  In Factory Settings
	PT_RST_SHDN_RISK=2048,			///<  Could cause shutdown during factory reset
	// Typical configurations
	PT_VOL_RO=PT_RO|PT_VOL,			///<  Volatile R/O parameter
	PT_RO_RT=PT_RO|PT_RT,			///<  Read only real-time parameter
	PT_ROC_RT=PT_RO|PT_RT|PT_CLR,	///<  Read only real-time parameter/clear
	PT_RW_RT=PT_RT,					///<  R/W real-time parameter
	PT_RAM_RT=PT_RAM|PT_RT,			///<  R/W RAM real-time parameter
	PT_NV_RW=PT_NV,					///<  Non-volatile R/W parameter
	PT_NV_RWA=PT_NV|PT_ADV,			///<  Non-vol R/W parameter (advanced)
	PT_NV_RW_RT=PT_RT|PT_NV,		///<  Non-volatile R/W real-time parameter
	PT_CFG=PT_NV_RW|PT_IN_NODE_CFG,	///<  Non-vol R/W in config file item
	PT_CFGA=PT_CFG|PT_ADV,			///<  Non-vol R/W in config file item (advanced)
	PT_FCFG=PT_NV_RW|PT_IN_NODE_CFG|PT_IN_FACT_CFG,	// Factory setting in config file
	PT_FCFG_RT = PT_NV_RW | PT_IN_NODE_CFG | PT_IN_FACT_CFG | PT_RT,	// Factory setting in config file (real-time)
	PT_FAC=PT_NV_RW|PT_IN_FACT_CFG,	//	  Factory setting
	PT_MTR=PT_NV_RW|PT_IN_MTR_CFG,	///<  Non-vol R/W in motor section
	PT_MTR_R=PT_MTR|PT_RST_SHDN_RISK, //  Non-vol R/W in motor section; could cause shutdown during factory reset
	PT_FMTR=PT_NV_RW|PT_IN_MTR_CFG|PT_IN_FACT_CFG,	///<  Factory motor items  in config file
	PT_ALL=PT_NV_RW|PT_IN_ALL_CFG,	///<  Non-vol R/W extended config
	PT_UNKNOWN=-1
} paramTypes;

typedef enum _signTypes  {			// Sign extension handling
	ST_UNSIGNED,					// 0 - unsigned 0 ... 2^n
	ST_SIGNED,						// 1 - signed -2^n-1 ... 2^n-1
	ST_POS_ONLY						// 2 - signed 0 ... 2^n-1
} signTypes;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
		 Format that the application layer uses to describe a parameter
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef union _appNodeParam {
	unsigned bits;
	struct {
		unsigned param	: 7;		// Parameter base number
		unsigned option : 1;		// Option bit
		unsigned bank	: 2;		// Parameter bank
	} fld;
} appNodeParam;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\brief Parameter value container. 

	This container is used to access the bit-wise view of a non-numeric
	parameter via the \a raw member. The \a BufferSize member will be
	set to the length in the buffer received from the node. The \a
	exists member will be set if the drive has a current value in its
	cache. If the parameter has a numeric equivalent the \a value member
	will be set to it.
**/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#pragma pack(push,1)				// Align to bytes boundaries

typedef struct _paramValue {
	double value;					///< Current numeric value (if exists)
	nodebool exists;				///< Value present
	nodebool isPolled;				// Value is polled (not used)
	packetbuf raw;					///< The "raw" octet value
#ifdef __cplusplus
	_paramValue() {
		value = 0;
		exists = false;
		isPolled = false;
	}
#endif
} paramValue;
#pragma pack(pop)
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
							Parameter scaling holder
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
typedef struct _paramscale {
	nodeUNIstr unitName;			// Unit name ptr
	unitTypes unitType;				// Unit type
	double scaleFromBase;			// base unit * scale = new unit
} paramscale;
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *
							 Parameter Information
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef enum _initType {
	RAM,	// RAM only type
	NV,		// EE only type
	NVRV,	// EE w/ RAM virt shadow (write to RAM updates EE)
	NVR,	// EE w/ RAM shadow (need to write to both)
	RO		// Read-only
} initType;

typedef struct _factoryInitData  {
	initType type;					// Parameter type
	nodeulong paramNum;				// Parameter number
	double value;					// Initialize value
	nodelong minVersion;			// Minimum version (on ClearPath, a negative
									// value indicate the version when the default
									// param was removed from or made restrictive 
									// on the FW)
	nodeulong fwOption;				// Firmware option
	nodeulong hwPlat;				// Setting for hardware platform
} factoryInitData;

// This is the enumeration from ValuesLib.dll that is used to translate 
// node specific parameter numbers into the generic values used by the
// PhysDriveObj node implementations.
typedef enum _ParameterNumbers{
    PARAM_NULL = 0,
    PARAM_KV = 1,
    PARAM_KI = 2,
    PARAM_KP = 3,
    PARAM_KFV = 4,
    PARAM_KFA = 5,
    PARAM_KFJ = 6,
    PARAM_KIP = 7,
    PARAM_KII = 8,
    PARAM_KNP = 9,
    PARAM_KNV = 10,
    PARAM_KB = 11,
    PARAM_KFF = 12,
    PARAM_TRQ_LIM = 13,
    PARAM_SAMP_RATE = 14,
    PARAM_CONFIG_REG = 15,
    PARAM_RO = 16,
    PARAM_STEP_RES = 17,
    PARAM_ENC_DENS = 18,
    PARAM_ENC_RESOL = 19,
    PARAM_POLES = 20,
    PARAM_ROM_SUM = 21,
    PARAM_ANA_IN_OFFS = 22,
    PARAM_TRK_ERR_LIM = 23,
    PARAM_RMS_LIM = 24,
    PARAM_RMS_TC = 25,
    PARAM_ERR_CODES = 26,
    PARAM_TP_IR = 27,
    PARAM_TP_IS = 28,
    PARAM_MON_AMPL = 29,
    PARAM_MON_VAR = 30,
    PARAM_IN_RANGE_WIN = 31,
    PARAM_TRQ_DIG = 32,
    PARAM_MECH_ANGLE = 33,
    PARAM_TP_ANALOG = 34,
    PARAM_TP_IOP = 35,
    PARAM_FUZZ_AP = 36,
    PARAM_BUS_V = 37,
    PARAM_MTR_OHMS = 38,
    PARAM_MTR_ELECT_TC = 39,
    PARAM_I_MAX = 40,
    PARAM_MTR_KE = 41,
    PARAM_SPEED_LIM = 42,
    PARAM_MV_DN_TC = 43,
    PARAM_COM_CAP = 44,
    PARAM_RASCON = 45,
    PARAM_TRQ_FLDBACK = 46,
    PARAM_HS_THRESHOLD = 47,
    PARAM_HS_TC = 48,
    PARAM_TRQ_BIAS = 49,
    PARAM_TRQ_REL_TC = 50,
    PARAM_TRQ_FILT = 51,
    PARAM_MON_FILT = 52,
    PARAM_ANA_VEL_MAX = 53,
    PARAM_EE_UPD_ACK = 54,
    PARAM_SERVO_DELAY = 55,
    PARAM_READY_DELAY = 56,
    PARAM_LPB_WINDOW = 57,
    PARAM_NV_MODIFIED = 58,
    PARAM_SERIAL_NUMBER = 59,
    PARAM_RMS_MAX = 60,
    PARAM_RMS_MAX_TC = 61,
    PARAM_OPTION_REG = 62,
    PARAM_VECT_RATE = 63,
    PARAM_HW_REV = 64,
    PARAM_FUZZY_APERTURE = 65,
    PARAM_ANTI_HUNT_HYSTERESIS = 66,
    PARAM_POSN_CMD = 67,
    PARAM_POSN_MEAS = 68,
    PARAM_VEL_CMD = 69,
    PARAM_VEL_MEAS = 70,
    PARAM_RMS_LVL = 71,
    PARAM_STATUS_REG = 72,
    PARAM_ADC_MAX = 73,
    PARAM_BUS_V_MEAS = 74,
    PARAM_TRQ_MEAS = 75,
    PARAM_TRQ_CMD = 76,
    PARAM_TEMPERATURE = 77,
    PARAM_TP16V = 78,
    PARAM_TP5V = 79,
    PARAM_TRQ_FLDBACK_NEG = 80,
    PARAM_TRQ_FLDBACK_POS = 81,
    PARAM_TRQ_FLDBACK_HS = 82,
    PARAM_TRQ_FLDBACK_NEG_TC = 83,
    PARAM_TRQ_FLDBACK_POS_TC = 84,
    PARAM_TRQ_FLDBACK_HS_TC = 85,
    PARAM_TRQ_FLDBACK_MVDN_TC = 86,
    PARAM_TRQ_FLDBACK_MVDN = 87,
    PARAM_STATUS_RT_REG = 88,
    PARAM_OVER_V_TRIP = 89,
    PARAM_LPB_THRESH = 90,
    PARAM_HS_STOP_VEL = 91,
    PARAM_NO_COMM_START_TRQ = 92,
    PARAM_KNVAH = 93,
    PARAM_TRQ_SCALE = 94,
    PARAM_MIN_TRK_ERR = 95,
    PARAM_MAX_TRK_ERR = 96,
    PARAM_POSN_CAP_EN = 97,
    PARAM_ANA_IN_CONFIG = 98,
    PARAM_CONFIG1_REG = 99,
    PARAM_COM_CONFIG = 100,
    PARAM_GEAR_RES = 101,
    PARAM_ESTOP_DECEL = 102,
    PARAM_RMS_PEAK = 103,
    PARAM_MTR_TEMP_LOCKOUT = 104,
    PARAM_DRV_ENBL_LOCKOUT = 105,
    PARAM_THERM_SHUTDOWN_DELAY = 106,
    PARAM_VEL_FILTER = 107,
    PARAM_NO_COM_TIME = 108,
    PARAM_LOAD_MTR_RATIO = 109,
    PARAM_COUPLING_ERR_LIM = 110,
    PARAM_KPL = 111,
    PARAM_POSN_MTR = 112,
    PARAM_POSN_LOAD = 113,
    PARAM_AHFUZZ2 = 114,
    PARAM_GEAR_MAIN = 115,
    PARAM_LD_SPEED_LIM = 116,
    PARAM_RUNTIME_ERR = 117,
    PARAM_AMBIENT_TEMP = 118,
    PARAM_IR_CAL_FACTOR = 119,
    PARAM_IS_CAL_FACTOR = 120,
    PARAM_PARTID = 121,
    PARAM_COMM_MASK = 122,
    PARAM_COMM_FACTOR = 123,
    PARAM_CAL_ANA = 124,
    PARAM_CAL_ANA_OFFS = 125,
    PARAM_FAN_ON_TEMP = 126,
    PARAM_IB_TC = 127,
    PARAM_IB_TRIP = 128,
    PARAM_I_TRIP = 129,
    PARAM_IB = 130,
    PARAM_IB_PEAK = 131,
    PARAM_I_TP_FILT = 132,
    PARAM_BUS_V_OFFS = 133,
    PARAM_BUS_V_CAL = 134,
    PARAM_BUS_V_LOW = 135,
    PARAM_REGEN_ON_V = 136,
    PARAM_REGEN_OFF_V = 137,
    PARAM_REGEN_PWR_LIM = 138,
    PARAM_REGEN_PWR_TC = 139,
    PARAM_REGEN_DMP_PWR_LIM = 140,
    PARAM_IB_OFFS = 141,
    PARAM_IB_CAL = 142,
    PARAM_TEMP_WARN = 143,
    PARAM_TEMP_LIM = 144,
    PARAM_FAN_WARN_SPD = 145,
    PARAM_FAN_FAULT_SPD = 146,
    PARAM_IB_TRIP_RMS = 147,
    PARAM_SHUTDOWN_DETAIL_REG = 148,
    PARAM_OUTPUT_REG = 149,
    PARAM_INPUT_REG = 150,
    PARAM_FAN_SPD = 151,
    PARAM_AC_FREQ = 152,
    PARAM_RMS_LIM_3PH = 153,
    PARAM_I_OFFS_INHIBIT = 154,
    PARAM_AC_MON_FILT = 155,
    PARAM_KM_FACT = 156,
    PARAM_MTR_OHMS_MIN = 157,
    PARAM_AH_HOLDOFF = 158,
    PARAM_ACC_LIM = 159,
    PARAM_DEC_LIM = 160,
    PARAM_VEL_LIM = 161,
    PARAM_HEAD_DX = 162,
    PARAM_TAIL_DX = 163,
    PARAM_HT_VEL_LIM = 164,
    PARAM_POSN_MTR_INDX = 165,
    PARAM_POSN_LD_INDX = 166,
    PARAM_TSPD = 167,
    PARAM_CFG_TUNE = 168,
    PARAM_CFG_FEATURE = 169,
    PARAM_CFG_HW = 170,
    PARAM_CFG_APP = 171,
    PARAM_PART_NUM = 172,
    PARAM_EE_VER = 173,
    PARAM_ROM_UPD_ACK = 174,
    PARAM_ALERT_REG = 175,
    PARAM_WARN_RT_REG = 176,
    PARAM_WARN_MASK_REG = 177,
    PARAM_WARN_ACCUM_REG = 178,
    PARAM_STATUS1_SW_ACCUM = 179,
    PARAM_STATUS1_ACCUM = 180,
    PARAM_STATUS1_RT = 181,
    PARAM_STATUS1_ATTN_MASK = 182,
    PARAM_ON_TIME = 183,
    PARAM_ALERT_MASK_REG = 184,
    PARAM_STATUS1_ATTN_RISE = 185,
    PARAM_STOP_TYPE = 186,
    PARAM_WATCHDOG_TIME = 187,
    PARAM_IEX_STATUS = 188,
    PARAM_IEX_GLITCH_CNT = 189,
    PARAM_IEX_GLITCH_PEAK = 190,
    PARAM_IEX_INPUT_REG = 191,
    PARAM_IEX_IN_RISE_REG = 192,
    PARAM_IEX_IN_FALL_REG = 193,
    PARAM_IEX_IN_REG_MASK = 194,
    PARAM_IEX_IN_RISE_REG_MASK = 195,
    PARAM_IEX_IN_FALL_REG_MASK = 196,
    PARAM_IEX_GLITCH_LIM = 197,
    PARAM_IEX_OUT_REG = 198,
    PARAM_IEX_USER_OUT_REG = 199,
    PARAM_IEX_STOP_OUT_REG = 200,
    PARAM_STEP_RUN_TRQ = 201,
    PARAM_STEP_IDLE_TRQ = 202,
    PARAM_STEP_ACC_TRQ = 203,
    PARAM_STEP_MICRO = 204,
    PARAM_RAS_MAX_VEL = 205,
    PARAM_TRQ_FLDBACK_STEP_TC = 206,
    PARAM_CMD_TUNE_REG = 207,
    PARAM_OUT_SRC = 208,
    PARAM_CMD_TUNE_ATTACK = 209,
    PARAM_CMD_TUNE_GSTOP = 210,
    PARAM_CMD_TUNE_MODE = 211,
    PARAM_CMD_TUNE_DELAY = 212,
    PARAM_JOG_VEL_LIM = 213,
    PARAM_JOG_ACC_LIM = 214,
    PARAM_COUPLING_ERR = 215,
    PARAM_TRK_ERR = 216,
    PARAM_A_AFTER_DIST = 217,
    PARAM_B_BEFORE_DIST = 218,
    PARAM_AT_POSN_LOC = 219,
    PARAM_COMM_ANGLE_LIM = 220,
    PARAM_HLESS_RAMPUP_TIME = 221,
    PARAM_HLESS_SWEEP_TIME = 222,
    PARAM_HLESS_SETTLE_TIME = 223,
    PARAM_SOFT_LIM_POSN_POS = 224,
    PARAM_SOFT_LIM_POSN_NEG = 225,
    PARAM_AH_GAIN = 226,
    PARAM_MOVE_DWELL = 227,
    PARAM_KZERO = 228,
    PARAM_IZERO = 229,
    PARAM_TGT_WIN = 230,
    PARAM_STAB_WIN = 231,
    PARAM_KP_UNDERDAMPED = 232,
    PARAM_KP_CRITICAL = 233,
    PARAM_KI_UNDERDAMPED = 234,
    PARAM_KI_CRITICAL = 235,
    PARAM_KOUT = 236,
    PARAM_IOUT = 237,
    PARAM_R2S_TRQ_LIM = 238,
    PARAM_R2S_VEL0 = 239,
    PARAM_R2S_VEL1 = 240,
    PARAM_R2S_VEL2 = 241,
    PARAM_R2S_VEL3 = 242,
    PARAM_VELKNOB_MAX_CW_VEL = 243,
    PARAM_VELKNOB_MAX_CCW_VEL = 244,
    PARAM_VELKNOB_VEL_PER_CNT = 245,
    PARAM_ABSPOSN_HOMING_VEL = 246,
    PARAM_ABSPOSN_POSN0 = 247,
    PARAM_ABSPOSN_POSN1 = 248,
    PARAM_ABSPOSN_POSN2 = 249,
    PARAM_ABSPOSN_POSN3 = 250,
    PARAM_SPINONPWR_VEL = 251,
    PARAM_M2S_DIST_TIL_SW = 252,
    PARAM_STOP_QUAL_TC = 253,
    PARAM_VELFLW_DEADBAND = 254,
    PARAM_VELFLW_MAX_FREQ = 255,
    PARAM_VELFLW_MIN_FREQ = 256,
    PARAM_VELFLW_MAX_VEL = 257,
    PARAM_HOMING_OFFSET = 258,
    PARAM_TRQFLW_MAX_TRQ = 259,
    PARAM_TRQFLW_OVERVEL_TC = 260,
    PARAM_TRQFLW_TRQ_FILT_TC = 261,
    PARAM_ENBL_TC = 262,
    PARAM_ENBL_PULSE_TC = 263,
    PARAM_IN1_IN2_TC = 264,
    PARAM_INPUT_STIM = 265,
    PARAM_VEL_MAX_HLFB = 266,
    PARAM_STEP_RES_ACTIVE = 267,
    PARAM_BASE_UNIT = 268,
    PARAM_TP1_65V = 269,
    PARAM_TP12V = 270,
    PARAM_INCPOSN_DIST0 = 271,
    PARAM_INCPOSN_DIST1 = 272,
    PARAM_INCPOSN_DIST2 = 273,
    PARAM_INCPOSN_DIST3 = 274,
    PARAM_HLESS_RDG = 275,
    PARAM_HLESS_SETUP = 276,
    PARAM_HLESS_AMPS = 277,
    PARAM_RMS_INIT = 278,
    PARAM_BUS_V_MAX = 279,
    PARAM_HLFB_DUTY = 280,
    PARAM_IN_FREQ_OVR = 281,
    PARAM_IN_DUTY_OVR = 282,
    PARAM_PULSE_VEL_ADJ = 283,
    PARAM_PULSE_INCR_ADJ = 284,
    PARAM_SET_FLAGS = 285,
    PARAM_RUNTIME_FLAGS = 286,
    PARAM_CLAMP_TRQ = 287,
    PARAM_CLAMP_TC = 288,
    PARAM_AT_STROKE_MIN = 289,
    PARAM_AT_STROKE_MAX = 290,
    PARAM_AT_MAX_VEL = 291,
    PARAM_AT_MAX_ACC = 292,
    PARAM_AT_MAX_TRQ = 293,
    PARAM_AT_MAX_VOLTS = 294,
    PARAM_KP_ZERO = 295,
    PARAM_KP_OUT = 296,
    PARAM_KI_ZERO = 297,
    PARAM_KI_OUT = 298,
    PARAM_KT = 299,
    PARAM_KV_BASE = 300,
    PARAM_KII_0 = 301,
    PARAM_KII_1 = 302,
    PARAM_KII_2 = 303,
    PARAM_KII_3 = 304,
    PARAM_KII_4 = 305,
    PARAM_KII_5 = 306,
    PARAM_KII_6 = 307,
    PARAM_KII_7 = 308,
    PARAM_KII_8 = 309,
    PARAM_KII_9 = 310,
    PARAM_KIP_0 = 311,
    PARAM_KIP_1 = 312,
    PARAM_KIP_2 = 313,
    PARAM_KIP_3 = 314,
    PARAM_KIP_4 = 315,
    PARAM_KIP_5 = 316,
    PARAM_KIP_6 = 317,
    PARAM_KIP_7 = 318,
    PARAM_KIP_8 = 319,
    PARAM_KIP_9 = 320,
    PARAM_STOP_QUAL_VEL = 321,
    PARAM_ALT_VEL_LIM = 322,
    PARAM_HLESS_VOLTS = 323,
    PARAM_PWM_CMD_PCT = 324,
    PARAM_IN_FREQ = 325,
    PARAM_IN_DUTY = 326,
    PARAM_R_0 = 327,
    PARAM_R_1 = 328,
    PARAM_R_2 = 329,
    PARAM_R_3 = 330,
    PARAM_R_4 = 331,
    PARAM_R_5 = 332,
    PARAM_R_6 = 333,
    PARAM_R_7 = 334,
    PARAM_R_8 = 335,
    PARAM_R_9 = 336,
    PARAM_IN1_TC = 337,
    PARAM_BNF_MAX_CW_VEL = 338,
    PARAM_BNF_MAX_CCW_VEL = 339,
    PARAM_MAX_STEP_RATE = 340,
    PARAM_RMS_LIM_MSP = 341,
    PARAM_RMS_TC_MSP = 342,
    PARAM_MAX_TRAVEL_DIST = 343,
    PARAM_HOMING_ACCEL = 344,
    PARAM_HS_DELAY_TIME = 345,
    PARAM_HOMING_DECEL = 346,
    PARAM_DIST_INTO_SWITCH = 347,
    PARAM_RAS_DELAY = 348,
    PARAM_DRV_KP_FACTOR = 349,
    PARAM_DRV_KPZ_FACTOR = 350,
    PARAM_DRV_KI_FACTOR = 351,
    PARAM_DRV_FINE_TUNE = 352,
    PARAM_BURST_MIN_IN_FREQ = 353,
    PARAM_KR = 354,
    PARAM_VELKNOB_TARGET = 355,
    PARAM_TRQFLW_MAX_SPEED = 356,
    PARAM_AH_VOLT_FILT_TC = 357,
    PARAM_DRV_MODES = 358,
    PARAM_BUS_AT_ENBL = 359,
    PARAM_KNOB_SPD_LIM = 360,
    PARAM_AUTO_RAS_CODE = 361,
    PARAM_ACC_ENH_AMPL = 362,
    PARAM_ACC_ENH_PHASE = 363,
    PARAM_MECH_POSN = 364,
    PARAM_SPINONPWR_VEL_DRV = 365,
    PARAM_HOMING_TRQ_LIM = 366,
    PARAM_VEL_MEAS_FILT = 367,
    PARAM_INDEX_COUNT = 368,
    PARAM_INERTIA = 369,
    PARAM_STATIC_FRICTION = 370,
    PARAM_VISCOUS_FRICTION = 371,
    PARAM_IN_RANGE_VEL = 372,
    PARAM_KIP_ADJ = 373,
    PARAM_KII_ADJ = 374,
    PARAM_EFF_KPZERO = 375,
    PARAM_EFF_KPOUT = 376,
    PARAM_EFF_KIZERO = 377,
    PARAM_EFF_KIOUT = 378,
    PARAM_EFF_KIP = 379,
    PARAM_EFF_KII = 380,
    PARAM_VOLTSQ_CMD = 381,
    PARAM_HLESS_ENTRY_MOTION = 382,
    PARAM_HLESS_VERIFY_MAX_MOTION = 383,
    PARAM_HLESS_VERIFY_MIN_MOTION = 384,
    PARAM_HLESS_VERIFY_TRQ_TIME = 385,
    PARAM_HLESS_VERIFY_RAMP_TIME = 386,
    PARAM_POWERUP_HOLDOFF_TIME = 387,
    PARAM_USER_DESC = 388,
    PARAM_CFG_FLDBK=389,			// Foldback config register
    PARAM_XPS_OUT_SRC_REG=390,		// Output bits sourced by CPS
    PARAM_XPS_INVERT_INPUT=391,		// CPS invert inputs
    PARAM_XPS_INVERT_OUTPUT=392,	// CPS invert outputs
    PARAM_XPS_FEAT_ENABLE=393,		// CPS enable config
	PARAM_XPS_FEAT_CAPTURE=394,		// CPS capture config
    PARAM_XPS_FEAT_TRIGGER=395,		// CPS trigger config
    PARAM_XPS_FEAT_NODE_STOP=396,	// CPS node stop config
    PARAM_XPS_FEAT_RESET_TIMER=397, // CPS reset timer config
    PARAM_XPS_FEAT_ATTN0=398,		// CPS attn-0 config
    PARAM_XPS_FEAT_ATTN1=399,		// CPS attn-1 config
    PARAM_XPS_FEAT_AT_HOME=400,		// CPS at home config
    PARAM_XPS_FEAT_POS_TRQ_FLBK=401,// CPS positive trq foldback config
    PARAM_XPS_FEAT_NEG_TRQ_FLBK=402,// CPS negative trq foldback config
    PARAM_XPS_FEAT_IN_POS_LIM=403,	// CPS in positive limit config
    PARAM_XPS_FEAT_IN_NEG_LIM=404,	// CPS in negative limit config
	PARAM_XPS_IN_SRC_REG=405,       // Input bits sourced by CPS
    PARAM_XPS_USER_IN_REG=406,      // User driven CPS Inputs
    PARAM_XPS_USER_OUT_REG=407,     // User driven CPS Outputs
	PARAM_XPS_ACTUAL_IN_REG=408,    // Drive driven CPS Inputs
    PARAM_XPS_IN_REG=409,           // Input to the CPS
	PARAM_DLY_TO_DISABLE,			// 410 Delay to disable for brakes
	PARAM_ATTN_MASK_DRVR,			// 411 Driver's attention mask
	PARAM_GRP_SHUTDOWN_MASK,		// 412 Group Shutdown items
	PARAM_INPUT_ACTIONS,			// 413 Input A/B Actions (ClearView only)
	PARAM_SOFT_LIM_POSN_1,			// 414 User defined soft limit
	PARAM_SOFT_LIM_POSN_2,			// 415 User defined soft limit
    PARAM_POSN_CAP_INA_HI_SPD,      // 416 Input A (high-speed) posn capture
    PARAM_POSN_CAP_INB,             // 417 Input B posn capture
	PARAM_COMM_LOW_V,				// 418 Minimum operating voltage
	PARAM_TP_SUPPLY1_V,				// 419 Supply1 voltage
	PARAM_REGEN_DUMP_RATE,			// 420 D-Cmd current to dump regen voltage
	PARAM_REGEN_RAMP_TIME,			// 421 Time to ramp up to full regen dump current
	PARAM_REGEN_TRIP_V,				// 422 Voltage to perform regen dump
	PARAM_UNDER_TEMP_TRIP,			// 423
	PARAM_ENCODER_TEMP,				// 424
	PARAM_STATOR_TEMP,				// 425
	PARAM_STATOR_TEMP_TRIP,			// 426
	PARAM_STATOR_TEMP_WARN,			// 427
	PARAM_PWR_STATUS,				// 428
	PARAM_VECTOR_DRIFT_LIM,			// 429
	PARAM_DRV_PB_SER_NUM,			// 430
	PARAM_DRV_PB_REV,				// 431
	PARAM_TP_PHASE_BOARD,			// 432
	PARAM_DRV_TEMP,                 // 433 Drive Temperature
	PARAM_ALERT_ACCUM_REG,			// 434 Accumulated alerts
	PARAM_CFG_APP1 = 436,			// 436 App Config 1
	PARAM_PWM_CMD_PCT_A,			// 437
	PARAM_IN_FREQ_A,				// 438
	PARAM_IN_DUTY_A,				// 439
	PARAM_ALT_TRQ_LIM,				// 440
	PARAM_ADV_TLIM_00,				// 441
	PARAM_ADV_TLIM_01,				// 442
	PARAM_ADV_TLIM_10,				// 443
	PARAM_ADV_TLIM_11,				// 444
	PARAM_ADV_TRQ_TC,				// 445
	PARAM_APP_PWMC_MAX_VEL,			// 446
	PARAM_RMS_SLOW_LIM,				// 447
	PARAM_RMS_SLOW_TC,				// 448
	PARAM_RMS_SLOW_LVL,				// 449
	PARAM_BUSV_ADJ_RATE,			// 450
	PARAM_SOFT_START_FREQ,			// 451
	PARAM_INA_DUTY_OVR,				// 452
	PARAM_PWMC_INA_DUTY,			// 453
	PARAM_POSN_REC_VEL,				// 454
	PARAM_POSN_REC_ACC,				// 455
	PARAM_DRV_MIN_OPER_VOLTS,		// 456
	PARAM_PWR_AC_LOSS_TC = 458,		// 458
	PARAM_IB_TC_SLOW=460,			// 460
	PARAM_TEMP_LIM_USER,			// 461
	PARAM_EVENT_SHUTDOWN_MASK,		// 462
	PARAM_PWR_AC_WIRING_ERROR_TC,	// 463
} configKeys;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\brief Detailed parameter information.

	This container provides more detailed internal information about the
	type of a parameter.
**/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef struct _paramInfo {
	nodebool reciprical;			// Node value is reciprical of this value
	nodeushort signextend;	   		// Value is signed	(signTypes)
	paramTypes paramType;			///< Parameter type
	unitTypes unitType;				///< Unit type
	nodeulong paramSize;			// Number of octets expected in parameter
	double scaleFromBase;			// <node octets>/scaleFromBase = value
	configKeys keyID;				// Key index for configuration file	PARAM_*
	nodeulong descID;				// Resource ID for description
} paramInfo;

typedef struct _paramInfoLcl paramInfoLcl;	// Forward reference structure def
typedef struct _byNodeDB byNodeDB;

typedef double (MN_DECL *unitConverter)(nodebool toUnit, 
									    nodeaddr theNode,
										appNodeParam parameter, 
										double fromValue,
										byNodeDB *nodeData);
// If paramSize is negative, this represents a variable sized parameter with a 
// maximum size equal to abs(paramSize).
typedef struct _paramInfoLcl  {
	paramInfo info;					// Node information
	unitConverter converter;		// Optional unit converter
} paramInfoLcl;

// Database for class of device parameters
typedef struct _byNodeClassDB {
	paramChangeFunc paramChngFunc;	// Ptr to parameter change function
} byNodeClassDB;
	
// Parameter bank holder - holds the parameter fixed information and the
// current value.
typedef struct _paramBank  {
	nodeulong nParams;				// Number of parameters in bank
	const paramInfoLcl *fixedInfoDB;// Fixed parameter info (static alloc)
	paramValue *valueDB;			// Value database (dynamic alloc)
} paramBank;

typedef struct _byNodeDiagStats
{
										// Application Network
										// -------------------
	Uint16 appNetFragPktCtr;			// Fragment counter
	Uint16 appNetBadChksumCtr;			// Checksum counter
	Uint16 appNetStrayCtr;				// Stray data counter
	Uint16 appNetOverrunCtr;			// Overrun counter
										
										// Diagnostic Network
										// -------------------
	Uint16 diagNetFragPktCtr;			// Fragment counter
	Uint16 diagNetBadChksumCtr;			// Checksum counter
	Uint16 diagNetStrayCtr;				// Stray data counter
	Uint16 diagNetOverrunCtr;			// Overrun counter

	Uint16 linkBusLow;					// Counter of link voltage low 
	void clear() {
		appNetFragPktCtr = 0;			// Fragment counter
		appNetBadChksumCtr = 0;			// Checksum counter
		appNetStrayCtr = 0;				// Stray data counter
		appNetOverrunCtr = 0;			// Overrun counter
							
		diagNetFragPktCtr = 0;			// Fragment counter
		diagNetBadChksumCtr = 0;		// Checksum counter
		diagNetStrayCtr = 0;			// Stray data counter
		diagNetOverrunCtr = 0;			// Overrun counter
		linkBusLow = 0;				
	}
	_byNodeDiagStats() {
		clear();
	}
} byNodeDiagStats;



// iscClassSetup function will allocate and initialize this structure.

typedef void (*deleteFunc)(multiaddr theMultiAddr);


typedef struct _byNodeDB  {
	devID_t theID;					// Device and Firmware type.
	nodeulong rank;					// Rank within the node class
	unsigned bankCount;				// Number of parameter banks
	paramBank *paramBankList;		// Ptr to the parameter bank(s)
	byNodeClassDB *pClassInfo;		// Class common information
	deleteFunc delFunc;				// Cleanup and delete function
	void *pNodeSpecific;			// Ptr to node specific 
	_byNodeDB() {
		paramBankList = NULL;
		pNodeSpecific = NULL;
		pClassInfo = NULL;
		delFunc = NULL;
		bankCount = 0;
		rank = 0;
	}
} byNodeDB;



 /* !end!																	*
 ****************************************************************************/
/// \endcond
// end INTERNAL_DOC
/// @}
#endif

/*========================================================================== 
	END OF FILE mnParamDefs.h
  ========================================================================== */

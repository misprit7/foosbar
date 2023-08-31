//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc-private/cpmRegs.h $
// $Date: 01/24/2017 15:30 $
// $Workfile: cpmRegs.h $
//
/// \file
/// \brief C API: Private ClearPath-SC data structures
//
// NAME
//		cpmRegs.h
//
// DESCRIPTION:
///		This header file defines the private ClearPath_SC motor node registers.
//
// CREATION DATE:
//		12/21/2011 
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2011-2017 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			   *
//******************************************************************************
#ifndef __CPMREGS_H__
#define	__CPMREGS_H__

//******************************************************************************
// NAME																		   *
// 	cpmRegs.h constants

//																			   *
//******************************************************************************
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * MONITOR PORT SETTINGS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifndef __TI_COMPILER_VERSION__             // Not needed on dsp

// These are the same as Meridian ISC for now (TODO)
typedef monTestPoints cpmMonVars;
typedef iscTuneSyncs cpmTuneSyncs;

// VB Friendly structure
/**
    \brief Monitor Port Settings structure.

    This structure controls the selection of the diagnostic test points within
    the drive.
**/
typedef struct _cpmMonState {
    double          gain;       ///<  Gain setting
    double          filterTC;   ///<  Output filter time constant (ms)
    monTestPoints   var;        ///<  Test point to display
    cpmTuneSyncs   tuneSync;   ///<  Is a bit field
} cpmMonState;

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef PACK_BYTES struct _cpmMonNodeState {
    nodeushort      var;        // 16-bit variable selector
    nodelong        gain;       // 32-bit gain
    nodeshort       filter;     // 16-bit filter TC
    nodeshort       tuneSync;   // 16-bit (should be bitfield)
} cpmMonNodeState;
#endif //not dsp

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * STIMULUS GENERATOR SETTINGS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// Stimulus mode types
#if 0
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
#endif

// Stimulus Request Packet Format used to create stimulus settings
// and to extract stimulus state.

typedef PACK_BYTES struct _cpmStimCmdPkt {
    nodeshort       Mode;       // Operational mode
    nodeshort       Period;     // Period in sample-times
    nodelong        Amplitude;  // Amplitude in mode units
    // - short form "classic" request end
    nodeshort       Slew;       // Profiled slew sample-times
    nodeshort       Dwell;      // Dwell sample-times
    // - short form "profiled" request end
    nodeshort       Status;     // Status bits (status request only)
} cpmStimCmdPkt;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
// Command sizes
#define CP_STIM_CMD_PKT_PROF_SIZE (1+4*OCTET_SIZE(nodeshort)+OCTET_SIZE(nodelong))
#define CP_STIM_CMD_PKT_SIZE (1+2*OCTET_SIZE(nodeshort)+OCTET_SIZE(nodelong))
#define CP_STIM_CMD_STATUS_SIZE 1
#define CP_STIM_RESP_STATUS_OCTETS 14

// VB/C friendly stimulus request structure
/**
    \brief Stimulus Move Generator Settings structure.

    This structure controls the internal tuning stimulus move generator.
**/
typedef struct _cpmStimState {
    double          amplitude;  ///< Amplitude mode specific units
    double          period;     ///< Stimulus period for "toggle" types
    long            slew;
    long            dwell;
    long            bits;
    stimModes       mode;       // Operational mode
} cpmStimState;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * DRIVE COMMAND NUMBERS
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
typedef enum _cpmCmds {
    SC_CMD_GET_PARAM = MN_CMD_GET_PARAM0,  // 0 Get parameter (bank 0)
    SC_CMD_SET_PARAM = MN_CMD_SET_PARAM0,  // 1 Set parameter (bank 0)
    SC_CMD_GET_PARAM1 = MN_CMD_GET_PARAM1, // 2 Get parameter (bank 1)
    SC_CMD_SET_PARAM1 = MN_CMD_SET_PARAM1, // 3 Set parameter (bank 1)
    SC_CMD_NODE_STOP  = MN_CMD_NODE_STOP,  // 4 E-stop (Node Stop, Stim Cancel)
    SC_CMD_NET_ACCESS = MN_CMD_NET_ACCESS, // 5 Get net access level
    SC_CMD_USER_ID = MN_CMD_USER_ID,       // 6 Get/Set the user ID
    SC_CMD_CHK_BAUD_RATE = MN_CMD_CHK_BAUD_RATE, // 7 Get OK for proposed baud rate
    SC_CMD_ALERT_CLR = MN_CMD_ALERT_CLR,   // 8 Clear non-serious Alert reg bits
    SC_CMD_ALERT_LOG = MN_CMD_ALERT_LOG,   // 9 Read, clear, mark epoch of Alert log

    // Start of node specific commands
    SC_CMD_MOTOR_FILE=MN_CMD_COMMON_END,   // 16 Motor file name
    SC_CMD_ADD_POSN=19,                    // 19 Adjust servo position
    SC_CMD_ADD_POSN_LD,                    // 20 Adjust open loop load encoder
    SC_CMD_SYNC_POSN,                      // 21 Synchronize position
    SC_CMD_GET_SET_MONITOR,                // 22 Monitor port setting
    SC_CMD_GET_SET_STIMULUS,               // 23 Stimulus settings
    SC_CMD_RE_VECTOR,                      // 24 Reset the vector search
    SC_CMD_DATA_ACQ,                       // 25 Data acquisition/scope feature
    SC_CMD_TEST_GEN,                       // 26 Test Generator

    // Short form position motion initiators
    SC_CMD_MOVE_POSN_ABS=64+MG_MOVE_STYLE_NORM_ABS,// 64 Mv posn: abs
    SC_CMD_MOVE_POSN_REL=64+MG_MOVE_STYLE_NORM,    // 65 Mv posn: relative
    SC_CMD_MOVE_POSN_ABS_TRIG=64+MG_MOVE_STYLE_NORM_TRIG_ABS,// 66 Mv posn: abs
    SC_CMD_MOVE_POSN_REL_TRIG=64+MG_MOVE_STYLE_NORM_TRIG,  // 67 Mv posn: relative
    // Short form velocity motion initiator
    SC_CMD_MOVE_VEL=68,                    // 68 Move velocity
    SC_CMD_MOVE_VEL_TRIG,                  // 69 Move velocity

    // Full feature motion initiator
    SC_CMD_MOVE_POSN_EX,                   // 70 Move Posn: expanded
    SC_CMD_MOVE_VEL_EX                     // 71 Move Velocity: expanded
} cpmCmds;


#ifndef __TI_COMPILER_VERSION__             // Skip DSP C problematic defs
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// POSITIONAL MOVE COMMANDS AND DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Expanded Profile move command format
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef PACK_BYTES struct _cpmMoveProfiledExPacket {
    int style                 : 8;  // Format of move (mgPosnStyle)
    int value                 :32;  // Distance of the motion
} cpmMovePosnExPacket;
#ifdef _MSC_VER
#pragma pack(pop)                   // Align to bytes boundaries
#endif

#endif // not dsp

//                                                                            *
//*****************************************************************************

// Constants for RASgroup in option register
#if 0 // TODO this model does not need this, causes compile issues
enum RASOPT_LISTS {
    RASOPT_AUTO = 0,                         // Automatically selected RAS
    RASOPT_MANUAL_SK = 1,                   // Short Manual List
    RASOPT_MANUAL_HP = 2,                    // Full Manual List
    RASOPT_AUTO_FULL = 3                    // Auto RAS, but allow full list
};

// TODO: We will use isc list for here as these are the same DS 10/27/2015
// RAS Constants for common settings
enum CP_RAS_CODES {
    CP_RAS_OFF = 0,                    // 0  OFF
    CP_RAS_3MS,                        // 1  3 ms
    CP_RAS_5MS,                        // 2  5 ms
    CP_RAS_9MS,                        // 3  9 ms
    CP_RAS_15MS,                       // 4  15 ms
    CP_RAS_24MS,                       // 5  24 ms
    CP_RAS_44MS                        // 6  44 ms
};
#endif

//*****************************************************************************
/**
    \brief Encoder Quality Register.

	This is the register used to quantify the encoder alignment quality when
	the encoder is setup for the alignment mode and the motor is spun.
**/
typedef union _cpmEncQualReg {
	Uint32	bits[6];						// Broad view
	struct _Fld {
		unsigned aEvents			: 4;	// A events
		int							: 8;	//
		int 	valid				: 1;	// Record is valid
		int							: 3;
		int		indexCount			: 4;	// Directional index count
		unsigned iEvents			: 4;	// Indx Events
		// Raw data (counts of CPU clocks)
		int16	indexRise0;					// Rising index event from CAP1
		int16	indexRise1;					// Rising index event from CAP3
		int16	indexFall0;					// Falling index event from CAP2
		int16	indexFall1;					// Falling index event from CAP4
		int16	aRise0;						// Rising A event from CAP1
		int16	aRise1;						// Rising A event from CAP3
		int16	aFall0;						// Falling A event from CAP2
		int16	aFall1;						// Falling A event from CAP4
		int16   mechAngle;					// (mech angle) 2.14
		int16	Pad;						// Extra padding
	} Fld;
	void reset() {
		bits[0] &= ~0xFFFFL;
	}
} cpmEncQualReg;
//                                                                            *
//*****************************************************************************


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Number of bytes in each of these commands
#define SC_CMD_MOVE_PROF_LEN       5
#define SC_CMD_MOVE_SPLINE_LEN     10
#define SC_CMD_MOVE_PROF_EX_LEN    5
#define SC_CMD_MOVE_FLUSH_LEN      1
#define SC_CMD_MOVE_VEL_EX_LEN     8
#define SC_CMD_GET_PLA_LEN         2
#define SC_CMD_SET_PLA_BASE_LEN        2       // + fuses
#define SC_CMD_ADD_POSN_LEN            5
#define SC_CMD_SYNC_POSN_LEN       1
#define SC_CMD_GET_ACQUISITION_LEN 1
#define SC_CMD_ACQUIRE_CONTROL_LEN 5

// Velocity scaling factor
#define SC_VEL_MOVE_SCALE (1LL<<MV_VEL_Q)  // Velocity scaling factor
/// \endcond

#ifndef __TI_COMPILER_VERSION__             // Not needed on dsp
//******************************************************************************
// NAME																		   *
// 	cpmRegs.h function prototypes
//
// DESCRIPTION
/**
	\brief Internal prototypes.

	Identifies the hardware configuration of the motor/drive.
**/
//
// Initialize the parameter local copy of the parameter table
cnErrCode cpmInitialize(
			multiaddr theMultiAddr);

cnErrCode cpmInitializeEx(
			multiaddr theMultiAddr,
			nodebool warmInitialize);
//																			 *
//****************************************************************************
#endif //__TI_COMPILER_VERSION__
#endif
//============================================================================
//	END OF FILE cpmRegs.h
//============================================================================

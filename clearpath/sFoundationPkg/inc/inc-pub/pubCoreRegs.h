//***************************************************************************
// DESCRIPTION:
/**
	\file
	\brief Universal Register Definitions.

	These registers are typically implemented as unions. The members of the
	union include:
	-a bit-wide interface named \a bits
	-one or more node specific structs to access node specific fields. For
	ClearPath-SC this member is named "cpm".
	-The Meridian-ISC member is named either "isc" or "fld" depending on its
	original legacy name.
**/
//
// NAME
//      pubCoreRegs.h
//
// CREATION DATE:
//      06/19/2008 13:46:39
//
// COPYRIGHT NOTICE:
//      (C)Copyright 2008-2016 Teknic, Inc.  All rights reserved.
//
//      This copyright notice must be reproduced in any copy, modification,
//      or portion thereof merged into another program. A copy of the
//      copyright notice must be included in the object library of a user
//      program.
//                                                                           *
//****************************************************************************
#ifndef __PUBCOREREGS__H__
#define __PUBCOREREGS__H__


//****************************************************************************
// NAME                                                                      *
//  pubCoreRegs.h headers
//
    #include "tekTypes.h"
	#include "pubRegFldCommon.h"
	#include "pubIscRegs.h"
#if defined(IMPL_NODE_CP)
	#include "pubClearPathRegs.h"
#endif
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC) || defined(_CLEARPATH)
	#include "pubCpmRegs.h"
#endif
//                                                                           *
//****************************************************************************


//****************************************************************************
// NAME                                                                      *
//  pubCoreRegs.h constants
//
													/** \cond INTERNAL_DOC **/
#define ATTN_MASK_OCTETS 4
													/** \endcond **/
//                                                                           *
//****************************************************************************


//****************************************************************************
// Insure structs are byte packed on target
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
//****************************************************************************



//****************************************************************************
// NAME                                                                      *
//  optionReg union
//
// DESCRIPTION
																/** \cond ExpertDoc **/
/**
    \brief Universal Factory setup and option control register.

    This register describes the various model-specific information.
**/
//
typedef union PACK_BYTES _optionReg {
	/**
	 *  Hardware Platforms
	 */
	enum _hwPlatforms {
		HW_CLEARPATH,
		HW_AC_GREEN
	};
	/**
		Bit-wide access for options
	**/
    Uint32 bits; 
    // - - - - - - - - - - - - - - - - - - - -
    // COMMON PRODUCT OPTION DEFINITIONS
    // - - - - - - - - - - - - - - - - - - - -
	/**
		\brief Universal Option Field Definitions.
	**/
    struct _optionsCommonFlds {
        /*                   -----------  ----------------------------------- *
         *                         bit #  description                         *
         *                   -----------  ----------------------------------- */
		/// Factory setup modes
        int Setups          :4; // 0-3:   Factory setup modes
		/// Product options installed
        unsigned Product    :2; // 4-5:   Product options
		/// Advanced feature supported
		unsigned Advanced	:1; // 6 :	  Advanced features available
														/// INTERNAL_DOC
		unsigned VectorLock	:1; // 7 :    Lock Vector Mode
														///
		/// Option board(s) installed
        unsigned OptionBrd  :2; // 8-9:   Installed option board configuration
        unsigned IgnoreIndx :1; // 10:	  Prevent Ref Offset on index
														/// INTERNAL_DOC
        int					:1; // 11 :
														///
		/// Hardware platform code
        unsigned HwPlatform :4; // 12-15: Hardware platform
														/// INTERNAL_DOC
        int					:9; // 16-24
														///
		/// Special debugging flags
        unsigned Debug      :3; // 25-27: Debug flags
    };
    struct _drvCommonFlds {
        /*                   -----------  ----------------------------------- *
         *                         bit #  description                         *
         *                   -----------  ----------------------------------- */
        int VoltMode        : 1; // 0:  Volt mode setup
        int RuntimeCal      : 1; // 1:  1=Enable run time calibration
        int ClockCal        : 1; // 2:  1=clock calibration
        int DirCwPositive   : 1; // 3:  1=CW direction is positive motion

        unsigned Product    : 3; // 4:  3-bit product number
                                 // 5:  ""
                                 // 6:	""
        /**
    		\brief Vector Lock Diagnostic Enable

            If the motor is vector oriented, such as a brushless motor
            this will force the vector to freeze.
        **/
        unsigned VectorLock : 1; // 7   Lock Vector Mode

        /**
    		\brief Missing index tolerance

    		When index checking is set, this field is the number of
    		index pulses that can be missing before an error is flagged.
    		0 = any missed index is an error
        **/
        unsigned IndexTolerance	: 2;    // 8-9
        /**
    		\brief Inhibit motor index processing for vector refinement.

            If set, the encoder's index is ignored for vector processing. This
    		is useful for development purposes as well as factory motor setup
    		when combined with #CpHwConfigReg::SensorlessMode mode.
        **/
    	unsigned IgnoreIndx	 : 1;//  10 Prevent Ref Offset on index
    	unsigned NoVectUpdate: 1;//  11 do not apply the angle from the enhanced sensorless algorithm
		/// Hardware platform code
    	unsigned HwPlatform :4; // 12-15: Hardware platform (enum _hwPlatforms)
   };
	/**
		Access to common fields among nodes.
	**/
	struct _optionsCommonFlds Common;
	/**
		Access to common fields among drives.
	**/
	struct _drvCommonFlds DrvCommon;
    // - - - - - - - - - - - - - - - - - - - -
    // DRIVE PRODUCT OPTION DEFINITIONS
    // - - - - - - - - - - - - - - - - - - - -
														/// INTERNAL_DOC
    /// Access to Meridian ISC option fields
	iscOptionsFlds DrvFld;
#if defined(IMPL_NODE_CP)
    // - - - - - - - - - - - - - - - - - - - -
    // CLEARPATH PRODUCT OPTION DEFINITIONS
    // - - - - - - - - - - - - - - - - - - - -
	/// Access to common fields among ClearPath motors
    clearPathOptionsFlds ClrPathFld;            
#endif
														///
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC)
	/// ClearPath-SC option register view
	cpmOptionsFlds cpm;
#endif
	#ifdef __cplusplus
														/// INTERNAL_DOC
	/// Clear bits on construction
    _optionReg() {                          
        bits = 0;
    }
		#ifdef __TI_COMPILER_VERSION__
		// Returns TRUE if the option register has us in debug mode
		tBOOL InDebugMode() {
			return((Common.Debug > 0) && (Common.Debug < 7));
		}
		#endif
    #endif
														///
} optionReg;
															/** \endcond **/
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME																		  *
//  alertReg union
//
// DESCRIPTION
// Alert Register Size in octets: 96 bits = 3 32-bit entities = 12 octets
#define ALERTREG_SIZEOCTETS 12
/**
    \brief Universal Alert and Warning Register Type

    The alert register defines the shutdown state of the node. When a node enters a shutdown
    or potentially non-operable condition, the node will set bits in this
    register to tell the application what went wrong. Alerts range from serious to minor. 

	- Hardware/Software failures during power on self testing and
	internal consistency checks that occur during operation. These include items such as as:
		- Sensor failures
		- Component failures
		- Sensors drifting out of their nominal ranges
		- Non-volatile configuration errors
	- Self preservation shutdowns where the node shuts down to protect itself from being damaged.
		- Over motor bus voltage
		- Shorted circuits
		- Excessive power delivery to motor
		- Jams
	- Application errors such as:
		- Parameters out-of-range for requested operations such as commanding a node past it's soft limits

    Most shutdowns can be cleared via the sFnd::IStatus::AlertsClear function to allow
    continued node operations, however fatal errors cannot be cleared by this
    mechanism and require a power-cycle or node restart to clear them.

	\note See the specific bit for potential recovery actions to allow node operations to continue.
	If the Status Register's [HwFailure](@ref _cpmStatusRegFlds::HwFailure) bit is asserted, this is a
	hardware failure and recovery is probably not possible.

	
	\see _cpmAlertFlds For the complete list of ClearPath-SC alert Fields.
	\see sFnd::IStatus::Alerts for access to this register.
 **/
typedef union MN_EXPORT PACK_BYTES _alertReg {
    /// "Parallel" view of register as three unsigned 32-bit integers.
    Uint32 bits[3];
													/** \cond ISC_DOC **/
    /** 
		\brief Common fields for all Meridian products. 
		
		Usually you would use the more specific items such as the drvFld
		to look for specific shutdowns. This member is included within the 
		device specific members.
    **/
    commonAlertFlds Common;
													/** \endcond **/
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC)
	/**
		\brief Access to ClearPath-SC specific alert fields.

	**/
	cpmAlertFlds cpm;
#endif
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
													/** \cond ISC_DOC **/
    /**
		\brief Field access to the Drive Specific items.
	**/
	iscAlertFlds DrvFld;
													/** \endcond **/
    // If we have C++, include these useful member functions.
													/** \cond INTERNAL_DOC **/
    #ifdef __cplusplus
		/// Clear values back to zeroes
		void clear() {
			bits[0] = 0;
			bits[1] = 0;
			bits[2] = 0;
		}
													/** \endcond  **/
		/// Test if all bits are non-zero
		/**
		Returns true if all fields in the register are zero.

		\CODE_SAMPLE_HDR

		// get an alertReg reference, and check directly for alerts
		if (myNode.Status.Alerts.Value().isClear())
		{
		printf("Node has no alerts!");
		}

		\endcode
		**/
		bool isClear() {
			return (bits[0] | bits[1] | bits[2]) == 0;
		}
		/// Test if any bits are non-zero
		/**
		Returns true if any fields in the register is asserted.

		\CODE_SAMPLE_HDR

		// get an alertReg reference, and check directly for alerts
		if (myNode.Status.Alerts.Value().isInAlert())
		{
		printf("Node has alerts!");
		}

		\endcode
		**/
		bool isInAlert() {
			return (bits[0] | bits[1] | bits[2]) != 0;
		}
#ifndef __TI_COMPILER_VERSION__
		/**
		\brief Update a string with the set fields in the Alert Register.

		\param[in,out] buffer  Point to string buffer to update.
		\param[in] size  The size of the buffer in characters including the null
		character.
		\return the parameter \a buffer.

		\CODE_SAMPLE_HDR
		char myBuffer[500];
		cout << myNode.Status.Alert.Value.StateStr(myBuffer, sizeof(myBuffer));
		\endcode
		**/
		char * StateStr(char *buffer, size_t size) {
			return (cpm.StateStr(buffer, size));
		}
#endif
													/** \cond INTERNAL_DOC **/
		/// Number of Uint32 bits in structure
		enum { N_BITS = 3};
        #ifdef __TI_COMPILER_VERSION__
            bool InShutdown();
            bool InLockdown();
            bool InPwrEvent();
            bool InPwrEventShutdown();
            bool InBlockMotionShutdown();
            bool InBlockMotionSelfClearShutdown();
            bool InNodeNeedsRepairShutdown();
            bool InSeriousShutdown();
            bool InKillOutputsShutdown();
            bool InAlert();
        #endif
        // Constructor, create with bits cleared
        _alertReg() {                   // Clear bits on construction
            STATIC_ASSERT(ALERTREG_SIZEOCTETS==12);
			clear();
        }
    #endif
													/** \endcond **/
} alertReg;

													/** \cond INTERNAL_DOC **/
// These bit numbers must match the locations in the alertReg union
// They are used by coding that requires either a mask or a bit number.
/*
    Bit Number index for the Alert Register

    These number can be used to construct masks.
*/
typedef enum _drvAlertBits {
	DRV_ALERT_FW_BAD_ACQ_STATE_BIT              = 32,
	DRV_ALERT_FW_MOVEGEN_MISC_BIT               = 33,
	DRV_ALERT_FW_MOVEGEN_OFF_TARGET_BIT         = 34,
	DRV_ALERT_FW_MOVEGEN_RANGE_BIT              = 35,
	DRV_ALERT_FW_BAD_RAS_REQUEST_BIT            = 36,
	DRV_ALERT_FW_MOVE_BUF_UNDERRUN_BIT          = 37,
	DRV_ALERT_FW_BAD_RAS_VELOCITY_BIT           = 38,
	DRV_ALERT_FW_MOVE_SPEC_ALTERED_BIT          = 39,
	DRV_ALERT_HW_PHASE_SENSOR_BIT               = 40,   // HW Shdn
	DRV_ALERT_FW_MOVEGEN_LIMSW_BIT              = 41,
	DRV_ALERT_FW_MOVEGEN_SOFTLIM_BIT            = 42,
	DRV_ALERT_FW_MOVE_BUF_OVERRUN_LOCKDN_BIT    = 43,
	DRV_ALERT_IO_5V_SUPPLY_OVERLOAD_BIT         = 44,   // mtn Shdn
	DRV_ALERT_IO_BRAKE_OVERLOAD_BIT             = 45,
	DRV_ALERT_IO_GPO0_OVERLOAD_BIT              = 46,
	DRV_ALERT_IO_GPO1_OVERLOAD_BIT              = 47,
	DRV_ALERT_IO_GPO_DETECTOR_BIT               = 48,
	DRV_ALERT_IO_GPO_OVERLOAD_BIT               = 49,
	DRV_ALERT_IO_INVALID_BIT       		        = 50,
	DRV_ALERT_IO_INVALID_LOCKDN_BIT     		= 51,
	DRV_ALERT_AC_POWER_LOST_BIT					= 52,
	DRV_ALERT_IEX_LINK_GLITCH_BIT               = 53,
	DRV_ALERT_IEX_LINK_MULTI_GLITCH_BIT         = 54,
	DRV_ALERT_MTR_UNDERTEMP_BIT			        = 55,
	DRV_ALERT_MTR_COMM_BAD_SEQ_BIT              = 56,   // mtn Shdn
	DRV_ALERT_MTR_COMM_BAD_STATE_BIT            = 57,   // mtn Shdn
	DRV_ALERT_MTR_VECTOR_ERR_BIT                = 58,   // mtn Shdn
	DRV_ALERT_MTR_VECTOR_CRIT_ERR_BIT           = 59,   // mtn Shdn
	DRV_ALERT_MTR_ENC_GLITCH_BIT                = 60,   // mtn Shdn
	DRV_ALERT_MTR_ENC_OVERSPEED_BIT             = 61,   // mtn Shdn
	DRV_ALERT_LD_ENC_GLITCH_BIT                 = 62,   // mtn Shdn
	DRV_ALERT_LD_ENC_OVERSPEED_BIT              = 63,   // mtn Shdn
	DRV_ALERT_MTR_PHASE_OVERLOAD_BIT            = 64,   // mtn Shdn
	DRV_ALERT_MTR_THERMAL_OVERLOAD_BIT          = 65,   // mtn Shdn
	DRV_ALERT_MTR_BAD_SETUP_PARAMS_BIT          = 66,   // sw Shdn
	DRV_ALERT_SVO_HARDSTOP_RELEASE_BIT          = 67,   // mtn Shdn
	DRV_ALERT_SVO_TRACKING_BIT                  = 68,   // mtn Shdn
	DRV_ALERT_SVO_TRACKING_CRIT_BIT             = 69,   // mtn Shdn
	DRV_ALERT_SVO_RMS_OVERLOAD_BIT              = 70,   // mtn Shdn
	DRV_ALERT_SVO_RMS_CRIT_OVERLOAD_BIT         = 71,   // mtn Shdn
	DRV_ALERT_MTR_COUPLING_ERROR_BIT            = 72,   // mtn Shdn
	DRV_ALERT_SVO_STEP_NOISE_BIT                = 73,   // mtn Shdn
	DRV_ALERT_SVO_VOLT_SAT_BIT                  = 74,   // mtn Shdn
	DRV_ALERT_SVO_TRQ_SAT_BIT                   = 75,   // mtn Shdn
	DRV_ALERT_NOCOMM_DIST_ERROR_BIT             = 76,   // mtn Shdn
	DRV_ALERT_NOCOMM_DIRECTION_ERROR_BIT        = 77,   // mtn Shdn
	DRV_ALERT_NOCOMM_FAIL_BIT					= 78,	
	DRV_ALERT_MTR_ENC_INDEX_MISSING_WARN_BIT	= 79,
	DRV_ALERT_TEMP_AMBIENT_BIT                  = 80,   // mtn Shdn (drive hot)
	DRV_ALERT_TEMP_HEATSINK_BIT                 = 81,   // mtn Shdn (stator hot)
	DRV_ALERT_BUS_OVERCURRENT_BIT               = 82,   // mtn Shdn
	DRV_ALERT_BUS_OVERVOLTAGE_BIT               = 83,   // mtn Shdn
	DRV_ALERT_BUS_VOLTAGE_LOW_BIT               = 84,   // mtn Shdn
	DRV_ALERT_BUS_RMS_OVERLOAD_BIT              = 85,   // mtn Shdn
	DRV_ALERT_FAN_SPEED_LOW_BIT                 = 86,   // mtn Shdn
	DRV_ALERT_MTR_ENC_INDEX_MISSING_BIT         = 87,
	DRV_ALERT_NOCOMM_HW_FAULT_BIT				= 88,
	DRV_ALERT_NOCOMM_BUS_LOW_BIT				= 89,
	DRV_ALERT_NOCOMM_SW_FAULT_BIT				= 90,
	DRV_ALERT_FW_MOVEGEN_LIMSW_LOCKDN_BIT       = 91,
	DRV_ALERT_FW_MOVEGEN_SOFTLIM_LOCKDN_BIT     = 92,
	DRV_ALERT_MTR_ENC_INDEX_MISPLACED_BIT		= 93,
} drvAlertBits;

// These bit numbers must match the locations in the alertReg union
// They are used by coding that requires either a mask or a bit number.
/*
    Bit Number index for the Alert Register

    These number can be used to construct masks.
*/
typedef enum _cpmAlertBits {
    CPM_ALERT_FW_BAD_RAS_REQUEST_BIT            = DRV_ALERT_FW_BAD_RAS_REQUEST_BIT,     // 36
    CPM_ALERT_FW_MOVE_BUF_UNDERRUN_BIT          = DRV_ALERT_FW_MOVE_BUF_UNDERRUN_BIT,   // 37
    CPM_ALERT_FW_BAD_RAS_VELOCITY_BIT           = DRV_ALERT_FW_BAD_RAS_VELOCITY_BIT,    // 38
    CPM_ALERT_FW_MOVE_SPEC_ALTERED_BIT          = DRV_ALERT_FW_MOVE_SPEC_ALTERED_BIT,   // 39
    CPM_ALERT_HW_PHASE_SENSOR_BIT               = DRV_ALERT_HW_PHASE_SENSOR_BIT,        // 40 HW Shdn
    CPM_ALERT_FW_MOVEGEN_LIMSW_BIT              = DRV_ALERT_FW_MOVEGEN_LIMSW_BIT,       // 41
    CPM_ALERT_FW_MOVEGEN_SOFTLIM_BIT            = DRV_ALERT_FW_MOVEGEN_SOFTLIM_BIT,     // 42
    CPM_ALERT_FW_MOVE_BUF_OVERRUN_LOCKDN_BIT    = DRV_ALERT_FW_MOVE_BUF_OVERRUN_LOCKDN_BIT,// 43
    CPM_ALERT_IO_INVALID_BIT                    = DRV_ALERT_IO_INVALID_BIT,             // 50
    CPM_ALERT_IO_INVALID_LOCKDN_BIT             = DRV_ALERT_IO_INVALID_LOCKDN_BIT,      // 51
    CPM_ALERT_AC_POWER_LOST_BIT                 = DRV_ALERT_AC_POWER_LOST_BIT,          // 52
    CPM_ALERT_AC_PHASE_WIRING_ERROR_BIT         = 53,                                   // 53
    CPM_ALERT_MTR_UNDERTEMP_BIT                 = DRV_ALERT_MTR_UNDERTEMP_BIT,          // 55
    CPM_ALERT_MTR_VECTOR_ERR_BIT                = DRV_ALERT_MTR_VECTOR_ERR_BIT,         // 58 mtn Shdn
    CPM_ALERT_MTR_ENC_GLITCH_BIT                = DRV_ALERT_MTR_ENC_GLITCH_BIT,         // 60 mtn Shdn
    CPM_ALERT_MTR_ENC_OVERSPEED_BIT             = DRV_ALERT_MTR_ENC_OVERSPEED_BIT,      // 61 mtn Shdn
    CPM_ALERT_MTR_PHASE_OVERLOAD_BIT            = DRV_ALERT_MTR_PHASE_OVERLOAD_BIT,     // 64 mtn Shdn
    CPM_ALERT_MTR_BAD_SETUP_PARAMS_BIT          = DRV_ALERT_MTR_BAD_SETUP_PARAMS_BIT,   // 66 sw Shdn
    CPM_ALERT_SVO_HARDSTOP_RELEASE_BIT          = DRV_ALERT_SVO_HARDSTOP_RELEASE_BIT,   // 67 mtn Shdn
    CPM_ALERT_SVO_TRACKING_CRIT_BIT             = DRV_ALERT_SVO_TRACKING_CRIT_BIT,      // 69 mtn Shdn
    CPM_ALERT_SVO_RMS_OVERLOAD_BIT              = DRV_ALERT_SVO_RMS_OVERLOAD_BIT,       // 70 mtn Shdn
    CPM_ALERT_SVO_RMS_CRIT_OVERLOAD_BIT         = DRV_ALERT_SVO_RMS_CRIT_OVERLOAD_BIT,  // 71 mtn Shdn
    CPM_ALERT_SVO_STEP_NOISE_BIT                = DRV_ALERT_SVO_STEP_NOISE_BIT,         // 73 mtn Shdn
    CPM_ALERT_SVO_VOLT_SAT_BIT                  = DRV_ALERT_SVO_VOLT_SAT_BIT,           // 74 mtn Shdn
    CPM_ALERT_SVO_TRQ_SAT_BIT                   = DRV_ALERT_SVO_TRQ_SAT_BIT,            // 75 mtn Shdn
    CPM_ALERT_NOCOMM_DIST_ERROR_BIT             = DRV_ALERT_NOCOMM_DIST_ERROR_BIT,      // 76 mtn Shdn
    CPM_ALERT_NOCOMM_DIRECTION_ERROR_BIT        = DRV_ALERT_NOCOMM_DIRECTION_ERROR_BIT, // 77 mtn Shdn
    CPM_ALERT_NOCOMM_FAIL_BIT                   = DRV_ALERT_NOCOMM_FAIL_BIT,            // 78
    CPM_ALERT_MTR_ENC_INDEX_MISSING_WARN_BIT    = DRV_ALERT_MTR_ENC_INDEX_MISSING_WARN_BIT, // 79
    CPM_ALERT_TEMP_DRIVE_BIT                    = DRV_ALERT_TEMP_AMBIENT_BIT,           // 80 mtn Shdn
    CPM_ALERT_TEMP_STATOR_BIT                   = DRV_ALERT_TEMP_HEATSINK_BIT,          // 81 mtn Shdn
    CPM_ALERT_BUS_OVERCURRENT_BIT               = DRV_ALERT_BUS_OVERCURRENT_BIT,        // 82 mtn Shdn
    CPM_ALERT_BUS_OVERVOLTAGE_BIT               = DRV_ALERT_BUS_OVERVOLTAGE_BIT,        // 83 mtn Shdn
    CPM_ALERT_BUS_VOLTAGE_LOW_BIT               = DRV_ALERT_BUS_VOLTAGE_LOW_BIT,        // 84 mtn Shdn
    CPM_ALERT_BUS_RMS_OVERLOAD_BIT              = DRV_ALERT_BUS_RMS_OVERLOAD_BIT,       // 85 mtn Shdn
    CPM_ALERT_BUS_PWR_MISMATCH_BIT              = 86,                                   // 86 Power issue (v1.6)
    CPM_ALERT_MTR_ENC_INDEX_MISSING_BIT         = DRV_ALERT_MTR_ENC_INDEX_MISSING_BIT,  // 87
    CPM_ALERT_BUS_VOLTAGE_UNDER_OPERATING_V_BIT = 88,                                   // 88 Power issue (v1.6)
    CPM_ALERT_FW_MOVEGEN_LIMSW_LOCKDN_BIT       = DRV_ALERT_FW_MOVEGEN_LIMSW_LOCKDN_BIT,// 91
    CPM_ALERT_FW_MOVEGEN_SOFTLIM_LOCKDN_BIT     = DRV_ALERT_FW_MOVEGEN_SOFTLIM_LOCKDN_BIT, // 92
    CPM_ALERT_MTR_ENC_INDEX_MISPLACED_BIT       = DRV_ALERT_MTR_ENC_INDEX_MISPLACED_BIT,// 93
    CPM_ALERT_STEPS_DURING_POSN_RECOVERY_BIT    = 94,                                   // 94
} cpmAlertBits;

//                                                                            *
//*****************************************************************************
													/** \endcond **/


													/** \cond INTERNAL_DOC **/
//*****************************************************************************
//  NAME                                                                      *
//      alertLogEntry structure and definitions
//
// CREATION DATE:
//      02/11/2010 17:40:47
//
//  DESCRIPTION:
/**
    \brief Alert Log Entry Register.

    The \e reason code will describe the format of the \e tstamp field.
    The following table describes the formats.
        ------------------------------------------------------------------
        |     code            |  tstamp meaning                          |
        ------------------------------------------------------------------
        ALERT_LOG_EPOCH_CODE    seconds since 1/1/2010 00:00:00Z (UTC)
        All others              tenths of seconds of unit up-time.

    A log file processor will read the oldest entry and if an
    ALERT_LOG_EPOCH_CODE is detected, the following timestamps may use
    this entry to offset the timestamp to represent a time in the local
    locale.
**/
typedef struct _alertLogEntry {
    Uint32 Timestamp;               ///< Time stamp code
	union _alertReasons {
    	commonAlertBits common;
		drvAlertBits isc;
		cpmAlertBits cpm;
		Uint32 bits;
		// Common translators
		void operator=(const commonAlertBits updVal) { common = updVal; }
		void operator=(const drvAlertBits updVal) { isc = updVal; }
		void operator=(const cpmAlertBits updVal) { cpm = updVal; }
		bool operator==(const commonAlertBits lVal) { return common == lVal; }
		bool operator!=(const commonAlertBits lVal) { return common != lVal; }
		// Common constructors
		_alertReasons(const commonAlertBits initVal) { common=initVal; }
		_alertReasons(const drvAlertBits initVal) { isc=initVal; }
		_alertReasons(const cpmAlertBits initVal) { cpm=initVal; }
		_alertReasons() { bits = 0; }
	} Reason;            ///< Reason and/or description
} alertLogEntry;
// - - - - - - - - - - - - - - - - -
// Alert command arguments
// - - - - - - - - - - - - - - - - -
// Clear out all entries
#define ALERT_ARG_CLEAR         ALERT_LOG_FREE_CODE
// Update/create the wall clock time epoch value
#define ALERT_ARG_GET_OLDEST    0
//                                                                            *
//*****************************************************************************
													/** \endcond **/


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// STATUS REGISTER UNIONS AS "REGISTERS" WITH BIT-WISE AND FLD ACCESS
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

 													/** \cond ISC_DOC **/
//*****************************************************************************
// NAME                                                                       *
//  lowAttnReg union
//
// DESCRIPTION
/**
    \brief <i>Lower Attentionable</i> portion of the <i>Status Register</i>.

    This portion of the Status Register (#mnStatusReg) can be used to generate
    attention packets.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef union PACK_BYTES _lowAttnReg {
	/**
		Bit-wise parallel access to the lower 16-bits of the status register.
	**/
	Uint16 bits;
	/**
		Common fields in the lower 16-bits of the status register
	**/
	lowAttnCommonFlds Common;
	/**
		Meridian ISC view of the lower 16-bits of the status register.
	**/
	iscLowAttnFlds DrvFld;
} lowAttnReg;
//                                                                            *
//*****************************************************************************
													/** \endcond **/


 													/** \cond ISC_DOC **/
//*****************************************************************************
// NAME                                                                       *
//  nonAttnReg union
//
// DESCRIPTION

/**
    \brief Non-Attentional portion of the <i>Status Register</i>.

    The most significant sixteen bit portion of the <i>Status Register</i>
    (#mnStatusReg).
    This portion of the register does not generate any attention packets.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef union PACK_BYTES _nonAttnReg {
    /// Register view as an integer
    Uint16 bits;
    /**
		Common non-attentionable fields
	**/
    nonAttnCommonFlds Common;
	/**
		Meridian ISC non-attentionable fields
	**/
    iscNonAttnFlds DrvFld;
	// 12/21/2015 Delete me as this is handled by the new status register def
	//ClearPathNonAttnReg ClrPathFld;
} nonAttnReg;                       ///< Field access for the non-attention
                                    ///< part of Status Register.
//                                                                            *
//*****************************************************************************
													/** \endcond **/


 													/** \cond ISC_DOC **/
//*****************************************************************************
// NAME                                                                       *
//  plaInReg union
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
typedef union PACK_BYTES _plaInReg {
    /// View of register as an integer
    Uint16 bits;
	/// Common fields across all devices in this register part
	plaInCommonFlds Common;
	/**
		Meridian ISC view of the middle 16-bits of the status register. These
		bits feed the PLA feature.
	**/
	iscPlaInFlds DrvFld;
} plaInReg;
//                                                                            *
//*****************************************************************************
													/** \endcond **/



 													/** \cond ISC_DOC **/
//*****************************************************************************
//  NAME                                                                      *
//      plaOutReg union
//
// CREATION DATE:
//      02/17/2010 16:08:32
//
//  DESCRIPTION:
/**
	\cond ISC_DOC
    \brief PLA/Output Register Types.

    This is a container that defines the outputs from the PLA structure
    as well as the internal nexus to electrical outputs and internal
    "feature" strobes.

    The #iscGetUserOutputReg and #iscSetUserOutputReg functions are typically
    used to control this register. Autonomous control of this register can be
    done via the PLA Feature. 

	\see \ref ISCplaPage

    To check on the operations of the Output Register there are a number of
    accumulating registers based on the sample-by-sample basis. These read-only
    functions can access what happened to the output register since the last
    request
        - #iscGetOutputReg gets a copy of the current state
        - #iscGetOutputRiseReg gets and clears the bits that have risen since
        the last request.
        - #iscGetOutputFallReg gets and clears the bits that have risen since
        the last request.

    \warning
    For multi-threaded applications care must be taken to provide
    serialization for the read-modify-write actions as well as accumulating
    the Rise and Fall transition registers for use by your application.

    The following diagram shows the layout of this register.
        \image html "ISC_Out_Reg.png"
        \image latex "ISC_Out_Reg.eps"
**/
typedef union PACK_BYTES _plaOutReg {
    /// Integer view of the output register
    Uint16 bits;
    // Common outputs for all nodes
    struct _plaOutCommonFlds {
        int na0_4       : 5;
        /**
			\brief Generate node stop on asserting edge.

            The asserting edge (0->1) of this bit will initiate the node stop
            programmed in the Stop Type register (#ISC_P_STOP_TYPE).
        **/
        int NodeStop    : 1;    // Issue NodeStop
    } Common;                   ///< Output fields common to all nodes.
    /// Meridian ISC Specific Outputs
	iscOutFlds DrvFld;
#if defined(IMPL_NODE_CP)
	/// ClearPath
	clearPathOutFlds ClearPathFld;
#endif
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC)
	/// ClearPath-SC specific definitions
	cpmOutFlds cpm;
#endif
	#ifdef __cplusplus
    /// Zero out bits.
    void clear() {
        bits = 0;
    };
    #endif
} plaOutReg;
//                                                                            *
//*****************************************************************************
														/** \endcond **/


														/** \cond ISC_DOC **/
//*****************************************************************************
//  NAME                                                                      *
//      plaFuses structure
//
// CREATION DATE:
//      10/12/2010 16:13
//
//  DESCRIPTION:
/**
    \brief PLA fuse buffer.

    These bits are used to program a section of a node's PLA feature. All 
	areas of the PLA are represented by this buffer.

	\see \ref ISCplaPage
	\see \ref plaFuseBuffer

    Expected usage:
    \code
    plaFuseBuffer fuses;
    // Select the OR area
    fuses.FuseArea = plaFuseBuffer::PLA_OR;
    // Select output # 7
    fuses.FuseIndex.fld.Index = 7;
    // Select AND term # 1
    fuses.Fuses.orFuse.and01 = 1;
    \endcode

**/
typedef union _plaFuses {
    /// Integer view the fuses.
    Uint16 bits;
    /// The bits in this structure select the input bits or their complement
    /// to be ANDed together.
    plaInReg andFuse;
    /// The bits in this structure select the AND outputs to be
    /// ORed together to make an output bit.
    struct  _orFuses {
        int and00        : 1;                ///< Select AND term 00
        int and01        : 1;                ///< Select AND term 01
        int and02        : 1;                ///< Select AND term 02
        int and03        : 1;                ///< Select AND term 03
        int and04        : 1;                ///< Select AND term 04
        int and05        : 1;                ///< Select AND term 05
        int and06        : 1;                ///< Select AND term 06
        int and07        : 1;                ///< Select AND term 07
        int and08        : 1;                ///< Select AND term 08
        int and09        : 1;                ///< Select AND term 09
        int and10        : 1;                ///< Select AND term 10
        int and11        : 1;                ///< Select AND term 11
        int and12        : 1;                ///< Select AND term 12
        int and13        : 1;                ///< Select AND term 13
        int and14        : 1;                ///< Select AND term 14
        int and15        : 1;                ///< Select AND term 15
    } orFuse;                               ///< OR term selector type.
    /// The bits in this structure select the output bits.
    plaOutReg outFuse;
    #ifdef __cplusplus
    /// C++ Helper Constructor with no bits set.
    _plaFuses() {
        bits = 0;
    };
    /// C++ Helper Constructor with an integer setup.
    _plaFuses(Uint16 initialBits) {
        bits = initialBits;
    };
    #endif
} plaFuses;
//                                                                            *
//*****************************************************************************
														/** \endcond **/


														/** \cond ISC_DOC **/
//*****************************************************************************
//  NAME                                                                      *
//      plaFuseBuffer structure
//
// CREATION DATE:
//      10/12/2010 16:13
//
//  DESCRIPTION:
/**
    \brief PLA term buffer.

    The \a FuseArea and \a FuseIndex fields of this register select a section of
    the PLA feature within a node. For the #netPLAget function this \a Fuses
    (#plaFuses) area contains the fuses retrieved from the specified area of
    the PLA. For the #netPLAset function the \a Fuses area contains the fuses
    to program.

	\section plaAreaSect Fuse Areas

    The \a FuseArea field selects the specific fuse area in the PLA array.
    
    The PLA has the following areas:
		- AND terms that AND the inputs to feed an OR term.
		- NOT AND terms AND the inverted inputs to feed an OR term.
		- OR terms are associated to an output bit to be controlled by
		the values present on the input register.
		- OPTION fuses allow access to internal features such as
		the output inverter to convert AND terms into OR terms via
		the DeMorgan process.
		- The architectural area allows generic access to determine the
		count of terms available in the PLA areas.
		.

	\section plaIndexSect Index

	The PLA fuse buffer's \a FuseIndex field selects a specific set of fuses
	within the selected area. These fuses are sixteen bit arrays that connect
	the intersecting input to the specified function. In the AND area this is
	the equivalent to a sixteen input AND gate. For the OR area this is a
	sixteen bit OR gate  ORing the selected AND terms together.

	The index for current devices runs from 0 to 15.

	\see \ref ISCplaPage
	\see \ref plaFuses

    Expected use cases:
        \code
        plaFuseBuffer fuses;
        // Copy GP Timer expire bit to AND(0)
        // Select the AND area
        fuses.FuseArea = plaFuseBuffer::PLA_AND;
        // Assign to term # 0
        fuses.FuseIndex.fld.Index = 0;
        // Select the TimerExpired input bit
        fuses.Fuses.andFuse.DrvFld.TimerExpired = 1;
        \endcode

        \code
        // Setup for OR area for drive enable and clear fuses.
        plaFuseBuffer fuses;
        fuses.Setup(plaFuseBuffer::PLA_OR, ISC_PLAOUT_ENABLE_BIT, 0x0);
        \endcode
**/
typedef struct _plaFuseBuffer {
    /// PLA fuse area enumeration.
    typedef enum _PLA_AREAS {
        PLA_AND,                /* 0 */ ///< Inputs AND fuses area.
        PLA_NAND,               /* 1 */ ///< Inverted inputs AND fuses area.
        PLA_OR,                 /* 2 */ ///< OR of AND outputs.
        PLA_OPTION,             /* 3 */ ///< Optional fuse area.
        PLA_ARCH                /* 4 */ ///< Architectural information area.
    } PLA_AREAS;
    /// Fuse Area selector.
    PLA_AREAS FuseArea;
    /// PLA Fuse Index
    typedef union _plaIndex {
        Uint16   bits;                  ///< Bit-wise access
        struct {
            BFld Index      : 7;        ///< Zero based index number.
            BFld EEalso     : 1;        ///< Access the non-volatile fuses.
        } fld;                          ///< Field Access for the index.
		/// Constructs PLA index of zero accessing the RAM item.
        _plaIndex() {
            bits = 0;
        };
    } plaIndex;                       	// PLA Index type.
	/// Index and options selection.
    plaIndex FuseIndex;               	
    /// Fuse settings.
    plaFuses Fuses;
    /// Option Area Indices
    typedef enum _PLA_OPT_INDICES {
		/**
			Output Inverters.

			This set of fuses is useful for converting a series of OR
			terms into one AND term using DeMorgan's theorem.
		**/
        OPTION_INVERTER                 /* 0 */ 
    } PLA_OPT_INDICES;
    /// Architectural Area Indices.
    typedef enum _ARCH_INDICES {
        AND_TERMS,                      /* 0 */ ///< Count of AND terms.
        OR_TERMS,                       /* 1 */ ///< Count of OR terms.
        OPTION_REGS                     /* 2 */ ///< Count of option registers.
    } ARCH_INDICES;
    #ifdef __cplusplus
    /// C++ Helper Constructor (default).
    _plaFuseBuffer() {
        FuseArea = PLA_AND;
        FuseIndex.bits = 0;
        Fuses.bits = 0;
    };
    /// C++ Helper Constructor (initialized).
    _plaFuseBuffer(PLA_AREAS area, Uint16 index, Uint16 fuses) {
        FuseArea = area;
        FuseIndex.bits = index;
        Fuses.bits = fuses;
    };
    /// C++ Initializer for writing
    void Setup(PLA_AREAS area, Uint16 index, Uint16 fuses) {
        FuseArea = area;
        FuseIndex.bits = index;
        Fuses.bits = fuses;
    };
    /// C++ Initializer for reading
    void Setup(PLA_AREAS area, Uint16 index) {
        FuseArea = area;
        FuseIndex.bits = index;
        Fuses.bits = 0;
    };
    #endif
} plaFuseBuffer;                                ///< PLA fuse buffer type.
//																			  *
//*****************************************************************************
														/** \endcond **/

														/** \cond CPM_ADV **/
//*****************************************************************************
//  NAME                                                                      *
//      xpsFuseBuffer structure
//
// CREATION DATE:
//      09/29/2015 16:13
//
//  DESCRIPTION:
/**
    \brief XPS term buffer.

    The \a FuseArea and \a FuseIndex fields of this register select a section of
    the XPS feature within a node. For the #netXPSget function this \a Fuses
    (#xpsFuses) area contains the fuses retrieved from the specified area of
    the XPS. For the #netXPSset function the \a Fuses area contains the fuses
    to program.

	\section xpsAreaSect Fuse Areas

    The \a FuseArea field selects the specific fuse area in the XPS array.

    The XPS has the following areas:
    	- INPUT_OPTION fuses allow inversion of the inputs before entering
    	the OR section
		- OR terms are associated to an output bit to be controlled by
		the values present on the input register.
		- OPTION fuses allow access to internal features such as
		the output inverter to convert AND terms into OR terms via
		the DeMorgan process.
		- The architectural area allows generic access to determine the
		count of terms available in the XPS areas.

	\section xpsIndexSect Index

	The XPS fuse buffer's \a FuseIndex field selects a specific set of fuses
	within the selected area. These fuses are sixteen bit arrays that connect
	the intersecting input to the specified function. In the AND area this is
	the equivalent to a sixteen input AND gate. For the OR area this is a
	sixteen bit OR gate  ORing the selected AND terms together.

	The index for current devices runs from 0 to 15.

	\see \ref ISCplaPage
	\see \ref plaFuses

    Expected use cases:
        \code
        plaFuseBuffer fuses;
        // Copy GP Timer expire bit to AND(0)
        // Select the AND area
        fuses.FuseArea = plaFuseBuffer::XPS_AND;
        // Assign to term # 0
        fuses.FuseIndex.fld.Index = 0;
        // Select the TimerExpired input bit
        fuses.Fuses.andFuse.DrvFld.TimerExpired = 1;
        \endcode

        \code
        // Setup for OR area for drive enable and clear fuses.
        plaFuseBuffer fuses;
        fuses.Setup(plaFuseBuffer::XPS_OR, ISC_XPSOUT_ENABLE_BIT, 0x0);
        \endcode
**/
typedef struct _xpsFuseBuffer {
    /// XPS fuse area enumeration.
    typedef enum _XPS_AREAS {
    	XPS_IN_OPTION,          /* 0 */ ///< Optional Input fuse area.
        XPS_OR,                 /* 1 */ ///< OR of AND outputs.
        XPS_OPTION,             /* 2 */ ///< Optional fuse area.
        XPS_ARCH                /* 3 */ ///< Architectural information area.
    } XPS_AREAS;
    /// Fuse Area selector.
    XPS_AREAS FuseArea;
    /// XPS Fuse Index
    typedef union _xpsIndex {
        Uint16   bits;                  ///< Bit-wise access
        struct {
            BFld Index      : 7;        ///< Zero based index number.
            BFld EEalso     : 1;        ///< Access the non-volatile fuses.
        } fld;                          ///< Field Access for the index.
		/// Constructs XPS index of zero accessing the RAM item.
        _xpsIndex() {
            bits = 0;
        };
    } xpsIndex;                       	// XPS Index type.
	/// Index and options selection.
    xpsIndex FuseIndex;
    /// Fuse settings.
    plaFuses Fuses;
    /// Option Area Indices
    typedef enum _XPS_OPT_INDICES {
		/**
			Output Inverters.

			This set of fuses is useful for converting a series of OR
			terms into one AND term using DeMorgan's theorem.
		**/
        OPTION_INVERTER                 /* 0 */
    } XPS_OPT_INDICES;
    /// Architectural Area Indices.
    typedef enum _ARCH_INDICES {
        AND_TERMS,                      /* 0 */ ///< Count of AND terms.
        OR_TERMS,                       /* 1 */ ///< Count of OR terms.
        OPTION_REGS                     /* 2 */ ///< Count of option registers.
    } ARCH_INDICES;
    #ifdef __cplusplus
    /// C++ Helper Constructor (default).
    _xpsFuseBuffer() {
        FuseArea = XPS_OR;
        FuseIndex.bits = 0;
        Fuses.bits = 0;
    };
    /// C++ Helper Constructor (initialized).
    _xpsFuseBuffer(XPS_AREAS area, Uint16 index, Uint16 fuses) {
        FuseArea = area;
        FuseIndex.bits = index;
        Fuses.bits = fuses;
    };
    /// C++ Initializer for writing
    void Setup(XPS_AREAS area, Uint16 index, Uint16 fuses) {
        FuseArea = area;
        FuseIndex.bits = index;
        Fuses.bits = fuses;
    };
    /// C++ Initializer for reading
    void Setup(XPS_AREAS area, Uint16 index) {
        FuseArea = area;
        FuseIndex.bits = index;
        Fuses.bits = 0;
    };
    #endif
} xpsFuseBuffer;                                ///< XPS fuse buffer type.
//																			  *
//*****************************************************************************
												/** \endcond **/




//*****************************************************************************
//  NAME                                                                      *
//      mnOutReg structure
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
						/** \cond INTERNAL_DOC **/
/**
    \brief Universal Output Register Type.

	This object is used to represents the state of an <i>Output Register</i>
	in a node. 

	\todo add link to class
**/
typedef union MN_EXPORT PACK_BYTES _mnOutReg 
{
    /// Integer view of the output register
    Uint16 bits;
	// Meridian ISC outputs (TODO - Restore for ISC support)
	//plaOutReg isc;
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC)
	/// ClearPath-SC specific field access
	cpmOutFlds cpm;
#endif
	//
    #ifdef __cplusplus
    /// Zero out all the fields.
    void clear() {
        bits = 0;
    }
	/**
	Return true if all fields are clear
	**/
	bool isClear() {
		return bits == 0;
	}
												
	/// Construct with all zeros
	_mnOutReg() 
	{
		clear();
	}
												
    #endif
} mnOutReg;
									/** \endcond  **/
//                                                                         *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  mnStatusReg union
//
// DESCRIPTION
/**
    \brief Universal Status Register Type

    This Union defines the Universal Status Register for all types of Nodes.

	The Status register is used to reflect the state of the Node.  Status events
	occur when bits/fields of this register are set/cleared.    

	The lower 32 bits of this register can be used to generate an asynchronous
	attention packet to signal the host an event occurred.


	\see sFnd::ValueStatus Class for accessing Status registers within the Node.
	\see cpmStatusRegFlds For the Complete list of ClearPath-SC alert Fields.
	\see \ref AttentionsPage for more information regarding Attentions.
**/
typedef union MN_EXPORT PACK_BYTES _mnStatusReg {
    /// Integer view as three 16-bit integers.
    Uint16 bits[3];
	/// Long integer view that represents the attentional bits
	Uint32 attnBits;
													/** \cond ISC_DOC **/
    /**
        Access to common status field for all nodes.
    **/
    commonStatusFlds Common;
													
	/**
		Access to the status for the Meridian ISC. This is the new
		style flat accessor.
	**/
	iscStatusRegFlds isc;
	/**
		Access to the status register for the Meridian ISC compatible with
		the sFoundation's first release.
	**/
	iscStatusRegOldFlds Fld;
													/** \endcond **/
	/**
		\brief Access to the ClearPath-SC status fields
	**/
	cpmStatusRegFlds cpm;
													/** \cond INTERNAL_DOC **/
    #ifdef __cplusplus
	/// Indices into bitwise register access by functional purpose
	typedef enum _bitsTypes {
		LOW_ATTN,		///< Lower 16-bits of the attentionable portion
		IN_REG, 		///< Upper 16-bits of the attentionable portion
		NON_ATTN		///< Non-attentionable portion
	} bitsTypes;
	/// The number of Uint16 items in the register.
	enum {N_BITS = 3};
													
    /**
		Clear the register bits in this instance.
	**/
    void clear() {
        bits[LOW_ATTN] = bits[IN_REG] = bits[NON_ATTN] = 0;
    }
	/**
		See if the register is all zeros.
	**/
	bool isClear() {
		return (bits[LOW_ATTN] | bits[IN_REG] | bits[NON_ATTN]) == 0;
	}
																/** \endcond **/

#ifndef __TI_COMPILER_VERSION__
	/**
		\brief Update a string with the set fields in the Status Register.

		\param[in,out] buffer  Point to string buffer to update.
		\param[in] size  The size of the buffer in characters including the null
		character.
		\return the parameter \a buffer.

		\CODE_SAMPLE_HDR
		char myBuffer[500];
		cout << myNode.Status.RT.Value.StateStr(myBuffer, sizeof(myBuffer));
		\endcode
	**/
	char * StateStr(char *buffer, size_t size) {
		return (cpm.StateStr(buffer, size));
	}
#endif
													/** \cond INTERNAL_DOC **/
	// Construct a cleared register
	_mnStatusReg() {
		clear();
	}
    #endif
													/** \endcond **/
} mnStatusReg;
/** \cond INTERNAL_DOC **/
/// Alias to older usage
typedef mnStatusReg attnReg;			// 1/5/16 Made equivalent
// True size of the mnStatusReg
#define MN_STATUS_REG_OCTETS	6
/** \endcond **/
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  mnAuditData structure
//
// DESCRIPTION
													/** \cond SC_EXPERT **/
/**
    \brief Motion Audit data type.

    This structure is updated with the VB friendly results of the
	motion audit collection command. This typically queried after the
	reference move has occurred. 
	
	\note This feature appears on ISC firmware version 5.6 and later and
	ClearPath-SC with the advanced feature set.
**/
typedef struct PACK_BYTES _mnAuditData {
	double			LowPassRMS;		///< Low Pass RMS of monitor port value
	double			LowPassMaxPos;	///< Maximum positive of monitor port value
	double			LowPassMaxNeg;	///< Maximum negative of monitor port value
	double			HighPassRMS;	///< High Pass RMS of monitor port value
	double			DurationMS;		///< Duration (msec)
	nodelong		MaxTrackingPos;	///< Maximum positive tracking
	nodelong		MaxTrackingNeg;	///< Maximum negative tracking
} mnAuditData;
													/** \endcond  **/
													/** \cond ISC_DOC **/
/// Backwards compatible typename
typedef struct _mnAuditData iscDataCollect;
													/** \endcond **/
//                                                                            *
//*****************************************************************************

//*****************************************************************************
// NAME                                                                       *
//  mnAppConfigReg union
//
// DESCRIPTION										/** \cond ExpertDoc **/
/**
\brief Universal Application Configuration Register Type

This union defines a container for all types of nodes to control application
settings.
**/
typedef union PACK_BYTES _mnAppConfigReg {
	/// Bit-wise generic access
	nodeulong		bits;
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC)
	/// ClearPath-SC based register
	cpmAppConfigFlds cpm;
#endif
	/// Meridian ISC based register
	iscAppConfigFlds isc;
} mnAppConfigReg;
													/** \endcond  **/
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  mnHwConfigReg union
//
// DESCRIPTION
												/** \cond ExpertDoc **/
/**
\brief Universal Hardware Configuration Register Type

This union defines a container for all types of hardware configurations on 
nodes.
**/
typedef union PACK_BYTES _mnHwConfigReg {
	/// Bit-wise generic access
	nodeulong		bits;
#if !defined(__TI_COMPILER_VERSION__) || defined(_CPM_SC)
	/// ClearPath-SC based register
	cpmHwConfigFlds cpm;
#endif
	/// Meridian ISC based register
	iscHwConfigFlds isc;
} mnHwConfigReg;
												/** \endcond **/
//                                                                            *
//*****************************************************************************




//*****************************************************************************
// Restore packing rules
#ifdef _MSC_VER
#pragma pack(pop)
#endif
//*****************************************************************************

#endif
//=============================================================================
//  END OF FILE pubCoreRegs.h
//=============================================================================


//*****************************************************************************
// $Archive: /ClearPath SC/Firmware/cp1x/inc_pub/pubRegFldCommon.h $
// $Revision: 7 $ $Date: 7/08/16 3:00p $
// $Workfile: pubRegFldCommon.h $
//
// DESCRIPTION:
/**
	\file	
	\brief Definitions of the common parts of various registers.

	
**/
// CREATION DATE:
//		2015-12-24 10:59:53
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2010-2016  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
#ifndef __PUBREGFLDCOMMON_H__
#define	__PUBREGFLDCOMMON_H__


//*****************************************************************************
// NAME																          *
// 	pubRegFldCommon.h headers
//
#include "tekTypes.h"
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	pubRegFldCommon.h function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	pubRegFldCommon.h constants
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	pubRegFldCommon.h static variables
//
// 

//																			  *
//*****************************************************************************

//****************************************************************************
// NAME                                                                      *
//  commonAlertFlds struct
//
// DESCRIPTION
/**
    \brief Alert bits common to all nodes.

    This union defines the common areas of the node's Alert Register. All
    nodes will use these definitions.
 **/
typedef struct PACK_BYTES _commonAlertFlds {
    //                              ------------------------------------------
    //                              |len|bit #|description                   |
    //                              ------------------------------------------
    /**
        \brief Node power-on self test has failed.
		
		Node power-on self test has failed.

		Appears in:

        Remedies:
        - Replace node.
    */
    unsigned FwSelfTest             : 1; /* 0 */ // Firmware self-test failed
    /**
        \brief Network buffer overrun.

        Network buffer overrun.

		Appears in:

        Remedies:
        - Check command pacing.
    */
    unsigned FwNetBufferOverrun     : 1; /* 1 */ // Network buffer overrun
    /**
        \brief Internal Firmware Error 0.

        Internal Firmware Error 0.

		Appears in: AlertReg

        Remedies:
        - Update firmware.
    */
    unsigned FwError0               : 1; /* 2 */ // Internal Error 0
    /**
        \brief Internal Firmware Error 0.

        Internal Firmware Error 0.

		Appears in: AlertReg

        Remedies:
        - Update firmware.
    */
    unsigned FwError1               : 1; /* 3 */ // Internal Error 1
    /**
        \brief Internal Firmware Error due to stack issues.

        Internal Firmware Error due to stack issues.

		Appears in: AlertReg

        Remedies:
        - Update firmware.
    */
    unsigned FwStackOverrun         : 1; /* 4 */ // Firmware Stack problem
    /**
        \brief The DSP watchdog timer has restarted the node.

        The DSP watchdog timer has restarted the node.

        Appears in:

        Remedies:
        - Check link power supply for brown-outs.
        - Update firmware.
    */
    unsigned FwWatchdogRestarted    : 1; /* 5 */ // DSP Watchdog restarted node
    /**
        \brief Invalid setting detected.

        An invalid setting is detected that prevents the drive from running

        Appears in: AlertReg

        Remedies:
        - Clear to factory defaults and load a valid configuration file.
        - Replace node
    */
    unsigned FwInvalidConfiguration : 1; /* 6 */ // DSP Watchdog restarted node
	/** \cond INTERNAL_DOC **/
    unsigned na7_10                 : 4; // 7-10
	/** \endcond **/
    /**
		\brief Power Supply Problem.

        Onboard power supply out of regulation or missing. This condition is 
        stored in internal log for factory use.

		Appears in: AlertReg

        Remedies:
        - Replace node.
    */
    unsigned HwPowerProblem         : 1; /* 11 */ // Bad power supply
    /**
		\brief DSP clock problem.

        The DSP clock did not initialize properly. Most likely you will
        not be able to communicate with this node. This condition is stored in internal log
        for factory use.

        Appears in: AlertReg

        Remedies:
        - Replace node.
    */
    unsigned HwClockProblem         : 1; /* 12 */ // Clock Bad
    /**
        \brief The node's non-volatile memory is broken.

        The node's non-volatile memory is broken.

        Appears in: AlertReg

        Remedies:
        - Replace node.
    */
    unsigned HwEEPROMdead           : 1; /* 13 */ // EEPROM hw bad
    /**
		\brief Flash content drifting.

        The run-time test of the firmware has determined the flash memory
        is drifting.

        Appears in: AlertReg

        Remedies:
        - Replace or reflash node.
    */
    unsigned HwFlashCorrupt         : 1; /* 14 */ // Runtime corruption test
    /**
		\brief Flash changed due to update.

        This bit is asserted after the flash memory is updated with new firmware
        or has drifted from the known image. Normally the Teknic supplied
        flash updater will reset this when it clears the configuration out of
        the node as the firmware is updated.

		Appears in: AlertReg

        Remedies:
        - If the firmware has not been recently updated, replace this node.
        - Reload the node's configuration file using the APS application.
        - Ensure the Flash Update Acknowledge parameter has been set to the
        node's checksum.
    */
    unsigned HwFlashChanged         : 1; /* 15 */ // POR test, firmware updated
    /**
        \brief The power-on self test has determined the RAM is bad.

        The power-on self test has determined the RAM is bad.

        Appears in:

        Remedies:
        - Replace the node
    */
    unsigned HwRAM                  : 1; /* 16 */ // POR test, RAM problem
    /**
		\brief ADC failed self-test.
		
		The power-on self test has determined the Analog to Digital
        converter or its related analog hardware is bad.

		Appears in:

		Remedies:
        - Replace the node
    */
    unsigned HwADC                  : 1; /* 17 */ // POR test, ADC problem
    /**
		\brief ADC Saturated

        ADC in saturation mode.

		Appears in: AlertReg

        Remedies:
        - Check for new firmware.
        - Replace node.
    */
    unsigned HwADCsat				: 1; // 18 Unexpected ADC saturation
	/** \cond INTERNAL_DOC **/
    unsigned na19_23                : 5; // 19-23
	/** \endcond **/
    /**
		\brief Link/Node power supply low.

        The link and node's power supply is low. This could be due to an
        overloaded supply or extremely long cables.

		Appears in:

        Remedies:
        - Check link power supply
    */
    unsigned NetVoltageLow          : 1; /* 24 */ // Link voltage brown-out
    /**
		\brief Network activity watchdog node stop.

        The node has entered a node stop state due to a lack of communications
        from the host. If the node's Stop Type is set to
        STOP_TYPE_IGNORE this feature is defeated.

		Appears in: AlertReg, WarnReg

        Remedies:
        - Ensure the application requests some real-time parameter
        or run a command within the watchdog time.
        - Adjust or disable the network activity watchdog parameter.
    */
    unsigned NetWatchdog            : 1; /* 25 */ // Network watchdog shutdown
													/** \cond INTERNAL_DOC **/
    unsigned na26_27                : 2; // 26-27
													/** \endcond **/
    /**
		\brief Node E-Stopped.

        The node has entered an E-Stop state and will no longer accept motion
        commands until this condition is cleared.  This code is not cleared
        with the IStatus::AlertsClear function to prevent unintended motion.

		Appears in: AlertReg

        Remedies:
		- Issue a IMotion::
        - Issue a netNodeStop command with the STOP_TYPE_CLR_ESTOP argument
        to allow motion to be re-issued.
    */
    unsigned EStopped               : 1; /* 28 */ // In E-Stopped State
    /**
		\brief Configuration file out of date for firmware.

        The node's internal configuration file is out of date for the loaded
        firmware.

		Appears in: AlertReg

        Remedies:
        - Clear to factory defaults and load a valid configuration file.
    */
    unsigned ConfigOutOfDate        : 1; /* 29 */ // Non-Volatile mismatch / Firmware
    /**
		\brief Internal firmware error.

        Internal firmware error.

		Appears in:

        Remedies:
        - Update node firmware to newer release.
    */
    unsigned RunTimeErr             : 1; /* 30 */ // Generic runtime error
	/** \cond INTERNAL_DOC **/
    unsigned na31                   : 1; // 31
	/** \endcond **/
} commonAlertFlds;

// These bit numbers must match the locations in the alertReg union
// They are used by coding that requires either a mask or a bit number.
/**
    \brief Common Alert Bit numbers.

    These numbers can be used to construct masks into the alert/warning
    register.
**/
/** \cond INTERNAL_DOC **/
typedef enum _commonAlertBits {
    ALERT_FW_SELFTEST_BIT                       = 0,
    ALERT_FW_NET_BUF_OVERRUN_BIT                = 1,
    ALERT_FW_ERROR0_BIT                         = 2,
    ALERT_FW_ERROR1_BIT                         = 3,
    ALERT_FW_STACK_OVERRUN_BIT                  = 4,
    ALERT_FW_WATCHDOG_RESTARTED_BIT             = 5,
    ALERT_FW_INVALID_CONFIGURATION_BIT			= 6,
	ALERT_HW_POWER_BIT							= 11,   // Hw Shdn
    ALERT_HW_CLOCK_PROBLEM_BIT                  = 12,   // HW Shdn
    ALERT_HW_EEPROM_DEAD_BIT                    = 13,   // HW Shdn
    ALERT_HW_FLASH_CORRUPT_BIT                  = 14,   // sw Shdn
    ALERT_HW_FLASH_CHANGED_BIT                  = 15,
    ALERT_HW_RAM_BIT                            = 16,   // HW Shdn
    ALERT_HW_ADC_BIT                            = 17,   // Hw Shdn
    ALERT_HW_ADC_SATURATION						= 18,
    ALERT_NET_VOLTAGE_LOW_BIT                   = 24,   // sw Shdn
    ALERT_NET_WATCHDOG_BIT                      = 25,
    ALERT_NET_ESTOPPED_BIT                      = 28,   // mtn Shdn
    ALERT_NET_CONFIG_OUT_OF_DATE_BIT            = 29,   // sw Shdn
    ALERT_RUN_TIME_ERR_BIT                      = 30,    // sw Shdn
	ALERT_LOG_EPOCH_CODE                        = 0xFE,
	ALERT_LOG_FREE_CODE                         = 0xFF
} commonAlertBits;
/** \endcond **/
//                                                                           *
//****************************************************************************

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// STATUS REGISTER COMMON FIELD DEFINITIONS
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//*****************************************************************************
// NAME                                                                       *
//  lowAttnCommonFlds struct
//
// DESCRIPTION
/**
 	\cond ISC_DOC
    \brief <i>Lower Attentionable</i> portion of the <i>Status Register</i>.

    This portion of the Status Register (#mnStatusReg) can be used to generate
    attention packets and has common features across all models.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct PACK_BYTES _lowAttnCommonFlds {
   /**
		\brief Warning is present.

        This bit asserts when the <i>Warning Register</i> contents,
        ANDed with the user's <i>Warning Mask
        Register</i> contents, is non-zero.

        \remark
        This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld Warning        : 1;    // 0  00 Warning is present
    /**
		\brief User alert is present.

        This bit asserts when the <i>Alert Register</i> contents,
        ANDed with the user's <i>Alert Mask
        Register</i> contents,is non-zero.

        \remark
        This bit can generate an attention request to the host for
        autonomous handling.
    **/
    BFld UserAlert      : 1;    // 1  01 User Alert is present
    /**
		\brief Not ready for operation.

        This bit asserts when the node is not ready. Consult the
        <i>Status Register</i> (#mnStatusReg) and if indicated
		the <i>Alert Register</i> (#alertReg), for the reasons why the node is 
		not ready.

        If the shutdown is clearable, the user application can try to 
        clear the alerts with the IStatus::AlertsClear function. If the 
		<i>Enable Request</i> for the drive is still asserted and
        there are no other shutdown reasons, the node will
        automatically become ready.

        \remark
        This bit can generate an attention request to the host for
        autonomous handling.
	**/
    BFld NotReady       : 1;    // 2  02 Not Rdy for Operation
} lowAttnCommonFlds;
//                                                                           *
//****************************************************************************

//*****************************************************************************
// NAME                                                                       *
//  plaInCommonFlds struct
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
typedef struct PACK_BYTES _plaInCommonFlds {
   /**
        This bit represents the receipt of a node stop event. Depending
        on the method the node stop was initiated with, the node may
        or may not accept motion commands and may not allow changes to
        the output register. If the node stop involved an E-Stop request
        this bit will remain on until the E-Stop state is cleared.

        Node stops with the <i>Quiet</i> bit set will not assert
        this bit.

        \see \ref ISCnodeStopPage
    **/
    BFld NodeStopped    : 1;    // 0  16: node stop
	/** \cond INTERNAL_DOC **/
    BFld na0_15         : 15;
	/** \endcond **/
} plaInCommonFlds;
//                                                                           *
//****************************************************************************

//*****************************************************************************
// NAME                                                                       *
//  nonAttnCommonFlds struct
//
// DESCRIPTION

/**
    \brief Common Non-Attentional portion of the <i>Status Register</i>.

    The most significant sixteen bit portion of the <i>Status Register</i>
    (#mnStatusReg).
    This portion of the register does not generate any attention packets.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct PACK_BYTES _nonAttnCommonFlds {
	/** \cond INTERNAL_DOC **/
    BFld				: 3;
	/** \endcond **/
    /**
		\brief Node in <i>Controlled Stop</i>.

        The Node Stop feature has forced the node into its <i>
        Controlled</i> state.

    **/
    BFld InCtrlStop     : 1;    // 3  35: Output Register is controlled
    /**
		\brief Fan running.

        For units with a fan, this field is asserted when the
        fan is on.
    **/
    BFld FanOn          : 1;    // 4  36: Fan is running
	/** \cond INTERNAL_DOC **/
    BFld		        : 5;
	/** \endcond **/
    /**
		\brief Hardware failure detected.

        The unit's shutdown is caused by a hardware failure. This
        unit should be replaced.
    **/
    BFld HwFailure      : 1;    // 10 42: Replace the node
} nonAttnCommonFlds;
//                                                                           *
//****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  commonStatusFlds struct
//
// DESCRIPTION

/**
    \brief Common portions of the <i>Status Register</i>.

	This is the structure defined for Meridian ISC's status register.

	\see \ref mnStatusReg for details about accessing this type of register.
**/
typedef struct PACK_BYTES _commonStatusFlds {
	/**
		\brief Lower attentionable register.
	**/
	lowAttnCommonFlds LowAttn;
	/**
		\brief Input register with partial attentionable fields.
	**/
	plaInCommonFlds PlaIn;
	/**
		\brief Non-attentionable fields.
	**/
	nonAttnCommonFlds NonAttn;
} commonStatusFlds;
//                                                                           *
//****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  sensorlessMethodsReg union
//
// DESCRIPTION
/**
\brief Sensorless start methods.

This bit-mapped control register definitions.
**/
union sensorlessMethodsReg {
	/// Numeric representation of sensorless start methods.
	typedef enum _sensorlessMethods {
		INDUCTANCE=1,					///<  1 Pulse voltages and measure current response
		INDEX_SEARCH=2,					///<  2 Lock vector and move to an index
		MAG_ALIGN_UNDAMPED=4,			///<  4 30 degree lock vector sweep/undamped
		MAG_ALIGN_DAMPED=8
	} sensorlessMethods;
	/// Field access to sensorless start methods
	struct _sensorlessFlds {
		BFld Inductance : 1;
		BFld IndexSearch : 1;
		BFld MagAlignUndampled : 1;
		BFld MagAlignDamped : 1;
	} fld;
	/// Bit-wise representation of methods
	sensorlessMethods bits;
};
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  Travel Limit Switch Handling enumeration
//
// DESCRIPTION
/**
	\brief Generic Travel Limit handling enumeration.
**/
/// General Purpose Input/Limit Switch handling
typedef enum _mnLimitModes {
	/*!
		Ignores General Purpose inputs for travel limits.
	**/
	LIMIT_MODE_OFF,                     //  0: ignore limits
	/*!
		Travel Limits use General Purpose inputs.
	**/
	LIMIT_MODE_POSITION,                //  1: Limit position
	/*!
		Not implemented (Torque OFF)
	**/
	LIMIT_MODE_TORQUE,                  //  2
	/*!
		Travel limits set using node positional limit parameters
		and General Purpose inputs.
	**/
	LIMIT_MODE_POSN_INS_AND_SOFT = -3,	// -3:
	/*!
		Travel limits set using node positional limit parameters.
	**/
	LIMIT_MODE_SOFT_POSN = -4			//  4: Software position limit
} mnLimitModes;                           ///< GPI/Limit Switch Modes
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//  High Level Feedback modes
//
// DESCRIPTION
/**
	\brief High Level Feedback modes.
**/
typedef enum _mnHLFBmodes {
	HLFB_BRAKE_OFF,					///<  0 Brake off (Servo On), not shutdown
	HLFB_ASG,						///<  1 All Systems Go
	HLFB_SPEED,						///<  2 PWM output proportional to measured speed
	HLFB_FORCE_OFF,					///<  3 Always off (test mode)
	HLFB_FORCE_ON,					///<  4 Always on (test mode)
	HLFB_INDEX,						///<  5 Output on when index occurs
	HLFB_IN_RANGE,					///<  6 Position in-range
	HLFB_TORQUE_ASG,				///<  7 Torque/ASG, 5%=-100, 95%=100
	HLFB_TORQUE						///<  8 Torque out 5%=-100, 50%=0, 95%=100
} mnHLFBmodes;
//                                                                            *
//*****************************************************************************



//*****************************************************************************
// NAME                                                                       *
//  Disable Action enumerations
//
// DESCRIPTION
/**
 * Disable actions enumeration
 *
 * These values determine the drive's actions to take upon disable, LPB
 * or AC Loss.
 **/
typedef enum _mnDisableActions {
	/**
	 * Use dynamic brake to stop
	 *
	 * This potentially is the most abrupt way to stop as it uses the
	 * shorted motor windings to dissipate the kinetic energy of the
	 * axis.
	 **/
	DYNAMIC_BRAKE,
	/**
	 * Controlled deceleration to a stop.
	 *
	 * Uses the motion constraints defined by the current move to
	 * ramp to a stop then disable the drive.
	 **/
	DECEL_TO_STOP,
	/**
	 * Coast to a stop.
	 *
	 * Free wheel the axis until it stops from its own friction.
	 **/
	COAST
} mnDisableActions;
//                                                                            *
//*****************************************************************************


 #endif // __PUBREGFLDCOMMON_H__
//============================================================================= 
//	END OF FILE pubRegFldCommon.h
//=============================================================================

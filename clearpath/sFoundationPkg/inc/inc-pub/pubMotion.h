
//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubMotion.h $
// $Revision: 70 $ $Date: 12/09/16 4:03p $
// $Workfile: pubMotion.h $
/**
	\file pubMotion.h

	Motion Related Definitions

	This C/C++ header defines the data types related to the motion programming
	interfaces.
**/
// NAME
//      pubMotion.h
//
// DESCRIPTION
//     Suite of motion related constants.
//
// CREATION DATE:
//      06/26/2008 18:01:23
//      09/24/2009 16:56:34 Refactored from iCmdController.h
//
// COPYRIGHT NOTICE:
//      (C)Copyright 2008-2011 Teknic, Inc.  All rights reserved.
//
//      This copyright notice must be reproduced in any copy, modification,
//      or portion thereof merged into another program. A copy of the
//      copyright notice must be included in the object library of a user
//      program.
//                                                                            *
//*****************************************************************************
#ifndef __PUBMOTION_H__
#define __PUBMOTION_H__


//*****************************************************************************
// NAME                                                                       *
//  pubMotion.h headers
//
    #include "tekTypes.h"
//                                                                            *
//*****************************************************************************



//*****************************************************************************
// NAME                                                                       *
//  pubMotion.h function prototypes
//
//                                                                            *
//*****************************************************************************



//*****************************************************************************
// NAME                                                                       *
//  pubMotion.h constants
//

															/** \cond INTERNAL_DOC **/
#define MV_VEL_Q    17                  // Q factor for velocity
															/** \endcond **/

#if defined(_WIN32)||defined(_WIN64)
#pragma pack(push,1)                // Align to bytes boundaries
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
															/** \cond ISC_DOC **/
/**
    Positional Move "Shapes" and types as an integer code.

    This enumerated list provides an easy-to-specify list of valid
    motion type codes. Using the base level #mgPosnStyle structure provides
    an easy-to-use translation from this enumerated list to the proper
    set of bit fields. The generic field access should be avoided as
    this structure allows for invalid combinations of motion styles and
    options.

	\ref ISCmovePositionalPage
**/
typedef enum _mgPosnStyles {
// Absolute moves
    MG_MOVE_STYLE_NORM_ABS,             ///< Move triangle / trapezoid abs target
    MG_MOVE_STYLE_ASYM_ABS=0x08,        ///< Move / programmable Acc/Dec limit  abs target
    MG_MOVE_STYLE_TAIL_ABS=0x10,        ///< Move / tail velocity segment  abs target
    MG_MOVE_STYLE_HEAD_ABS=0x20,        ///< Move / head velocity segment  abs target
    MG_MOVE_STYLE_HEADTAIL_ABS=0x30,    ///< Move / head-tail velocity segments  abs target
// Relative moves
    MG_MOVE_STYLE_NORM=0x01,            ///< Move triangle / trapezoid
    MG_MOVE_STYLE_ASYM=0x09,            ///< Move / programmable Acc/Dec limit
    MG_MOVE_STYLE_TAIL=0x11,            ///< Move / tail velocity segment
    MG_MOVE_STYLE_HEAD=0x21,            ///< Move / head velocity segment
    MG_MOVE_STYLE_HEADTAIL=0x31,        ///< Move / head-tail velocity segments
// Absolute moves/trigger
    MG_MOVE_STYLE_NORM_TRIG_ABS=0x02,   ///< Move triangle / trapezoid abs target/trigger
    MG_MOVE_STYLE_ASYM_TRIG_ABS=0x0a,   ///< Move / programmable Acc/Dec limit  abs target abs target/trigger
    MG_MOVE_STYLE_TAIL_TRIG_ABS=0x12,   ///< Move / tail velocity segment abs target/trigger
    MG_MOVE_STYLE_HEAD_TRIG_ABS=0x22,   ///< Move / head velocity segment abs target/trigger
    MG_MOVE_STYLE_HEADTAIL_TRIG_ABS=0x32,///< Move / head-tail velocity segments  abs target
// Relative moves/trigger
    MG_MOVE_STYLE_NORM_TRIG=0x03,       ///< Move triangle / trapezoid  / trigger
    MG_MOVE_STYLE_ASYM_TRIG=0x0b,       ///< Move / programmable Acc/Dec limit /trigger
    MG_MOVE_STYLE_TAIL_TRIG=0x13,       ///< Move / tail velocity segment / trigger
    MG_MOVE_STYLE_HEAD_TRIG=0x23,       ///< Move / head velocity segment / trigger
    MG_MOVE_STYLE_HEADTAIL_TRIG=0x33,   ///< Move / head-tail velocity segments / trigger
// Modifier
    MG_MOVE_STYLE_DWELL_OPTION = 0x04   ///< Create post move dwell (newer f/w)
} mgPosnStyles;

#define MG_REL_TRIG_MASK    0x3
															/** \endcond **/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
															/** \cond ISC_DOC **/
/**
    Positional Move "Shapes" Data Structure.

    For most applications it is simpler to pass in the #mgPosnStyles code
    for any function to select a desired style.

    For example:
    \code
    iscForkMoveEx(someNode, 1000, MG_MOVE_STYLE_HEADTAIL);
    \endcode

    \note Some combinations of style selectors are not valid.
**/
typedef union _mgPosnStyle {
    /// Move Style as an integer code.
    mgPosnStyles styleCode;         // Move style
    /// Field oriented style selectors
    struct {
        int     relative    :1;     ///< Relative moves
		/**
			Wait for Trigger

			Cause this move to await a trigger event before starting. This
			will cause the #nonAttnReg::TriggerArmed field to set until
			a trigger event releases this move.

			\see \ref netTrigger
		**/
        int     wait        :1;
        int     dwell       :1;     ///< Create post move dwell
        int     asym        :1;     ///< Absolute moves
        int     tail        :1;     ///< Tail section
        int     head        :1;     ///< Head section
		int		repeat		:1;		///< Cause move to repeat/dwell
		int		reciprocate :1;		///< Reciprocating moves/repeat
    } fld;                          ///< Field access to move styles.
    #ifdef __cplusplus
        /// Useful converter to convert integer code to style container.
        _mgPosnStyle(mgPosnStyles how) {styleCode = how;}
        _mgPosnStyle() {styleCode = MG_MOVE_STYLE_NORM;}
        void operator=(const mgPosnStyles &how) {styleCode = how;}
    #endif
} mgPosnStyle;
															/** \endcond **/
															/** \cond INTERNAL_DOC **/
// Structure based move API
typedef struct _mgMoveProfiledInfo {
    mgPosnStyles    type;           // Motion style: trap,head,tail, etc.
    nodelong           value;       // Position Target (based on style)
    #ifdef __cplusplus
        _mgMoveProfiledInfo() {type = MG_MOVE_STYLE_NORM;}
    #endif
} mgMoveProfiledInfo;
															/** \endcond **/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
															/** \cond ISC_DOC **/
/**
	Velocity Move Styles and types as an integer code.

	This enumerated type describes the velocity move styles.  This allows
	a simple conversion to the #mgVelStyle union.

	There are two basic velocity style moves:
		- Classic velocity mode
		- Skyline velocity mode

	\ref ISCmoveVelocityPage
	\ref ISCmoveSkylinePage

**/
typedef enum _mgVelStyles {
	/**
	Change the commanded velocity immediately.
	**/
    MG_MOVE_VEL_STYLE,                  // Regular change to new velocity
	/**
	Change to new commanded velocity after trigger event.
	**/
    MG_MOVE_VEL_STYLE_TRIG = 0x2,       // Start motion on trigger event
	/**
	Change to new commanded velocity and travel the commanded distance.
	**/
    MG_MOVE_SKY_STYLE = 0x4,			// Regular move segment
	/**
	Change to new commanded velocity and travel to the commanded location.
	**/
    MG_MOVE_SKY_STYLE_ABS = 0x5,		// Absolute position move segment
	/**
	Change to new velocity and travel the commanded distance after trigger.
	**/
    MG_MOVE_SKY_STYLE_TRIG = 0x6,		// Regular move segment
	/**
	Change to new velocity and travel to the commanded location after trigger.
	**/
    MG_MOVE_SKY_STYLE_TRIG_ABS = 0x7,	// Absolute position move segment
    /**
     * Regular velocity with no positional tracking shutdown
     */
    MG_MOVE_VEL_STYLE_NO_TRK = 0x8
} mgVelStyles;

/**
	This container holds the velocity move type specification.

	\remark With C++ you can use a #mgVelStyles enumeration to initialize
	this container using the constructor.
**/
typedef union _mgVelStyle {
    mgVelStyles styleCode;      ///< Move style
    struct {
  		/**
			Absolute Position.

			If this field is cleared, the target position is the position
			value relative to the start of move. If set, the position is the
			absolute position at the end of the move.

			\note This field is used only when \c skyline field is set.
		**/
        int absolute	:1; 
		/**
			Trigger Move.

			Cause this move to await a trigger event before starting. This
			will cause the #nonAttnReg::TriggerArmed field to set until
			a trigger event releases this move.

			\see \ref netTrigger
		**/
        int wait		:1;     // Wait for trigger
		/**
			Skyline Move.

			This field specifies a move segment that will attempt a ramp to
			the requested speed toward the target position using the current 
			acceleration limit and continue at this speed until it reaches 
			the target position.  When the position is reached this move is 
			complete and the next segment will begin. If the motion 
			specification is not achievable due to too short of distances or 
			insufficient acceleration values, the move may end not achieving 
			the desired velocity. If the terminating segment cannot achieve 
			the desired position within the current acceleration constraint, 
			the acceleration will be increased to hit the target without 
			overshooting and the #alertReg::FwMoveSpecAltered condition is 
			raised to signal the violation of the constraints. 

		    \image html "Move_Skyline.png"
		    \image latex "Move_Skyline.eps"
			
			\remark If the #mgVelStyle::absolute field is set this move
			segment specifies the end position as an absolute position versus
			relative to the start of the move.
		**/
        int skyline		:1;     // Skyline move segment
        /**
         * 	Suppress Tracking Shutdown
         *
         * 	This field tells the drive to ignore positional tracking
         * 	errors.
         */
        int noTrackingShutdown	:1;

    } styles;					///< Style type bit fields.
														/** \endcond **/
														/** \cond INTERNAL_DOC **/
	#ifdef __cplusplus
        _mgVelStyle(mgVelStyles how) {styleCode = how;}
        _mgVelStyle() {styleCode = MG_MOVE_VEL_STYLE;}
        void operator=(const mgVelStyles &how) {styleCode = how;}
    #endif
} mgVelStyle;
														/** \endcond **/


														/** \cond ISC_DOC **/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jerk Limiting Selections
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//*****************************************************************************
// NAME                                                                       *
//  rasSelector union
//
// DESCRIPTION
///     \brief gStop/RAS type and value specification

typedef union _rasSelector {        // Output RAS value
    Uint32 Bits;                    ///< gStop/RAS selection code number.
} rasSelector;
//                                                                            *
//*****************************************************************************
														/** \endcond **/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//					 NODE STOP FEATURE DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//*****************************************************************************
// NAME                                                                       *
//      mgStopStyles enumeration
//
// DESCRIPTION
/**
	\brief Motion stop strategies.

	This enumeration defines the action taken by the move generator when
	a node stop is issued.
**/
typedef enum _mgStopStyles {
	/**
		Kill Motion Abruptly

		This style terminates the motion immediately and flushes
		any pending motion buffered up.
	**/
	MG_STOP_STYLE_ABRUPT,              // 0 Kill Motion Abruptly
	/**
		Kill Motion with Deceleration

		This style ramps the speed down to zero using the EStopDecel rate.
	**/
	MG_STOP_STYLE_RAMP,                // 1 Kill Motion with the greater of the active and stop decel limits
															/** \cond INTERNAL_DOC **/
	MG_STOP_STYLE_AFTER_CYCLE,         // 2 After cycle completes
															/** \endcond **/
	/**
		Ignore Node Stop

		This style tells this node to ignore the default node
		stop requests.
	**/
	MG_STOP_STYLE_IGNORE,              // 3 Node stop ignore
	/**
		Kill Motion with Deceleration at the active decel rate

		This style ramps the speed down to zero using the deceleration rate
		of the active move.
	**/
	MG_STOP_STYLE_RAMP_AT_DECEL        // 4 Kill Motion with active decel limit
} mgStopStyles;
//                                                                            *
//*****************************************************************************


															/** \cond ISC_DOC **/
//*****************************************************************************
// NAME                                                                       *
//      iscStopFlds structure
//
// DESCRIPTION
/**
	\brief Meridian ISC stop type definition.

	This bit encoded register is used to specify the actions a motor will take
	when a default or explicit node stop action occurs.
**/
typedef struct _iscStopFlds {
/**
	Move Generator response

	This is what the motion generator will do for all non-Clearing actions.
**/
	mgStopStyles MotionAction : 3;
	// - - - - - - - - - - - -
	// Stop type modifiers
	// - - - - - - - - - - - -
	/// Clear modifier(s)
	unsigned Clear : 1;
	/// Lockout subsequent motion and enter E-Stopped State
	unsigned MotionLock : 1;
	/// Force the <i>Output Register</i> to the <i>Controlled
	/// Output Register</i>.
	unsigned IOControl : 1;
	/// Suppress update of <i>Status Register</i> making this
	/// a stealth <i>Node Stop</i>.
	unsigned Quiet : 1;
} iscNodeStopFlds;
//                                                                            *
//*****************************************************************************
															/** \endcond **/

//*****************************************************************************
// NAME                                                                       *
//      cpmNodeStopFlds structure
//
// DESCRIPTION
/**
	\brief ClearPath-SC stop type definition.

	This bit encoded register is used to specify the actions a motor will take
	when a default or explicit node stop action occurs.
**/
struct _cpmNodeStopFlds {
	/**
		\brief Move Generator Action to Take

		This is what the motion generator will do for all non-clearing actions.
	**/
	mgStopStyles MotionAction : 3;
	// - - - - - - - - - - - -
	// Stop type modifiers
	// - - - - - - - - - - - -
														/** \cond INTERNAL_DOC **/
	/**
		\brief Clear modifier(s)

		This field is set in conjunction with the \a MotionLock or \a Disable
		fields to clear the latching condition preventing normal operations
		of these features.
	**/
	unsigned Clear : 1;
														/** \endcond **/
	/**
		\brief Motion Lockout latch
		
		If this set, subsequent motion requests are rejected until this
		condition is cleared with the \a Clear modifier.
	**/
	unsigned MotionLock : 1;
														/** \cond INTERNAL_DOC **/
	unsigned : 1;
														/** \endcond **/
	/**
		\brief Suppress Node Stop Status
		
		The update of the <i>Status Register</i> is suppressed making this
		a stealth <i>Node Stop</i>.
	**/
	unsigned Quiet : 1;
	/**
		\brief Force disable after Motion Action is complete
		
		The node will disable the servo after the motion action completes. This
		can be used with the AutoBrake feature to engage the brake.

		\see sFnd::IBrakeControl
	**/
	unsigned Disable : 1;
};
/// ClearPath-SC Node Stop Request Field Definitions
typedef struct _cpmNodeStopFlds cpmNodeStopFlds;
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//      nodeStopCodes enumeration
//
// DESCRIPTION
/**
	\brief Commonly used <i>Node Stops</i> as a set of constants.

    This enumeration provides a convenient mechanism for issuing common
    <i>Node Stops</i>. The sFnd::IMotion::NodeStop function will
    accept these constants instead of creating and initializing a
	#_mgNodeStopReg object.

    \code
    // Stop the motion at node 'myNode' with ramp.
    myNode.Motion.NodeStop(STOP_TYPE_RAMP);
    \endcode
**/
enum _nodeStopCodes {
    /*!
        Stop the executing motion and flush any pending motion
        commands immediately.
    **/
    STOP_TYPE_ABRUPT        =0x00,		// Kill Motion Abruptly
    /*!
        Flush any pending motion commands and ramp the node's speed to
        zero using the <i>E-Stop Deceleration Rate.</i>
    **/
    STOP_TYPE_RAMP          =0x01,		// Kill Motion with decel limit
												/** \cond ISC_DOC **/
    /*!
        Ignore any node stop requests.
        \note This mode does not affect directed node stops via commands.
    **/					
    STOP_TYPE_IGNORE        =0x03,		// Node stop ignored
												/** \endcond **/
	/*!
		Kill Motion with Deceleration at the active decel rate and flush 
		any pending motion commands immediately.
		
	**/
	STOP_TYPE_RAMP_AT_DECEL = 0x04,		// Kill Motion with active decel limit
    /*!
        Stop the executing motion and flush any pending motion commands
        immediately.

        Future motion is blocked until a directed [NodeStopClear](@ref sFnd::IMotion::NodeStopClear)
		is sent.
    **/
    STOP_TYPE_ESTOP_ABRUPT  =0x10,		// E-Stop Abruptly
    /*!
        Flush any pending motion commands and ramp the node's speed to
        zero using the <i>E-Stop Deceleration Rate.</i>

        Future motion is blocked until a directed [NodeStopClear](@ref sFnd::IMotion::NodeStopClear)
		is sent.
    **/
    STOP_TYPE_ESTOP_RAMP    =0x11,		// E-Stop Ramping to stop
	/*!
		Flush any pending motion commands and ramp the node's speed to
		zero using the <i>active decel rate.</i>

		Future motion is blocked until a directed [NodeStopClear](@ref sFnd::IMotion::NodeStopClear)
		is sent.
	**/
	STOP_TYPE_ESTOP_RAMP_AT_DECEL = 0x14,		// E-Stop Abruptly
	/*!
		Stop the executing motion and flush any pending motion commands
        immediately.

		Once the node has stopped, the node will be disabled.
	**/
	STOP_TYPE_DISABLE_ABRUPT = 0x80,		// E-Stop Ramping to stop
	/*!
		Stop the executing motion and flush any pending motion commands
		immediately.

		Once the node has stopped, the node will be disabled.
	**/
	STOP_TYPE_DISABLE_RAMP = 0x80,		// E-Stop Ramping to stop
	/*!
		Stop the executing motion and flush any pending motion commands
		immediately.

		Once the node has stopped, the node will be disabled.
	**/
	STOP_TYPE_DISABLE_RAMP_AT_DECEL = 0x80,		// E-Stop Ramping to stop
     /*!
        This stop type will prevent future motion and set the <i>Output
        Register</i> enable state to disabled.

        This node stop can be canceled by a directed [NodeStopClear](@ref sFnd::IMotion::NodeStopClear)
		command.
    **/
    STOP_TYPE_ESTOP_DISABLE_RAMP =0x91, // E-Stop/ramp to 0/node disables
	// - - - - - - - - - - - - - - - - -
    // Node Stop Modifier Clear Masks
	// - - - - - - - - - - - - - - - - -				/** \cond ISC_DOC **/ 
	/*!
        This stop type clears the E-Stop condition, which blocks the motion
        commands from running.
    **/
    STOP_TYPE_CLR_ESTOP     =0x18,		// Node stop clear (E-Stop)
														
    /*!
        This stop type clears the Controlled Output condition.
    **/
    STOP_TYPE_CLR_CTRLD     =0x28,		// Restore output register state
														
    /*!
        This stop type clears the Disabled Stop condition.
    **/
    STOP_TYPE_CLR_DISABLE   =0x88,		// Restore enable state
    /*!
        This stop type clears all latched condition such as the
        <i>E-Stop</i> and the <i>Controlled Output</i> conditions.
    **/
    STOP_TYPE_CLR_ALL       =0xf8		// Clear all modifiers
														/** \endcond **/
};
/// \copybrief _nodeStopCodes
typedef enum _nodeStopCodes nodeStopCodes;
//                                                                            *
//*****************************************************************************


//*****************************************************************************
// NAME                                                                       *
//      mgNodeStopReg union
//
// DESCRIPTION
/**
    \brief Universal Node Stop Action Register Definition

    This container type is used to describe the actions taken when a node
	stop is issued. The node stop system can affect multiple features within
	the node.
	
	The following items are affected by issuing a node stop.
	- Motion
	- Output or Drive Enable States
	- Status

	The motion portion of the node stop specified by the \a Style field
	specifies the action to be taken by the move generator. These include:
	- Immediate cancellation of the motion and any queued motion
	- Ramp the current velocity to zero at a pre-specified deceleration rate
	and cancel queued motion 
	- Not to perform any action at all

	The \a EStop field controls what happens with subsequent motion requests. If
	non-zero, all motion requests will be rejected. This field is also referred
	to as the \a MotionLock field and the terms are interchangeable.

	For nodes that have a <i>General Purpose Output</i> feature, the 
	\a Controlled field defines the action taken by the outputs. For
	ClearPath-SC this does nothing.

	The \a Disable field controls the node's servo state at the end of
	motion. If non-zero, the motor will disable and enter the dynamic brake 
	mode.

	The \a Quiet field controls the posting of the Node Stop event to the 
	status register. 

	The \a EStop, \a Controlled and \a Disable states latch upon execution 
	block their respective features from operating in the normal fashion. 
	\a EStop in particular is a safety feature to prevent inadvertent motion
	to occur in your application until the originating reason has been cleared.

	To restore operations each of these features can be cleared by setting
	the \a Clear field to non-zero and setting the respective other field to
	non-zero and issuing a node stop. The STOP_TYPE_CLR_ALL enumeration
	provides a convenient way to clear all blocking actions.

	The commonly used node stops are available as enumerated constants 
	defined in the #nodeStopCodes enumeration.
**/
union _mgNodeStopReg {

																/** \cond ISC_DOC **/
	/**
		\brief Universal Node Stop Register Field Definitions.

		\copydetails _mgNodeStopReg

		\note Not all models support all options.
	**/
	struct _nodeStopFlds {
		/**
			\brief Move Generator Action Taken.

			This is what the motion generator will do for all non-Clearing 
			actions.
		**/
		mgStopStyles Style : 3;
		// - - - - - - - - - - - -
		// Stop type modifiers
		// - - - - - - - - - - - -
		/**
			\brief Clear latched modifier(s).

			This is used to restore operations after \a EStop, 
			\a Controlled, or \a Disable node stop has been issued. Set this
			field with along with the modifier you wish to clear. The \a
			Style field is ignored when this is set.
		**/
		unsigned Clear : 1;
		/**
			\brief Lockout subsequent motion.
			
			Prevent subsequent motion after node stop issued until the latch
			is cleared. This is sometimes called the E-Stop or MotionLock
			state. Clear this condition by issuing a node stop with this field
			set and the \a Clear field set.
		**/
		unsigned EStop : 1;
		/**
			\brief Force the <i>Output Register</i> to the <i>Controlled
			Output Register</i> state.

			Prevent subsequent output register changes until cleared. Clear 
			this condition by issuing a node stop with this field
			set and the \a Clear field set.

			\note This is Meridian-ISC only feature
		**/
		unsigned Controlled : 1;
		/**
			\brief Suppress update of <i>Status Register</i>.
			
			This makes this node stop a stealth <i>Node Stop</i>.
		**/
		unsigned Quiet : 1;
		/**
			\brief Force drive to disable after the <i>Motion Action</i> completes.

			Clear this condition by issuing a node stop with this field
			set and the \a Clear field set.
		**/
		unsigned Disable : 1;
	};													/** \endcond **/ 
	/** 
		\brief Bit-wide access to the Node Stop information.
	**/
	nodeStopCodes bits;
														/** \cond INTERNAL_DOC **/
	/**
		\brief Universal field view of the stop types.
	**/
	struct _nodeStopFlds fld;
														/** \endcond **/
	/**
		\brief ClearPath-SC field view of the stop types.
	**/
	cpmNodeStopFlds cpm;
														/** \cond ISC_DOC **/
	/**
		\brief Meridian ISC field view of the stop types.
	**/
	iscNodeStopFlds isc;
														/** \endcond **/
	#ifdef __cplusplus
														/** \cond INTERNAL_DOC **/
		// Constructors
		_mgNodeStopReg(int how) {
			bits = static_cast<nodeStopCodes>(how);
		};
		// Convert convenience \e mgStopCodes code to \e mgStopType.
		_mgNodeStopReg() {
			bits = static_cast<nodeStopCodes>(MG_STOP_STYLE_ABRUPT);
		};
														/** \endcond **/
		/**
			Convert the convenience \e mgStopCodes code to \e mgStopType.

			\param[in] how Enumerated value to setup node stop register.
		**/
		_mgNodeStopReg(nodeStopCodes how) {
			bits = static_cast<nodeStopCodes>(how);
		};
	#endif
};
/// \copydetails _mgNodeStopReg
typedef union _mgNodeStopReg  mgNodeStopReg;
														/** \cond INTERNAL_DOC **/
// Legacy alias
#define nodeStopTypes mgNodeStopReg
														/** \endcond **/
//                                                                            *
//*****************************************************************************


#ifdef _MSC_VER
#pragma pack(pop)                   // Restore to byte alignment boundaries
#endif

#endif   // __PUBMOTION_H__
//=============================================================================
//  END OF FILE pubMotion.h
//=============================================================================

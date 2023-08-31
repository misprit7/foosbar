//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubNetAPI.h $
// $Revision: 30 $ $Date: 12/09/16 4:05p $
// $Workfile: pubNetAPI.h $
//
// DESCRIPTION:
/**
	\file	
	\brief Network Constants and types for interacting with nodes.



**/
// CREATION DATE:
//		2015-12-02 16:32:18
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2015  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
#ifndef __PUBNETAPI_H__
#define __PUBNETAPI_H__


//*****************************************************************************
// NAME																          *
// 	pubNetAPI.h headers
//
	#include "tekTypes.h"

#ifndef __TI_COMPILER_VERSION__
	#include "pubCoreRegs.h"
	#include "mnErrors.h"
	#include "pubIscRegs.h"
#endif
	#include <string.h>
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	pubNetAPI.h constants
//
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NETWORK DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	The internally assigned unique address type for a node.

	The system initialization assigns this address for it's internal purposes. 
	It should only be used for error messages and the arguments of legacy
	C-based API calls.
**/
typedef Uint16 multiaddr;		// Combined net and node address
/**
	The internally assigned index type of a node on a serial port.

	Node are numbered from zero, starting from the transmit port of the host.
**/
typedef Uint16 nodeaddr;		// Node's address on its net
/**
	The serial port index type. 
	
	This corresponds to the index supplied during system initialization.
**/
typedef Uint16 netaddr;			// Net number (a.k.a. cNum)
/// The numeric value for an unassigned multiaddr.  
#define MN_UNSET_ADDR multiaddr(0xffffu)		// The illegal address value 
													/** \cond INTERNAL_DOC **/
/// Maximum # of serial ports we can specify.
#define NET_CONTROLLER_MAX 3 
/// Maximum # of nodes on a port
#define MN_API_MAX_NODES		16U

/// Maximum size of the user non-volatile data area
#define MN_USER_NV_SIZE		13U
													/** \endcond **/


/**
	\brief This enumeration describes the port open state. 
	
	Operations to nodes may be performed when the port is in the OPENED_ONLINE
	state.
**/
typedef enum _openStates {
	/**
	We do not know the current state
	**/
	UNKNOWN,
	/**
	A previously opened port was shutdown.
	**/
	CLOSED,
	/**
	The port is opened in FLASH mode.
	**/
	FLASHING,
	/**
	The port requested does not exist or is currently owned by another
	application.
	**/
	PORT_UNAVAILABLE,
	/**
	The port is opened and initial detection of nodes resulted
	in no detected nodes.  Communications with nodes will result
	in failures.
	**/
	OPENED_SEARCHING,
	/**
	The port is opened and we have detected nodes. Interactions with
	nodes can occur normally.
	**/
	OPENED_ONLINE
} openStates;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//							NODE/NET INFORMATION V2
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


#ifndef __TI_COMPILER_VERSION__
//*****************************************************************************
//																			  *
/**
	\brief The type of nodes attached to this port. 

	This list defines the types of adapters attached directly to the serial
	port. For ClearPath-SC systems this is usually the CPM_COMHUB.
**/
typedef enum _portTypes {
	/**
		Unspecified nodes
	**/
	UNKNOWN_NODES,				
	/**
		Meridian Family CON-MOD connection
	**/
	MERIDIAN_CONMOD,			
	/**
		ClearPath-SC Communications Hub connection
	**/
	CPM_COMHUB,	
	/**
		Node connected via generic serial connection
	**/
	GENERIC_SERIAL
} portTypes;
//																			  *
//*****************************************************************************

													/** \cond INTERNAL_DOC **/
//*****************************************************************************
//																			  *
/**
	\brief Container used to specify the communications port and operational 
	speed.
**/
typedef struct _controllerSpec {
	netRates PortRate;				///< Network Operational rate
#if defined(_WIN32)||defined(_WIN64)
	Uint16 PortNumber;              ///< Port number
#else
	char PortName[MAX_PATH];			///< Port device path
#endif
#ifdef __cplusplus
	_controllerSpec() {
		PortRate = MN_BAUD_108X;
	}
	#if defined(_WIN32)||defined(_WIN64)
		/**
			\brief Convenience Constructor
			\param number COM port number for this channel.
			\param rate Desired operational rate for this channel.
		**/ 
        _controllerSpec(Uint16 number, netRates rate = MN_BAUD_108X) {
			PortNumber = number;
			PortRate = rate;
        }
	#else
		/**
			\brief Convenience Constructor
			\param name Ptr to serial port device.
			\param rate Desired operational rate for this channel.
		**/ 
		_controllerSpec(const char * name, netRates rate = MN_BAUD_108X);
	#endif
#endif
} controllerSpec;
//																			  *
//*****************************************************************************
													/** \endcond **/

//*****************************************************************************
//																			  *
														/** \cond INTERNAL_DOC **/
/**
	\brief Serial Port specification type
	
	This container is used to specify the communications port, operational 
	speed and network type for a Meridian or ClearPath-SC network.
**/
struct MN_EXPORT _portSpec {
#if defined(_WIN32)||defined(_WIN64)
	netRates PortRate;				///< Network Operational rate
	Uint16 PortNumber;              ///< Port number
	portTypes PortType;				///< Type of port and nodes
#else
	netRates PortRate;				///< Network Operational rate
	char PortName[MAX_PATH];		///< Port device path
	portTypes PortType;				///< Type of port and nodes
#endif // defined(_WIN32)||defined(_WIN64)
#ifdef __cplusplus
	#if defined(_WIN32)||defined(_WIN64)
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// WINDOWS CONSTRUCTION
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
													/** cond INTERNAL_DOC **/
		/// Generic construction
		_portSpec() {
			PortRate = MN_BAUD_12X;
			PortNumber = 0;
			PortType = UNKNOWN_NODES;
		}
		/// Convert older buffer to new one
		_portSpec operator=(const _controllerSpec &oldForm) {
			PortRate = oldForm.PortRate;
			PortNumber = oldForm.PortNumber;
			PortType = MERIDIAN_CONMOD;
			return(*this);
		}
													/** endcond **/
		/**
			\brief Convenience Constructor
			\param[in] number COM port number for this channel.
			\param[in] rate Desired operational rate for this channel.
			\param[in] thePortType The type of adapter attached to this serial
			port.
		**/ 
        _portSpec(Uint16 number, 
				  portTypes thePortType,
				  netRates rate = MN_BAUD_108X) {
			PortNumber = number;
			PortType = thePortType;
        }
	#else  
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// NON-WINDOWS CONSTRUCTION (QNX/Linux)
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
													/** cond INTERNAL_DOC **/
		_portSpec() {
			PortRate = MN_BAUD_12X;
			PortName[0] = 0;
			PortType = UNKNOWN_NODES;
		}
		/**
			\brief Convenience Constructor
			\param name Ptr to serial port device.
			\param rate Desired operational rate for this channel.
		**/ 
        _portSpec(const char *name,  
						portTypes theNodeTypes,
						netRates rate = MN_BAUD_108X){
			PortRate = rate;
			PortType = theNodeTypes;
			strncpy(PortName, name, MAX_PATH);
		}
		_portSpec operator=(const _controllerSpec &oldForm) {
			PortRate = oldForm.PortRate;
			PortType = MERIDIAN_CONMOD;
			strncpy(PortName, oldForm.PortName, MAX_PATH);
			return(*this);
		}
													/** endcond **/
	#endif
#endif	//__cplusplus
};

/// \copybrief _portSpec
typedef struct _portSpec portSpec;
#endif // __TI_COMPILER_VERSION__

													/** \endcond  **/
//																			  *
//*****************************************************************************







//*****************************************************************************
// NAME	
//	BrakeControl enum
/**
	\brief State for the brake control feature. 
**/
enum _BrakeControls
{
	/**
		Brake activates automatically depending on the state of the node.  BrakeNum 0 will correspond to Brake 1 on the SC hub.  BrakeNum 1 will correspond to Brake 2 on the SC hub
	**/
	BRAKE_AUTOCONTROL,
	/**
		Engage the brake.
	**/
	BRAKE_PREVENT_MOTION,
	/**
		Disengage the brake
	**/
	BRAKE_ALLOW_MOTION,
	/**
		Use the brake output as a GPO that is asserted.
	**/
	GPO_ON,
	/**
		Use the brake output as a GPO that is deasserted.
	**/
	GPO_OFF
};
/// \copybrief _BrakeControls
typedef enum _BrakeControls BrakeControls;
//																			   *
//******************************************************************************





#ifndef __TI_COMPILER_VERSION__
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Standard Error Object
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** 
	Namespace for sFoundation class library.
**/
namespace sFnd {
/**
	\brief Error code and reason structure.
	
	Error code and reason structure.  When the sFoundation class library encounters an error, this item will be
	thrown.  This item can be used to identify the error, the address reporting the error, and get a brief description of the error.


	\CODE_SAMPLE_HDR
	try	{
		// Do some library function
	}
	// If there is a error print out details
	catch (mnErr theErr) {
		cout << theErr.TheAddr;
		cout << theErr.ErrorCode;
		cout << theErr.ErrMsg;
	}
	\endcode
**/
	typedef struct _mnErr {
		/**
			Address reporting the error.
		**/
		multiaddr TheAddr;
		/**
			The system Error Code number.
		**/
		cnErrCode ErrorCode;
		/**
			A printable error string that includes the ErrorCode.

			<table class="tableizer-table">
			<tr class="tableizer-firstrow"><th>cnErrCode</th><th>Value</th><th>Description</th></tr>
			<tr><td>MN_ERR_TIMEOUT,</td><td> 0x80040002</td><td>Failed due to time-out while waiting for attention.</td></tr>
			<tr><td>MN_ERR_DEV_ADDR,</td><td> 0x80040004</td><td> Device addressing problems.
			- Check that you are not trying to send commands to a port that does not exist.
			- Only address ports [0-2].</td></tr>
			<tr><td>MN_ERR_TOO_MANY,</td><td> 0x80040005</td><td>There are too many nodes on the net(16 Max).</td></tr>
			<tr><td>MN_ERR_RESP_FMT,</td><td> 0x80040006</td><td>Response is garbled. This indicates serious com issues.
			- Ensure network power supply is stable.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_OFFLINE,</td><td> 0x80040008</td><td>The node went offline.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_PARAM_RANGE,</td><td> 0x80040009</td><td>Parameter is out of range.</td></tr>
			<tr><td>MN_ERR_CLOSED,</td><td> 0x8004000c</td><td>Attempting to use a closed port/node.
			- Ensure ports are opened before accessing.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_PKT_ERR,</td><td> 0x80040011</td><td>Packet length does not equal buffer length.
			- Check for issues with the serial port.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_CMD_OFFLINE,</td><td> 0x8004001b</td><td>Command attempt while net offline.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_ATTN_OVERRUN,</td><td> 0x8004001e</td><td>Attention over-run error.  This indicates one or more attentions may have been lost due to incorrect attention aetup handling.
			- Ensure attentions are enabled at the [IAttnPort](@ref sFnd::IAttnPort) level.</td></tr>
			<tr><td>MN_ERR_BADARG,</td><td> 0x80040027</td><td>API function passed bad argument</td></tr>
			<tr><td>MN_ERR_SEND_FAILED,</td><td> 0x8004002a</td><td>The host serial port rejected a command.
			- Check that the host serial port is operating properly.</td></tr>
			<tr><td>MN_ERR_RESP_TIMEOUT,</td><td> 0x8004002e</td><td>Response from Node not found in time.
			- Ensure network power supply is stable.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_CANCELED,</td><td> 0x80040030</td><td>Command canceled during Node restart.</td></tr>
			<tr><td>MN_ERR_ADDR_RANGE,</td><td> 0x80040034</td><td>Response from out of range address. A node likely went offline.
			- Ensure network power supply is stable.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_RESET_FAILED,</td><td> 0x8004003b</td><td>Reset command failed, node isn't responding.
			- Ensure network power supply is stable.
			- Check conections, and wiring for intermittent connectivity issues.</td></tr>
			<tr><td>MN_ERR_FILE_WRITE,</td><td> 0x80040040</td><td>Configuration file failed to write to file location.
			- Check for correct directory.
			- Check for appropriate write-access for specified directory.
			- Check for adequate disk space.</td></tr>
			<tr><td>MN_ERR_FILE_OPEN,</td><td> 0x80040041</td><td>Configuration file failed to open.
			- Check for correct directory.
			- Check for appropriate read/write-access for specified directory.</td></tr>
			<tr><td>MN_ERR_FILE_WRONG,</td><td> 0x80040042</td><td>Configuration file is for wrong node.
			- Ensure the configuration file specified is for the correct Node type.</td></tr>
			<tr><td>MN_ERR_FILE_BAD,</td><td> 0x80040043</td><td>Configuration file corrupt   </td></tr>
			<tr><td>MN_ERR_FILE_ENABLED,</td><td> 0x80040044</td><td>Configuration file load requires node to be disabled.
			- Ensure node is disabled before attempting to load configuration file.</td></tr>
			<tr><td>MN_ERR_CMD_IN_ATTN,</td><td> 0x80040045</td><td>Node command attempted from within an Attention Callback.
			- Ensure your registered attention callback functions do not attempt to send commands to any node.</td></tr>
			<tr><td>MN_ERR_APP_NET_ONLY,</td><td> 0x80040046</td><td>Feature only available on App Net</td></tr>
			<tr><td>MN_ERR_24V_OFF,</td><td> 0x80000047</td><td>Canceled due to lack of 24V supply.
			- Ensure communication power supply is stable, and turned on.</td></tr>
			<tr><td>MN_ERR_CMD_ARGS,</td><td> 0x80040102</td><td> Illegal or missing command args</td></tr>
			<tr><td>MN_ERR_CMD_WR_FAIL,</td><td> 0x80040103</td><td> Attempt to write to a read-only parameter.</td></tr>
			<tr><td>MN_ERR_CMD_EEHW,</td><td> 0x80040104</td><td>Non-Volatile memory failure.
			- Check conections, and wiring for intermittent connectivity issues.
			- If problem persists, replace motor.</td></tr>
			<tr><td>MN_ERR_CMD_ACCESS_LVL,</td><td> 0x80040105</td><td>Access level violation.
			- Make sure that the diagnostic USB channel is in moniter mode through ClearView.</td></tr>
			<tr><td>MN_ERR_CMD_MV_FULL,</td><td> 0x80040106</td><td> Move Buffers full.
			- Allow moves in buffer to complete before adding additional moves.</td></tr>
			<tr><td>MN_ERR_CMD_MV_SPEC,</td><td>0x80040107</td><td>Move specification error.
			- Check that the commanded velocity is below the velocity limit for the node.
			- Ensure that the move distance is within the soft limits of the motor, and does not exxceed the numberspace (~8,000,000).</td></tr>
			<tr><td>MN_ERR_CMD_MV_ESTOPPED,</td><td>0x80040108</td><td>Motion attempt in E-Stopped state.
			- Clear E-Stop state before commanding motion.</td></tr>
			<tr><td>MN_ERR_CMD_MV_RANGE,</td><td>0x80040109</td><td>Motion attempt is out of range.
			- Ensure that the move distance is within the soft limits of the motor, and does not exxceed the numberspace (~8,000,000).</td></tr>
			<tr><td>MN_ERR_CMD_MV_SHUTDOWN,</td><td>0x8004010a</td><td> Motion attempt when drive is in shutdown.
			- Appropriatly clear shutdown before commanding motion.</td></tr>
			<tr><td>MN_ERR_CMD_MV_BLOCKED,</td><td>0x8004010c</td><td> Motion blocked non-disabling shutdown(ie: limit switches) present.
			- Remove cause of non-disabling shutdown.
			- Ensure commanded motion is in valid direction(ie: not attempting to move into an already asserted limit).</td></tr>
			<tr><td>MN_ERR_NODE_IN_MOTION,</td><td>0x80040110</td><td>Drive is in motion. Some node commands cannot be called during motion(ie: Initiating homing). </td></tr>
			<tr><td>MN_ERR_OFFLINE_XX,</td><td>0x800403a0-0x800403af</td><td> Node X detected offline Detected offline nodes are reported with this group. These are usually caused by a node restarting via communication power or firmware error.</td></tr>
			<tr><td>MN_ERR_INIT_FAILED_PORTS_X,</td><td>0x80040600-0x80040607</td><td> Port X opened OK but no nodes were found attached to the link.</td></tr>
			<tr><td>MN_ERR_PORT_FAILED_PORTS_X</td><td>0x80040700-0x80040707</td><td> Port  X failed to open.
			- Make sure you specify a working port.
			- Check that the USB/Serial cable is properly inserted.
			- Ensure no other applications are activly using the port(ie: ClearView).</td></tr>
			<tr><td>MN_ERR_BAUD_FAILED_PORTS_X</td><td> 0x80040800 - 0x80040807</td><td> Port X opened OK but doesn't support the requested baud rate.
			- Check that the serial port supports your requested baud rate.
			- If using the SC-Hub, use a baud rate of MN_BAUD_12X, or MN_BAUD_24X.</td></tr>
			</table>
		**/
		char ErrorMsg[512];
	} mnErr;
}
//																			  *
//*****************************************************************************
#endif



#ifndef __TI_COMPILER_VERSION__
//*****************************************************************************
// NAME	
//	ShutdownInfo struct
/**
	\brief Information to setup group shutdowns at a node.

	This is the main struct for the <i>Auto-shutdown </i> feature.  The struct consists of three parts:
	- Whether or not the node should respond to a Group Shutdown.
	- The Node Stop type for this node.
	- The [Status Register](@ref mnStatusReg) mask for this Node which will trigger a Group shutdown.

	\CODE_SAMPLE_HDR

	// The following lines of code show an example of how to create a Shutdown info object.

	// First we create the status mask object consisting of which status events on the node will trigger a group shutdown.
	// In this case we are using "AlertPresent", "Disabled", and "Input A"

	mnStatusReg myShutdownStatusMask;						//Create Status mask
	myShutdownStatusMask.cpm.AlertPresent = 1;				//Set desired fields
	myShutdownStatusMask.cpm.Disabled = 1;
	myShutdownStatusMask.cpm.InA = 1;

	// Next we create the ShutdownInfo object we will use to configure the Node with
	ShutdownInfo myShutdownInfo;							//Create ShutdownInfo Object
	myShutdownInfo.enabled = true;							// This sets the node to respond to Group Shutdowns
	myShutdownInfo.theStopType = STOP_TYPE_ABRUPT;			//Use an Abrupt Nodestop type for this node in response to a group shutdown
	myShutdownInfo.statusMask = myShutdownStatusMask;		// Configure the node to issue a group shutdown automatically based on our status mask

	//Finally we use configure the Port's first Node (Node 0) with our shutdown object
	myport.GrpShutdown.ShutdownWhen(0,myShutdownInfo);

	\endcode

	\see \ref CPMnodeStopPage
**/
struct _ShutdownInfo {
	/**
		Enables checking at this node.  If this is set to false, the node will not shutdown during a group shutdown.
	**/
	nodebool enabled;
	/**
		The kind of stop to assert when this feature fires off.
		\see \ref CPMnodeStopPage for a complete list of Node Stop types.
	**/
	mgNodeStopReg theStopType;
	/**
		Items to trigger a Group Shutdown stop when the Node's status register rises on the unmasked 
		bits of the \a statusMask. Set the bits to non-zero to enable the
		detection of this event.
	**/
	mnStatusReg statusMask; 
#ifdef __cplusplus
													/** \cond INTERNAL_DOC **/
	/// Construct our ClearPath COM Hub brake implementation
	_ShutdownInfo() {
		enabled = false;
	}
													/** \endcond **/
#endif
};
/// \copybrief _ShutdownInfo
typedef struct _ShutdownInfo ShutdownInfo;
//																			   *
//******************************************************************************
#endif // __TI_COMPILER_VERSION__

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Attention Request/Status Register
// 
// NOTE: arrays of this items will involve 32-bit spacings
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
													/** \cond SC_EXPERT **/
/**
	\brief <i>Attention Request</i> Register and Source Container.

	This register is the representation of an <i>Attention Packet
	</i>. 

	An \e AttentionReg.lBits equal to zero occurs when the function
	did not successfully get an attention.

	\see sFnd::IAttnPort::WaitForAttn
**/
typedef struct _mnAttnReqReg {
	multiaddr MultiAddr;				///< Node signaling change
	attnReg AttentionReg;				///< Status signaled
} mnAttnReqReg;
													/** \endcond  **/

#ifndef __TI_COMPILER_VERSION__
													/** \cond INTERNAL_DOC **/
//*****************************************************************************
// NAME																	      *
// 	pubNetAPI.h function prototypes
//
/**
	\brief C Based initialization function
	
	This is the expanded initializing function for combination Meridian 
	and ClearPath-SC systems.
**/
MN_EXPORT cnErrCode MN_DECL mnInitializeSystem(
			nodebool resetNodes,			// Set to reset nodes at startup
			nodeulong netCount,				// Number of nets to initialize (1..2)
			const portSpec controllers[]);	// Ptr to controller list

//																			  *
//*****************************************************************************
													/** \endcond **/
#endif

#endif // __PUBNETAPI_H__
//============================================================================= 
//	END OF FILE pubNetAPI.h
//=============================================================================

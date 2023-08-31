//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubMnNetDef.h $
// $Revision: 141 $ $Date: 12/09/16 4:01p $
// $Workfile: pubMnNetDef.h $
//
// DESCRIPTION:
///		\file
///		\brief Meridian Network definitions and data types.
///
///		Meridian Networked Product definitions for link layer and user APIs.
//
// CREATION DATE:
//		05/18/1998 12:44:53	  	- originally cnPub.h
//		June 05, 2009  18:18:13 - Refactored and improvements for Meridian 
//								  products
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************

#ifndef __PUBMNNETDEF_H__
#define __PUBMNNETDEF_H__

//*****************************************************************************
// !NAME!																      *
// 	pubMnNetDef.h headers included
//
	#include "tekTypes.h"
//
//*****************************************************************************




//*****************************************************************************
// !NAME!																	  *
// 	pubMnNetDef.h function prototypes
//

//
//*****************************************************************************



//*****************************************************************************
// !NAME!																	  *
// 	pubMnNetDef.h headers

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NETWORK DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
	\brief Supported communication rates.
	\public
	\ingroup SetupGrp

	Select one of these items to match the capabilities of the serial port
	attached to your network.
**/
typedef enum _netRates {
	MN_BAUD_1X  =9600,			///< 1x Net Speed
	MN_BAUD_12X =115200,		///< 12x Net Speed
	MN_BAUD_24X =230400,		///< 24x Net Speed
	MN_BAUD_48X =460800,		///< 48x Net Speed
	MN_BAUD_96X =921600,		///< 96x Net Speed
	MN_BAUD_108X=1036800		///< 108x Net Speed
} netRates;						///< Type for netRates

/// \cond INTERNAL_DOC

// Initial and Post Break Network Baud Rate
#define MN_NET_BAUD_RATE		MN_BAUD_1X
// Divider to convert to speed switch argument
#define MN_NET_RATE_DIVIDER		9600
// # msec for break operation
#define MN_BREAK_DURATION_MS 	40U
#define MN_BREAK_EXTEND_MS 		4U

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// PACKET DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// Max # of nodes on a network
#define MN_API_ADDR_MASK		(MN_API_MAX_NODES-1)
// This macro is TRUE if the <x> character is a start of packet type
#define MN_API_IS_START_PKT(x)  (x&0x80)
// Max # of 8-bit bytes in user payload
#define MN_HDR_LEN_MASK			0x1fU	// Note: 8<->7 conversion rounding
// Max 8-bit chars in MN_HDR_LEN_MASK length 7-bit packet
#define MN_API_PAYLOAD_MAX  	((MN_HDR_LEN_MASK*7)/8)
// Header length characters
#define MN_API_PACKET_HDR_LEN	2U
// Post payload character count
#define MN_API_PACKET_TAIL_LEN	1U
// Total maximum 8-bit packet length (less checksum)
#define MN_API_PACKET_MAX	(MN_API_PACKET_HDR_LEN+MN_API_PAYLOAD_MAX)
// Total maximum 7-bit items in 8-bit buffers + checksum
#define MN_NET_PACKET_MAX	(MN_API_PACKET_HDR_LEN+MN_HDR_LEN_MASK\
					        +MN_API_PACKET_TAIL_LEN)

#define FRAME_WRITE_TIMEOUT 1000U	// max # ms for a packet to send
#define FRAME_READ_TIMEOUT  1000U	// max # ms for to wait for packet
#define MN_MAX_RESET_TIME	1200U	// max # ms for node to reset

// Internal 'character' sizes for various "string-like" items
#define MN_USER_ID_SIZE		13U		// User ID maximum size
#define MN_USER_ID_SIZE_V2	25U		// User ID maximum size	(Net Protocal V2 Nodes)
#define MN_FILENAME_SIZE	25U		// Filename maximum size
#define MN_PART_NUM_SIZE	(MN_API_PAYLOAD_MAX-2)	// Part # string 

// Hinted reasonable buffer size. 
#define MN_UI_STR_BUF_SIZE	64U

// Fixed locations in <packetbuf>.Byte.Buffer
#define ADDR_LOC			0U		// Address location
#define LEN_LOC				1U		// Frame length location
#define CMD_LOC				2U		// Command location
#define EXTEND_CODE_LOC		2U		// Command location
#define RESP_LOC			2U		// Start of the response area
#define RESP_BUF_START		0U		// Location response data in buf->data

// type: field definitions
typedef enum _mnPktType {
	// Frames of these types honor flow control
	MN_PKT_TYPE_CMD,				// 0 Command 					ND_ERR_NET,			
	MN_PKT_TYPE_RESP,				// 1 Command Response 			ND_ERR_CMDOK,		
	MN_PKT_TYPE_ERROR,				// 2 Error Detected 			ND_ERR_LOGIC,		
	MN_PKT_TYPE_EXTEND_LOW,			// 3 Extension, low priority	ND_ERR_FRAME,		
	// Frames of these types do not honor flow control				
	MN_PKT_TYPE_ATTN_IRQ=-4,		// 4 Attn Request				ND_CTL_ATTN_REQ,	
	MN_PKT_TYPE_SET_ADDR=-3,		// 5 Set Address				ND_CTL_SET_ADDR,	
	MN_PKT_TYPE_TRIGGER=-2,			// 6 Event Trigger (i.e. Go)	ND_CTL_GO,			
	MN_PKT_TYPE_EXTEND_HIGH=-1		// 7 Extension, high priority	ND_CTL_EXT			
} mnPktType;

// Macro to determine high-priority packet type
#define MN_PKT_IS_HIGH_PRIO(pktType)	((pktType&4)!=0)


// src: field definitions
typedef enum _mnSrc {
	MN_SRC_HOST,					// 0 Host initiated packet
	MN_SRC_NODE=1					// 1 Node initiated packet (1 bit, so use -1)
} mnSrc;

// User API all purpose transmission / reception buffer. The field <bufSize> contains
//   the current length of <buffer> in 8-bit quantities. 
#ifdef __cplusplus
	#define MN_TYPE 	mnPktType
	#define MN_SRC		mnSrc
#else
	#define MN_TYPE		int
	#define MN_SRC		int
#endif
/// \endcond

/// Packet field definitions for use as an overlay for parsing.
typedef struct _packetFields {
	/// Destination/source address
	unsigned Addr		: 4;
	/// Packet Type	
	MN_TYPE PktType  	: 3;
	/// Start of packet indicator	
	unsigned StartOfPacket	: 1;
	#ifdef __TI_COMPILER_VERSION__
		/// unpacked high order bits, set to 0
		int Padding		: 8;	
	#endif
	/// Number of items in \p buffer
	unsigned PktLen     : 5;
	/// pkt specific modifier, otherwise 0	
	unsigned Mode	  	: 1;
	/// Command source field	
	unsigned Src		: 1;
	/// Always 0	
	int	Zero1			: 1;	
	#ifndef __ASM_HEADER__
		#ifdef __cplusplus
		/// Setup base and unused fields in a packet.
		void SetupHdr() {
			Mode = Zero1 = 0;
			StartOfPacket = 1;
		};

		/// Setup headers for a fully specified packet type
		void SetupHdr(MN_TYPE thisType, unsigned thisAddr, MN_SRC thisSrc) {
			SetupHdr();
			PktType = thisType;
			Addr = thisAddr;
			Src = thisSrc;
		};

		/// Setup headers for a packet for a host packet.
		void SetupHdr(MN_TYPE thisType, unsigned thisAddr) {
			SetupHdr(thisType, thisAddr, MN_SRC_HOST);
		};
		#endif
	#endif
} packetFields;				///< Type for packet fields

/// Packet buffer used for driver internal and external APIs.
typedef union _packetbuf {
	struct _pktBytes {
		/// Buffer of octets to or from the link
		nodechar Buffer[MN_NET_PACKET_MAX];
		/// Count of characters in \p Buffer
		nodeulong BufferSize;			// Buffer size now
	} Byte;								///< Octet accessor.
	/// Packet field access. 
	packetFields Fld;
	// bufferSize should be less than MN_NET_PACKET_MAX
#ifdef __cplusplus
	// C++ Construct with all zeros
	_packetbuf() {
		for (unsigned int i=0; i<MN_NET_PACKET_MAX; i++) {
			Byte.Buffer[i] = 0;
		}
		Byte.BufferSize = 0;
	}
	_packetbuf(void*){
	}
#endif
} packetbuf;

/// Option bit to OR into a parameter number to allow access to special
/// handling for the particular types of parameters. The most common use
/// is for parameters that have a non-volatile shadow value. Setting
/// this mask will access the non-volatile part of the parameter without
/// affecting the run-time value.
#define PARAM_OPT_MASK	0x80			// Definition of parameter # option bit

///	@}


/// \cond INTERNAL_DOC
 
// MN_PKT_TYPE_EXTEND_HIGH sub types
typedef enum _mnCtlExtHigh {
	MN_CTL_EXT_NOP,					 // 0  Do Nothing
	MN_CTL_EXT_RESET,				 // 1  Reset Node
	MN_CTL_EXT_REV_ADDR,			 // 2  Reverse address node
	MN_CTL_EXT_NODE_STOP,			 // 3  E-Stop nodes
	MN_CTL_EXT_NET_DIAG_INFO,		 // 4  Network diagnostic result
	MN_CTL_EXT_NET_CHECK,			 // 5  Cross-net integrety check
	MN_CTL_EXT_CRASH_TEST,			 // 6  Uncontrolled reset of node
	MN_CTL_EXT_WATCHDOG_TERMINATION, // 7  Elicit reset via watchdog timer
	MN_CTL_EXT_ENTER_MULTI_BYPASS,	 // 8  Put all but one node in bypass mode
	MN_CTL_EXT_BAUD_RATE			 // 9  Change network baud rate
} mnCtlExtHigh;

// MN_PKT_TYPE_EXTEND_LOW sub types
typedef enum _mnCtlExtLow {
	MN_CTL_EXT_LOW_NOP,				 // 0  Do Nothing
	MN_CTL_EXT_DATA_ACQ,			 // 1  Data acquistion packet
	MN_CTL_EXT_PARAM_CHANGED,		 // 2  Changed parameter(s) notification
	MN_CTL_EXT_HOST_ALIVE			 // 3  Host "ping" packet
} mnCtlExtLow;

//  Packet field definitions for use as overlay to aid parsing
typedef struct _extendedFields {
	mnCtlExtHigh Code	: 7;	// Extended code
	int unused			: 1;	// 0
	#if !(defined(_WIN32)||defined(_WIN64))
		int Padding		: 8;	// unpacked high order bits, set to 0
	#endif
	unsigned OptData	: 8;	// 
} extendedFields;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ERROR PACKET DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef enum _mnErrClasses {
	ND_ERRCLS_NET,				// 00: Network detected error
	ND_ERRCLS_CMD,				// 01: Command processing error
	ND_ERRCLS_EXTEND=3			// 03: Extended error classes
} mnErrClasses;


typedef enum _mnNetErrs {
	ND_ERRNET_FRAG,				// 00: Packet fragment detected
	ND_ERRNET_CHKSUM,			// 01: Bad checksum detected
	ND_ERRNET_STRAY,			// 02: Stray Data Found
	ND_ERRNET_PORT_OVERRUN,		// 03: Serial Port overran receive
	ND_ERRNET_FRAME,			// 04: Serial Port detected a framing error
	ND_ERRNET_RXPARITY,			// 05: Serial Port detected a parity error
	ND_ERRNET_BABBLE			// 06: Net detected babble
} mnNetErrs;

typedef enum _mnCmdErrs {
	ND_ERRCMD_INTERNAL,			// 00: Generic internal error
	ND_ERRCMD_CMD_UNK,			// 01: Command unknown on this node
	ND_ERRCMD_ARGS,				// 02: Illegal or missing command args
	ND_ERRCMD_WR_FAIL,			// 03: Attempt to write read-only info
	ND_ERRCMD_EEHW,				// 04: NVM broken
	ND_ERRCMD_ACCESS_LVL,		// 05: Insufficient access level
	// The following MV errors must match internal numbering (plus offset)	
	ND_ERRCMD_MV_FULL,			// 06: Move Buffers full
	ND_ERRCMD_MV_SPEC,			// 07: Move spec error
	ND_ERRCMD_MV_ESTOPPED,		// 08: Motion attempt in E-Stopped state
	ND_ERRCMD_MV_RANGE,			// 09: Move distance out of range
	ND_ERRCMD_MV_SHUTDOWN,		// 10: Motion attempt in shutdown/disabled state
	ND_ERRCMD_IEX_ERROR,		// 11: IEX interaction while stopped
	ND_ERRCMD_MV_BLOCKED,		// 12: Motion attempt in non-disabling shutdown
	ND_ERRCMD_MV_HOMING,		// 13: Motion attempt while homing
					// NOTE: leave gap for additional MV errors
	ND_ERRCMD_DRV_IN_MOTION=16	// 16: Command not accepted due to move in progress
} mnCmdErrs;
// Offset to add to internal MV errors to translate them into mnCmdErrs
#define MV_ERR_BASE	(ND_ERRCMD_MV_FULL-1)

// Overlay on data
typedef union _netErrGeneric {
	nodeushort bits;				// Broad 16-bit view
	struct {
		unsigned ErrCode	: 5;	// Basic error code
		mnErrClasses ErrCls	: 2;	// Error Class
		unsigned Optional	: 7;	// (Optional)
	} Fld;
} netErrGeneric;
// Optional depends on the mnNetErrs code
// For ND_ERRNET_STRAY, this will be filled in with the stray count

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SET ADDRESS PACKET DEFINITIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// MN_PKT_TYPE_SET_ADDR packet field definitions
typedef union _setAddrPkt {
	nodechar bytes[3];
	struct {
			unsigned addr		: 4;	// Destination/source address
			MN_TYPE pktType  	: 3;	// Packet Type
			int StartOfPacket	: 1;	// Start of packet indicator

			unsigned PktLen     : 5;	// Number of items in \p buffer
			unsigned Err    	: 1;	// Error in configuration flag
			MN_SRC Src			: 1;	// Command source field
			int	Zero1			: 1;	// Always 0
	} Fld;
} setAddrPkt;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CORE Command set. If implemented by the a node, these are their command
// numbers.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef enum _mncmds {
	MN_CMD_GET_PARAM0 		= 0,	// Get a parameter from bank 0  (0-255)
	MN_CMD_SET_PARAM0 		= 1,	// Set a parameter from bank 0  (0-255)
	MN_CMD_GET_PARAM1	    = 2,	// Get parameter from bank 1
	MN_CMD_SET_PARAM1		= 3,	// Set parameter from bank 1 (256-511)
	MN_CMD_NODE_STOP		= 4,	// Stop node motion (aka Stim Cancel)
	MN_CMD_NET_ACCESS		= 5,	// Set net access level
	MN_CMD_USER_ID			= 6,	// Get/Set the user ID
	MN_CMD_CHK_BAUD_RATE	= 7,	// Get OK for proposed baud rate
	MN_CMD_ALERT_CLR		= 8,	// Clear non-serious Alert reg bits
	MN_CMD_ALERT_LOG		= 9,	// Read, clear & mark epoch of Alert log
	MN_CMD_PLA				=10,	// PLA setup and inspection
	MN_CMD_GET_PARAM2		=11,	// Get a parameter from bank 2 (512-639)
	MN_CMD_SET_PARAM2		=12,	// Set a parameter from bank 2 (512-639)
	MN_CMD_COMMON_END		=16,	// Node specific cmds can start here
	ASUM_CMD				=80		// Run an ASUM command (send data to app proc)	
} mncmds; 


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CORE Parameter set. If implemented by the a node, these are their command
// numbers.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CORE parameters
typedef enum _mnparams {
	MN_P_NODEID,					// 0  00 Device ID
	MN_P_FW_VERSION,				// 1  01 FW version
	MN_P_HW_VERSION,				// 2  02 HW version
	MN_P_RESELLER_ID,				// 3  03 Reseller ID
	MN_P_SER_NUM,					// 4  04 Unit serial number
	MN_P_OPTION_REG,				// 5  05 Unit options
	MN_P_ROM_SUM_ACK,				// 6  06 Firmware update ack
	MN_P_ROM_SUM,					// 7  07 Firmware checksum
	MN_P_SAMPLE_PERIOD,				// 8  08 Sample period (nsec)
	MN_P_ALERT_REG,					// 9  09 Shutdown register
	MN_P_STOP_TYPE,					// 10 0a Node Stop Type
	MN_P_WATCHDOG_TIME,				// 11 0b Watchdog time constant
	MN_P_NET_STAT,					// 12 0c Network Status
	MN_P_STATUS_ACCUM_REG,			// 13 0d Status accum register
	MN_P_STATUS_ATTN_RISE_REG,		// 14 0e Status attn/rise register
	MN_P_STATUS_ATTN_MASK,			// 15 0f Status mask register
	MN_P_STATUS_RT_REG,				// 16 10 Status reg (realtime)
	MN_P_TIMESTAMP,					// 17 11 8-bit timestamp
	MN_P_TIMESTAMP16,				// 18 12 16-bit timestamp
	MN_P_PART_NUM,					// 19 13 Part Number String
	MN_P_EE_UPD_ACK,				// 20 14 EE Update Acknowlegde
	MN_P_EE_VER,					// 21 15 EE Version Number
	MN_P_STATUS_FALL_REG,			// 22 16 Status fall register
	MN_P_HW_CONFIG_REG,				// 23 17 Hardware config/setup reg
	MN_P_APP_CONFIG_REG,			// 24 18 Feature config/setup reg
    MN_P_OUT_REG			= 32,   // 32 20 Output register
    MN_P_OUT_RISE_REG,              // 33 21 Output rise edge
    MN_P_OUT_FALL_REG,              // 34 22 Output fall edge
    MN_P_CTL_STOP_OUT_REG,          // 35 24 Controlled output register
	MN_P_USER_OUT_REG,				// 36 24 User output register
	MN_P_WARN_REG			= 70,	// 70 46 Warning register
	MN_P_WARN_MASK_REG		= 71,	// 71 47 Warning register mask
	MN_P_ALERT_MASK_REG		= 72,	// 72 48 Alert register mask


	MN_P_ON_TIME			= 89,	// 89 Unit powered on time
	MN_P_USER_RAM0			= 90,	// 90 User RAM parameter
	MN_P_USER_DATA_NV0,				// 91 User NV (13-bytes)
	MN_P_USER_DATA_NV1,				// 92 User NV (13-bytes)
	MN_P_USER_DATA_NV2,				// 93 User NV (13-bytes)
	MN_P_USER_DATA_NV3,				// 94 User NV (13-bytes)

	MN_P_NETERR_APP_CHKSUM=104,		// 104 68 Application Net Checksum counter
	MN_P_NETERR_APP_FRAG,			// 105 69 Application Net Fragment counter
	MN_P_NETERR_APP_STRAY,			// 106 6a Application Net Stray data counter
	MN_P_NETERR_APP_OVERRUN,		// 107 6b Application Net Overrun counter

	MN_P_NETERR_DIAG_CHKSUM=108,	// 108 6c Diagnostic Net Checksum counter
	MN_P_NETERR_DIAG_FRAG,			// 109 6d Diagnostic Net Fragment counter
	MN_P_NETERR_DIAG_STRAY,			// 110 6e Diagnostic Net Stray data counter	
	MN_P_NETERR_DIAG_OVERRUN,		// 111 6f Diagnostic Net Overrun counter	
	MN_P_NETERR_LOW_VOLTS=116		// 116 74 Diagnostic Net Brownout counter
} mnParams;

// Version 1 common parameter order
typedef enum _mnParams1 {
	MN_P1_NODEID,					// 0  00 Device ID
	MN_P1_FW_VERSION,				// 1  01 FW version
	MN_P1_HW_VERSION,				// 2  02 HW version
	MN_P1_RESELLER_ID,				// 3  03 Reseller ID
	MN_P1_SER_NUM,					// 4  04 Unit serial number
	MN_P1_OPTION_REG,				// 5  05 Unit options
	MN_P1_ROM_SUM_ACK,				// 6  06 Firmware update ack
	MN_P1_ROM_SUM,					// 7  07 Firmware checksum
	MN_P1_SAMPLE_PERIOD,			// 8  08 Sample period (nsec)
	MN_P1_ALERT_REG,				// 9  09 Shutdown register
	MN_P1_STOP_TYPE,				// 10 0a Node Stop Type
	MN_P1_WATCHDOG_TIME,			// 11 0b Watchdog time constant
	MN_P1_NET_STAT,					// 12 0c Network Status
	MN_P1_STATUS_ACCUM_REG,			// 13 0d Status accum register
	MN_P1_STATUS_ATTN_RISE_REG,		// 14 0e Status attn/rise register
	MN_P1_STATUS_ATTN_MASK,			// 15 0f Status mask register
	MN_P1_STATUS_RT_REG,			// 16 10 Status reg (realtime)
	MN_P1_TIMESTAMP,				// 17 11 8-bit timestamp
	MN_P1_TIMESTAMP16,				// 18 12 16-bit timestamp
	MN_P1_PART_NUM,					// 19 13 Part Number String
	MN_P1_EE_UPD_ACK,				// 20 14 EE Update Acknowlegde
	MN_P1_EE_VER,					// 21 15 EE Version Number
	MN_P1_STATUS_FALL_REG,			// 22 16 Status fall register
	MN_P1_HW_CONFIG_REG,			// 23 17 Hardware config/setup reg
	MN_P1_APP_CONFIG_REG,			// 24 18 Feature config/setup reg
	MN_P1_WARN_REG,					// 25 19 Warnings accumulated register
	MN_P1_WARN_RT_REG,				// 26 1a Warning real-time register
	MN_P1_WARN_MASK_REG,			// 27 1b Warning Attn Mask register
	MN_P1_ALERT_MASK_REG,			// 28 1c Alert Attn Mask register
	MN_P1_ON_TIME,					// 29 1d Powered On Time

	MN_P1_USER_RAM0			= 31,	// 31 1F User RAM parameter
	MN_P1_USER_DATA_NV0,			// 32 20 User NV (13-bytes)
	MN_P1_USER_DATA_NV1,			// 33 21 User NV (13-bytes)
	MN_P1_USER_DATA_NV2,			// 34 22 User NV (13-bytes)
	MN_P1_USER_DATA_NV3,			// 35 23 User NV (13-bytes)
	MN_P1_NETERR_APP_CHKSUM,		// 36 24 Application Net Checksum counter
	MN_P1_NETERR_APP_FRAG,			// 37 25 Application Net Fragment counter
	MN_P1_NETERR_APP_STRAY,			// 38 26 Application Net Stray data counter
	MN_P1_NETERR_APP_OVERRUN,		// 39 27 Application Net Overrun counter
	MN_P1_NETERR_DIAG_CHKSUM,		// 40 28 Diagnostic Net Checksum counter
	MN_P1_NETERR_DIAG_FRAG,			// 41 29 Diagnostic Net Fragment counter
	MN_P1_NETERR_DIAG_STRAY,		// 42 2a Diagnostic Net Stray data counter	
	MN_P1_NETERR_DIAG_OVERRUN,		// 43 2b Diagnostic Net Overrun counter	
	MN_P1_NETERR_LOW_VOLTS,			// 44 2c Diagnostic Net Brownout counter

	MN_P1_USER_OUT_REG,				// 50 32 User output register
	MN_P1_CTLD_OUT_REG,				// 51 33 Controlled output register
	MN_P1_OUT_REG,					// 52 34 Real-Time Output Register
	MN_P1_OUT_RISE_REG,				// 53 35 Accumulated risen outputs
	MN_P1_OUT_FALL_REG				// 54 36 Accumulated fallen outputs
} mnParams1;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Network Access Status and Identifier information overlay
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sources types for execContext.source
typedef enum _mnNetSrcs {
	MN_SRC_APP_NET,
	MN_SRC_DIAG_NET,
	MN_SRC_UNKNOWN_NET=-1				// -1 Unknown source
} mnNetSrcs;

// Net access level constants for use with MN_CMD_NET_ACCESS command
typedef enum _mnAccessLvls {
	MN_ACCESS_LVL_READ_ONLY=0,			// Read-only access - current net
	MN_ACCESS_LVL_TUNE=1,				// Allow tuning param access - current net
	MN_ACCESS_LVL_FULL=2,				// All commands and modes work - current net
	MN_ACCESS_LVL_FACTORY=3				// Factory access
} mnAccessLvls;

#ifdef _cplusplus
	#define MN_NSRCS mnNetSrcs
#else
	#define MN_NSRCS unsigned
#endif

typedef union _mnNetStatus {
	Uint16 bits;
	struct {
		mnNetSrcs NetSource		: 2;	// Network Source
		unsigned NetAccessApp	: 2;	// Access level (app)
		unsigned NetAccessDiag	: 2;	// Access level (diag)
		unsigned NetParamVer	: 2;	// Base parameter version
	} Fld;
} mnNetStatus;
#define NETDIAG_SPARE_WORDS				7
/// \endcond


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
	\brief Network Diagnostic statistics.

	This container hold the results of collected network diagnostics. It
	has the statistics for the \e Application and \e Diagnostic nets.
*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef struct _mnNetDiagStats {		// Application Network
										// -------------------
	Uint16 AppNetFragPktCtr;			///< Fragment counter for Application Channel
	Uint16 AppNetBadChksumCtr;			///< Checksum counter for Application Channel
	Uint16 AppNetStrayCtr;				///< Stray data counter for Application Channel
	Uint16 AppNetOverrunCtr;			///< Overrun counter for Application Channel
										
										// Diagnostic Network
										// -------------------
	Uint16 DiagNetFragPktCtr;			///< Fragment counter for Application Channel
	Uint16 DiagNetBadChksumCtr;			///< Checksum counter for Application Channel
	Uint16 DiagNetStrayCtr;				///< Stray data counter for Application Channel
	Uint16 DiagNetOverrunCtr;			///< Overrun counter for Application Channel
	Uint16 NetVoltsLowCtr;				///< Network Power Low for the link
	Uint16 Spare[NETDIAG_SPARE_WORDS];	///< Future expansion

	#ifndef __ASM_HEADER__
		#ifdef __cplusplus
			void Clear() {
				AppNetFragPktCtr
				= AppNetBadChksumCtr
				= AppNetStrayCtr
				= AppNetOverrunCtr
				= DiagNetFragPktCtr
				= DiagNetBadChksumCtr
				= DiagNetStrayCtr
				= DiagNetOverrunCtr = 0;
				NetVoltsLowCtr = 0;
			};
			
			_mnNetDiagStats() {
				Clear();
			};
		#endif
	#endif
} mnNetDiagStats;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// 						  COMMON SCALING FACTORS
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/// \cond INTERNAL_DOC
#define MN_POSN_Q	0
/// \endcond 
 
// 																			  *
//*****************************************************************************
#endif


//============================================================================= 
//	END OF FILE pubMnNetDef.h
// ============================================================================

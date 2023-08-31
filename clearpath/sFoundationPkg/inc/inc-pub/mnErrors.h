//***************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/mnErrors.h $
// $Revision: 55 $ $Date: 12/09/16 2:00p $
// $Workfile: mnErrors.h $
//
// DESCRIPTION:
/**
	\file
	\brief Error Code Definitions for sFoundation. 

	This is the master list of driver error codes. The codes are grouped
	by host detected, node elicited and initialization types.

	Some codes are bit encoded with a base code and a network number
	encoded into the code.
**/
//
// CREATION DATE:
//		12/10/2003 20:16:40
// 
// COPYRIGHT NOTICE:
// 		(C)Copyright 2003-2016  Teknic, Inc.  All rights reserved.
//
// 		This copyright notice must be reproduced in any copy, modification, 
// 		or portion thereof merged into another program. A copy of the 
// 		copyright notice must be included in the object library of a user
// 		program.
//
/***************************************************************************/

#ifndef __MNERRORS_H__
#define __MNERRORS_H__


//***************************************************************************
// NAME																	    *   
// 	mnErrors.h constants
//
//
/// \cond INTERNAL_DOC
// Standard return codes
#define MN_ERR_BASE					0x80040000L
// Basic error ranges
#define MN_ERR_CMD_ERR_BASE			0x80040100L
#define MN_ERR_NET_ERR_BASE			0x80040120L
#define MN_ERR_EXTEND_ERR_BASE		0x80040140L
#define MN_ERR_NETBREAK_ERR_BASE	0x80040300L
#define MN_ERR_NETBREAK_ERR_END		(MN_ERR_NETBREAK_ERR_BASE+0xff)
/// \endcond
   
/**
	\brief S-Foundation API error codes

	These codes are returned by the S-Foundation functions. For the 
	most part a return of MN_OK signals the requested operation executed
	correctly.

	Use the infcErrCodeStrA or infcErrCodeStrW functions to convert an 
	error code into a more detailed message. 
**/
typedef enum _cnErrCode {
	MN_OK,				    	///< 0x0:      	   function succeeded
	MN_ERR_FAIL=MN_ERR_BASE+1,	///< 0x80040001:   function generically failed
	MN_ERR_TIMEOUT,				///< 0x80040002:   failed due to time-out
	MN_ERR_CHECKSUM,			///< 0x80040003:   failed due to checksum failure
	MN_ERR_DEV_ADDR,			///< 0x80040004:   device addressing problems
	MN_ERR_TOO_MANY,			///< 0x80040005:   there are too many nodes on the net
	MN_ERR_RESP_FMT,			///< 0x80040006:   response is garbled
	MN_ERR_RESP_ADDR,			///< 0x80040007:   response is from wrong address
	MN_ERR_OFFLINE,				///< 0x80040008:   the node is offline
	MN_ERR_PARAM_RANGE,			///< 0x80040009:   parameter is out of range
	MN_ERR_MEM_LOW,				///< 0x8004000a:   memory is low
	MN_ERR_OS,					///< 0x8004000b:   operating system error
	MN_ERR_CLOSED,				///< 0x8004000c:   attempting to use a closed device
	MN_ERR_VALUE,				///< 0x8004000d:   the value requested is invalid
	MN_ERR_FRAME,				///< 0x8004000e:   command frame error
	MN_ERR_UNKN_DEVICE,			///< 0x8004000f:   attempting to use unknown device
	MN_ERR_DEV_WAIT_TO,			///< 0x80040010:   time-out waiting for command access
	MN_ERR_PKT_ERR,				///< 0x80040011:   Packet length != buffer length err
	MN_ERR_BUF_OVERFLOW,		///< 0x80040012:   RX buffer overflow
	MN_ERR_RX_ACCESS,			///< 0x80040013:   RX buffer access/sync problem
	MN_ERR_NOT_IMPL,			///< 0x80040014:   feature not implemented
	MN_ERR_PORT_PROBLEM,		///< 0x80040015:   Serial port driver open failure
	MN_ERR_TX_BUSY,				///< 0x80040016:   TX Buffer full
	MN_ERR_IN_SERIAL_PORT,		///< 0x80040017:   In serial port mode
	MN_ERR_PARAM_NOT_INIT,		///< 0x80040018:   Parameter Database not initialized
	MN_ERR_TEST_INCOMPLETE,		///< 0x80040019:   Initialization tests incomplete due to old firmware
	MN_ERR_IRQ_FAILED,			///< 0x8004001a:   Interrupt interface appears broken
	MN_ERR_CMD_OFFLINE,			///< 0x8004001b:   Command attempt while net offline
	MN_ERR_ADDR_LOCK,			///< 0x8004001c:   Address is locked
	MN_ERR_RESOURCE_BUSY,		///< 0x8004001d:   A resource is busy, request canceled
	MN_ERR_ATTN_OVERRUN,		///< 0x8004001e:   ATTN FIFO over-run
	MN_ERR_NO_TEST_INIT_ERR,	///< 0x8004001f:   Old firmware/initialization failure
	MN_ERR_P2P_MOVE_TIMEOUT,	///< 0x80040020:   Move segment time-out
	MN_ERR_P2P_DELAY_TIMEOUT,	///< 0x80040021:   Delay segment time-out
	MN_ERR_DEPRECATED,			///< 0x80040022:   Deprecated API element attempt
	MN_ERR_NET_ERRS_DETECT,		///< 0x80040023:   Network errors detected
	MN_ERR_NO_NODES_DETECTED,	///< 0x80040024:   No nodes are detected on both the diag and app net
	MN_ERR_DATAACQ_INVALID,		///< 0x80040025:   There is an invalid dataAcq point detected; Reasons: Dropped packet, data overflow
	MN_ERR_DATAACQ_EMPTY,		///< 0x80040026:   There is no data acq data point available
	MN_ERR_BADARG,				///< 0x80040027:   API function passed bad argument
	MN_ERR_WRONG_NODE_TYPE,		///< 0x80040028:   Interaction with wrong node type
	MN_ERR_BAUD_NOT_SUPPORTED,	///< 0x80040029:   Port does not support request rate
	MN_ERR_SEND_FAILED,			///< 0x8004002a:   Send failed
	MN_ERR_SEND_LOCKED,			///< 0x8004002b:   Could not acquire NC command interface 
	MN_ERR_SEND_UNLOCK,			///< 0x8004002c:   Could not release NC command interface
	MN_ERR_RESP_FAILED,			///< 0x8004002d:   Response failed to transfer
	MN_ERR_RESP_TIMEOUT,		///< 0x8004002e:   Response not found in time
	MN_ERR_CMD_TIMEOUT,			///< 0x8004002f:   Command timeout
	MN_ERR_CANCELED,			///< 0x80040030:   Command canceled
	MN_ERR_THREAD_CREATE,		///< 0x80040031:   Thread create error
	MN_ERR_UNSOLICITED,			///< 0x80040032:   Unsolicted response detected
	MN_ERR_DEF_RESP_TO,			///< 0x80040033:   Deferred response time-out
	MN_ERR_ADDR_RANGE,			///< 0x80040034:   Response from out of range address
	MN_ERR_WRONG_SRC,			///< 0x80040035:   Response from wrong source
	MN_ERR_FLUSH_MANY,			///< 0x80040036:   Flush finding many characters
	MN_ERR_PRIO_FAILED,			///< 0x80040037:   Failed to set thread priority
	MN_ERR_NET_DIAG_FAILED,     ///< 0x80040038:   Network diagnostic problems detected
	MN_ERR_RD_COMM_IRQ,			///< 0x80040039:   Could not start the read COMM IRQs
	MN_ERR_NULL_RETURN,		   	///< 0x8004003a:   Read returned NULL buffer
	MN_ERR_RESET_FAILED,		///< 0x8004003b:   Reset command failed
	MN_ERR_LOG_CHANGED,			///< 0x8004003c:   Error log changed during read
	MN_ERR_DATAACQ_OVERRUN,		///< 0x8004003d:   Data acquisition over-run
	MN_ERR_HOST_APP_OVERRUN,	///< 0x8004003e:   Host's application port over-run
	MN_ERR_HOST_DIAG_OVERRUN,	///< 0x8004003f:   Host's diagnostic port over-run
	MN_ERR_FILE_WRITE,			///< 0x80040040:   Configuration file failed to write
	MN_ERR_FILE_OPEN,			///< 0x80040041:   Configuration file failed to open   
	MN_ERR_FILE_WRONG,			///< 0x80040042:   Configuration file is for wrong node   
	MN_ERR_FILE_BAD,			///< 0x80040043:   Configuration file corrupt   
	MN_ERR_FILE_ENABLED,		///< 0x80040044:   Configuration file load requires node to be disabled.   
	MN_ERR_CMD_IN_ATTN,			///< 0x80040045:   Command attempted from within an Attention Callback.
	MN_ERR_APP_NET_ONLY,		///< 0x80040046:   Feature only available on App Net
	MN_ERR_24V_OFF,				///< 0x80000047:   Canceled due to lack of 24V supply
								// 0x48   
								// 0x49   


	/* ----------------------- MN_ERR_CMD ----------------------------*/
	MN_ERR_CMD_INTERNAL=MN_ERR_CMD_ERR_BASE,			
								// 0x100: Generic internal error
	MN_ERR_CMD_CMD_UNK,			///< 0x80040101: Command unknown on this node
	MN_ERR_CMD_ARGS,			///< 0x80040102: Illegal or missing command args
	MN_ERR_CMD_WR_FAIL,			///< 0x80040103: Attempt to write read-only info
	MN_ERR_CMD_EEHW,			///< 0x80040104: Non-Volatile memory failure
	MN_ERR_CMD_ACCESS_LVL,		///< 0x80040105: Access level violation
	MN_ERR_CMD_MV_FULL,			///< 0x80040106: Move Buffers full
	MN_ERR_CMD_MV_SPEC,			///< 0x80040107: Move specification error
	MN_ERR_CMD_MV_ESTOPPED,		///< 0x80040108: Motion attempt in E-Stopped state
	MN_ERR_CMD_MV_RANGE,		///< 0x80040109: Motion attempt is out of range
	MN_ERR_CMD_MV_SHUTDOWN,		///< 0x8004010a: Motion attempt / drive in shutdown
	MN_ERR_CMD_IEX_ERROR,		///< 0x8004010b: IEX interaction while stopped
	MN_ERR_CMD_MV_BLOCKED,		///< 0x8004010c: Motion blocked / non-disabling shutdown present
								// 0x10d	Reserved for move errors
								// 0x10e	Reserved for move errors
								// 0x10f	Reserved for move errors
	MN_ERR_NODE_IN_MOTION=MN_ERR_CMD_ERR_BASE+0x10,	///< 0x80040110:   Drive is in motion
	
	MN_ERR_CMD_ERR_END, // Used for class of error range
	/* -------------------- END OF MN_ERR_CMD GROUP ------------------*/

	/* ------------------------- MN_ERR_NET --------------------------------*/
	MN_ERR_NET_FRAG=MN_ERR_NET_ERR_BASE,	
								///< 0x80040120: Packet fragment detected
	MN_ERR_NET_CHKSUM,			///< 0x80040121: Bad checksum detected
	MN_ERR_NET_STRAY,			///< 0x80040122: Stray Data Found
	MN_ERR_NET_PORT_OVERRUN,	///< 0x80040123: Serial Port overran receive

	MN_ERR_NET_ERR_END,	// Used for class of error range
	/* ---------------------- END OF MN_ERR_NET ----------------------------*/

	/* -------------------- NODE BREAK ERROR GROUP ---------------------*/
 	/// Network Break detected on the other channel between node \e xx 
	/// and the node \e yy.
	MN_ERR_BRK_OTHER_00_01=MN_ERR_NETBREAK_ERR_BASE,
                                // 0x80040300: Net Break OTHER: node 00-01	
	MN_ERR_BRK_OTHER_01_02,     // 0x80040301: Net Break OTHER: node 01-02 
	MN_ERR_BRK_OTHER_02_03,     // 0x80040302: Net Break OTHER: node 02-03 
	MN_ERR_BRK_OTHER_03_04,     // 0x80040303: Net Break OTHER: node 03-04 
	MN_ERR_BRK_OTHER_04_05,     // 0x80040304: Net Break OTHER: node 04-05 
	MN_ERR_BRK_OTHER_05_06,     // 0x80040305: Net Break OTHER: node 05-06 
	MN_ERR_BRK_OTHER_06_07,     // 0x80040306: Net Break OTHER: node 06-07 
	MN_ERR_BRK_OTHER_07_08,     // 0x80040307: Net Break OTHER: node 07-08 
	MN_ERR_BRK_OTHER_08_09,     // 0x80040308: Net Break OTHER: node 08-09 
	MN_ERR_BRK_OTHER_09_10,     // 0x80040309: Net Break OTHER: node 09-10 
	MN_ERR_BRK_OTHER_10_11,     // 0x8004030a: Net Break OTHER: node 10-11 
	MN_ERR_BRK_OTHER_11_12,     // 0x8004030b: Net Break OTHER: node 11-12 
	MN_ERR_BRK_OTHER_12_13,     // 0x8004030c: Net Break OTHER: node 12-13 
	MN_ERR_BRK_OTHER_13_14,     // 0x8004030d: Net Break OTHER: node 13-14 
	MN_ERR_BRK_OTHER_14_15,     // 0x8004030e: Net Break OTHER: node 14-15 
	MN_ERR_BRK_OTHER_15_HOST, 	// 0x8004030f: Net Break OTHER: node 15-host
								
	/// Network Break detected on the application channel between node \e xx 
	/// and the node \e yy.
	MN_ERR_BRK_APP_00_01=MN_ERR_NETBREAK_ERR_BASE+0x10,		
								// 0x80040310: Net Break APP: node 00-01
	MN_ERR_BRK_APP_01_02,		// 0x80040311: Net Break APP: node 01-02 
	MN_ERR_BRK_APP_02_03,		// 0x80040312: Net Break APP: node 02-03 
	MN_ERR_BRK_APP_03_04,		// 0x80040313: Net Break APP: node 03-04 
	MN_ERR_BRK_APP_04_05,		// 0x80040314: Net Break APP: node 04-05 
	MN_ERR_BRK_APP_05_06,		// 0x80040315: Net Break APP: node 05-06 
	MN_ERR_BRK_APP_06_07,		// 0x80040316: Net Break APP: node 06-07 
	MN_ERR_BRK_APP_07_08,		// 0x80040317: Net Break APP: node 07-08 
	MN_ERR_BRK_APP_08_09,		// 0x80040318: Net Break APP: node 08-09 
	MN_ERR_BRK_APP_09_10,		// 0x80040319: Net Break APP: node 09-10 
	MN_ERR_BRK_APP_10_11,		// 0x8004031a: Net Break APP: node 10-11 
	MN_ERR_BRK_APP_11_12,		// 0x8004031b: Net Break APP: node 11-12 
	MN_ERR_BRK_APP_12_13,		// 0x8004031c: Net Break APP: node 12-13 
	MN_ERR_BRK_APP_13_14,		// 0x8004031d: Net Break APP: node 13-14 
	MN_ERR_BRK_APP_14_15,		// 0x8004031e: Net Break APP: node 14-15 
	MN_ERR_BRK_APP_15_TO_HOST,	// 0x8004031f: Net Break APP: node 15-host

	/** 0x80040320: Network Break detected on the diagnostic channel between 
	    node \e 0 and the node \e 1.
	**/
	MN_ERR_BRK_DIAG_00_01=MN_ERR_NETBREAK_ERR_BASE+0x20,		
	/** 0x80040321: Network Break detected on the diagnostic channel between 
	    node \e 1 and the node \e 2.
	**/
	MN_ERR_BRK_DIAG_01_02,		// 0x80040321: Net Break DIAG: node 01-02
	/** 0x80040322: Network Break detected on the diagnostic channel between 
	    node \e 2 and the node \e 3.
	**/
	MN_ERR_BRK_DIAG_02_03,		// 0x80040322: Net Break DIAG: node 02-03 
	/** 0x80040323: Network Break detected on the diagnostic channel between 
	    node \e 3 and the node \e 4.
	**/
	MN_ERR_BRK_DIAG_03_04,		// 0x80040323: Net Break DIAG: node 03-04 
	/** 0x80040324: Network Break detected on the diagnostic channel between 
	    node \e 4 and the node \e 5.
	**/
	MN_ERR_BRK_DIAG_04_05,		// 0x80040324: Net Break DIAG: node 04-05 
	/** 0x80040325: Network Break detected on the diagnostic channel between 
	    node \e 5 and the node \e 6.
	**/
	MN_ERR_BRK_DIAG_05_06,		// 0x80040325: Net Break DIAG: node 05-06 
	/** 0x80040326: Network Break detected on the diagnostic channel between 
	    node \e 6 and the node \e 7.
	**/
	MN_ERR_BRK_DIAG_06_07,		// 0x80040326: Net Break DIAG: node 06-07 
	/** 0x80040327: Network Break detected on the diagnostic channel between 
	    node \e 7 and the node \e 8.
	**/
	MN_ERR_BRK_DIAG_07_08,		// 0x80040327: Net Break DIAG: node 07-08 
	/** 0x80040328: Network Break detected on the diagnostic channel between 
	    node \e 8 and the node \e 9.
	**/
	MN_ERR_BRK_DIAG_08_09,		// 0x80040328: Net Break DIAG: node 08-09 
	/** 0x80040329: Network Break detected on the diagnostic channel between 
	    node \e 9 and the node \e 10.
	**/
	MN_ERR_BRK_DIAG_09_10,		// 0x80040329: Net Break DIAG: node 09-10 
	/** 0x8004032a: Network Break detected on the diagnostic channel between 
	    node \e 10 and the node \e 11.
	**/
	MN_ERR_BRK_DIAG_10_11,		// 0x8004032a: Net Break DIAG: node 10-11 
	/** 0x8004032b: Network Break detected on the diagnostic channel between 
	    node \e 11 and the node \e 12.
	**/
	MN_ERR_BRK_DIAG_11_12,		// 0x8004032b: Net Break DIAG: node 11-12 
	/** 0x8004032c: Network Break detected on the diagnostic channel between 
	    node \e 12 and the node \e 13.
	**/
	MN_ERR_BRK_DIAG_12_13,		// 0x8004032c: Net Break DIAG: node 12-13 
	/** 0x8004032d: Network Break detected on the diagnostic channel between 
	    node \e 13 and the node \e 14.
	**/
	MN_ERR_BRK_DIAG_13_14,		// 0x8004032d: Net Break DIAG: node 13-14 
	/** 0x8004032e: Network Break detected on the diagnostic channel between 
	    node \e 14 and the node \e 15.
	**/
	MN_ERR_BRK_DIAG_14_15,		// 0x8004032e: Net Break DIAG: node 14-15 
	/** 0x8004032f: Network Break detected on the diagnostic channel between 
	    node \e 15 and the host.
	**/
	MN_ERR_BRK_DIAG_15_TO_HOST,	// 0x8004032f: Net Break DIAG: node 15-host 

	/// Network Break detected on the application channel between node \e xx 
	/// and the host.
	MN_ERR_BRK_APP_00_HOST=MN_ERR_NETBREAK_ERR_BASE+0x30,		
	MN_ERR_BRK_APP_01_HOST,		// 0x330-0x33f Between node xx and host
	MN_ERR_BRK_APP_02_HOST,
	MN_ERR_BRK_APP_03_HOST,
	MN_ERR_BRK_APP_04_HOST,
	MN_ERR_BRK_APP_05_HOST,
	MN_ERR_BRK_APP_06_HOST,
	MN_ERR_BRK_APP_07_HOST,
	MN_ERR_BRK_APP_08_HOST,
	MN_ERR_BRK_APP_09_HOST,
	MN_ERR_BRK_APP_10_HOST,
	MN_ERR_BRK_APP_11_HOST,
	MN_ERR_BRK_APP_12_HOST,
	MN_ERR_BRK_APP_13_HOST,
	MN_ERR_BRK_APP_14_HOST,
	MN_ERR_BRK_APP_15_HOST,

	/**
	0x80040340 Network Break detected on the diagnostic channel between node
	\e 0 and the host.
	**/		
	MN_ERR_BRK_DIAG_00_HOST=MN_ERR_NETBREAK_ERR_BASE+0x40,
	/**
	0x80040341 Network Break detected on the diagnostic channel between node
	\e 1 and the host.
	**/		
	MN_ERR_BRK_DIAG_01_HOST, 
	/**
	0x80040342 Network Break detected on the diagnostic channel between node
	\e 2 and the host.
	**/		
	MN_ERR_BRK_DIAG_02_HOST,
	/**
	0x80040343 Network Break detected on the diagnostic channel between node
	\e 3 and the host.
	**/		
	MN_ERR_BRK_DIAG_03_HOST,
	/**
	0x80040344 Network Break detected on the diagnostic channel between node
	\e 4 and the host.
	**/		
	MN_ERR_BRK_DIAG_04_HOST,
	/**
	0x80040345 Network Break detected on the diagnostic channel between node
	\e 5 and the host.
	**/		
	MN_ERR_BRK_DIAG_05_HOST,
	/**
	0x80040346 Network Break detected on the diagnostic channel between node
	\e 6 and the host.
	**/		
	MN_ERR_BRK_DIAG_06_HOST,
	/**
	0x80040347 Network Break detected on the diagnostic channel between node
	\e 7 and the host.
	**/		
	MN_ERR_BRK_DIAG_07_HOST,
	/**
	0x80040348 Network Break detected on the diagnostic channel between node
	\e 8 and the host.
	**/		
	MN_ERR_BRK_DIAG_08_HOST,
	/**
	0x80040349 Network Break detected on the diagnostic channel between node
	\e 9 and the host.
	**/		
	MN_ERR_BRK_DIAG_09_HOST,
	/**
	0x8004034a Network Break detected on the diagnostic channel between node
	\e 10 and the host.
	**/		
	MN_ERR_BRK_DIAG_10_HOST,
	/**
	0x80040341 Network Break detected on the diagnostic channel between node
	\e 11 and the host.
	**/		
	MN_ERR_BRK_DIAG_11_HOST,
	/**
	0x8004034c Network Break detected on the diagnostic channel between node
	\e 12 and the host.
	**/		
	MN_ERR_BRK_DIAG_12_HOST,
	/**
	0x8004034d Network Break detected on the diagnostic channel between node
	\e 13 and the host.
	**/		
	MN_ERR_BRK_DIAG_13_HOST,
	/**
	0x8004034e Network Break detected on the diagnostic channel between node
	\e 14 and the host.
	**/		
	MN_ERR_BRK_DIAG_14_HOST,
	/**
	0x8004034f Network Break detected on the diagnostic channel between node
	\e 15 and the host.
	**/		
	MN_ERR_BRK_DIAG_15_HOST,

	/// Network Break detected between node \e xx and the host.
	MN_ERR_BRK_00_HOST=MN_ERR_NETBREAK_ERR_BASE+0x50,		
	MN_ERR_BRK_01_HOST,			// 0x340-0x34f Between node xx and host
	MN_ERR_BRK_02_HOST,
	MN_ERR_BRK_03_HOST,
	MN_ERR_BRK_04_HOST,
	MN_ERR_BRK_05_HOST,
	MN_ERR_BRK_06_HOST,
	MN_ERR_BRK_07_HOST,
	MN_ERR_BRK_08_HOST,
	MN_ERR_BRK_09_HOST,
	MN_ERR_BRK_10_HOST,
	MN_ERR_BRK_11_HOST,
	MN_ERR_BRK_12_HOST,
	MN_ERR_BRK_13_HOST,
	MN_ERR_BRK_14_HOST,
	MN_ERR_BRK_15_HOST,

	// Detected offline nodes are reported with this group. These are
	// usually caused by a node restarting via power or firmware error.
	MN_ERR_OFFLINE_00=MN_ERR_NETBREAK_ERR_BASE+0xa0,
							    // 0x800403a0: Node 0  detected offline
	MN_ERR_OFFLINE_01,	    	// 0x800403a1: Node 1  detected offline
	MN_ERR_OFFLINE_02,	    	// 0x800403a2: Node 2  detected offline
	MN_ERR_OFFLINE_03,	    	// 0x800403a3: Node 3  detected offline
	MN_ERR_OFFLINE_04,	    	// 0x800403a4: Node 4  detected offline
	MN_ERR_OFFLINE_05,	    	// 0x800403a5: Node 5  detected offline
	MN_ERR_OFFLINE_06,	    	// 0x800403a6: Node 6  detected offline
	MN_ERR_OFFLINE_07,	    	// 0x800403a7: Node 7  detected offline
	MN_ERR_OFFLINE_08,	    	// 0x800403a8: Node 8  detected offline
	MN_ERR_OFFLINE_09,	    	// 0x800403a9: Node 9  detected offline
	MN_ERR_OFFLINE_10,	    	// 0x800403aa: Node 10 detected offline
	MN_ERR_OFFLINE_11,	    	// 0x800403ab: Node 11 detected offline
	MN_ERR_OFFLINE_12,	    	// 0x800403ac: Node 12 detected offline
	MN_ERR_OFFLINE_13,	    	// 0x800403ad: Node 13 detected offline
	MN_ERR_OFFLINE_14,	    	// 0x800403ae: Node 14 detected offline
	MN_ERR_OFFLINE_15,	    	// 0x800403af: Node 15 detected offline
	
	// Special Diagnostic Errors
	MN_ERR_BRK_DIAG_LOC_UNKNOWN=MN_ERR_NETBREAK_ERR_BASE+0xFC,	
								///< 0x800403fc: Net Break Diag: unknown location
	MN_ERR_BRK_APP_LOC_UNKNOWN,	///< 0x800403fd: Net Break App: unknown location
    MN_ERR_BRK_LOC_UNKNOWN,    	///< 0x800403fe: Net Break unknown
	MN_ERR_NO_NET_CONNECTIVITY,	///< 0x800403ff: Cannot detect net connectivity

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Initialization bit oriented error codes
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	/**
		\brief Initialization Error(s)

		The following codes describe the failing port initialization issue.
		The 8 LSBs represent the problem ports with LSB = port 0.

		0x80040600-0x80040607
	**/
	MN_ERR_INIT_FAILED_BASE=0x80040600,
	/**
	0x80040601: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 0.
	**/
	MN_ERR_INIT_FAILED_PORTS_0,			// 0x80040601 Port 0 problem		
	/**
	0x80040602: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 1.
	**/
	MN_ERR_INIT_FAILED_PORTS_1,			// 0x80040602: Port 1 problem
	/**
	0x80040603: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 0 & 1.
	**/
	MN_ERR_INIT_FAILED_PORTS_1_0,		// 0x80040603: Port 0&1 problem
	/**
	0x80040604: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 2.
	**/
	MN_ERR_INIT_FAILED_PORTS_2,			// 0x80040604: Port 2 problem
	/**
	0x80040605: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 2 & 0.
	**/
	MN_ERR_INIT_FAILED_PORTS_2_0,		// 0x80040605: Port 2&0 problem
	/**
	0x80040606: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 2 & 1.
	**/
	MN_ERR_INIT_FAILED_PORTS_2_1,		// 0x80040606: Port 2&1 problem
	/**
	0x80040607: Port opened OK but no nodes were found attached to the link.	
 	Problems detected at port index: 2 & 1 & 0.
	**/
	MN_ERR_INIT_FAILED_PORTS_2_1_0,		// 0x80040607: Port 2&1&0 problem

	/**
		\brief Port Open Error(s)

		The following codes describe the failing port failed to open issue.
		The 8 LSBs represent the problem ports with LSB = port 0.

		0x80040700-0x80040707
	**/
	MN_ERR_PORT_FAILED_BASE = 0x80040700,
	/**
	0x80040701: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 0.
	**/
	MN_ERR_PORT_FAILED_PORTS_0,			// 0x80040701: Port 0 problem		
	/**
	0x80040702: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 1.
	**/
	MN_ERR_PORT_FAILED_PORTS_1,			// 0x80040702: Port 1 problem
	/**
	0x80040703: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 0 & 1.
	**/
	MN_ERR_PORT_FAILED_PORTS_1_0,		// 0x80040703: Port 0&1 problem
	/**
	0x80040704: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 2.
	**/
	MN_ERR_PORT_FAILED_PORTS_2,			// 0x80040704: Port 2 problem
	/**
	0x80040705: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 2 & 0.
	**/
	MN_ERR_PORT_FAILED_PORTS_2_0,		// 0x80040705: Port 2&0 problem
	/**
	0x80040706: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 2 & 1.
	**/
	MN_ERR_PORT_FAILED_PORTS_2_1,		// 0x80040706: Port 2&1 problem
	/**
	0x80040707: Port failed to open. Make sure you specify a working port or check
	that the USB cable is properly inserted. 
	Problems detected at port index: 2 & 1 & 0.
	**/
	MN_ERR_PORT_FAILED_PORTS_2_1_0,		// 0x80040707: Port 2&1&0 problem
	/**
		\brief Initialization Error	Due to Unsupported Port Rate

		The following codes describe the failing port initialization issue.
		The 8 LSBs represent the problem ports with LSB = port 0

		0x80040800-0x80040807
	**/
	MN_ERR_BAUD_FAILED_BASE = 0x80040800,
	/**
	0x80040801: Port opened OK but doesn't support the requested baud rate. 
	Problems detected at port index: 0.
	**/
	MN_ERR_BAUD_FAILED_PORTS_0,			// 0x80040801: Port 0 problem		
	/**
	0x80040802: Port opened OK but doesn't support the requested baud rate.
	Problems detected at port index: 1.
	**/
	MN_ERR_BAUD_FAILED_PORTS_1,			// 0x80040802: Port 1 problem
	/**
	0x80040803: Port opened OK but doesn't support the requested baud rate.
	Problems detected at port index: 0 & 1.
	**/
	MN_ERR_BAUD_FAILED_PORTS_1_0,		// 0x80040803: Port 0&1 problem
	/**
	0x80040804: Port opened OK but doesn't support the requested baud rate.
	Problems detected at port index: 2.
	**/
	MN_ERR_BAUD_FAILED_PORTS_2,			// 0x80040804: Port 2 problem
	/**
	0x80040805: Port opened OK but doesn't support the requested baud rate.
	Problems detected at port index: 2 & 0.
	**/
	MN_ERR_BAUD_FAILED_PORTS_2_0,		// 0x80040805: Port 2&0 problem
	/**
	0x80040806: Port opened OK but doesn't support the requested baud rate.
	Problems detected at port index: 2 & 1.
	**/
	MN_ERR_BAUD_FAILED_PORTS_2_1,		// 0x80040806: Port 2&1 problem
	/**
	0x80040807: Port opened OK but doesn't support the requested baud rate.
	Problems detected at port index: 2 & 1 & 0.
	**/
	MN_ERR_BAUD_FAILED_PORTS_2_1_0		// 0x80040807: Port 2&1&0 problem

	/* --------------------- END OF NODE ERROR GROUP ----------------------*/

} cnErrCode;
/***************************************************************************/

#endif
//=========================================================================== 
//	END OF FILE mnErrors.h
//===========================================================================

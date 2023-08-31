//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/inc-private/lnkAccessCommon.h $
// $Date: 01/19/2017 17:39 $
// $Workfile: lnkAccessCommon.h $
//
// DESCRIPTION:
///		\file
///		This file contains the platform independent definitions and data for
///		a Meridian node network.
//
// CREATION DATE:
//		01/29/2010 16:05:21
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
/// \cond INTERNAL_DOC

#ifndef __LNKACCESSCOMMON_H__
#define __LNKACCESSCOMMON_H__
//*****************************************************************************
// !NAME!																      *
// 	lnkAccessCommon.h headers included
//
	#ifndef DOXYGEN_SHOULD_SKIP_THIS
		// StdLib inclusions
		#include <queue>
		// Our inclusions
		#include "tekTypes.h"
		#include "tekThreads.h"
		#include "mnParamDefs.h"
		#include "lnkAccessAPI.h"	
		#include "pubSysCls.h"
		#include "pubCpmCls.h"
	#endif
//																			  *
//*****************************************************************************

// Just need a pointer to this
class CSerialEx;

//*****************************************************************************
// !NAME!																	  *
// 	lnkAccessCommon.h constants
//
// Depth of the polled error event list
#define ERR_CNT_MAX 			8
// Depth of the polled network event list
#define NET_EVENTS_MAX 			64
// Trace Dump file "type" number in the header. Change if the header changes
#define DUMP_TYPE_RANGE_STEP	100				// Groups of compatible files
// V2 style, Meridian. Less than this are ControlPoint style files
#define DUMP_TYPE_NUM_V2_BASE	200				// V2 style
// V3 style/packed trace records
#define DUMP_TYPE_NUM_V3_BASE	(DUMP_TYPE_NUM_V2_BASE+DUMP_TYPE_RANGE_STEP)
// Current version number, change if file structure changes
#define DUMP_TYPE_NUM			DUMP_TYPE_NUM_V3_BASE

// Depth of the Trace Buffer for sent commands
#define SEND_DEPTH				4096
// Depth of the Trace Buffer for received responses
#define RECV_DEPTH				SEND_DEPTH
// Depth of the Attention Buffer. Add one more than this will yield overflow.
#define ATTN_OVERFLOW_LVL		32
// Depth of the Data Acquisition buffer at each node.
#define DATAACQ_OVERFLOW_LVL	2000
// Number of simultaneous command in ring default
#define N_CMDS_IN_RING			3

// XML based error text
#define LNK_ACCESS_XML_ERR_TXT "/MNuserDriver20.xml"

void DUMP_PKT(netaddr cNum, const char *msg, packetbuf *buf, bool backArrow=false);

//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	mnClassInfo structure
//
// DESCRIPTION
//	A node class information structure. It stores the counts the count of this
//  type of node as well the addresses on this network where they exist.
//
// Holds the count and addresses on a network where these nodes live.
EXTERN_C typedef struct _mnClassInfo {
	nodeulong count;				// Count of nodes
	multiaddr node[MN_API_MAX_NODES];// List of nodes found
} mnClassInfo;
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	autoBrakeInfo structure
//
// DESCRIPTION
//	This structure sets which node (if enabled is true) will control the
//	.
//
typedef struct _autoBrakeInfo{
	nodebool enabled;
	multiaddr theNode;
	BrakeControls brakeMode;
}autoBrakeInfo;
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	txTraceBuf & rxTraceBuf tracing structures
//
// DESCRIPTION
//	These structures hold the.
//
// Forward references
typedef struct _respTrackInfo respTrackInfo;
typedef struct _respNodeList  respNodeList;

#if defined(_MSC_VER)||defined(__GNUC__)
#pragma pack(push, 1)
#endif
// Transmission Traces (V3)
typedef struct _txTraceBuf {
	double timeStamp;			// Time command sent to NC
	Uint32 order;				// Send serial number
	Uint32 depth;				// Queue depth
	packetbuf packet;			// Copy of the command sent
	nodebool failed;			// Failed send
	Uint16 pad;					// Pad to 60 bytes(dword)
#ifdef __cplusplus
	_txTraceBuf() {
		timeStamp = 0;
		order = 0;
		depth = 0;
		failed = 0;
		pad = 0;
	}
#endif
} txTraceBuf;

// Reception Traces (V3)
typedef struct _rxTraceBuf {
	double timeStamp;			// Time response received
	Uint32 sendCnt;				// By node count
	Uint32 sendSer;				// Assumed sender serial number
	Uint32 order;				// By network response count
	packetbuf packet;			// Copy of the response received
	Uint32  __pad;				// Unused (padding to preserve aligment)
	cnErrCode error;			// Error code of result
#ifdef __cplusplus
	_rxTraceBuf() {
		timeStamp = 0;
		sendCnt = 0;
		sendSer = 0;
		order = 0;
		__pad = 0;
		error = MN_OK;
	}
#endif
} rxTraceBuf;
#if defined(_MSC_VER)||defined(__GNUC__)
#pragma pack(pop)
#endif
// Version 2 "unpacked" buffers for trace dumps
// Transmission Traces
typedef struct _txTraceBuf200 {
	double timeStamp;			// Time command sent to NC
	Uint32 order;				// Send serial number
	Uint32 depth;				// Queue depth
	packetbuf packet;			// Copy of the command sent
	nodebool failed;			// Failed send
#ifdef __cplusplus
	_txTraceBuf200() {
		timeStamp = 0;
		order = 0;
		depth = 0;
		failed = 0;
	}
#endif
} txTraceBuf200;

// Reception Traces
typedef struct _rxTraceBuf200 {
	double timeStamp;			// Time response received
	Uint32 sendCnt;				// By node count
	Uint32 sendSer;				// Assumed sender serial number
	Uint32 order;				// By network response count
	packetbuf packet;			// Copy of the response received
	txTraceBuf *cmd;			// Matched command
	cnErrCode error;			// Error code of result
#ifdef __cplusplus
	_rxTraceBuf200() {
		timeStamp = 0;
		sendCnt = 0;
		sendSer = 0;
		order = 0;
		cmd = NULL;
		error = MN_OK;
	}
#endif
} rxTraceBuf200;
//
//	This structure defines the header of the trace dump file.
//
#if defined(_MSC_VER)||defined(__GNUC__)
#pragma pack(push, 1)
#endif
#define TRACE_FILEPATH_LEN	260
typedef struct _traceHeader {
	Uint32 hdrLen;					// Header length
	Uint32 type;					// Trace type
	Uint32 numOfNodes;				// Number of nodes
	Uint32 traceLen;				// Number of trace records
	Uint32 nextRXrecord;			// Next trace record
	devID_t nodeTypes[MN_API_MAX_NODES];	// Node types
	Uint32 version;					// Driver version
	Uint32 kernVersion;				// Kernel version
	Uint32 pad32;
	// Time trace was saved
	#if (defined(_WIN32)||defined(_WIN64))
		struct _timeb timestamp;
	#else
		// Time stamp compatible with Windows
		struct _timeb {
		        Uint64 time;
		        Uint16 millitm;
		        int16 timezone;
		        int16 dstflag;
				Uint16 pad;			// Align to Win32 version
		} timestamp;
	#endif
	Uint32 nextTXrecord;			// Next TX trace item
	Uint32 nTXrecords;				// Number of TX records
	Uint32 nRXrecords;				// Number of RX records
	mnNetDiagResults diags;			// Diagnostic results
	char filename[TRACE_FILEPATH_LEN];	// DLL path used
} traceHeader;
#if defined(_MSC_VER)||defined(__GNUC__)
#pragma pack(pop)
#endif
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	mnNetInvRecords structure
//
// DESCRIPTION
//	This structure holds the overview of a channel's inventory of nodes and the
//	serial port specification.	There are one of these structures per serial 
//	port.
//
class netStateInfo;			// Forward reference
class mnNetInvRecords {
public:
	// This structure holds this network's locations and types of nodes
	// detected upon enumeration. It is used to aid in locating nodes and 
	// for determining if the configuration changed.
	struct _netInfoByType {
		bool isValid;
		// Total # nodes on this port
		nodeulong NumOfNodes;
		// Number of Meridian Drives
		mnClassInfo mnDrvInfo;
		// ClearPath Motors
		mnClassInfo mnClearPathInfo;
		// ClearPath SC Motors
		mnClassInfo mnCpScInfo;
		// Last device ID set
		devID_t LastIDs[MN_API_MAX_NODES];	
		// Construction
		_netInfoByType() {
			isValid = false;
		}
	};

	// ---------------------------------
	// Tracing State
	// ---------------------------------
	nodebool TraceArmed;				// If set dump file will create on error
	nodebool TraceActive;				// Tracing enabled
	nodebool TraceActiveReq;			// Tracing enable request
	Uint32 TraceCount;					// Count of automatic dumps attempted this session
	Uint32 sendSerNum;					// Sending serial number
	Uint32 respSerNum;					// Receive serial number
	txTraceBuf *txTraces;				// Trace buffer SEND_DEPTH deep
	size_t txTraceIndx;
	rxTraceBuf *rxTraces;				// Trace buffer RECV_DEPTH deep
	size_t rxTraceIndx;
	CCCriticalSection TXlogLock;		// Transmit log critical section
	CCCriticalSection RXlogLock;		// Receive log critical section
	// Record in the log file what we sent and when
	unsigned logSend(
				packetbuf *cmd,
				cnErrCode theErr,
				double timeStamp);
	// Record in the log file what we received and when
	double logReceive(
				packetbuf *readBuf,
			    cnErrCode theErr,
			    respTrackInfo *fillInfo,
			    double timeStamp);

	// - - - - - - - - - - - - - - - -//
	// == Net Specific Information == //
	// - - - - - - - - - - - - - - - -//
	// Disposable state information
	netStateInfo *pNCS;
	// Current node information, value caches, construction/destruction
	byNodeDB NodeInfo[MN_API_MAX_NODES];
	// Set when something was discovered for diagStats
	nodebool diagsAvailable[MN_API_MAX_NODES+1];
	// Autonomously delivered stats
	mnNetDiagStats diagStats[MN_API_MAX_NODES+1];
	// The diagnostics are running; Prevent RunCommand from running if this is set
	nodebool NetDiagBlock;				// Blocks diagnostics on this net
	cnErrCode NetDiagLastResult;		// Last network diagnostic result
	cnErrCode OfflineRootErr;			// Offline root error
	nodeaddr OfflineErrNode;			// Node reporting root error
	nodebool brake0Saved;				// Brake states at close time
	nodebool brake1Saved;

	// Port specifier: speed, number, etc.
	portSpec PhysPortSpecifier;
	// Number of simultaneous commands allowed in ring
	nodeulong NumCmdsInRing;

	// Initializing "stack"	counter. Maintained by infcSetInitializeMode.
	// When this counter decrements back to zero, we signal we are "online",
	// likewise when we increment from zero we go "offline". This accounting
	// is necessary to allow internal and external usage of the diagnostic
	// functions and prevents extraneous and premature "online" signalling.
	int16 Initializing;
	// Controls if auto discovery is allowed
	nodebool AutoDiscoveryEnable;
	// Controls if the "keep alive" poll is allowed
	nodebool KeepAlivePollEnable;
	// State to restore the "keep alive" poll to when we go online
	nodebool KeepAlivePollRestoreState;

	// - - - - - - - - - - - - - - - -//
	// == By Node Type Information == //
	// - - - - - - - - - - - - - - - -//
	// Current information
	struct _netInfoByType InventoryNow;
	// Information saved when net went online
	struct _netInfoByType InventoryLast;

	// Current attention states
	attnReg drvrAttnMask[MN_API_MAX_NODES];
	attnReg attnMask[MN_API_MAX_NODES];
	nodeulong attnCleanupMask[MN_API_MAX_NODES];
	// Flag to suppress attentions during setups
	int16 attnInitializing;

	// - - - - - - - - - - - - - - - -//
	// == State for class library  == //
	// - - - - - - - - - - - - - - - -//
	// Pointer to our shadow class library representation
	sFnd::IPort *pPortCls;
	// Serial port online state for class library
	openStates OpenState;
	openStates OpenStateLast;
	// Set next open state
	void OpenStateNext(openStates next);
	// Connection Type and status
	mnNetStatus AccessInfo[MN_API_MAX_NODES];
	// Nodes class instances
	sFnd::INode *pNodes[MN_API_MAX_NODES];

	// - - - - - - - - - - - - - - - - - - - - //
	// ==  User Driver Feature Information  == //
	// - - - - - - - - - - - - - - - - - - - - //

	// - - - - - - - - - - - - - - - - - - - - -
	// Auto-brake feature state for ClearPath-SC
	// - - - - - - - - - - - - - - - - - - - - -
	// Maximum brakes for this controller
	static const size_t MAX_BRAKES = 2;
	// Information related to the auto-brake
	typedef struct _autoBrakeInfo {
		nodebool enabled;						// Brake is enabled to run
		multiaddr relatedNode;					// Controlling node
		BrakeControls brakeMode;				// Brake mode
		// Construct a auto-controleld, unassigned brake
		_autoBrakeInfo() {
			enabled = true;
			brakeMode = BRAKE_AUTOCONTROL;
			relatedNode = MN_UNSET_ADDR;
		}
	} autoBrakeInfo;
	// State recorded from API
	autoBrakeInfo autoBrake[MAX_BRAKES];
	// Setup the brake state to current node/net state.
	void setupBrakes();

	// - - - - - - - - - - - - - - - - - - - - -
	// NodeStop on Status Event feature
	// - - - - - - - - - - - - - - - - - - - - -
	// When set, we will generate shutdown on next poll cycle
	nodebool GroupShutdownRequest;
	// State recorded from user API
	ShutdownInfo GroupShutdownInfo[MN_API_MAX_NODES];
	// This is the last sample of the CTS line whose rise
	// edge causes can cause a group shutdown.
	nodebool GroupLastExtRequest;

	// Clear our counts and validity
	void clearNodes(bool portIsClosed);
	// Construct as cleared, dormant.
	mnNetInvRecords();
	~mnNetInvRecords();
	// Returns true if serial port is open
	bool PortIsOpen() 
	{
		return OpenState == FLASHING
			|| OpenState == OPENED_SEARCHING
			|| OpenState == OPENED_ONLINE;
	}
};
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	profileStats structure
//
// DESCRIPTION
//	This structure holds time profiling information used to evaluate efficiency.
typedef struct _profileStats {
	// Command Timing
	double cmdSendTime;
	double cmdSendTimeMax;
	unsigned cmdsSent;
	double cmdSendTimeSum;
	// Response Timing
	double respTime;
	double respTimeMax;
	unsigned responses;
	double responseTimeSum;
	// Read Thread Info
	unsigned readIterations;
	unsigned readsPerformed;
	double readsPerfSum;
	double readIterSum;
	double readWaitSum;
	double readTToLockSum;
	double readTToStartSum;
	unsigned readTToStartCount;
	// Run Command Stats
	double tToSend;
	double tAfterSend;
	double tToDeferExit;
	double tToWaitExit;
} profileStats;
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	dataAcqInfo structure
//
// DESCRIPTION
//	This structure a node's data acquisition information.
//
typedef struct _dataAcqInfo {
	// Interface Lock
	CCCriticalSection AcqLock;
	// Point not transferred to application
	std::queue<mnDataAcqPt> Points;
	// Sequence check
	nodelong SeqCheck;
	// Set when have lost samples in Points
	nodebool Overflow;
	// Samples since the start of acquisition
	nodeulong SampleCount;
	// The time between samples
	double SampRateMilliSec;
	// Construction initialization
	_dataAcqInfo() {
		SampRateMilliSec = 0;
		SeqCheck = 0;
		Overflow = FALSE;
		SampleCount = 0;
	}
} dataAcqInfo;
//																			  *
//*****************************************************************************






//*****************************************************************************
// NAME																          *
// 	respTrackInfo & respNodeList structure
//
// DESCRIPTION
//	Command and Response tracking
//
//  This is a command response tracking database element. After a command is
//	sent the information in this structure is used to copy the response to the
//	waiting thread and signal its restart.
typedef struct _respTrackInfo {
	struct _respTrackInfo *next;		// Ptr to next response waiting thread
	nodebool bufOK;						// TRUE if the buf has data
	packetbuf *buf;						// User's response location
	// This event blocks the commanding thread from running until
	// the response is detected.
	CCEvent evtRespWait;
	Uint32 sendSerNum;					// Sending serial number
	Uint32 nSentAtAddr;					// sendCnt for this node
	double cmdStartAt;					// Time-stamp at start of infcSendCommand
	double funcStartAt;					// Time-stamp at start of infcRunCommand
	mnCompletionInfo stats;				// Command completion statistics
	// Construct an empty tracking info record
	_respTrackInfo() {
		bufOK = false;
		buf = NULL;
		next = NULL;
		sendSerNum = nSentAtAddr = 0;
		cmdStartAt = 0;
	}
} respTrackInfo;

// This is the main by-node tracking database element. It holds the list of
// expected responses for a particular node as well as error information and
// some by-node statistics.
typedef struct _respNodeList {
	respTrackInfo *head;				// Head (oldest) waiter on the net
	respTrackInfo *tail;				// Tail (newest) waiter on the net
	packetbuf errPkt;					// Error packet
	Uint32 sendCnt;						// Sending count
	Uint32 respCnt;						// Receive count
	// Construct an empty by node tracker
	_respNodeList() {
		head = tail = NULL;
		sendCnt = respCnt = -1;
		errPkt.Byte.BufferSize = 0;
	}
} respNodeList;
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	autoDiscoverThread class
//
// DESCRIPTION
//		This is a thread that attempts to detect nodes across the serial port 
//		and bring them back online.
//
class netStateInfo;							// Forward reference
class autoDiscoverThread : public CThread {
private:
	// This flag forces discover to park at halt state because we have found
	// our network working again.
	nodebool m_goneOnline;
	// This flag reflects the parked status of the thread.
	nodebool m_parked;
	// This flag causes our discovery loop to exit
	nodebool m_exitDiscovery;
	// Our net context
	netStateInfo *pNCS;
	// The error that caused us to start
	cnErrCode m_faultingErr;

	// Internal operations sync event, used for starting and stopping
	// the thread for diagnostics and other purposes.
	CCEvent m_InternalSync;
	// This is the interlocking event for cross thread handshaking
	CCEvent m_InternalSyncAck;

	// Run control interlocks
	CCCriticalSection m_critSect;

public:
	autoDiscoverThread();
	~autoDiscoverThread();

	// Returns TRUE if Running
	nodebool IsRunning() {
		return(!m_parked && !Terminating());
	}

	// Force the autodiscover back to halted state
	void Halt();

	// Start node discovery process
	void Start(cnErrCode theErr);

	// Override terminate to allow proper exit
	HANDLE Terminate();

	// Override terminate and wait to insure proper exit
	void TerminateAndWait();

	// Wait for thread to park
	void WaitUntilParked() {
		if (m_parked)
			return;
		m_InternalSyncAck.WaitFor();
	}

	// Autodiscover loop running. We are allowed to keep
	// waiting for network recovery if this is true.
	bool WaitingForNet() ;


protected:
	int Run(void *context);					// Control function
};
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	readPktThread class
//
// DESCRIPTION
//		This is a thread that waits for packets to arrive from the serial port
//		and allows blocked threads waiting for responses to restart or
//		other internal functions to start due to their arrival.
//
class netStateInfo;							// Forward reference
class readPktThread : public CThread {
public:
	// Read thread states
	typedef enum _readThreadStates {
		READ_UNKNOWN,						// 0 Unknown state
		READ_RUNNING,						// 1 Thread is running
		READ_HALT_REQ,						// 2 Halt is requested/pending
		READ_HALTED,						// 3 Thread is halted
		READ_SHUTDOWN_REQ,					// 4 Shutdown requested/pending
		READ_SHUTDOWN						// 5 Thread has exited
	} readThreadStates;

private:
	readThreadStates m_state;				// Running state
	readThreadStates m_lastState;			// Last state for debugging
	// Internal operations sync event, used for starting and stopping
	// the thread for diagnostics and other purposes.
	CCEvent m_InternalSync;
	// This is the interlocking event for cross thread handshaking
	CCEvent m_InternalSyncAck;
	CCCriticalSection ReadLock;				// Internal structure lock
	CCCriticalSection StartStopLock;		// Internal structure lock
	netStateInfo *pNCS;						// Our net context

public:
	// Construction/Destruction
	readPktThread();
	~readPktThread();

	// Start/Stop API
	void Start();
	void Stop();
	void ForceStop();

	// Change read thread state
	void NextState(readThreadStates nextState);

	// State Info
	nodebool IsRunning() {
		return(m_state == READ_RUNNING);
	}

protected:
	int Run(void *context);				// Control function

private:
	//void constructInit(netStateInfo *state);
};
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		netPollerThread	class
//
//  CREATION DATE:
//		2014-05-21 11:29:40
//
//	DESCRIPTION:
/**		
	This class defines a class that periodically polls the device ID from 
	node 0. This insures a net disconnect is timely reported and in the
	case of VB apps, prevents the watchdog timer from prematuring expiring
	during UI stalls.

**/
//	SYNOPSIS:
class netStateInfo;
class netPollerThread : public CThread 
{
private:
	// This is the interlocking event for cross thread handshaking
	CCEvent m_InternalSyncAck;
	// If this is set, the polling can run
	CCEvent m_RunControl;
	CCCriticalSection m_critSection;		// Internal structure lock
	netStateInfo *pNCS;						// Our net context
	// Set when halted
	bool m_halted;

public:
	// Construction/Description
	netPollerThread(netStateInfo *pTheNetInfo);
	~netPollerThread();

	// CThread overrides for terminate
	void *Terminate();

	// Start/Stop API
	void Start();
	void Stop();
	// Returns true if halted and parked
	bool Halted() {
		return m_halted;
	}
protected:
	int Run(void *context);				// Control function

private:
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																          *
// 	netStateInfo class
//
// DESCRIPTION
//	By network specific state information. It contains the
//	statistics as well as the response tracking databases. This remains a
//	structure for compatibility with the C API.
//
class netStateInfo {
public:
	// Event switch managed by the cmdsIdleEvt class. 
	CCEvent CmdsIdle;					// Signalled when cmds idle
	CCatomicUpdate m_cmdsInPlay;
	// RAII mechanism to prevent premature destruction of state by resetting
	// the CmdsIdle event when commands are in play. It should be created
	// in the infcRunCommand function to track the ports command activity.
	class cmdsIdleEvt {
	private:
		netStateInfo &m_netState;
		CCEvent &m_event;
	public:
		cmdsIdleEvt(netStateInfo &myNetState) :
			m_netState(myNetState), m_event(myNetState.CmdsIdle)
		{
			m_netState.m_cmdsInPlay.Incr();
			m_event.ResetEvent();
		}
		~cmdsIdleEvt() {
			if (m_netState.m_cmdsInPlay.Decr()==0) {
				m_event.SetEvent();
			}
		}
	};
	netaddr cNum;						// Port index number
	// Serial Port Instance for this network
	CSerialEx *pSerialPort;
	// Read Thread to read all pending packets from the serial interface.
	readPktThread ReadThread;
	// Overall port and network state
	mnNetSrcs OurNetConnector;			// Port this net is connected to
	// Set this to cause all threads to terminate
	nodebool SelfDestruct;				// Net wide self-destruct flag

	nodeulong nRespOutstanding;			// Number of responses waiting
	nodeulong nPktsSent;				// Number of packets sent
	nodeulong nPktsRcvd;				// Number of packets received

	size_t ErrListHeadPtr;				// Error head ptr
	size_t ErrListTailPtr;				// Error tail ptr
	infcErrInfo ErrList[ERR_CNT_MAX];	// Error circular list
	int errorRecursePrevent;			// Set to prevent error callback recursion

	// Net change circular list. This contains the last set of network changes 
	// for use by polled systems.
	size_t NetChgListHeadPtr;			// Net change list head ptr
	size_t NetChgListTailPtr;			// Net change list tail ptr
	NetworkChanges LastEvent;			// Last network change
	NetworkChanges NetChgList[NET_EVENTS_MAX];
	CCCriticalSection NetChgListLock;	// Network change lock

	// This holds the "I've been changed" signal from the firmware until any 
	// polled apps that would care (ie, APS) ask for it; this is
	// basically a list of clear-on-read bits
	nodebool paramsHaveChanged[MN_API_MAX_NODES];

	nodeulong RingCmdsMax;				// Max # of simultaneous cmds in ring
	CCSemaphore CmdPaceSemaphore;		// Command pacing semaphore
#ifdef _DEBUG
	long	SemaCount;
#endif
	CCCriticalSection cmdLock;			// Command/Response data Lock

	// Interrupt events
	CCEvent IrqEvent;					// IRQ event signaller
	CCEvent ReadCommEvent;				// Read pacing signaller
	CCEvent GroupShutdownEvent;			// Group Shutdown signaller

	Uint32 ctsCount;

	// ---------------------------------
	// Diagnostics State
	// ---------------------------------
	CCEvent CmdGate;					// Spin-out gate for user cmds

	// ---------------------------------
	// Auto-Discovery State
	// ---------------------------------
	autoDiscoverThread *pAutoDiscover;	// Thread to support auto-discovery

	// ---------------------------------
	// Command Tracking State
	// ---------------------------------
	// These are the command tracking information records
	// They contain house keepers, events and tracking info. They are assigned 
	// via the pTrkTail pointer.  When the tracking is complete it is
	// put back at the tail.
	respTrackInfo **pTrkFixedList;		// Ptr to pTrkInfo ptrs
	respTrackInfo **pTrkList;			// Ptr to pTrkInfo ptrs
	respTrackInfo **pTrkTail;			// Ptr to available tracker
	respNodeList respNodeState[MN_API_MAX_NODES];
	respNodeList controlNodeState;


	CCCriticalSection IOlock;			// ISC-TG I/O RMW lock
	CCCriticalSection ErrListLock;		// Error lock


	// ---------------------------------
	// Attention Interface state
	// ---------------------------------
	CCCriticalSection AttnLock;			// Attention buffer lock
	std::queue<mnAttnReqReg> AttnBuf;		// Buffer of attentions from the ring
	nodebool AttnOverrun;				// Set on attention loss

	// ---------------------------------
	// Data Acquisition Interface
	// ---------------------------------
	dataAcqInfo DataAcq[MN_API_MAX_NODES];
	// This flag causes the sequence checker to initialize on
	// first packet.
	nodebool DataAcqInit[MN_API_MAX_NODES];

	// ---------------------------------
	// Channel activity generator
	// ---------------------------------
	// Polling thread to insure network watchdog is refreshed when online
	netPollerThread *pPollerThread;
	Uint32 pollDelayTimeMS;

	// ---------------------------------
	// Construct or destroy our instance
	// ---------------------------------
	netStateInfo(nodeulong nRingCmdsMax, netaddr cNum);
	~netStateInfo();

	// ---------------------------------
	// PUBLIC API
	// ---------------------------------

	// Command/Response Data Structure lock
	void enterCmdLock();
	void exitCmdLock();

	// Inquire if current thread is a recovery or finding nodes thread.
	nodebool isRecoveryThread();

	// Inquire if current thread is the read thread.
	nodebool isReadThread();

	// Inquire if brakes are available for this net.
	nodebool brakeAvailable();

	// Tracking data base maintainence
	void removeThisDBitem(
				respTrackInfo *pRespInfo,
				respNodeList *pRespArea);

	void removeHeadDBitem(
				respNodeList *pRespArea);

	// Waits for network traffic to complete without sending any data
	void waitForIdle();
};
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	lnkAccessCommon.h function prototypes
//		These are all C style interfaces
//
#ifdef __cplusplus
extern "C" {
#endif
//
	// ---------------------------------
	// PACKET ORIENTED INTERFACES
	// ---------------------------------
	// Low Level logged command transmission without tracking
	cnErrCode MN_DECL infcSendCommandLog(
			nodeushort cNum,						// Port Index
			packetbuf *theCommand);			    // Ptr to buffered command

	// ---------------------------------
	// CALLBACK INTERFACES
	// ---------------------------------

	// Internal function to fire off the error callback
	void infcFireErrCallback(
			infcErrInfo *errInfo,
			bool forced = false);

	// Internal function to fire off the network state callback
	void infcFireParamCallback(
			paramChangeFunc func,
			const mnParamChg *chgItem);

	// Internal function to fire off when the network state changes
	void infcFireNetEvent(
			netaddr cNum,
			NetworkChanges newState);

	// ---------------------------------
	// SERIAL INTERFACE DIAGNOSTICS
	// ---------------------------------

	// Set/get the physical port number and speed related to this controller
	cnErrCode MN_DECL infcSetPortSpecifier(
			netaddr cNum,
			const portSpec *pPortNumber	// Port Specification
	);
	cnErrCode MN_DECL infcGetPortSpecifier(
			netaddr cNum,
			portSpec *pPortNumber			// Port Specification
	);

	// Control and monitor snoop of other network
	void infcSetOtherNetSnoop(
			netaddr cNum,
			nodebool TheDTRValue);
	void infcGetOtherNetSnoop(
			netaddr cNum,
			nodebool *TheDTRValue);

	// Run the network/flow break diagnostic
	cnErrCode infcBrokenRingDiag(
			netaddr cNum,
			nodebool foundNodes,
			unsigned nodeCount);

	// Scan through the LastKnown network and search for offline nodes
	// issuing error codes.
	cnErrCode infcProbeForOffline(
			netaddr cNum,
			nodeulong *nNodes);

	// Flush network via a set of NoOp packets and clear any responses out
	Uint32 infcFlushProc(
			netaddr cNum);


	// ---------------------------------
	// BRAKE INTERFACE API
	// ---------------------------------
	// Low level function for controlling the brake
	cnErrCode infcBrakeSet(
			netaddr cNum,
			size_t brakeNum,
			nodebool engaged,
			nodebool forced=false);
	cnErrCode infcBrakeGet(
			netaddr cNum,
			size_t brakeNum,
			nodebool *pEngaged,
			nodebool forced=false);


	cnErrCode MN_DECL infcSetDiagBlock(
			netaddr cNum,
			nodebool state);

	cnErrCode MN_DECL infcGetDiagBlock(
			netaddr cNum,
			nodebool *state);

	// Start the auto network node discovery thread
	void MN_DECL infcStartAutoNetDiscovery(
			netaddr cNum,
			cnErrCode initialError);

	// Halt the auto network node discovery thread
	void MN_DECL infcHaltAutoNetDiscovery(
			netaddr cNum);

	// Returns activity status of auto-discover process
	nodebool infcAutoDiscoverRunning(netaddr cNum);

	// ---------------------------------
	// TRACE DUMP MANAGEMENT
	// ---------------------------------
	// Created next dump file
	cnErrCode infcTraceDumpNext(
			netaddr cNum);

	// Returns a string terminated with directory delimiter for
	// automatically generated dump files. For Windows
	// this directory is "%TEMP%\Teknic\".
	void infcGetDumpDir(
			char *pStr,						// Ptr to string area
			nodelong maxLen);				// Maximum size of string area


	// ---------------------------------
	// RECORD KEEPING & SETUP
	// ---------------------------------
	// Register <inventory> as the last inventory
	cnErrCode infcUpdateInventory(
			netaddr cNum,
			mnNetInvRecords *inventory);
	cnErrCode infcInitDataAcq(
			multiaddr multiAddr,
			double SampleRateMicroSec);
	// Provides stacked online/offline support each
	// initializing==true must be balanced with a
	// initializing==false and the lastErr==some code. If
	// stack unwind==0 and lastErr==MN_OK we go online.
	cnErrCode MN_DECL infcSetInitializeMode(
			netaddr cNum,
			nodebool initializing,
			cnErrCode lastErr);
	cnErrCode MN_DECL infcGetInitializeMode(
			netaddr cNum,
			nodebool *initializing);

	// Set the port connection to node's net
	void infcSetNetSource(
			netaddr cNum,
			mnNetSrcs theConnector);

	// ---------------------------------
	// PARAMETER STATE LOCK INTERFACE
	// ---------------------------------

	// Register transaction locks
	void infcGetIOlock(netaddr cNum);
	void infcReleaseIOlock(netaddr cNum);

#ifdef __cplusplus
}
#endif
//																			  *
//*****************************************************************************
#endif
/// \endcond

//=============================================================================
//	END OF FILE lnkAccessCommon.h
// ============================================================================

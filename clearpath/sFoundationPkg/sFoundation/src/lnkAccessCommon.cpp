//******************************************************************************
// $Archive: /ClearPath SC/User Driver/src/lnkAccessCommon.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: lnkAccessCommon.cpp $
//
// DESCRIPTION:
/**
\file
\cond INTERNAL_DOC
\brief Low Level Link Function Programming Interface Implementation.

Implements the programming interfaces related to link wide functionality.

\addtogroup LinkGrp
\brief Channel Access API
- Include "meridianHdrs.h" in your project to use these functions.

These functions are related to channel operation activities. They provide
for creating a diagnostic dump file of the last 4096 commands and access
to event driven Attention Packet receptions.


Platform independent low level packet driver for Meridian Networks.
All coding in this module uses generic C++ or platform virtualizing
classes to interface to a target operating system.

Each "controller" represents a network of Meridian Nodes connected via
a asynchronous serial port. The controller number starts at zero and
represents the first net. The actual port number is specified at
startup time.

The port is is assumed offline until and "initializing" event occurs.
Usually this is initiated via the mnInitializeNets functions.
The controller is "started" when the port and associated resources
are created and opened.

Threads are created to read responses, run diagnostics and a number of
"housekeeping" threads for tracking responses.
A number of event objects are created to allow the various parts of the
driver to "sync" up with the actions of the application and the driver
itself.

Threaded commands that require a response are parked via an unsignaled
event object until the read thread detects and matches it up with this
command.

Multiple command starts can be initiated at once and are paced via
a semaphore. As responses are received, another command is allowed to
start.

When an error is detected, all application threads are blocked via
an unsignalled event object and the error recovery thread is invoked
to detect network restoration. Network diagnostics are performed and
the results sent to the application via callback. If the disruption
persists, the application will periodically be notified of the current
diagnostic results.

On network restoration, the configuration is checked for changes and
if appears the same, the application is notified that network is online
and the block on existing traffic is released.

If the LPT port interrupts are available, they are used to pace the
sending and receiving of packets. This allows a minimum of polling
in the driver and prevents the high priority threads from "spinning"
out on hardware or power-failures.

MNCLASSEX EVENTS
NODES_OFFLINE
Initial state until initialization or the state that occurs
when network errors have been detected.	 Network traffic attempts
are failed with code MN_ERR_CMD_OFFLINE.
NODES_ONLINE
After initialization this state is signalled to allow the
application to interact with the network.
NODES_ONLINE_NO_TEST
After initialization this state is signalled to allow the
application to interact with the network when older firmware
has been detected. Interaction is blocked unless the little
publicized infcSetInitMode function is called to unlock
the operation with older firmware. This feature is used by
our diagnostic software as well as the RPE to allow interaction
with older nets.
NODES_RESETTING
This event is fired at the start of an initializing or resetting
operation.
NODES_SENDING
This event is sent during the resetting phase to signal
the collection of node information has begun. A NODES_ONLINE
event usually follows if this process completes without error.
NODES_NO_PORT
This event is sent if the initializing function cannot open
the NT device driver for this port.
NODES_NO_NET_CONTROLLER
This event is sent if the initializing function cannot find
a Network Controller.
NODES_CHANGING_NET_CONTROLLER
This event is sent to the current controller as well as the
new controller the current controller is changed.
**/
// CREATION DATE:
//		02/08/2010 13:20:40
//
// COPYRIGHT NOTICE:
//		02/11/1998 16:34:00 Original ControlPoint implementation
//		06/09/2009 17:39 Refactored from ControlPoint implementation
//		(C)Copyright 1998-2014  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
/// \endcond																   *
//******************************************************************************

/// \cond INTERNAL_DOC
//******************************************************************************
// NAME																	       *
// 	lnkAccessCommon.cpp headers
//
#include "lnkAccessCommon.h"
#include "netCmdPrivate.h"
#include "SerialEx.h"
#include "netCmdAPI.h"
// Std Library
#include <fstream>
// System include files
#include <assert.h>
#if (defined(_WIN32)||defined(_WIN64))
#include <direct.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#endif
/// \endcond																   *
//******************************************************************************


///	\cond INTERNAL_DOC
//******************************************************************************
// NAME																	       *
// 	lnkAccessCommon.cpp local function prototypes
//
//
#ifndef _WIN32
int GetLastError() {
	return 1;
}
#endif

void infcCopyPktToPkt18(
	packetbuf18 *dest,
	packetbuf *src);

// Process a node initiate packet such as attentions and errors
void processNodeInitiatedPkt(netaddr cNum, nodeaddr respAddr,
	netStateInfo *pNCS, packetbuf &readBuf);

// Process an attention packet
void processAttention(mnAttnReqReg theAttn, netStateInfo *pNCS);

// Start the node auto-discovery process
void autoDiscoverStart(infcErrInfo *pErrInfo);
void autoDiscoverStart(netaddr cNum, cnErrCode theErr);
void attemptAutoDiscover(infcErrInfo *pErrInfo);

cnErrCode MN_DECL infcChangeLinkSpeed(
	netaddr cNum,
	netRates newRate);

/// \endcond																   *
//******************************************************************************



///	\cond INTERNAL_DOC
//******************************************************************************
// NAME																	       *
// 	lnkAccessCommon.cpp constants
//
//
#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// Time between net re-discovery attempts
const unsigned AUTODISCOVER_WAIT_MS = 500;
// Wrap around value (%d) for dump files: "root%d.ext"
#define DUMP_WRAP_NUM			10
//- - - - - - - - - - - - - - - - - - - -
// TRACING/DEBUG SETUP
//- - - - - - - - - - - - - - - - - - - -
#if defined(_MSC_VER)
#define DBG_LOG(x) OutputDebugString(_T(x))
#else
#define DBG_LOG(x)
#endif

#ifdef _DEBUG
#define T_ON TRUE
#else
#define T_ON FALSE
#endif
#define T_OFF FALSE

#define TRACE_LOW_PRINT			T_OFF	// Print low level activities
#ifndef TRACE_SEND_RESP
#define TRACE_SEND_RESP			T_OFF	// Print commands/responses
#endif
#define TRACE_HIGH_LEVEL		T_OFF	// Print high level activities
#define TRACE_RD_THRD			T_OFF	// Print high level read thread activities
#define TRACE_POLL_THRD			T_OFF	// Print polling thread activities
#define TRACE_START_STOP		T_OFF	// Print controller start/stop events
#define TRACE_ERR_HNDL			T_OFF	// Print error handler actions
#define TRACE_TICKLER			T_OFF	// Print tickler actions
#define TRACE_LOCKS				T_OFF	// Trace cmd lock
#define TRACE_CALLBACK			T_OFF	// Trace callback functions
#define TRACE_HEAP				T_OFF	// Enable heap tracing
#define TRACE_IRQ				T_OFF	// Trace interrupt / HIGH LEVEL
#define TRACE_THREAD			T_OFF	// Thread create/exit IDs
#define TRACE_HALT				T_OFF	// Force printing of just read thread actions
#define TRACE_SERIAL			T_OFF	// Trace serial/flash API
#define TRACE_ATTN				T_OFF	// Trace attentions recv'd
#define TRACE_DESTRUCT			T_OFF	// Trace construction/destruction
#define TRACE_DACQ_SEQ			T_OFF	// Trace data acq sequence issues
#define TRACE_BAUD				T_OFF	// Trace baud rate changes
#define TRACE_NET_ERR			T_ON	// Dump net error packets
#define TRACE_SIZES				T_OFF	// Dump base type sizes
#define TRACE_BRAKE				T_OFF	// Trace brake actions
#define TRACE_BREAK				T_OFF	// Trace serial port breaks sent
#define TRACE_NET_NATIVE		T_OFF	// Show link layer as raw bytes
#define TRACE_FLUSH				T_OFF	// Show flush activities

#ifndef COMM_EVT_MAX_MS
#define COMM_EVT_BRK_DLY_MS 100
#endif

#if TRACE_LOCKS
#define ENTER_LOCK(where) \
	_RPT2(_CRT_WARN, "%.1f enterLock	%s\n", infcCoreTime(), where); pNCS->enterCmdLock();
#define EXIT_LOCK(where) \
	_RPT2(_CRT_WARN, "%.1f exitLock	%s\n", infcCoreTime(), where); pNCS->exitCmdLock();
#else
#define ENTER_LOCK(where) pNCS->enterCmdLock()
#define EXIT_LOCK(where) pNCS->exitCmdLock()
#endif

#ifdef __linux
#ifdef __LP64__
	#define THREAD_RADIX "0x%x"
	#define THREAD_RADIX_LONG "0x%lx"
#else
	#define THREAD_RADIX "%d"
	#define THREAD_RADIX_LONG "%ld"
#endif
#else
	#define THREAD_RADIX "0x%x"
	#define THREAD_RADIX_LONG "0x%lx"
#endif

// Power Types
#define PWR_CNT 8
const char *pwrTypes[PWR_CNT] = {
	"None", "X", "Y", "XY", "Z", "XZ", "YZ", "XYZ"
};
/// \endcond																   *
//******************************************************************************


///	\cond INTERNAL_DOC
//******************************************************************************
// NAME																	       *
// 	lnkAccessCommon.cpp static variables
//
//
///////////////////////
// Callback pointers //
///////////////////////

// Error reporting function
infcErrCallback userErrCallbackFunc = NULL;
// Command completion function
infcCmdCompleteCallback userCmdCompleteFunc = NULL;
// Network State change callback
infcNetStateCallback userNetStateFunc = NULL;
// Invalidate cache function
infcInvalCacheCallback userInvalCacheFunc = NULL;
/// \endcond																   *
//******************************************************************************


///	\cond INTERNAL_DOC
//******************************************************************************
// NAME																	       *
// 	lnkAccessCommon.cpp global variables
//
// Global access to network state instances
//netStateInfo *pNet[NET_CONTROLLER_MAX] = {NULL};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// These might be overwritten by OS specific persistance mechanism
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Create a trace dump on exit of library
BOOL InfcDumpOnExit = TRUE;
// Global time-out setting
unsigned InfcRespTimeOut = FRAME_READ_TIMEOUT + FRAME_READ_TIMEOUT / 8;
// Default read thread priority
int InfcPrioBoostFactor = 0;
// Last dump file number
unsigned InfcLastDumpNumber = 0;
// Inhibits running diagnostics
BOOL InfcDiagnosticsOn = TRUE;
// When non-zero, the system is initializing. This should always
// equal the sum of the SysInventory[*].Initializing
int InitializingGlobal = 0;
/// \endcond																   *
//******************************************************************************


///	\cond INTERNAL_DOC
//******************************************************************************
// NAME																	       *
// 	lnkAccessCommon.cpp imported variables
//
//
// Count of specified ports
extern unsigned SysPortCount;
// Inventory records of where nodes are
extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];

// State that maybe restored via offline persistance
extern BOOL InfcDiagnosticsOn;			// Inhibits running diagnostics
extern unsigned InfcLastDumpNumber;		// Last Dump File number
extern unsigned InfcRespTimeOut;		// Response time-out setting

										// Global references
extern mnNetDiagResults NetDiags[NET_CONTROLLER_MAX];

// Variables for debug thread locking
extern CCEvent debugGate;
extern CCEvent debugThreadLockResponseGate;
extern bool inDebugging;
extern nodeulong lockedThreadID;

/// \endcond																   *
//******************************************************************************


///	\cond INTERNAL_DOC
//******************************************************************************
//	NAME																	   *
//		DUMP_PKT
//
//	DESCRIPTION:
//		Write buffer as hex debug string.
//
//	SYNOPSIS:
void DUMP_PKT(netaddr cNum, const char *msg, packetbuf *buf, bool backArrow)
{
	#ifdef _DEBUG
	unsigned i, bLen;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS)
		return;

	_RPT5(_CRT_WARN, "%.1f %s(%d) %d: %s ", infcCoreTime(), msg, cNum,
		(int)pNCS->nRespOutstanding, backArrow ? "<-" : "->");
	if (buf->Byte.BufferSize > MN_API_PACKET_MAX)
		bLen = MN_API_PACKET_MAX;
	else
		bLen = buf->Byte.BufferSize;
	#if TRACE_NET_NATIVE
	packetbuf nativeBuf;
	unsigned char chksum = 0;
	_RPT0(_CRT_WARN, "N[");
	pNCS->pSerialPort->convert8to7(*buf, nativeBuf);
	nativeBuf.Byte.BufferSize = nativeBuf.Fld.PktLen + MN_API_PACKET_HDR_LEN;
	for (i = 0; i<nativeBuf.Byte.BufferSize; i++) {
		chksum -= nativeBuf.Byte.Buffer[i];
		_RPT1(_CRT_WARN, "%02X ", 0xFF & nativeBuf.Byte.Buffer[i]);
	}
	_RPT1(_CRT_WARN, "%02X] ", 0xFF & chksum);
	#endif
	for (i = 0; i<bLen; i++) {
		_RPT1(_CRT_WARN, "%02x ", 0xFF & buf->Byte.Buffer[i]);
	}
	_RPT0(_CRT_WARN, "\n");
	#endif
}
/// \endcond																   *
//******************************************************************************


///	\cond INTERNAL_DOC
//******************************************************************************
//******************************************************************************
//						   netStateInfo IMPLEMENTATION
//******************************************************************************
//******************************************************************************
//	NAME																	   *
//		netStateInfo::netStateInfo
//
//	DESCRIPTION:
///		Construct the state for a single serial network.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
netStateInfo::netStateInfo(nodeulong ringCmdsMax,
	netaddr controllerNum)
	: CmdPaceSemaphore(ringCmdsMax, ringCmdsMax)
{
	extern int InfcPrioBoostFactor;					// Read thread prio boost
	cNum = controllerNum;							// Our index
	nodeaddr node;
	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f netStateInfo(new)(%d) constructing...\n",
		infcCoreTime(), cNum);
	#endif
	#ifdef USE_ACTIVEX
	mnCookie = 0;
	mnThreadID = 0;
	ChangeRecurse = 0;
	NetChangeMarshalled = TRUE;
	#endif

	pSerialPort = NULL;						// Insure pointer initialized
	ctsCount = 0;
	#ifdef _DEBUG
	SemaCount = 0;
	#endif
	OurNetConnector = MN_SRC_UNKNOWN_NET;
	ErrListHeadPtr = ErrListTailPtr = 0;
	NetChgListHeadPtr = NetChgListTailPtr = 0;
	AttnOverrun = FALSE;					// Nothing in error, yet
	LastEvent = NODES_OFFLINE;				// We start from offline
	pAutoDiscover = NULL;
	// Offline tickler
	SelfDestruct = false;					// No death yet

	RingCmdsMax = ringCmdsMax;				// Save for destruction time
											// Adjust select event objects
	CmdGate.SetEvent();
	// We start idle
	CmdsIdle.SetEvent();

	nPktsSent = nPktsRcvd = nRespOutstanding = 0;

	for (node = 0; node<MN_API_MAX_NODES; node++) {
		// Initialize the node response databases, assuming no waiters
		respNodeState[node].head = NULL;
		respNodeState[node].tail = NULL;
		respNodeState[node].errPkt.Byte.BufferSize = 0;
		respNodeState[node].sendCnt = 0;
		respNodeState[node].respCnt = 0;

		//while we're going through the nodes, mark that they haven't been changed
		paramsHaveChanged[node] = FALSE;
		// Force init on first data acquisition packet.
		DataAcqInit[node] = TRUE;
		// Nothing yet
		SysInventory[cNum].diagsAvailable[node] = FALSE;
		SysInventory[cNum].diagStats[node].Clear();
	}
	// Nothing yet
	SysInventory[cNum].diagsAvailable[MN_API_MAX_NODES] = FALSE;
	// Take care of the host's extra diags slot off the end
	SysInventory[cNum].diagStats[MN_API_MAX_NODES].Clear();

	controlNodeState.head = NULL;
	controlNodeState.tail = NULL;
	controlNodeState.errPkt.Byte.BufferSize = 0;
	controlNodeState.sendCnt = 0;
	controlNodeState.respCnt = 0;

	// Create our port
	pSerialPort = new CSerialEx;
	#if TRACE_SIZES
	_RPT2(_CRT_WARN, "CSerialEx size=%d(0x%x)\n",
		sizeof(CSerialEx), sizeof(CSerialEx));
	#endif
	assert(pSerialPort);

	// Create threads
	ReadThread.SetTerminateFlag(&SelfDestruct);

	// Create trackers for each possible command in progress on this net
	pTrkFixedList = (respTrackInfo **)malloc(sizeof(respTrackInfo *)*ringCmdsMax);
	pTrkList = (respTrackInfo **)malloc(sizeof(respTrackInfo *)*ringCmdsMax);
	for (node = 0; node<ringCmdsMax; node++) {
		pTrkFixedList[node] = new respTrackInfo;
		pTrkList[node] = pTrkFixedList[node];
	}
	assert(pTrkList);
	#if TRACE_SIZES
	_RPT2(_CRT_WARN, "respTrackInfo size=%d(0x%x)\n",
		sizeof(respTrackInfo)*ringCmdsMax,
		sizeof(respTrackInfo)*ringCmdsMax);
	_RPT1(_CRT_WARN, "sizeof(rxTraceBuf)=%d\n", sizeof(rxTraceBuf));
	_RPT1(_CRT_WARN, "sizeof(txTraceBuf)=%d\n", sizeof(txTraceBuf));
	_RPT1(_CRT_WARN, "sizeof(traceHeader)=%d\n", sizeof(traceHeader));
	#endif

	// Insure the tail is initialized
	pTrkTail = &pTrkList[ringCmdsMax - 1];
	// Create our discovery thread
	pAutoDiscover = new autoDiscoverThread;
	#if TRACE_SIZES
	_RPT2(_CRT_WARN, "autoDiscover size=%d(0x%x)\n",
		sizeof(autoDiscoverThread),
		sizeof(autoDiscoverThread));
	#endif
	// Get it started until parking point
	pAutoDiscover->LaunchThread(this);
	pAutoDiscover->WaitUntilParked();
	errorRecursePrevent = 0;


	// Lastly, start our read thread now that our state has settled in
	ReadThread.LaunchThread(this, InfcPrioBoostFactor);
	// Wait for it to start up and enter "halted" state
	ReadThread.ForceStop();

	// Initialize the polling rate and start in halted state
	pollDelayTimeMS = 250;
	pPollerThread = new	netPollerThread(this);
	pPollerThread->LaunchThread(this);

	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f netStateInfo(new)(%d) finished...\n",
		infcCoreTime(), cNum);
	#endif
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netStateInfo::~netStateInfo
//
//	DESCRIPTION:
///		Destruct the state for a single serial network.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
netStateInfo::~netStateInfo()
{
	//cnErrCode theErr = MN_OK;
	unsigned i;

	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	double sTime, eTime;						// Profiling time
	#endif

												// Bad news see DllMain MSDN definition of not allowing Sync Waits on
												// threads created with DLL operations. One must set a proxy "exit" event
												// set just before exit of thread control function to sync with end of
												// thread activity.  After this event is signalled it will be assumed that
												// no other activity is generated by the thread function.

												// Prevent more activities on this port
	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	sTime = infcCoreTime();
	_RPT2(_CRT_WARN, "%.1f ~netStateInfo(%d) starting...\n",
		sTime, cNum);
	#endif
	// Wait for commands to terminate
	if (pSerialPort && pSerialPort->IsOpen())
		CmdsIdle.WaitFor();
	// Start all the threads exiting, avoid error on these commands
	SelfDestruct = TRUE;
	// Force reporter to start and terminate
	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f ~netStateInfo(%d) deleting auto-discover...\n",
		sTime, cNum);
	#endif
	//// Let pending work drain out
	//if (!Offline) {
	//	waitForIdle();
	//}
	// Allow no more "work"
	///Offline = TRUE;
	// Get rid of the auto-discovery to avoid starting new commands
	if (pAutoDiscover) {
		delete pAutoDiscover;
		pAutoDiscover = NULL;
	}

	if (pPollerThread) {
		delete pPollerThread;
		pPollerThread = NULL;
	}

	// Restart the the waiting responses
	for (i = 0; i<RingCmdsMax; i++) {
		// Signal events waiting for responses
		if (pTrkFixedList && pTrkFixedList[i])
			pTrkFixedList[i]->evtRespWait.SetEvent();
	}
	// Make sure those waiting (re)start
	IrqEvent.SetEvent();
	ReadCommEvent.SetEvent();
	CmdGate.SetEvent();
	ReadThread.Start();

	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f ~netStateInfo(%d) about to free node info\n",
		infcCoreTime(), cNum);
	#endif

	// Wait for read thread to terminate to prevent access violations
	ReadThread.WaitForTerm();

	// Done with serial port now
	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f ~netStateInfo(%d) deleting serial port\n",
		infcCoreTime(), cNum);
	#endif
	if (pSerialPort) {
		delete pSerialPort;
		pSerialPort = NULL;
	}

	// Free all of our node class memory
	coreFreeAllNodeInfo(cNum);
	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f ~netStateInfo(%d) node info free\n",
		infcCoreTime(), cNum);
	#endif

	pTrkTail = NULL;
	if (pTrkList) {
		// Return the tracking database memory
		for (i = 0; i < RingCmdsMax; i++) {
			pTrkList[i] = NULL;
		}
		free(pTrkList);
		pTrkList = NULL;
	}
	if (pTrkFixedList) {
		// Return the tracking database memory
		for (i = 0; i < RingCmdsMax; i++) {
			if (pTrkFixedList[i]) {
				delete pTrkFixedList[i];
				pTrkFixedList[i] = NULL;
			}
		}
		free(pTrkFixedList);
		pTrkFixedList = NULL;
	}

	// Relieve the Initialization stack in inventory
	SysInventory[cNum].Initializing = 0;

	// This controller is all deleted
	#if TRACE_LOW_LEVEL || TRACE_DESTRUCT
	eTime = infcCoreTime();
	_RPT3(_CRT_WARN, "%.1f ~netStateInfo(%d) done, duration=%.1f\n",
		eTime, cNum, eTime - sTime);
	#endif
	SysInventory[cNum].clearNodes(true);
	// Let events propagate
	infcSleep(0);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netStateInfo::enterCmdLock
//
//	DESCRIPTION:
//		Enter command interface lock section
//
//	SYNOPSIS:
void netStateInfo::enterCmdLock() {
	if (!cmdLock.Lock()) {
		_RPT1(_CRT_WARN, "enterCmdLock%d: time-out\n", cNum);
	}
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netStateInfo::exitCmdLock
//
//	DESCRIPTION:
//		Exit lock section
//
//	RETURNS:
//		MN_OK if sucessfully updated
//
//	SYNOPSIS:
void netStateInfo::exitCmdLock()
{
	if (!cmdLock.Unlock()) {
		_RPT2(_CRT_WARN, "exitCmdLock: semaphore %d release err 0x%X\n",
			cNum, GetLastError());
	}
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netStateInfo::isRecoveryThread
//
//	DESCRIPTION:
//		Return true if the current thread is one of the recovery threads.
//
//	RETURNS:
//		bool
//
//	SYNOPSIS:
nodebool netStateInfo::isRecoveryThread()
{
	nodeulong curThread = CThread::CurrentThreadID();

	return ((pAutoDiscover && (pAutoDiscover->ThreadID() == curThread)));
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netStateInfo::isReadThread
//
//	DESCRIPTION:
//		Return true if the current thread is the read thread.
//
//	RETURNS:
//		bool
//
//	SYNOPSIS:
nodebool netStateInfo::isReadThread()
{
	nodeulong curThread = CThread::CurrentThreadID();

	return ((ReadThread.ThreadID() == curThread));
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		netStateInfo::brakeAvailable
//
//	DESCRIPTION:
//		Return true if brakes are available and powered on this net.
//
//	RETURNS:
//		bool
//
//	SYNOPSIS:
nodebool netStateInfo::brakeAvailable()
{
	return (pSerialPort && pSerialPort->GetDSR());
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::logSend
//
//	DESCRIPTION:
//		Log the sent item and return the TX serial number.
//
//	RETURNS:
//		this entry's serial number
//
//	SYNOPSIS:
unsigned mnNetInvRecords::logSend(packetbuf *cmd, cnErrCode theErr, double timeStamp)
{
	unsigned nextSerNum = sendSerNum, indx;
	if (!pNCS) return nextSerNum;
	TXlogLock.Lock();
	// Save and increment the serial number
	nextSerNum = sendSerNum++;
	// Wrap the log file index
	indx = nextSerNum % SEND_DEPTH;
	// Setup issues!
	assert(txTraces);

	if (TraceActive) {
		txTraceBuf *pTxTrace;					// Pointer to tx traces
												// Make shortcut to transmit trace log entry for the rest of command
		pTxTrace = &txTraces[indx];
		txTraceIndx = indx;
		// Log the send number
		pTxTrace->order = nextSerNum;
		// Log when it left here
		pTxTrace->timeStamp = timeStamp;
		// Log the command itself
		pTxTrace->packet = *cmd;
		// Log command depth
		pTxTrace->depth = pNCS->nRespOutstanding;
		// Log as a failure
		pTxTrace->failed = (unsigned short)theErr;
	}
	TXlogLock.Unlock();
	#if defined(_DEBUG)
	if (theErr != MN_OK) {
		_RPT3(_CRT_WARN, "%.1f TX Error(%d): 0x%x\n", infcCoreTime(), pNCS->cNum, theErr);
		DUMP_PKT(pNCS->cNum, "...", cmd);
	}
	#endif
	return(nextSerNum);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::logReceive
//
//	DESCRIPTION:
//		Log a receive packet.
//
//	RETURNS:
//		elapsed command time if known, else 0
//
//	SYNOPSIS:
double mnNetInvRecords::logReceive(
	packetbuf *readBuf,
	cnErrCode theErr,
	respTrackInfo *fillInfo,
	double timeStamp)
{
	size_t rxSerNum, rxTraceIndex;
	rxTraceBuf *pRXtrc;
	txTraceBuf *pTXtrc;
	double eTime = 0;
	if (!pNCS) return 0;
	// Setup issues!
	assert(rxTraces);
	assert(txTraces);

	RXlogLock.Lock();
	rxSerNum = respSerNum++;
	// Wrap the log item
	rxTraceIndex = rxTraceIndx = rxSerNum % RECV_DEPTH;
	// Setup "shortcuts"
	pRXtrc = &rxTraces[rxTraceIndex];
	// Do actual tracing work if turned on
	if (TraceActive) {
		// Log the serial number of received packet
		pRXtrc->order = (Uint32)rxSerNum;
		// Log the receipt time
		pRXtrc->timeStamp = timeStamp;
		// Log the packet
		pRXtrc->packet = *readBuf;
		if (fillInfo) {
			// Expected a response here
			pTXtrc = &txTraces[fillInfo->sendSerNum%SEND_DEPTH];
			pRXtrc->sendCnt = fillInfo->nSentAtAddr;
			pRXtrc->sendSer = fillInfo->sendSerNum;
			eTime = pRXtrc->timeStamp - pTXtrc->timeStamp;
		}
		else {
			// Unexpected response or error
			pRXtrc->sendCnt = -1;
			pRXtrc->sendSer = -1;
		}
	}
	// Record the error code
	pRXtrc->error = theErr;
	RXlogLock.Unlock();
	#if 1 //defined(_DEBUG)
	//	if (theErr != MN_OK) {
	if (theErr != MN_OK && pNCS->cNum == 1) {
		_RPT4(_CRT_WARN, "%.1f RX Error(%d): 0x%x, thread=" THREAD_RADIX_LONG "\n",
			infcCoreTime(), pNCS->cNum, theErr, infcThreadID());
	}
	#endif
	return(eTime);
}
//																			   *
//******************************************************************************


//****************************************************************************
//	NAME																	 *
//		netStateInfo::removeHeadDBitem
//
//	DESCRIPTION:
//		Remove the head item from the <respArea> nodelist and relieve the
//		command pacing semaphore by one item.
//
// 		NOTE: The response database should be locked when this function is
//		called.
//
//	SYNOPSIS:
void netStateInfo::removeHeadDBitem(
	respNodeList *pRespArea)
{
	respTrackInfo *pThisInfo, *pNextInfo;		// Parsers of response DB
	BOOL dataOK;

	// No node list
	if ((pRespArea == NULL) || (pRespArea->head == NULL)) {
		_RPT0(_CRT_WARN, "removeHeadDBitem: NULL response or head!\n");
		return;
	}

	// Init parsers
	pThisInfo = pRespArea->head;
	pNextInfo = pThisInfo->next;
	// Unlink the head and point at tail
	pRespArea->head = pNextInfo;
	if (pRespArea->head == NULL) {
		// Clean up our tail if we are empty
		pRespArea->tail = NULL;
	}
	// Signal we have something
	//if (pThisInfo->evtRespWait.isOK()) {
	//_RPT2(_CRT_WARN," removeHeadDB wait event set hndl=0x%0x, %.2f\n",
	//	pThisInfo->hRespWait, infcCoreTime());
	pThisInfo->evtRespWait.SetEvent();
	// Synchronize with others waiting
	pThisInfo->evtRespWait.WaitFor();
	//}
	// Return this tracker to pool
	++pTrkTail;					// Create space at the end
	*pTrkTail = pThisInfo;
	// Release the command semaphore allowing one more
	//_RPT1(_CRT_WARN, "returnHead @ 0x%x\n", pThisInfo);
	//_RPT2(_CRT_WARN, "Release PACE(removeDBhead) cmd=%d rank=%d\n", pThisInfo->sendSerNum, nRespOutstanding);
	// There is one less to expect now
	#if defined(_DEBUG)
	dataOK = CmdPaceSemaphore.Unlock(1, &SemaCount);
	//if ((SysInventory[cNum].NumCmdsInRing-SemaCount)!=nRespOutstanding) {
	//	_RPT3(_CRT_WARN,"%.1f removeHeadDBitem semaphore mismatch! @%d we think %d\n",
	//			infcCoreTime(), (SysInventory[cNum].NumCmdsInRing-SemaCount),
	//			nRespOutstanding);
	//}
	#else
	dataOK = CmdPaceSemaphore.Unlock();
	#endif
	if (dataOK) {
		// Make sure the read thread keeps running
		if (nRespOutstanding > 0)
			if (--nRespOutstanding > 0)
				ReadThread.Start();
	}
	else {
		infcErrInfo semaErr;
		int semaErrCode = GetLastError();
		semaErr.errCode = MN_ERR_SEND_UNLOCK;
		semaErr.cNum = cNum;
		semaErr.node = semaErrCode;
		infcFireErrCallback(&semaErr);
		_RPT1(_CRT_ERROR, "removeHeadDBitem: semaphore release err 0x%X\n",
			semaErrCode);
	}
	//_RPT1(_CRT_WARN, "returnThis @ 0x%x\n", pThisInfo);
}
//																			 *
//****************************************************************************



//******************************************************************************
//	NAME																	   *
//		netStateInfo::removeThisDBitem
//
//	DESCRIPTION:
//		This is the function called to remove a database item from the running
//		queue. The command pacing semaphore is relieved by one item.
//
// 		NOTE: The response database should be locked when this function is
//		called.
//
//	SYNOPSIS:
void netStateInfo::removeThisDBitem(
	respTrackInfo *pRespInfo,
	respNodeList *pRespArea)
{
	respTrackInfo *pThisInfo, *pLastInfo;		// Parsers of response DB
	BOOL dataOK;

	// No node list
	if ((pRespArea == NULL) || (pRespInfo == NULL) || pRespArea->head == NULL)
		return;

	// Init parsers
	pThisInfo = pLastInfo = pRespArea->head;
	// Track down database links looking for us
	while (pThisInfo != NULL) {
		if (pThisInfo == pRespInfo) {
			// It's us, remove our entry, are we head?
			if (pThisInfo == pRespArea->head) {
				// Yes, change head to next
				pRespArea->head = pRespInfo->next;
			}
			else {
				// Unlink us from the chain
				pLastInfo->next = pThisInfo->next;
			}
			// Clean up the tail, if we are it
			if (pRespInfo == pRespArea->tail) {
				pRespArea->tail = NULL;
			}
			break;
		}
		// Look at the next item in the list
		pLastInfo = pThisInfo;
		pThisInfo = pThisInfo->next;
	}
	if (pThisInfo == NULL) {
		// Item not found!
		_RPT0(_CRT_WARN, "removeDB did not find our item!\n");
		return;
	}
	// Signal we have something
	if (pRespInfo->evtRespWait.isOK()) {
		#if TRACE_LOW_PRINT
		_RPT2(_CRT_WARN, "%.1f removeDB wait event set hndl=0x%0lx\n",
			infcCoreTime(), reinterpret_cast<long>(pRespInfo->evtRespWait.GetHandle()));
		#endif
		pRespInfo->evtRespWait.SetEvent();
		// Synchronize with others waiting
		pRespInfo->evtRespWait.WaitFor();
	}
	// Return this tracker to the end of the pool
	++pTrkTail;
	*pTrkTail = pRespInfo;
	//_RPT1(_CRT_WARN, "returnThis @ 0x%x\n", pRespInfo);
	// Release the command semaphore allowing one more
	//_RPT2(_CRT_WARN, "Release PACE(removeDB) cmd=%d rank=%d\n", pRespInfo->sendSerNum, nRespOutstanding);
	// There is one less to expect now
	#ifdef _DEBUG
	dataOK = CmdPaceSemaphore.Unlock(1, &SemaCount);
	#else
	dataOK = CmdPaceSemaphore.Unlock();
	#endif
	if (dataOK) {
		if (nRespOutstanding > 0)
			if (--nRespOutstanding > 0)
				ReadThread.Start();
		//_RPT1(_CRT_WARN, "RemoveFromDB outstanding=%d\n",
		//				(int)nRespOutstanding);
	}
	else {
		infcErrInfo semaErr;
		int semaErrCode = GetLastError();
		semaErr.errCode = MN_ERR_SEND_UNLOCK;
		semaErr.cNum = cNum;
		semaErr.node = semaErrCode;
		infcFireErrCallback(&semaErr);
		_RPT1(_CRT_ERROR, "removeThisDBitem: semaphore release err 0x%X\n",
			semaErrCode);
	}

}
//																			  *
//*****************************************************************************



//******************************************************************************
//	NAME																	   *
//		netStateInfo::waitForIdle
//
//	DESCRIPTION:
//		Wait for all the network history to time-out.
//
//	SYNOPSIS:
void netStateInfo::waitForIdle()
{
	while (ReadThread.IsRunning() && !SelfDestruct && nRespOutstanding > 0) {
		infcSleep(100);
	}
}
/// \endcond																  *
//*****************************************************************************


//******************************************************************************
//******************************************************************************
//						readPktThread IMPLEMENTATION
//******************************************************************************
///	\cond INTERNAL_DOC
//******************************************************************************
//	NAME																	   *
//		readPktThread construction/destruction
//
//	DESCRIPTION:
///
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
readPktThread::readPktThread()
{
	#if TRACE_THREAD || TRACE_DESTRUCT
	_RPT1(_CRT_WARN, "%.1f readPktThread constructing\n",
		infcCoreTime());
	#endif
	#if (defined(_WIN32)||defined(_WIN64))
	SetDLLterm(true);
	#endif
	// 4/29/11 DS Leads to issues with Win7 UseWithCOM();
	pNCS = NULL;
	// Insure we halt at HALTED state
	m_InternalSync.ResetEvent();
	m_state = m_lastState = READ_HALT_REQ;
}


readPktThread::~readPktThread() {
	#if TRACE_THREAD || TRACE_DESTRUCT
	double sTime = infcCoreTime();
	#endif
	//TerminateAndWait();
	#if TRACE_THREAD || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f readPktThread destructed, elapsed=%.1f\n",
		infcCoreTime(), infcCoreTime() - sTime);
	#endif
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		readPktThread::Run
//
//	DESCRIPTION:
///
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
int readPktThread::Run(void *context)
{
	cnErrCode theErr;
	nodeaddr respAddr;
	packetbuf readBuf;
	respTrackInfo *pFillInfo;
	respNodeList *pNodeList;
	double eTime = 0;	// Execution time
	double rxTime;

	// Initialize our netStateInfo pointer to allow interactions with
	// the network.
	pNCS = static_cast<netStateInfo *>(context);
	mnNetInvRecords &theNet = SysInventory[pNCS->cNum];

	#define RD_TIME_TRC 0
	#if RD_TIME_TRC
	double lTime, // Last time running start
		iTime; // Time since last run
	#endif
	#if TRACE_CMD_PROFILE
	double pTime = infcCoreTime();
	#endif
	infcErrInfo errInfo;				// Error reporting buffer
	BOOL waitOK = false;
	BOOL doTheRead, ctsEvent;

	#if TRACE_RD_THRD||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f readPktThread(%d): id=" THREAD_RADIX " starting\n",
		infcCoreTime(), pNCS->cNum, UIthreadID());
	#endif

	// Initializing
	m_InternalSync.SetEvent();			// Insure we drop through
	m_state = READ_HALT_REQ;

	while (!((m_pTermFlag != NULL) && *m_pTermFlag) && m_state != READ_SHUTDOWN) {
		//_RPT2(_CRT_WARN, "%.1f <1{%d}\n", infcCoreTime(), pNCS->nRespOutstanding);
		#if TRACE_RD_THRD
		if (m_state == READ_HALTED) {
			_RPT2(_CRT_WARN, "%.1f readThread(%d) halted and waiting for restart\n",
				infcCoreTime(), pNCS->cNum);
		}
		#endif
		// Read Thread Operation Gate: if not-signalled, wait for a
		// someone to restart us
		waitOK = m_InternalSync.WaitFor(INFINITE);
		if (!waitOK) {
			_RPT1(_CRT_ERROR, "readThread(%d): InternalSync failed\n", pNCS->cNum);
		}
		#if RD_TIME_TRC
		iTime = infcCoreTime() - lTime;
		lTime = infcCoreTime();
		#endif
		// Run the proper state
		//_RPT2(_CRT_WARN, "%.1f pre-Rlock>, state=%d\n", infcCoreTime(), m_state);
		// Protect read thread state from start/stops until settled out
		ReadLock.Lock();
		//_RPT2(_CRT_WARN, "%.1f Rlocked, state=%d\n", infcCoreTime(), m_state);

		switch (m_state) {
		case READ_RUNNING:
			// Allow state changes until end of cycle
			//_RPT1(_CRT_WARN, "%.1f pre-Runlock>\n", infcCoreTime());
			ReadLock.Unlock();
			//_RPT1(_CRT_WARN, "%.1f Runlock>\n", infcCoreTime());

			// Time to drop out and "poll"
			#define RD_THREAD_PREMPTIVE_WAIT 100

			if (theNet.PortIsOpen()) {
				// Wait for the interrupt to occur or the timeout
				waitOK = pNCS->ReadCommEvent.WaitFor(RD_THREAD_PREMPTIVE_WAIT);
			}
			else {
				// The port is not open any more, so halt ourselves
				ReadLock.Lock();
				NextState(READ_HALT_REQ);
				ReadLock.Unlock();
				break;
			}

			CSerialExErrReportInfo errReport;
			pNCS->pSerialPort->ErrorReportGet(&errReport);

			ctsEvent = pNCS->ctsCount != errReport.CTScnt
				&& m_state == READ_RUNNING
				&& (theNet.OpenState != FLASHING);

			if (ctsEvent && !((m_pTermFlag != NULL) && *m_pTermFlag)) {
				// Issue node stops if CTS gets de-asserted
				theNet.GroupShutdownRequest = true;
				pNCS->GroupShutdownEvent.SetEvent();
				pNCS->ctsCount = errReport.CTScnt;
			}

			//DEBUG
			//_RPT1(_CRT_WARN, "%.1f *\n", infcCoreTime());
			// Read should be OK if comm event signalled, we there is a packet,
			// the read thread is running and we are not in serial mode.
			doTheRead = waitOK
				&& pNCS->pSerialPort->IsPacketAvailable()
				&& m_state == READ_RUNNING
				&& (theNet.OpenState != FLASHING);


			if (doTheRead && !((m_pTermFlag != NULL) && *m_pTermFlag)) {
				// Yes, read it now
				readBuf.Byte.BufferSize = 0;					// Null in case of failure
				eTime = infcCoreTime();					// Make sure time is reset
				theErr = infcGetResponse(pNCS->cNum, &readBuf);
				rxTime = infcCoreTime();
				eTime = rxTime - eTime;

				// Lock other command starts and housekeepers out here
				ENTER_LOCK("readThreadProc");
				// How did the read go?
				if (theErr == MN_OK) {
					#if TRACE_SEND_RESP
					if (readBuf.Fld.Src == MN_SRC_HOST) {
						netErrGeneric *pGenErr = (netErrGeneric *)&readBuf.Byte.Buffer[2];
						//_RPT3(_CRT_WARN, "readThread    %.1f (%d) %d: <- ",
						//								infcCoreTime(),	pNCS->cNum,
						//								pNCS->nRespOutstanding);
						DUMP_PKT(pNCS->cNum, "readThread    ", &readBuf, true);
						//for (respAddr=0; respAddr<readBuf.Byte.BufferSize; respAddr++) {
						//	_RPT1(_CRT_WARN, "%02x ", 0xFF & readBuf.Byte.Buffer[respAddr]);
						//}
						if (readBuf.Fld.PktType == MN_PKT_TYPE_ERROR) {
							switch (pGenErr->Fld.ErrCls) {
							case ND_ERRCLS_NET:
								_RPT1(_CRT_WARN, " (net err %d)\n", pGenErr->Fld.ErrCode);
								break;
							case ND_ERRCLS_CMD:
								_RPT1(_CRT_WARN, " (cmd err %d)\n", pGenErr->Fld.ErrCode);
								break;
							default:
								_RPT2(_CRT_WARN, " (err cls %d, code=%d)\n", pGenErr->Fld.ErrCls,
									pGenErr->Fld.ErrCode);
								break;
							}
						}
						else {
							_RPT0(_CRT_WARN, "\n");
						}
					}
					#endif
					// Get address of the responder
					if (readBuf.Byte.BufferSize == 0) {
						theErr = MN_ERR_NULL_RETURN;
						// Log the outcome
						theNet.logReceive(&readBuf, theErr, NULL, rxTime);
						// Release cmd lock on database
						EXIT_LOCK("readThreadProc (nil buffer)");
					}
					else {
						// Extract address once to "automatic"
						respAddr = readBuf.Fld.Addr;
						// Set nodeList to appropriate response database
						if (MN_PKT_IS_HIGH_PRIO(readBuf.Fld.PktType)) {
							// This is a control packet
							pNodeList = &pNCS->controlNodeState;
						}
						else {
							// This is a regular packet
							pNodeList = &pNCS->respNodeState[respAddr];
						}
						// Update statistic
						pNodeList->respCnt++;
						// Get the head of our database item
						pFillInfo = pNodeList->head;
						// Source from the host?
						if (readBuf.Fld.Src == MN_SRC_HOST) {
							// This response originated from the host.

							// Did we find the expected response record in command DB?
							//  and not a network error
							netErrGeneric *pErrPkt = (netErrGeneric *)&readBuf.Byte.Buffer[RESP_LOC];
							bool isNetReject = ((unsigned)readBuf.Fld.PktType == MN_PKT_TYPE_ERROR)
								&& (pErrPkt->Fld.ErrCls == ND_ERRCLS_NET);
							if (isNetReject) {
								EXIT_LOCK("readThreadProc (isNetReject)");
								//In this case, the host noticed a net error while reading in
								// a packet; increment the host diagStats to signal to those who
								// might care

								infcErrInfo errInfo;
								nodeulong fakeAddr;
								// This will always appear @ 1+End
								fakeAddr = MN_API_MAX_NODES;

								// Setup core error messages
								errInfo.cNum = pNCS->cNum;
								errInfo.node = fakeAddr;
								// Let others know of the troubles
								// .. this maybe overridden on a case by case basis.
								errInfo.errCode = MN_ERR_NET_ERRS_DETECT;
								infcCopyPktToPkt18(&errInfo.response, &readBuf);

								mnNetDiagStats *pLog = &theNet.diagStats[fakeAddr];

								#if defined(_DEBUG)&&0
								if (!theNet.Initializing) {
									_RPT2(_CRT_WARN, "Host net reject: cNum=%d, pkterr=%d\n",
										pNCS->cNum, pErrPkt->Fld.ErrCode);
								}
								#endif

								//Set the type of net error that was detected
								if (pNCS->OurNetConnector == MN_SRC_APP_NET) {
									switch (pErrPkt->Fld.ErrCode) {
									case ND_ERRNET_FRAG:
										pLog->AppNetFragPktCtr++;
										break;
									case ND_ERRNET_CHKSUM:
										pLog->AppNetBadChksumCtr++;
										break;
									case ND_ERRNET_STRAY:
										pLog->AppNetStrayCtr++;
										break;
									case ND_ERRNET_PORT_OVERRUN:
										pLog->AppNetOverrunCtr++;
										errInfo.errCode = MN_ERR_HOST_APP_OVERRUN;
										// Show lead up issue
										infcTraceDumpNext(errInfo.cNum);
										break;
									default:
										_RPT1(_CRT_WARN, "Unknown host app net reject (%d)\n",
											pErrPkt->Fld.ErrCode);
									}
								}
								else {
									switch (pErrPkt->Fld.ErrCode) {
									case ND_ERRNET_FRAG:
										pLog->DiagNetFragPktCtr = 1;
										break;
									case ND_ERRNET_CHKSUM:
										pLog->DiagNetBadChksumCtr = 1;
										break;
									case ND_ERRNET_STRAY:
										pLog->DiagNetStrayCtr = 1;
										break;
									case ND_ERRNET_PORT_OVERRUN:
										pLog->DiagNetOverrunCtr = 1;
										errInfo.errCode = MN_ERR_HOST_DIAG_OVERRUN;
										// Show lead up issue
										infcTraceDumpNext(errInfo.cNum);
										break;
									default:
										_RPT1(_CRT_WARN, "Unknown host diag net reject (%d)\n",
											pErrPkt->Fld.ErrCode);
									}
								}

								theNet.diagsAvailable[fakeAddr] = TRUE;
								#if TRACE_NET_ERR
								_RPT2(_CRT_WARN, "%.1f readThread(%d) host detected err: \n", 
									infcCoreTime(), pNCS->cNum);
								//DUMP_PKT(cNum, (char *)"...", &readBuf);
								_RPT2(_CRT_WARN, "Net Diag Counters:\tApp Net\t\t\t\t"
									"Diag Net reporting node %d, net %d\n",
									fakeAddr, pNCS->cNum);
								_RPT0(_CRT_WARN, "\tfrags\tchks\tstray\tover\t\t"
									"frags\tchks\tstray\tover\n");
								_RPT4(_CRT_WARN, "%8d%8d%8d%8d",
									pLog->AppNetFragPktCtr, pLog->AppNetBadChksumCtr,
									pLog->AppNetStrayCtr, pLog->AppNetOverrunCtr);
								_RPT4(_CRT_WARN, "\t%8d%8d%8d%8d\n",
									pLog->DiagNetFragPktCtr, pLog->DiagNetBadChksumCtr,
									pLog->DiagNetStrayCtr, pLog->DiagNetOverrunCtr);
								#endif
								infcFireErrCallback(&errInfo);
							}
							// Expecting response?
							else if (pFillInfo != NULL) {
								// Yes, get and remove from the DB
								#if TRACE_LOW_PRINT&&TRACE_SEND_RESP
								if (MN_PKT_IS_HIGH_PRIO(readBuf.Fld.PktType)) {
									if (pFillInfo->nSentAtAddr == pNodeList->respCnt)
										_RPT2(_CRT_WARN, "R%d<%d>", respAddr,
											pNodeList->respCnt);
									else
										_RPT3(_CRT_WARN, "R%d<%d>->(%d)??",
											respAddr,
											pFillInfo->nSentAtAddr,
											pNodeList->respCnt);
								}
								else {
									if (pFillInfo->nSentAtAddr == pNodeList->respCnt)
										_RPT2(_CRT_WARN, "R%d(%d)",
											respAddr, pNodeList->respCnt);
									else
										_RPT3(_CRT_WARN, "R%d(%d)->(%d)??",
											respAddr, pFillInfo->nSentAtAddr,
											pNodeList->respCnt);
								}
								#endif
								// Say we got something and copy to user buffer
								pFillInfo->bufOK = TRUE;
								if (pFillInfo->buf != NULL) {
									*(pFillInfo->buf) = readBuf;
									//_RPT1(_CRT_WARN, "readThread => buf 0x%x\n", pFillInfo->buf);
								}
								// Record time it took to read the response
								pFillInfo->stats.rxTime = eTime;
								pFillInfo->stats.pResp = pFillInfo->buf;
								// Record receive time - send time + queue delay + overhead
								pFillInfo->stats.pacedTime = infcCoreTime() - pFillInfo->funcStartAt;
								// Record receive time - send time and packet received
								pFillInfo->stats.execTime
									= theNet.logReceive(&readBuf, theErr, pFillInfo, rxTime);
								// Signal performance outcomes
								if (userCmdCompleteFunc != NULL && theNet.TraceActive) {
									(*userCmdCompleteFunc)(pNCS->cNum, &pFillInfo->stats);
								}
								// We are done using this response, return it back to free pool
								pNCS->removeHeadDBitem(pNodeList);
								// Log the outcome
								EXIT_LOCK("readThreadProc (norm expect)");
								#if TRACE_HIGH_LEVEL&0
								_RPT2(_CRT_WARN, "cmd completed net %d in %.2f ms\n", pNCS->cNum, pFillInfo->stats.execTime);
								#endif

							} // IF (MN_SRC_HOST: expected response)
							else { // (!rejected and unexpected)
								   // MN_SRC_HOST: We are here because there is no record of a response expected.
								   // Save in case this is serious
								if (!pNCS->SelfDestruct) {
									errInfo.node = respAddr;
									infcCopyPktToPkt18(&errInfo.response, &readBuf);
									switch (readBuf.Fld.PktType) {
									case MN_PKT_TYPE_ERROR:
										theErr = coreGenErrCode(pNCS->cNum, &readBuf, (nodeaddr)errInfo.node);
										theNet.logReceive(&readBuf, theErr, NULL, rxTime);
										// Release cmd lock on database
										EXIT_LOCK("readThreadProc (unsol)");
										// Create dump file to show context
										_RPT2(_CRT_WARN, "readThread(%d) MN_PKT_TYPE_ERROR detected by node %d\n",
											pNCS->cNum, readBuf.Fld.Addr);
										// (TODO 11/18/2009 DS took out for experiment) infcTraceDumpNext(pNCS->cNum);
										break;
									default:
										// Issue an error at this point
										theErr = MN_ERR_UNSOLICITED;
										if (respAddr >= theNet.InventoryNow.NumOfNodes)
											theErr = MN_ERR_ADDR_RANGE;
										else
											theErr = MN_ERR_UNSOLICITED;
										// Log the outcome
										theNet.logReceive(&readBuf, theErr, NULL, rxTime);
										// Release cmd lock on database
										EXIT_LOCK("readThreadProc (unsol)");
										// Tick the debug log with this information
										_RPT2(_CRT_WARN, "readThread(%d): unsolicited packet response: node=%d\n",
											pNCS->cNum, respAddr);
										DUMP_PKT(pNCS->cNum, "...", &readBuf, true);
										break;
									}
								}
								else {
									EXIT_LOCK("readThreadProc (unexpected)");
								}
							}
						}  // src==MN_SRC_HOST
						else {  // src==MN_SRC_NODE
								// Packet claims it is from node and therefore not expected at
								// the host. These packets are typically: error reports,
								// attention notifications or data acquisition information.

								// Log the reception
							theNet.logReceive(&readBuf, theErr, NULL, rxTime);
							// Prevent deadlock on any callbacks we no longer interact with cmd database
							EXIT_LOCK("readThread: src=MN_SRC_NODE");
							// Process the buffer and signal information
							processNodeInitiatedPkt(pNCS->cNum, respAddr, pNCS, readBuf);
						} // src=MN_SRC_MODE
					} // if (bufferSize==0)
				}
				else { // theErr != MN_OK (infcGetResponse failed)
					if (theNet.OpenState == FLASHING) {
						EXIT_LOCK("readThread: inserial");
						// Halt ourselves
						ReadLock.Lock();
						NextState(READ_HALT_REQ);
						ReadLock.Unlock();
						break;
					}
					else {
						// Shutting down
						if (((m_pTermFlag != NULL) && *m_pTermFlag)) {
							EXIT_LOCK("readThread(read failure, shutting down)");
							goto exit_thread;
						}
						// Read failed, create a "null" receive record in the trace
						_RPT3(_CRT_WARN, "%.1f readThread(%d): read failed code=0x%x\n",
							infcCoreTime(), pNCS->cNum, theErr);
						EXIT_LOCK("readThread(read failure)");
					}
				} // (2) infcGetResponse
			} // (1) if (doRead && !*m_pTermFlag)
			break;
		case READ_HALT_REQ:
			#if TRACE_RD_THRD
			_RPT2(_CRT_WARN, "%.1f readThread(%d), halt requested\n", infcCoreTime(), pNCS->cNum);
			#endif
			// Reset all acknowledgments
			m_InternalSyncAck.ResetEvent();
			NextState(READ_HALTED);
			// Fall into halted state and let halt at InternalSync
			// no break OK (special line for Code Analyzer to suppress warning)
		case READ_HALTED:
			#if TRACE_RD_THRD
			_RPT2(_CRT_WARN, "%.1f readThread(%d), halted\n", infcCoreTime(), pNCS->cNum);
			#endif
			// Release function waiting for this stage of halt
			m_InternalSyncAck.SetEvent();
			// We park before the switch until woken up in potentially new state
			m_InternalSync.ResetEvent();
			// Allow read state changes
			ReadLock.Unlock();
			break;
		case READ_SHUTDOWN_REQ:
			#if TRACE_RD_THRD
			_RPT2(_CRT_WARN, "%.1f readThread(%d), shutdown requested\n", infcCoreTime(), pNCS->cNum);
			#endif
			m_InternalSyncAck.ResetEvent();
			NextState(READ_SHUTDOWN);
			// Allow read state changes
			ReadLock.Unlock();
			break;
		case READ_SHUTDOWN:
			// Can't really get here as this is loop exit condition
			#if TRACE_RD_THRD
			_RPT1(_CRT_WARN, "readThread(%d), exit shutdown\n", pNCS->cNum);
			#endif
			// Honor the exit request
			m_InternalSyncAck.SetEvent();
			#if TRACE_RD_THRD||(TRACE_HIGH_LEVEL&&TRACE_THREAD)
			_RPT3(_CRT_WARN, "%.1f readThread(%d), ID=" THREAD_RADIX
				" exit via shutdown request!\n",
				infcCoreTime(), pNCS->cNum, UIthreadID());
			#endif
			m_InternalSyncAck.SetEvent();
			// Allow read state changes
			ReadLock.Unlock();
			return(300 + pNCS->cNum);
		default:
			_RPT2(_CRT_ERROR, "readThread(%d): Unknown state %d\n", pNCS->cNum,
				m_state);
			NextState(READ_SHUTDOWN);
			m_InternalSyncAck.SetEvent();
			// Allow read state changes
			ReadLock.Unlock();
			return(399);
		};
		// Yield while waiting for the response. This allows waiters
		// to restart before processing another response.
		infcSleep(0);
	} // while (m_state != READ_SHUTDOWN)
	m_InternalSyncAck.SetEvent();
	ReadLock.Lock();
	NextState(READ_SHUTDOWN);
	ReadLock.Unlock();
exit_thread:
	#if TRACE_RD_THRD|TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f readPktThread(%d), ID=" THREAD_RADIX
		" exit via loop termination!\n",
		infcCoreTime(), pNCS->cNum, UIthreadID());
	#endif
	return(200 + pNCS->cNum);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		readPktThread::NextState
//
//	DESCRIPTION:
///		Change the operational state of the read thread.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
void readPktThread::NextState(readThreadStates nextState)
{
	#define TRC_NEXT TRACE_RD_THRD & 0
	#if TRC_NEXT
	static double sTime = 0, eTime = 0;
	#endif

	// Remember last state for diagnositic purposes
	if (nextState != m_state)
		m_lastState = m_state;

	//_RPT1(_CRT_WARN, "NextReadState: %d\n", nextState);
	switch (nextState) {
	case READ_RUNNING:
		// Insure ack event is reset on re-start
		m_InternalSync.ResetEvent();
		#if TRC_NEXT
		_RPT1(_CRT_WARN, "NextReadState %d running\n", cNum);
		sTime = infcCoreTime();
		#endif
		break;
	case READ_HALTED:
		#if TRC_NEXT
		eTime = infcCoreTime();
		_RPT2(_CRT_WARN, "NextReadState %d read thread ran for %.2f ms\n",
			cNum, eTime - sTime);
		#endif
		break;
	case READ_HALT_REQ:
		// Reset the acknowledgment
		m_InternalSync.ResetEvent();
		break;
	default:
		// We don't care
		break;
	}
	m_state = nextState;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		readPktThread::ForceStop
//
//	DESCRIPTION:
///
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
void readPktThread::ForceStop()
{
	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
	_RPT0(_CRT_WARN, "ReadThreadHalt Forced (no thread)...waiting on event");
	#endif
	// Wait for creation and park @ HALTED
	m_InternalSync.SetEvent();
	m_InternalSyncAck.WaitFor(INFINITE);
	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
	_RPT0(_CRT_WARN, "ReadThreadHalt OK (no thread)..Halted\n");
	#endif
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		readPktThread::Stop
//
//	DESCRIPTION:
///		Stop and park the read thread in an idle mode. This function returns
///		when the read thread is parked, beware of deadlock!
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
void readPktThread::Stop()
{
	StartStopLock.Lock();
	// If not created, we are halted by definition. State will be setup
	// after the thread reaches the halted state as denoted by the
	// setting of the internal event object.
	if (!pNCS) {
		ForceStop();
		StartStopLock.Unlock();
		return;
	}

	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
	if (m_state != READ_HALTED)
		_RPT2(_CRT_WARN, "%.1f ReadThreadHalt %d requested...",
			infcCoreTime(), pNCS->cNum);
	#endif

	// Lock down the read state
	ReadLock.Lock();

	switch (m_state) {
	case READ_SHUTDOWN_REQ:
	case READ_SHUTDOWN:
		#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
		_RPT0(_CRT_WARN, "shutting down\n");
		#endif
		// We are essentially halted, leave state alone
		ReadLock.Unlock();
		break;
	case READ_HALTED:
		#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
		//_RPT0(_CRT_WARN, "ReadThreadHalt: already halted\n");
		#endif
		pNCS->ReadCommEvent.ResetEvent();
		// Already halted, do nothing.
		ReadLock.Unlock();
		break;
	case READ_HALT_REQ:
		// Waiting for halt already, keep waiting
		#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
		_RPT0(_CRT_WARN, "waiting to park\n");
		#endif
		// Prevent new external commands
		pNCS->CmdGate.ResetEvent();
		// Force read thread to restart
		pNCS->ReadCommEvent.SetEvent();
		ReadLock.Unlock();
		// Wait for it to park
		m_InternalSyncAck.WaitFor(INFINITE);
		#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
		_RPT1(_CRT_WARN, "%.1f ReadThreadHalt: parked\n",
			infcCoreTime());
		#endif
		break;
	case READ_RUNNING:
		// We are still running, should we stop now?
		//if (m_state == READ_RUNNING) {
		// Next state is halt request
		NextState(READ_HALT_REQ);
		// Insure thread still running
		m_InternalSync.SetEvent();
		// Pretend data arrived
		pNCS->ReadCommEvent.SetEvent();
		// Allow thread to react
		ReadLock.Unlock();
		// Wait for it to stop
		#ifdef _DEBUG
		{
			BOOL waitOK = m_InternalSyncAck.WaitFor(15000);
			if (!waitOK) {
				_RPT1(_CRT_WARN, "%.1f ReadThreadHalt wait failed\n",
					infcCoreTime());
			}
		}
		#else
		m_InternalSyncAck.WaitFor(15000);
		#endif
		//}
		//else {
		//	// The new information take precendence
		//	ReadLock.Unlock();
		//	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
		//		_RPT1(_CRT_WARN, "%.1f ReadThreadHalt halt skipped\n",
		//					infcCoreTime());
		//	#endif
		//	// Allow new external commands
		//	pNCS->CmdGate.SetEvent();
		//}
		break;
	default:
		// Oops, pretend we are halted
		NextState(READ_HALTED);
		break;
	}
	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
	_RPT2(_CRT_WARN, "%.1f ReadThreadHalt(%d) halted\n",
		infcCoreTime(), pNCS->cNum);
	#endif
	StartStopLock.Unlock();
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		readPktThread::Start
//
//	DESCRIPTION:
///		(Re)Start the read thread to accept packets.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
void readPktThread::Start()
{
	// Prevent access to internal structures
	StartStopLock.Lock();
	ReadLock.Lock();
	// If no serial port, we don't need to start or we are running now
	if (!pNCS || m_state == READ_RUNNING
		|| !pNCS->pSerialPort || m_state == READ_SHUTDOWN) {
		#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
		_RPT2(_CRT_WARN, "%.1f readThreadStart %d request ignored...\n",
			infcCoreTime(), pNCS ? pNCS->cNum : 99);
		#endif
		ReadLock.Unlock();
		StartStopLock.Unlock();
		return;
	}

	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
	_RPT2(_CRT_WARN, "%.1f readThreadStart %d requested...\n",
		infcCoreTime(), pNCS->cNum);
	#endif

	//SD 8/16/2017: Removed to prevent packets from being cleared if they come in during Start()
	//pNCS->ReadCommEvent.ResetEvent();	

	// Set state to running
	NextState(READ_RUNNING);

	// Make sure the thread restarts
	switch (m_state) {
	case READ_SHUTDOWN_REQ:
	case READ_SHUTDOWN:
		// Shutting down, don't allow start
		ReadLock.Unlock();
		break;
	case READ_UNKNOWN:
		// Should not get here, but assume same as halted
		m_InternalSyncAck.ResetEvent();
		NextState(READ_HALTED);
		// Allow thread to run
		m_InternalSync.SetEvent();
		// Sync threads
		m_InternalSync.WaitFor(INFINITE);
		// Fall into the HALT request logic to wait for
		// halt and then start thread
		// no break OK (special line for Code Analyzer to suppress warning)
	case READ_HALT_REQ:
		#if TRACE_RD_THRD
		_RPT2(_CRT_WARN, "%.1f readThreadStart(%d) from halt requested\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		// Allow read thread to run
		ReadLock.Unlock();
		// Wait until halted
		m_InternalSyncAck.WaitFor(INFINITE);
		ReadLock.Lock();
		// Fall into the halted state, as that is where we are now
		// to restart the thread
		// no break OK (special line for Code Analyzer to suppress warning)
	case READ_HALTED:
		// Switch to the running state
		NextState(READ_RUNNING);
		// Allow thread to restart from wait prior to switch statement
		m_InternalSync.SetEvent();
		#if TRACE_RD_THRD || TRACE_HALT
		_RPT2(_CRT_WARN, "%.1f readThreadStart(%d) restarting\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		ReadLock.Unlock();
		break;
	case READ_RUNNING:
		#if TRACE_RD_THRD & 0
		_RPT2(_CRT_WARN, "%.1f readThreadStart(%d) from running\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		ReadLock.Unlock();
		// Allow read thread to run again
		m_InternalSync.SetEvent();
		break;
	default:
		#if TRACE_RD_THRD
		_RPT2(_CRT_WARN, "%.1f readThreadStart(%d) (unlock#2b)\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		ReadLock.Unlock();
		_RPT2(_CRT_ASSERT, "readThreadStart(%d) unknown read state handling %d\n",
			pNCS->cNum, m_state);
		break;
	}
	//////  READ CRITICAL SECTION RELEASED PAST HERE ////////
	infcSleep(0);					// Release scheduling quanta
	#if TRACE_HIGH_LEVEL & TRACE_START_STOP || TRACE_HALT
	_RPT2(_CRT_WARN, "%.1f readThreadStart %d finished\n",
		infcCoreTime(), pNCS->cNum);
	#endif

	StartStopLock.Unlock();
}
/// \endcond																  *
//*****************************************************************************

///	\cond INTERNAL_DOC
//*****************************************************************************
//	NAME																	  *
//		infcHaltAutoNetDiscovery/Proc
//
//	DESCRIPTION:
//		Kill the offline tickler and wait for exit.
//
//	SYNOPSIS:
void MN_DECL infcHaltAutoNetDiscovery(netaddr cNum)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Insure our net is still there
	if (!pNCS)
		return;

	if (pNCS->pAutoDiscover) {
		_RPT2(_CRT_WARN, "%.1f infcHaltAutoNetDiscovery(%d): stopping\n",
			infcCoreTime(), cNum);
		pNCS->pAutoDiscover->Halt();
		_RPT2(_CRT_WARN, "%.1f infcHaltAutoNetDiscovery(%d): stopped\n",
			infcCoreTime(), cNum);
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		attemptAutoDiscover
//
//	DESCRIPTION:
//		This will attempt to start the auto-discovery thread if this feature
//		is enabled and create dump file.
//
//	SYNOPSIS:

void attemptAutoDiscover(infcErrInfo *pErrInfo) {
	netaddr cNum = pErrInfo->cNum;
	//	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (InfcDiagnosticsOn
		&& SysInventory[cNum].AutoDiscoveryEnable
		&& IsNetBreakErr(pErrInfo->errCode)) {
		autoDiscoverStart(pErrInfo);
	}
}

void attemptAutoDiscover(netaddr cNum, cnErrCode theErr) {
	infcErrInfo errInfo;
	errInfo.cNum = cNum;
	errInfo.errCode = theErr;
	errInfo.node = -1;
	attemptAutoDiscover(&errInfo);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		autoDiscoverStart
//
//	DESCRIPTION:
//		Start a thread that will attempt to detect attached nodes and restore
//		the basic network connectivity. It will use the passed in error as
//		basis for how to poll for active nodes.
//
//	SYNOPSIS:
void autoDiscoverStart(netaddr cNum, cnErrCode theErr) {
	infcErrInfo errInfo;
	errInfo.cNum = cNum;
	errInfo.errCode = theErr;
	errInfo.node = -1;
	autoDiscoverStart(&errInfo);
}

void autoDiscoverStart(infcErrInfo *pErrInfo)
{
	packetbuf zero;

	netaddr cNum = pErrInfo->cNum;
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return;
	mnNetInvRecords &theNet = SysInventory[cNum];
	netStateInfo *pNCS = theNet.pNCS;
	// Don't bother to start auto-discovery if port in not open. The
	// situation will not get better!

	#if TRACE_TICKLER
	_RPT2(_CRT_WARN, "%.1f autoDiscoverStart(%d): initiating\n",
		infcCoreTime(), cNum);
	#endif

	if (!(theNet.AutoDiscoveryEnable || !InfcDiagnosticsOn)
		|| !pNCS->pSerialPort->IsOpen()) {
		_RPT2(_CRT_WARN, "%.1f autoDiscoverStart(%d): blocked from running\n",
			infcCoreTime(), cNum);
		return;
	}

	// Don't start during initializing or they don't want to run this
	if (theNet.Initializing || InitializingGlobal) {
		//	if (theNet.Initializing) {
		_RPT3(_CRT_WARN, "%.1f autoDiscoverStart(%d) ignored: initializing, global=%d\n",
			infcCoreTime(), cNum, InitializingGlobal);
		return;
	}
	// Is there a auto-discover thread running now or we are shutting down?
	if ((pNCS->pAutoDiscover && pNCS->pAutoDiscover->IsRunning())
		|| pNCS->SelfDestruct) {
		// Yes, let it keep running
		_RPT2(_CRT_WARN, "%.1f autoDiscoverStart(%d): running or already shutdown\n",
			infcCoreTime(), cNum);
		return;
	}
	// No, prevent commands from running
	pNCS->CmdGate.ResetEvent();

	// Record this as the first reason we went offline
	if (SysInventory[cNum].OpenState == OPENED_ONLINE) {
		if (theNet.OfflineRootErr == MN_OK) {
			theNet.OfflineRootErr = pErrInfo->errCode;
			theNet.OfflineErrNode = pErrInfo->node;
		}
	}
	// Mark that we are recovering
	SysInventory[cNum].OpenStateNext(OPENED_SEARCHING);
	// Get this started
	pNCS->pAutoDiscover->Start(pErrInfo->errCode);
	#if TRACE_HIGH_LEVEL||TRACE_TICKLER
	_RPT2(_CRT_WARN, "%.1f autoDiscoverStart(%d)...\n",
		infcCoreTime(), cNum);
	#endif
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcStartAutoNetDiscovery
//
//	DESCRIPTION:
//		Start a thread that will attempt to detect attached nodes and restore
//		the basic network connectivity. It will use the passed in error as
//		basis for how to poll for active nodes. This used by the higher level
//		initialization functions to start auto-discovery during startup
//		time.
//
//	SYNOPSIS:
void MN_DECL infcStartAutoNetDiscovery(
	netaddr cNum,
	cnErrCode initialError) {


	infcErrInfo dummyErr;
	dummyErr.cNum = cNum;
	dummyErr.node = MULTI_ADDR(cNum, 0);
	dummyErr.errCode = initialError;
	if (initialError != MN_OK) {
		autoDiscoverStart(&dummyErr);
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		autoDiscoverThread construction/destruction
//
//	DESCRIPTION:
///		Construct an offline tickler thread. This thread will periodically
///		poll the serial port for the presence of nodes and bring the network
///		back online. If the network was previously online, a test is made
///		to locate "crashed" nodes.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
autoDiscoverThread::autoDiscoverThread()
{
	#if TRACE_THREAD || TRACE_DESTRUCT
	_RPT1(_CRT_WARN, "%.1f autoDiscoverThread constructed\n",
		infcCoreTime());
	#endif
	#if (defined(_WIN32)||defined(_WIN64))
	// 4/29/11 DS Leads to issues with Win 7 UseWithCOM();
	SetDLLterm(true);
	#endif
	m_goneOnline = false;
	m_parked = true;
	m_exitDiscovery = false;
	pNCS = NULL;
	m_faultingErr = MN_OK;
}

autoDiscoverThread::~autoDiscoverThread() {
	#if TRACE_THREAD || TRACE_DESTRUCT
	double sTime = infcCoreTime();
	_RPT2(_CRT_WARN, "%.1f autoDiscoverThread destructing, id=" THREAD_RADIX "\n",
		infcCoreTime(), UIthreadID());
	#endif
	// Kill our thread
	TerminateAndWait();
	#if TRACE_THREAD || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f autoDiscoverThread destructed, elapsed=%.1f\n",
		infcCoreTime(), infcCoreTime() - sTime);
	#endif
}

void *autoDiscoverThread::Terminate() {
	void *rVal;
	// Force exit of discovery loop
	m_exitDiscovery = true;
	m_ThreadParkedEvent.SetEvent();
	// Wait for it to park
	//WaitUntilParked();
	//WaitForTerm();
	// Get things started
	rVal = CThread::Terminate();
	// Cause termination now, the error code is ignored due to termination
	// flag priority.
	#if TRACE_TICKLER && TRACE_LOW_PRINT
	_RPT1(_CRT_WARN, "%.1f autoDiscoverThread::Terminate\n",
		infcCoreTime());
	#endif
	return(rVal);
}

void autoDiscoverThread::TerminateAndWait() {
	#if TRACE_TICKLER && TRACE_LOW_PRINT
	_RPT1(_CRT_WARN, "%.1f autoDiscoverThread::TerminateAndWait\n",
		infcCoreTime());
	#endif
	// Get loops to terminate and thread function to exit
	Terminate();
	// Wait for function to return
	CThread::WaitForTerm();
	//_RPT1(_CRT_WARN,"%.1f autoDiscoverThread::TerminateAndWait (terminated)\n",
	//				infcCoreTime());
}


//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		autoDiscoverThread::WaitingForNet
//
//	DESCRIPTION:
///		This function returns TRUE if we are still waiting for the network
///		to come online.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
bool autoDiscoverThread::WaitingForNet() {
	bool retVal = (!m_goneOnline							// Still offline
		&& !((m_pTermFlag != NULL) && *m_pTermFlag)		// Not terminating thread
		&& !m_exitDiscovery								// Not exiting
														// We park if network closes or we enter serial port mode
		&& pNCS->pSerialPort->IsOpen()
		// Not in "serial port" mode
		&& (SysInventory[pNCS->cNum].OpenState != FLASHING))
		|| (SysInventory[pNCS->cNum].OpenState == OPENED_SEARCHING);
	#if TRACE_TICKLER
	_RPT3(_CRT_WARN, "%.1f autoDiscoverStart(%d): WaitingForNet=%d\n",
		infcCoreTime(), pNCS->cNum, retVal);
	#endif
	return retVal;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		autoDiscoverThread::Run
//
//	DESCRIPTION:
///		This is the thread control function. It does the polling and test
///		sequencing.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
int autoDiscoverThread::Run(void *context)
{
	// Initialize our netStateInfo pointer to allow interactions with
	// the network.
	ULONG exitCode = 666;
	pNCS = static_cast<netStateInfo *>(context);
	if (!pNCS)
		return(exitCode);
	netaddr cNum = pNCS->cNum;
	mnNetInvRecords &theNet = SysInventory[cNum];
	#if TRACE_TICKLER||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d): starting, id=" THREAD_RADIX "\n",
		infcCoreTime(), cNum, UIthreadID());
	#endif
	cnErrCode theErr = MN_OK;
	nodebool inventoryChanged = false;
	nodebool hasConnectivity;
	nodebool engagedCmdGate = false;
	nodebool forcedOffline = false;

	//DBG_LOG("Auto-discover starting\n");
	// This loop runs until our owner terminates us. It parks at the
	// m_ThreadParked Event until signalled. If thread termination is not
	// in store, we will assume something bad happened, take our network
	// offline, create a dump file to log the reason and then attempt
	// to reconnect to our network by running a series of diagnostic
	// processes. If the newly contacted network appears different the
	// the network that went away the NODE_CHANGING_NETWORK_CONFIG event
	// will we signalled and the system will re-enumerate.
	while ((m_pTermFlag == NULL) || (!(*m_pTermFlag)))
	{
		#if TRACE_TICKLER
		_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d): parking, id=" THREAD_RADIX "\n",
			infcCoreTime(), cNum, UIthreadID());
		#endif
		// Signal we are parked now atomically
		m_critSect.Lock();
		m_parked = true;
		m_ThreadParkedEvent.ResetEvent();
		// Release autoDiscoverThread::Halt if this isn't initial start
		m_InternalSyncAck.SetEvent();
		// Allow commands again and release any waiters for our completion.
		pNCS->CmdGate.SetEvent();
		engagedCmdGate = false;
		m_critSect.Unlock();
		// Wait for supervision to restart us....
		m_ThreadParkedEvent.WaitFor();

		// If we have been asked to exit, do it now
		if (m_pTermFlag && *m_pTermFlag) {
			break;
		}
		// Someone started us, atomically change our state
		m_critSect.Lock();
		// Signal we are not parked now
		m_parked = false;
		// Reset our loop gater to prevent restart
		m_ThreadParkedEvent.SetEvent();
		m_InternalSyncAck.ResetEvent();
		engagedCmdGate = true;
		// Prevent RunCommand from running
		pNCS->CmdGate.ResetEvent();
		//m_critSect.Unlock();
		exitCode = 600 + cNum;
		// Reset diag counter
		unsigned cnt = 0;
		// Assume nothing changed yet
		forcedOffline = m_goneOnline = inventoryChanged = false;
		#if TRACE_TICKLER
		_RPT4(_CRT_WARN, "%.1f autoDiscoverThread(%d): woke up, "
			"exiting=%d, id=" THREAD_RADIX "\n",
			infcCoreTime(), cNum,
			m_exitDiscovery, UIthreadID());
		#endif

		// If we restarted due to errors, create our next dump file and
		// send system offline.
		if (!((m_pTermFlag != NULL) && *m_pTermFlag)
			&& !m_exitDiscovery && SysInventory[cNum].PortIsOpen()) {
			if (SysInventory[cNum].Initializing == 0) {
				// Create a dump file if we can
				infcTraceDumpNext(cNum);
			}
			// Send us offline and save inventory if we were previously online
			forcedOffline = true;
			infcSetInitializeMode(cNum, TRUE, m_faultingErr);
			#if TRACE_TICKLER
			_RPT2(_CRT_WARN, "%.1f autoDiscoverThread(%d): forcing offline\n",
				infcCoreTime(), cNum);
			#endif
		}
		m_critSect.Unlock();
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// Start looking for the network to come online, then park
		// waiting for another event unless we are shutting down.
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		// This inner loop exits when the network is restored
	restartOnlineSearch:
		while (((WaitingForNet() && !m_goneOnline) || forcedOffline) && !m_exitDiscovery) {
			// Give things time to settle out.
			infcSleep(AUTODISCOVER_WAIT_MS);
			// Attempt to clear out issues
			pNCS->pSerialPort->Flush();
			cnt++;
			#if TRACE_HIGH_LEVEL|TRACE_TICKLER
			_RPT5(_CRT_WARN, "%.1f in autoDiscoverThread(%d) restarted #%d, forced=%d, "
				"thread ID=" THREAD_RADIX "\n",
				infcCoreTime(), cNum, cnt, forcedOffline, UIthreadID());
			#endif
			// Wants to stop after sleep?
			if (!WaitingForNet()) {
				#if TRACE_TICKLER
				_RPT2(_CRT_WARN, "%.1f in autoDiscoverThread(%d) post-flush cancel\n",
					infcCoreTime(), cNum);
				#endif
				break;
			}
			// Perform proper recovery strategy based on the initial error.
			if (IsNetClassErr(theNet.OfflineRootErr)) {
				#if TRACE_HIGH_LEVEL|TRACE_TICKLER
				_RPT4(_CRT_WARN, "%.1f in autoDiscoverThread(%d) net class err 0x%x,"
					"thread ID=" THREAD_RADIX "\n",
					infcCoreTime(), cNum, theNet.OfflineRootErr, UIthreadID());
				#endif
				// Network based error was detected, such as time-out. Get
				// original count of nodes.
				nodeulong nNodes = SysInventory[cNum].InventoryNow.NumOfNodes;
				#if TRACE_BREAK
				_RPT3(_CRT_WARN, "%.1f in autoDiscoverThread(%d) calling infcResetNetRate,"
					"thread ID=" THREAD_RADIX "\n",
					infcCoreTime(), cNum, UIthreadID());
				#endif
				// Restore network back to default speed and test for
				// connectivity using the serial "break" condition.
				theErr = infcResetNetRate(cNum);
				// Try and release port if port that is the issue
				#if TRACE_HIGH_LEVEL|TRACE_TICKLER|TRACE_BREAK
				_RPT4(_CRT_WARN, "%.1f in autoDiscoverThread(%d) infcResetNetRate err=0x%x,"
					"thread ID=" THREAD_RADIX "\n",
					infcCoreTime(), cNum, theErr, UIthreadID());
				#endif
				if (theErr != MN_OK) {
					infcFireNetEvent(cNum, NODES_OFFLINE);
					// Close the port an attempt to re-open again
					#if TRACE_HIGH_LEVEL|TRACE_TICKLER|TRACE_BREAK
					_RPT4(_CRT_WARN, "%.1f in autoDiscoverThread(%d) connectivity bad, err=0x%x,"
						"thread ID=" THREAD_RADIX "\n",
						infcCoreTime(), cNum, theErr, UIthreadID());
					#endif
					continue;
				}
				else {
					m_goneOnline = true;
					forcedOffline = false;
				}
				#ifdef __QNX__
				// 2014/10/1 JS: Because the break detection is "faked" in QNX,
				// just assume there is no connectivity before going into the
				// broken ring diagnostics.
				hasConnectivity = false; //(theErr == MN_OK);
				#else
				hasConnectivity = (theErr == MN_OK);
				#endif

				#if 0// TO_KILL
				{
					WCHAR foo[100];
					if (hasConnectivity)
						wsprintf(foo, _T("AD: brk OK\n"));
					else
						wsprintf(foo, _T("AD: brk failed 0x%x\n"), theErr);
					OutputDebugStr(foo);
				}
				#endif
				// If this fails, we do not have connectivity yet, or lopsided
				// connectivity with other network.
				if (WaitingForNet() && !hasConnectivity) {
					// No connectivity, try and run the network break diagnostic
					// via other channel on Meridian hardware nodes. We only do
					// this for Meridian CON-MOD connected nodes.
					if (SysInventory[cNum].PhysPortSpecifier.PortType == MERIDIAN_CONMOD) {
						#if TRACE_HIGH_LEVEL||TRACE_TICKLER
						_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d) "
							"look for broken ring, err=0x%x\n",
							infcCoreTime(), cNum, theErr);
						#endif
						theErr = infcBrokenRingDiag(cNum, hasConnectivity, nNodes);
						if (theErr != MN_OK && theErr != MN_ERR_SEND_FAILED) {
							// Signal back diagnostic results back to the app.
							// This will usually be the bit encoded break location.
							infcErrInfo errRep;
							errRep.errCode = theErr;
							errRep.cNum = cNum;
							errRep.node = MN_API_UNKNOWN_ADDR;
							errRep.response.bufferSize = 0;
							// Force, call it if it exists
							infcFireErrCallback(&errRep, true);
							continue;
						}
					}
				}
				// Want's early quit?
				if (!WaitingForNet())
					break;
				// We seem to be back online. Check for nodes that are talking but
				// have spontaneously reset. This is determined by sending a simple
				// get parameter command and if the node has reset we will receive
				// our original command back instead of a response type packet.
				theErr = infcProbeForOffline(cNum, &nNodes);
				if (theErr != MN_OK) {
					#if TRACE_HIGH_LEVEL||TRACE_TICKLER
					_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d) probe for offline nodes err=%x\n",
						infcCoreTime(), cNum, theErr);
					#endif
					infcErrInfo errRep;
					errRep.errCode = theErr;
					errRep.cNum = cNum;
					errRep.node = MULTI_ADDR(cNum, theErr & MN_API_ADDR_MASK);
					errRep.response.bufferSize = 0;
					// Call it if it exists
					infcFireErrCallback(&errRep);
					continue;
				}
				// Restore original baud rate and check things out
				theErr = infcChangeLinkSpeed(cNum, SysInventory[cNum].PhysPortSpecifier.PortRate);
				if (theErr != MN_OK)
					continue;
				// Exit discovery loop and put network back online
				exitCode = 800 + cNum;
				m_goneOnline = true;
			}
			else {
				// Unclassified error, assume this was a use error
				#if TRACE_HIGH_LEVEL|TRACE_TICKLER
				_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d) errors unhandled, "
					"assumed use err type=0x%x\n",
					infcCoreTime(), cNum, theNet.OfflineRootErr);
				#endif
				//if (!((m_pTermFlag != NULL) && *m_pTermFlag)) {
				//	// Verify we still are OK
				//	theErr = netEnumerate(cNum);
				//	// Glitched out, try again
				//	if (theErr != MN_OK)
				//		continue;
				//}
				// Seems like the network came back, a non-error
				exitCode = 700 + cNum;
			} // unhandled auto-discover error
		} // while (WaitingForNet() && !m_goneOnline)

		  // We get here because was appear to be online
		#if TRACE_TICKLER
		_RPT5(_CRT_WARN, "%.1f autoDiscoverThread(%d) going online, initsema=%d"
			", m_exitDiscovery=%d, m_goneOnline=%d\n",
			infcCoreTime(), cNum,
			SysInventory[cNum].Initializing, m_exitDiscovery, m_goneOnline);
		#endif
		// Reset the value cache in case the network went away due to power
		// disruption.
		coreInvalidateValCache(cNum);
		// Go back online if we can
		nodebool shouldTerm = Terminating() || m_exitDiscovery;
		if (!shouldTerm) {
			if (m_goneOnline) {
				// Assume OK at start
				inventoryChanged = false;
				// Verify we still are OK and look for node count changes
				theErr = netEnumerate(cNum);
				// Glitched out, try again
				if (theErr != MN_OK) {
					_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d): "
									 "failed netEnumerate, err=0x%x\n",
									 infcCoreTime(), cNum, theErr);
					m_goneOnline = false;
					goto restartOnlineSearch;
				}

				// Check for net size change
				if (SysInventory[cNum].InventoryLast.NumOfNodes
					!= SysInventory[cNum].InventoryNow.NumOfNodes) {
					#if TRACE_HIGH_LEVEL||TRACE_TICKLER
					_RPT4(_CRT_WARN, "%.1f autoDiscoverThread(%d) node count change"
						", now=%d, was=%d\n",
						infcCoreTime(), cNum,
						SysInventory[cNum].InventoryNow.NumOfNodes,
						SysInventory[cNum].InventoryLast.NumOfNodes);
					#endif
					// Node count changed, rebuild internal net information to
					// reflect changes.
					inventoryChanged = true;
					exitCode = 500 + cNum;
				}
				// Verify the nodes types are the same as last time
				nodeaddr nAddr;
				for (nAddr = 0; nAddr<SysInventory[cNum].InventoryNow.NumOfNodes; nAddr++) {
					devID_t dID;
					theErr = coreBaseGetParameterInt(cNum, nAddr,
						MN_P_NODEID, &dID.devCode);
					if (theErr == MN_OK
						&&SysInventory[cNum].InventoryNow.LastIDs[nAddr].fld.devType != dID.fld.devType) {
						_RPT2(_CRT_WARN, "%.1f autoDiscoverThread(%d): "
							"Network nodes different!\n",
							infcCoreTime(), cNum);
						// Rebuild node info, node types changed
						inventoryChanged = TRUE;
						break;
					}
					// If we failed any test, keep offline, and try again
					if (theErr != MN_OK)
						break;
				}
				// If we failed any of the post-online checks, start from scratch
				// to go online.
				if (theErr != MN_OK)
					continue;
				// Signal change if we are still allowed to come online
				if (inventoryChanged
					&& !((m_pTermFlag != NULL) && *m_pTermFlag)) {
					_RPT2(_CRT_WARN, "%.1f autoDiscoverThread(%d): "
						"Network nodes different count!\n",
						infcCoreTime(), cNum);
					// NOTE: In VB this is NODES_CHANGING_NETWORK_CONFIG
					infcFireNetEvent(cNum, NODES_CHANGING_NET_CONTROLLER);
				}
				// Wait for new kind of error to occur before we start again.
				theNet.OfflineRootErr = MN_OK;
				forcedOffline = false;
				// Allow us to go online
				theErr = infcSetInitializeMode(cNum, FALSE, theErr);
				if (theErr != MN_OK) {
					m_goneOnline = false;
				}
				// Allow next diagnostic to play
				theNet.NetDiagLastResult = theErr;
			}
			// Release init mode if we have halt requested
			else if (m_exitDiscovery) {
				m_goneOnline = false;
				// Relieve initializing count without events
				if (SysInventory[cNum].Initializing>0) {
					SysInventory[cNum].Initializing--;
					if (InitializingGlobal) InitializingGlobal--;
				}
			}
		}
		#if TRACE_TICKLER
		if (shouldTerm) {
			_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d): terminating, id="
				THREAD_RADIX "\n",
				infcCoreTime(), cNum, UIthreadID());
		}
		else {
			_RPT4(_CRT_WARN, "%.1f autoDiscoverThread(%d): going back online=%d, id="
				THREAD_RADIX "\n",
				infcCoreTime(), cNum, m_goneOnline, UIthreadID());
			_RPT5(_CRT_WARN, "%.1f autoDiscoverThread(%d) initializing (non-term)=%d"
				",forceOffline=%d,inventoryChanged=%d \n",
				infcCoreTime(), cNum,
				SysInventory[cNum].Initializing, forcedOffline,
				inventoryChanged);
		}
		#endif
	} // while waiting shutdown
	  // Relieve our initialization state
	if (forcedOffline)
		infcSetInitializeMode(cNum, FALSE, m_faultingErr);

	#if TRACE_TICKLER||TRACE_DESTRUCT
	_RPT5(_CRT_WARN, "%.1f autoDiscoverThread(%d) thread ID=" THREAD_RADIX ","
		" exiting code=%d, cnErr=%x\n",
		infcCoreTime(), cNum, UIthreadID(), exitCode, theErr);
	_RPT3(_CRT_WARN, "%.1f autoDiscoverThread(%d) exiting, initializing=%d\n",
		infcCoreTime(), cNum,
		SysInventory[cNum].Initializing);
	#endif
	// Make exit appear same as "parked"
	m_critSect.Lock();
	m_parked = true;
	m_InternalSyncAck.SetEvent();
	// Insure we don't leave gate closed if we closed it
	if (engagedCmdGate)
		pNCS->CmdGate.SetEvent();
	m_critSect.Unlock();
	#if TRACE_HIGH_LEVEL
	//DBG_LOG("Auto-discover exiting\n");
	#endif
	return(exitCode);

}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		autoDiscoverThread::Halt
//
//	DESCRIPTION:
/**
Force the auto-discover thread to enter its halted and parked state.

\param[in,out] xxx description
\return description

**/
//	SYNOPSIS:
void autoDiscoverThread::Halt()
{
	m_critSect.Lock();
	#if TRACE_LOW_PRINT||TRACE_TICKLER
	_RPT2(_CRT_WARN, "%.1f autoDiscoverThread::Halt: init cnt=%d\n",
		infcCoreTime(), SysInventory[pNCS->cNum].Initializing);
	#endif
	// Already parked, ignore the request
	if (m_parked) {
		m_critSect.Unlock();
		return;
	}
	if (!m_exitDiscovery) {
		m_exitDiscovery = true;
		// Wait for the thread to park
		m_critSect.Unlock();
		// Wait until it stops
		m_InternalSyncAck.WaitFor();
	}
	else {
		// Already exited
		m_InternalSyncAck.SetEvent();
		m_critSect.Unlock();
	}
	return;
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		autoDiscoverThread::Start
//
//	DESCRIPTION:
/**
Start the auto-discover process due to incoming reason.

\param[in,out] xxx description
\return description

**/
//	SYNOPSIS:
void autoDiscoverThread::Start(cnErrCode theErr) {
	// If serial port is closed, don't start discovery
	// we can never go online.
	if (!pNCS || !pNCS->pSerialPort || !pNCS->pSerialPort->IsOpen())
		return;
	m_critSect.Lock();
	// Already parked?
	if (m_parked) {
		// Yes, restart thread
		m_parked = false;
		m_exitDiscovery = false;
		m_faultingErr = theErr;
		#if TRACE_LOW_PRINT||TRACE_TICKLER
		_RPT2(_CRT_WARN, "%.1f autoDiscoverThread::Start from parked: init cnt=%d\n",
			infcCoreTime(), SysInventory[pNCS->cNum].Initializing);
		#endif
		// Signal we can restart the thread
		m_ThreadParkedEvent.SetEvent();
		m_critSect.Unlock();
	}
	else {
		// Not parked yet, wait for thread to park
		// before restarting.
		#if TRACE_LOW_PRINT||TRACE_TICKLER
		_RPT2(_CRT_WARN, "%.1f autoDiscoverThread::Restarting: init cnt=%d\n",
			infcCoreTime(), SysInventory[pNCS->cNum].Initializing);
		#endif
		m_critSect.Unlock();
		m_InternalSyncAck.WaitFor();
		// It will restart now from parked state.
		Start(theErr);
	}
}
/// \endcond																  *
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//						   PUBLIC C STYLE INTERFACES
//*****************************************************************************
//*****************************************************************************

/// \cond INFC_CLIB

//*****************************************************************************
//	NAME																	  *
//		infcSleep
//
//	DESCRIPTION:
///		Efficiently waste time.
///
/// 	\param milliseconds Pauses the current thread for \a milliseconds.
//
//	SYNOPSIS:
MN_EXPORT void MN_DECL infcSleep(
	Uint32 milliseconds)
{
	CThread::Sleep(milliseconds);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcResetNetRate
//
//	DESCRIPTION:
///		Restore the network back to it's power-on default rate.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcResetNetRate(
	netaddr cNum)
{
	CSerial::SERAPI_ERR serErr;
	cnErrCode theErr = MN_OK;
	CSerialExErrReportInfo currentErrors;
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Port exists?
	if (!pNCS || !pNCS->pSerialPort)
		return(MN_ERR_PORT_PROBLEM);

	// Prevent poller from running
	if (pNCS->pPollerThread) pNCS->pPollerThread->Stop();

	#if TRACE_BAUD
	_RPT2(_CRT_WARN, "%.1f infcResetNetRate(%d) to 9600\n", infcCoreTime(), cNum);
	#endif
	// Setup/restore our port to initial settings
	bool dtrState = pNCS->pSerialPort->GetDTR();
	bool rtsState = pNCS->pSerialPort->GetRTS();

	// Stop packet processing
	pNCS->ReadThread.Stop();
	// Start ignoring incoming data
	pNCS->pSerialPort->AutoFlush(true);

	serErr = pNCS->pSerialPort->Setup(MN_NET_BAUD_RATE,
		CSerial::EData8,
		CSerial::EParNone,
		CSerial::EStop1,
		dtrState ? CSerial::EDTRSet
		: CSerial::EDTRClear,
		rtsState ? CSerial::ERTSSet
		: CSerial::ERTSClear);
	// How did we do?
	switch(serErr) {
	case CSerial::API_ERROR_SUCCESS:
		// All OK, continue
		break;
	case CSerial::API_ERROR_PORT_SETUP:
		_RPT2(_CRT_WARN, "%.1f infcResetNetRate(%d) default net rate not supported!\n",
			infcCoreTime(), cNum);
		return(MN_ERR_BAUD_NOT_SUPPORTED);
	default:
		_RPT3(_CRT_WARN, "%.1f infcResetNetRate(%d) port unavailable! Err=0x%x\n",
			infcCoreTime(), cNum, serErr);
		return(MN_ERR_PORT_PROBLEM);
	}

	// Clear history of break and network errors
	pNCS->pSerialPort->ErrorReportClear();

	for (int i = 0; i<5; i++) {
		// Send a break
		#if TRACE_SERIAL || TRACE_BAUD
		_RPT2(_CRT_WARN, "%.1f infcResetNetRate(%d) sending break\n",
			infcCoreTime(), cNum);
		#endif
		serErr = pNCS->pSerialPort->Break(MN_BREAK_DURATION_MS);
		if (serErr) {
			#if 0
			WCHAR foo[100];	   // TO_KILL
			wsprintf(foo, _T("Break Failed 0x%x\n"), serErr);
			OutputDebugStr(foo);
			#endif
			_RPT3(_CRT_WARN, "%.1f infcResetNetRate(%d) sending break failed, err=0x%x\n",
				infcCoreTime(), cNum, serErr);
			return(MN_ERR_PORT_PROBLEM);
		}
		// Wait for break to cycle around
		infcSleep(Uint32(COMM_EVT_BRK_DLY_MS*1.2));
		// Did we see one?
		pNCS->pSerialPort->ErrorReportGet(&currentErrors);
		// Basic connectivity if we saw break cycle around
		if (currentErrors.BREAKcnt == 0) {
			_RPT2(_CRT_WARN, "%.1f infcResetNetRate(%d) no BREAK was detected\n",
				infcCoreTime(), cNum);
			theErr = MN_ERR_NO_NET_CONNECTIVITY;
			//03/23/2011 JS: Flush the port to get it moving again
			infcFlush(cNum);
			continue;
		}
		else {
			theErr = MN_OK;
		}

		#if TRCE_LOW_PRINT|TRACE_BREAK
		_RPT2(_CRT_WARN, "%.1f infcResetNetRate(%d) BREAK was detected\n",
			infcCoreTime(), cNum);
		#endif

		break;
	}
	// Make sure we are in packet mode past here, we are running channel
	// at "default" rate past here.
	pNCS->pSerialPort->SetMode(SERIALMODE_MERIDIAN_7BIT_PACKET);

	// Clear history of break and network errors
	pNCS->pSerialPort->ErrorReportClear();

	// Allow data to flow
	pNCS->pSerialPort->AutoFlush(false);
	#if TRACE_BAUD
	_RPT3(_CRT_WARN, "%.1f infcResetNetRate(%d) exiting, err=%d\n",
		infcCoreTime(), cNum, theErr);
	#endif
	return(theErr);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcChangeLinkSpeed
//
//	DESCRIPTION:
/**
	From the current port and net speed, send the command to change to 
	the new speed. After successful speed change command on the link, the
	port will be changed to match the new rate on the net.

 	\param cNum port index
	\return Success code, MN_OK if executed OK

 	Detailed description.
**/
//	SYNOPSIS:
cnErrCode MN_DECL infcChangeLinkSpeed(
	netaddr cNum,
	netRates newRate)
{
	packetbuf theCmd, theResp;
	cnErrCode theErr = MN_OK;

	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS)
		return(MN_ERR_CLOSED);
	// Port exists?
	if (!pNCS->pSerialPort)
		return(MN_ERR_PORT_PROBLEM);

	// Bounds check request
	if (newRate <= 0)
		return(MN_ERR_BADARG);

	#if TRACE_BAUD||TRACE_HIGH_LEVEL
	_RPT3(_CRT_WARN, "%.1f infcChangeLinkSpeed(%d) rate=%d\n", infcCoreTime(), cNum, newRate);
	#endif
	// Create command to change to new rate.
	theCmd.Fld.SetupHdr(MN_PKT_TYPE_EXTEND_HIGH, 0);
	theCmd.Fld.PktLen = 2;
	theCmd.Byte.Buffer[CMD_LOC] = MN_CTL_EXT_BAUD_RATE;
	theCmd.Byte.Buffer[CMD_LOC + 1] = newRate / MN_NET_RATE_DIVIDER;
	// Make sure request works out perfectly
	if ((theCmd.Byte.Buffer[CMD_LOC + 1] * MN_NET_RATE_DIVIDER) != newRate) {
		theErr = MN_ERR_BADARG;
		return(MN_ERR_BADARG);
	}
	theCmd.Byte.BufferSize = theCmd.Fld.PktLen + MN_API_PACKET_HDR_LEN;
	theErr = infcRunCommand(cNum, &theCmd, &theResp);
	if (theErr != MN_OK) {
		// Attempt to restore default baud rate @ nodes, ignoring outcome.
		infcResetNetRate(cNum);
	}
	else {
		#if TRACE_BAUD
		_RPT3(_CRT_WARN, "%.1f infcChangeLinkSpeed(%d) nodes now @ %d\n",
			infcCoreTime(), cNum, newRate);
		#endif
		// Setup/restore our port to user's settings
		bool dtrState = pNCS->pSerialPort->GetDTR();
		bool rtsState = pNCS->pSerialPort->GetRTS();
		// Setup port to new settings
		CSerial::SERAPI_ERR winErr = pNCS->pSerialPort->Setup(newRate,
										CSerial::EData8, CSerial::EParNone,
										CSerial::EStop1,
										dtrState ? CSerial::EDTRSet
										: CSerial::EDTRClear,
										rtsState ? CSerial::ERTSSet
										: CSerial::ERTSClear);
		// How did we do?
		switch (winErr) {
		case CSerial::API_ERROR_SUCCESS:
			// This is OK
			#if TRACE_BAUD
			_RPT3(_CRT_WARN, "%.1f infcChangeLinkSpeed(%d) port now @ %d\n",
				infcCoreTime(), cNum, newRate);
			#endif
			// Save the current rate
			SysInventory[cNum].PhysPortSpecifier.PortRate = newRate;
			// Clean up diagnostic cruft from speed change
			netDiagsClr(cNum);
			// We are all set
			break;
		case CSerial::API_ERROR_PORT_SETUP:
			// Report we have issues with the request
			theErr = MN_ERR_BAUD_NOT_SUPPORTED;
			_RPT4(_CRT_WARN, "%.1f infcChangeLinkSpeed(%d) port doesn't support %d, "
				"thread ID=" THREAD_RADIX_LONG "\n",
				infcCoreTime(), cNum, newRate, infcThreadID());
			break;
		case CSerial::API_ERROR_PORT_UNAVAILABLE:
		default:
			// We failed recover due to some port problem: removed device,
			// handle issue, etc.
			theErr = MN_ERR_PORT_PROBLEM;
			_RPT2(_CRT_WARN, "%.1f infcChangeLinkSpeed(%d), port unavailable\n",
				infcCoreTime(), cNum);
			break;
		}
	}
	return theErr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcSetNetRate
//
//	DESCRIPTION:
///		Set the network adapter rate to run at the new rate. The network
///		rate command is transmitted and when returned back to the host
///		the port will change to the new network communications rate.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL  infcSetNetRate(
	netaddr cNum,
	netRates newRate)
{
	cnErrCode theErr = MN_OK;

	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS)
		return(MN_ERR_CLOSED);
	// Port exists?
	if (!pNCS->pSerialPort)
		return(MN_ERR_PORT_PROBLEM);

	// Bounds check request
	if (newRate <= 0)
		return(MN_ERR_BADARG);
	#if TRACE_BAUD||TRACE_HIGH_LEVEL
	_RPT3(_CRT_WARN, "%.1f infcSetNetRate(%d) rate=%d\n", infcCoreTime(), cNum, newRate);
	#endif
	// Block recovery and error processing
	SysInventory[cNum].Initializing++;

	// Stop polling thread to prevent interference
	infcBackgroundPollControl(cNum, false);

	// Try it a few times if it doesn't work out (HACK)
	for (int i = 0; i<3; i++) {
		// Send break and restore baud back to default rate. This will kill
		// any data acquisition that maybe running and about to interfere.
		#if TRACE_BREAK
		_RPT3(_CRT_WARN, "%.1f in infcSetNetRate(%d) calling infcResetNetRate,"
			"thread ID=" THREAD_RADIX_LONG "\n",
			infcCoreTime(), cNum, infcThreadID());
		#endif
		theErr = infcResetNetRate(cNum);
		if (theErr != MN_OK)
			continue;
		// Run it and await its response
		#if TRACE_BAUD
		_RPT3(_CRT_WARN, "%.1f infcSetNetRate(%d) setting nodes to %d\n",
			infcCoreTime(), cNum, newRate);
		#endif
		theErr = infcChangeLinkSpeed(cNum, newRate);
		// Allow a few retries
		if (theErr == MN_OK)
			break;
	}
	// Signal error if this didn't work out
	if (theErr != MN_OK) {
		// Create errir
		infcErrInfo errInfo;
		errInfo.cNum = cNum;
		errInfo.errCode = theErr;
		errInfo.node = -1;			// HOST detected
		infcFireErrCallback(&errInfo);
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Done and running at the new rate.
	// - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Unstack initialization/recovery and error processing
	SysInventory[cNum].Initializing--;
	// Restart the poller if we can and it was running at start.
	if (SysInventory[cNum].Initializing == 0
		&& SysInventory[cNum].OpenState == OPENED_ONLINE) {
		// Turn on the polling, keeping existing error
		cnErrCode pollErr
			= infcBackgroundPollControl(cNum,
				SysInventory[cNum].KeepAlivePollRestoreState);
		theErr = (theErr == MN_OK) ? pollErr : theErr;
	}
	return(theErr);
}
//																			    *
//*******************************************************************************

//******************************************************************************
//	NAME																	   *
//		infcRunCommand
//
//	DESCRIPTION:
//		This function will send the buffer specified in <theCommand> and
//		wait for the response and store it in <theResponse>. If no response
//		is desired, a NULL pointer is passed to <theResponse> parameter. In
//		this case, the function will always returns MN_OK.
//
//		Commands are run by:
//			1) acquiring the command semaphore to limit the number of
//			   simultaneous commands in the network
//			2) the command lock semaphore is then acquired
//			3) the command is sent
//			4) the response database is created and updated with the desired
//			   handling for this command
//		    5) The command lock is released
//			6) If no response is desired, this function returns MN_OK
//			   else
//			   We go to sleep waiting for the read thread to wake us up via
//			   an APC being queued to this thread.
//			7) If there is a valid response, this function returns MN_OK, else
//			   it returns the appropriate code.
//
//	RETURNS:
//		Standard return codes
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcRunCommand(
	netaddr cNum,
	packetbuf *theCommand,				// pointer to filled in command
	packetbuf *theResponse)				// pointer to response area
{
	double funcStartAt = infcCoreTime();					// Time we started this function
	cnErrCode theErr = MN_OK;
	respNodeList *pRespArea;								// By node address & type data areas
	respTrackInfo *pRespInfo;								// Thread / response info data
	BOOL sleepOK;
	BOOL dataOK, inRecovery;
	mnCompletionInfo stats;									// Command completion statistics

	register netStateInfo *pNCS;							// Quick access to net info

															// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	// We must have defined buffers
	if (!theResponse || !theCommand)
		return MN_ERR_BADARG;

	// Initialize fast pointer to our data
	mnNetInvRecords &theNet = SysInventory[cNum];
	pNCS = theNet.pNCS;
	// RAII Lock on pNCS until return
	netStateInfo::cmdsIdleEvt idleChecker(*pNCS);

	// Don't allow if the port is not open
	if (!pNCS || !pNCS->pSerialPort || !pNCS->pSerialPort->IsOpen())
		return(MN_ERR_CLOSED);

	// Don't do in serial port mode
	if (theNet.OpenState == FLASHING) {
		return(MN_ERR_IN_SERIAL_PORT);
	}

	// Cannot run commands from the read thread
	if (pNCS->isReadThread()) {
		return(MN_ERR_CMD_IN_ATTN);
	}


	theCommand->Fld.StartOfPacket = 1;
	theResponse->Byte.BufferSize = 0;
	theResponse->Fld.PktLen = 0;
	//#ifdef _DEBUG
	//	theResponse->Byte.Buffer[0] = 0x00;
	//	theResponse->Byte.Buffer[1] = 0x11;
	//	theResponse->Byte.Buffer[10] = 0xaaU;
	//	theResponse->Byte.Buffer[11] = 0xbbU;
	//#endif

	// Check for outstanding responses
	if (pNCS->nRespOutstanding == 0 && inDebugging) {
		if (SysPortCount > 1) {
			// Check for outstanding responses on other ports
		}
		else {
			// Release the debugging thread lock gate, allowing us to step 
			// through code
			debugThreadLockResponseGate.SetEvent();
		}
	}

	// Check if the current thread should hit the gate
	if (inDebugging) {
		if (CThread::CurrentThreadID() != lockedThreadID) {
			// Blocks new commands from being sent
			debugGate.WaitFor(INFINITE);
		}
	}

	// Are we in recovery thread?
	inRecovery = pNCS->isRecoveryThread();

	// If diagnostic is running we slowly cancel user's requests to
	// prevent application spinout.
	if (!inRecovery && !pNCS->CmdGate.WaitFor(FRAME_WRITE_TIMEOUT))
		return MN_ERR_CANCELED;

	// Block here if too many commands are attempted at once, the read
	// thread will release each of these if the send is successful.
	// If the transmission fails, we releave this semaphore.
	sleepOK = pNCS->CmdPaceSemaphore.Lock(INFINITE);

	// Too long to release, this should only occur if there is a deadlock
	if (!sleepOK) {
		//NOTE: Since the semaphore wait time is now infinite, we should never
		// get in here!
		infcErrInfo semaErr;
		_RPT1(_CRT_WARN, "infcRunCommand(%d): pace semaphore release err, deadlocked?\n",
			cNum);
		semaErr.errCode = MN_ERR_SEND_LOCKED;
		semaErr.cNum = cNum;
		semaErr.node = -1;
		infcCopyPktToPkt18(&semaErr.response, theCommand);
		infcFireErrCallback(&semaErr);
		// Log the problem
		theNet.logSend(theCommand, MN_ERR_SEND_LOCKED, infcCoreTime());
		// We are screwed, kill all pending work to flush
		infcFlush(cNum);
		return(MN_ERR_SEND_LOCKED);
	}
	// Attempt to send command while not initializing?
	if (!inRecovery && SysInventory[cNum].OpenState != OPENED_ONLINE
		&& !(SysInventory[cNum].Initializing)) {
		// Log the send attempt and the error it caused
		theNet.logSend(theCommand, MN_ERR_CMD_OFFLINE, infcCoreTime());
		// Prevent leaking locks!
		#ifdef _DEBUG
		pNCS->CmdPaceSemaphore.Unlock(1, &pNCS->SemaCount);
		#else
		pNCS->CmdPaceSemaphore.Unlock();
		#endif
		return(MN_ERR_CMD_OFFLINE);
	}

	// Setup the response area for this command while sending/wait.
	if (MN_PKT_IS_HIGH_PRIO(theCommand->Fld.PktType)) {
		// Control packets are not node related, queue separately
		pRespArea = &pNCS->controlNodeState;
		#if TRACE_LOW_PRINT&&TRACE_SEND_RESP
		_RPT2(_CRT_WARN, "\nS%d<%d>", theCommand->Fld.Addr, pRespArea->sendCnt + 1);
		#endif
	}
	else {
		// Regular packets queue at each node
		pRespArea = &pNCS->respNodeState[theCommand->Fld.Addr];
		#if TRACE_LOW_PRINT&&TRACE_SEND_RESP
		_RPT2(_CRT_WARN, "\nS%d(%d)", theCommand->Fld.Addr, pRespArea->sendCnt + 1);
		#endif
	}
	//
	// Lock out the other threads until we have this sent out
	// and the response database entries have been made.
	// The rule for responses is that by node, responses are in order, but
	// not nodes are allowed to return out of the transmit order.
	// For example:
	// - command #1 is issued to node 3
	// - command #2 is issued to node 0 (NC)
	// - command #3 is issued to node 3
	//
	// The responses could come back #2, #1, #3 or #1, #2, #3.  Responses
	// from an individual node are always processed the order transmitted.
	// If a node can finish ahead of another node in the ring, it may
	// transmit its result ahead of the transmit order.
	//
	ENTER_LOCK("infcRunCommand (cmd init)");

	// Assign a response tracking database element.
	if (!pNCS->pTrkTail) {
		EXIT_LOCK("infcRunCommand(going away)");
		// Prevent leaking locks!
		#ifdef _DEBUG
		pNCS->CmdPaceSemaphore.Unlock(1, &pNCS->SemaCount);
		#else
		pNCS->CmdPaceSemaphore.Unlock();
		#endif
		return(MN_ERR_CMD_OFFLINE);
	}
	pRespInfo = *pNCS->pTrkTail;
	*pNCS->pTrkTail-- = NULL;
	//_RPT2(_CRT_WARN, "infcRunCommand: trk list %d @ 0x%x\n", pNCS->pTrkTail-pNCS->pTrkList, pRespInfo);
	if (pNCS->pTrkTail < &pNCS->pTrkList[-1]) {
		_RPT0(_CRT_ASSERT, "infcRunCommand: oops using illegal tracker\n");
	}

	// Initialize the response database tracking info
	pRespInfo->stats.cmd = *theCommand;		// Save our command
	pRespInfo->next = NULL;					// We are always a leaf
	pRespInfo->buf = theResponse;			// Where to finally store resp
	pRespInfo->bufOK = FALSE;				// Nothing here yet
	pRespInfo->funcStartAt = funcStartAt;	// Record function start
	pRespInfo->nSentAtAddr = ++pRespArea->sendCnt;

	if (theErr == MN_OK) {
		// Record time when command hits the net
		pRespInfo->cmdStartAt = infcCoreTime();
		++pNCS->nRespOutstanding;
		theErr = infcSendCommand(cNum, theCommand);
		if (theErr != MN_OK) {
			--pNCS->nRespOutstanding;
		}
		pRespInfo->stats.sendTime = infcCoreTime() - pRespInfo->cmdStartAt;
	}
	// If we sent command, continue processing for the expected response.
	if (theErr == MN_OK) {
		// We expect one to return
		pRespInfo->stats.ringDepth = pNCS->nRespOutstanding;

		// Initialize database pointers if first time through here
		if (pRespArea->head == NULL) {
			pRespArea->head = pRespInfo;	// Head = first one
			pRespArea->tail = NULL;		// Reset the tail
		}
		// Link previous item to this one if there was one
		if (pRespArea->tail != NULL) {
			pRespArea->tail->next = pRespInfo;
		}
		// Tail always points to latest sent item
		pRespArea->tail = pRespInfo;		// DB tail ptr to the end
											// Save our serial number
		pRespInfo->sendSerNum = theNet.logSend(theCommand, theErr, pRespInfo->cmdStartAt);
		// Make sure response wait event is unsignalled??
		pRespInfo->evtRespWait.ResetEvent();

		// Make sure the read thread starts running
		pNCS->ReadThread.Start();

		//_RPT1(_CRT_WARN, "ReadThreadState: %d\n", pNCS->readThreadState );
		// Unlock previous lock now that the response database is updated
		EXIT_LOCK("infcRunCommand (restart read)");
		// This thread is waiting for the response, wait for event or timeout
		#if TRACE_LOW_PRINT&&TRACE_SEND_RESP
		_RPT0(_CRT_WARN, "W");
		#endif
		//_RPT1(_CRT_WARN, "%.1f start response wait\n", infcCoreTime());
		BOOL waitOK = pRespInfo->evtRespWait.WaitFor(InfcRespTimeOut);
		#if TRACE_LOW_PRINT&&TRACE_SEND_RESP
		_RPT1(_CRT_WARN, ".<%d>", sleepOK);
		#endif
		if (!waitOK) {
			// -------------------------------------------------- //
			// We are restarted via time-out                      //
			// -------------------------------------------------- //
			theErr = MN_ERR_RESP_TIMEOUT;
			//_RPT2(_CRT_WARN, "%.1f cmd timeout! hndl=0x%x\n",
			//				 infcCoreTime(),pRespInfo->hRespWait);
			DUMP_PKT(cNum, "**Timeout cmd ", &pRespInfo->stats.cmd);
			ENTER_LOCK("infcRunCommand (timeout)");
			// Add failure to the rx log file
			if (SysInventory[cNum].TraceActive) {
				packetbuf nullPkt;

				// Create null packet buffer
				nullPkt.Byte.BufferSize = 0;
				nullPkt.Byte.Buffer[0] = nullPkt.Byte.Buffer[1] = 0;
				// Assign a receive trace record
				theNet.logReceive(&nullPkt, MN_ERR_RESP_TIMEOUT, pRespInfo, infcCoreTime());
			}
			if (SysInventory[cNum].OpenState == OPENED_ONLINE) {
				// Send off the error callback, fill in the relevant
				// error information
				infcErrInfo errInfo;
				errInfo.errCode = theErr;
				errInfo.cNum = cNum;
				infcCopyPktToPkt18(&errInfo.response, theCommand);
				errInfo.node = theCommand->Fld.Addr;
				// Notify the user
				infcFireErrCallback(&errInfo);
			}
			// Remove this as an expected item
			pNCS->removeThisDBitem(pRespInfo, pRespArea);
			EXIT_LOCK("infcRunCommand (resp timeout)");
		}
		//else {
		//	//_RPT1(_CRT_WARN, "infcRunCommand: response wait OK\n", sleepOK);
		//}
	}
	else {
		_RPT2(_CRT_WARN, "infcRunCommand: failed send 0x%0x @ %f\n", theErr, infcCoreTime());
		// Return the tracking DB item
		pNCS->pTrkTail++;
		*pNCS->pTrkTail = pRespInfo;
		// Release the command semaphore allowing one more (if even possible)
		// - note we have the command lock when we get here, don't forget to
		// release it.
		#ifdef _DEBUG
		dataOK = pNCS->CmdPaceSemaphore.Unlock(1, &pNCS->SemaCount);
		#else
		dataOK = pNCS->CmdPaceSemaphore.Unlock();
		#endif
		if (dataOK) {
			// Make sure read thread keeps running
			if (pNCS->nRespOutstanding > 0) {
				if (--pNCS->nRespOutstanding > 0) {
					pNCS->ReadThread.Start();
				}
			}
		}
		else {
			infcErrInfo semaErr;
			int semaErrCode = GetLastError();
			_RPT1(_CRT_ERROR, "readThread: semaphore release err 0x%X\n",
				semaErrCode);
			semaErr.errCode = MN_ERR_SEND_UNLOCK;
			semaErr.cNum = cNum;
			semaErr.node = semaErrCode;
			EXIT_LOCK("infcRunCommand: (sema rel err)");
			infcFireErrCallback(&semaErr);
			theErr = (cnErrCode)GetLastError();
			_RPT1(_CRT_ERROR, "infcRunCommand: semaphore release err 0x%X\n",
				theErr);
			theNet.logSend(theCommand, MN_ERR_SEND_UNLOCK, infcCoreTime());
			return(theErr);
		}
		// Release lock, started upon command attempt
		EXIT_LOCK("infcRunCommand (send fail)");
		// Command failed, make sure there is someone to clean up
		// any remaining items.
		pNCS->ReadThread.Start();
		// Log transfer timeout to a special error
		if (theErr == MN_ERR_TIMEOUT) {
			theErr = MN_ERR_SEND_FAILED;
		}
		theNet.logSend(theCommand, theErr, infcCoreTime());
		// Send off the callback if it exists
		{
			infcErrInfo errInfo;
			// Fill in the relevant information
			errInfo.errCode = theErr;
			errInfo.cNum = cNum;
			infcCopyPktToPkt18(&errInfo.response, theCommand);
			errInfo.node = 0;
			// Notify the user
			infcFireErrCallback(&errInfo);
		}
	}
	// Return the last error
	return theErr;
}
//																			 *
//******************************************************************************


//****************************************************************************
//	NAME
//		infcOnline
//
//	DESCRIPTION:
//		Returns TRUE if the port is open and ready to accept characters.
//
//	RETURNS:
//		nodebool
//
//	SYNOPSIS:
MN_EXPORT nodebool MN_DECL infcOnline(netaddr cNum)
{
	// In range?
	if (cNum >= NET_CONTROLLER_MAX)
		return(false);
	return SysInventory[cNum].OpenState == OPENED_ONLINE;
}
/// \endcond																  *
//****************************************************************************


/// \cond INTERNAL_DOC
//*****************************************************************************
//	NAME																	  *
//		infcStartController
//
//	DESCRIPTION:
//		This function will open our NT device driver returning a handle to
//		that device for future use. This handle should be passed to the
//		NCstopController function when operations are complete or the
//		application is terminating.
//
//		If a controller is currently open, this call will always close it
//		down before re-opening.
//
//	RETURNS:
//		MN_OK if device opened and port at the default rate.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcStartController(
	netaddr cNum)					// cNum = the COM Port to be opened
{
	cnErrCode theErr = MN_OK;
	netStateInfo *pNCS;

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	#if TRACE_START_STOP
	_RPT2(_CRT_WARN, "%.1f infcStartController %d...", infcCoreTime(), cNum);
	#endif
	#ifdef _DEBUG
	// Look for trouble every restart
	infcHeapCheck("infcStartController: start");
	#endif
	// Hook to pointer based reference
	mnNetInvRecords &theNet = SysInventory[cNum];
	pNCS = theNet.pNCS;

	// Create logging on first open
	if (!theNet.txTraces) theNet.txTraces = new txTraceBuf[SEND_DEPTH];
	if (!theNet.rxTraces) theNet.rxTraces = new rxTraceBuf[RECV_DEPTH];

	// Our port is active and/or exists?
	if (pNCS) {
		// Yes, kill us
		theNet.TraceActive
			= theNet.TraceActiveReq | InfcDumpOnExit;

		pNCS->waitForIdle();

		// Allow only one starter at a time
		//pInitLock->Lock();
		// Destroy the old environment
		infcStopController(cNum);
		// This is gone now
		pNCS = NULL;
	}
	// We have no environment, make a new one now.
	if (!pNCS) {
		pNCS = SysInventory[cNum].pNCS = new netStateInfo(SysInventory[cNum].NumCmdsInRing, cNum);
		#if TRACE_SIZES
		_RPT2(_CRT_WARN, "infcStartController: netStateInfo size=%d(0x%x)\n",
			sizeof(netStateInfo(SysInventory[cNum].NumCmdsInRing, cNum)),
			sizeof(netStateInfo(SysInventory[cNum].NumCmdsInRing, cNum)));
		#endif
		// Did it work out?
		if (!pNCS) {
			return(MN_ERR_MEM_LOW);
		}
	}

	/****** Open and Setup Serial Port  *******/
	if (!pNCS->pSerialPort
		#if (defined(_WIN32)||defined(_WIN64))
		|| ((pNCS->pSerialPort)->OpenComPort(SysInventory[cNum].PhysPortSpecifier.PortNumber))
		!= CSerial::API_ERROR_SUCCESS
		#else
		|| ((pNCS->pSerialPort)->OpenComPort(SysInventory[cNum].PhysPortSpecifier.PortName))
		!= CSerial::API_ERROR_SUCCESS
		#endif
		) {
		// No port is detected - notify application
		infcFireNetEvent(cNum, NODES_NO_PORT);
		// Unlock
		//pInitLock->Unlock();
		return(MN_ERR_PORT_PROBLEM);
	}
	// Say port is basically open
	SysInventory[cNum].OpenStateNext(OPENED_SEARCHING);
	// Setup our port, if all is OK
	if (theErr == MN_OK) {
		CSerial::SERAPI_ERR winErr;
		// Setup basic port states / brakes on until proven otherwise
		winErr = pNCS->pSerialPort->Setup(MN_NET_BAUD_RATE,
			CSerial::EData8,
			CSerial::EParNone,
			CSerial::EStop1,
			CSerial::EDTRClear,
			CSerial::ERTSClear);
		// Translate port error to a better error codes
		switch (winErr) {
		case CSerial::API_ERROR_SUCCESS:
			// This is OK
			break;
		case CSerial::API_ERROR_PORT_SETUP:
			return(MN_ERR_BAUD_NOT_SUPPORTED);
		case CSerial::API_ERROR_PORT_UNAVAILABLE:
		default:
			return(MN_ERR_PORT_PROBLEM);
		}
		// Restore brake settings if restarting
		theErr = infcBrakeSet(cNum, 0, SysInventory[cNum].brake0Saved, true);
		if (theErr == MN_OK) 
			theErr = infcBrakeSet(cNum, 1, SysInventory[cNum].brake1Saved, true);
		if (theErr != MN_OK) 
			return theErr;

		// Prevent errors from reporting to user
		SysInventory[cNum].Initializing++;

		// Stop polling thread to prevent interference
		infcBackgroundPollControl(cNum, false);

		// Assume an NC for commands to run without MN_ERR_ADDR_RANGE error.
		SysInventory[cNum].InventoryNow.NumOfNodes = 0;

		// Connect our local pkt received event to the serial channel
		pNCS->pSerialPort->RegisterUserPktCommEvent(&pNCS->ReadCommEvent);
		if (SysInventory[cNum].OpenState != OPENED_ONLINE)
			SysInventory[cNum].OpenStateNext(OPENED_SEARCHING);
		SysInventory[cNum].Initializing--;

		// Restart the poller if we can and it was running at start.
		if (SysInventory[cNum].Initializing == 0) {
			// Turn on the polling, keeping existing error
			cnErrCode pollErr = infcBackgroundPollControl(cNum,
				SysInventory[cNum].KeepAlivePollRestoreState);
			theErr = (theErr == MN_OK) ? pollErr : theErr;
		}

	}
	// If we get here, we are up and running
	pNCS->ReadThread.Start();
	// Setup the port for use on Meridian network at power-up baud rate
	if (theErr != MN_OK) {
		return(theErr);
	}
	#if TRACE_BREAK
	_RPT3(_CRT_WARN, "%.1f in infcStartController(%d) calling infcResetNetRate,"
		"thread ID=" THREAD_RADIX_LONG "\n",
		infcCoreTime(), cNum, infcThreadID());
	#endif
	// Finalize with force net to initial rate and run connectivity test
	theErr = infcResetNetRate(cNum);
	//pInitLock->Unlock();							// Unlock
	#ifdef _DEBUG
	// Look for trouble created during init
	infcHeapCheck("infcStartController: exiting");
	#endif
	#if TRACE_START_STOP
	_RPT2(_CRT_WARN, "%.1f infcStartController %d done.\n", infcCoreTime(), cNum);
	#endif
	return theErr;

}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		infcStopController
//
//	DESCRIPTION:
//		Shutdown access to the previously started net controller
//		device.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcStopController(
	netaddr cNum)		// Index of controller desired

{
	netStateInfo *pNCS;
	cnErrCode theErr = MN_OK;
	#if TRACE_START_STOP
	_RPT2(_CRT_WARN, "%.1f infcStopController %d...\n",
		infcCoreTime(), cNum);
	#endif

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	// Convert to pointer form
	pNCS = SysInventory[cNum].pNCS;

	// Already Offline, say OK
	if (!pNCS)
		return MN_OK;

	// We are shutting down now	if currently open and running
	if (pNCS->pSerialPort->IsOpen()) {
		infcFireNetEvent(cNum, NODES_OFFLINE);
		// Kill off any workers using the port
		pNCS->pAutoDiscover->Terminate();
		pNCS->pAutoDiscover->WaitForTerm();

		pNCS->pPollerThread->Terminate();
		pNCS->pPollerThread->WaitForTerm();

		// Get current state, ignore errors that occurred
		infcBrakeGet(cNum, 0, &SysInventory[cNum].brake0Saved, true);
		infcBrakeGet(cNum, 1, &SysInventory[cNum].brake1Saved, true);
		// Start ignoring incoming data
		pNCS->pSerialPort->AutoFlush(true);
		pNCS->pSerialPort->Flush();
		pNCS->pSerialPort->Break(100);
		SysInventory[cNum].OpenStateNext(CLOSED);
		pNCS->ReadThread.Stop();
		pNCS->pSerialPort->Close();
		// Insure destruction can proceed
		pNCS->CmdsIdle.SetEvent();
	}
	delete SysInventory[cNum].pNCS;
	SysInventory[cNum].pNCS = NULL;

	// Unhook ourselves from the system
	SysInventory[cNum].clearNodes(true);
	#if TRACE_START_STOP
	_RPT3(_CRT_WARN, "%.1f infcStopController %d: stopped code=%x\n",
		infcCoreTime(), cNum, theErr);
	#endif
	return theErr;
}
//																			  *
//*****************************************************************************


//*******************************************************************************
//	NAME																	    *
//		makeNopPacket
//
//	DESCRIPTION:
//		Flush network received data and structures. It is assumed the data
//		structures are locked out prior to calling this.
//
//	RETURNS:
//		Number of characters flushed.
//
//	SYNOPSIS:
void makeNopPacket(packetbuf *pBuf, bool isHighPrio) {
	// Create a low level NOP to clear out any frags waiting
	pBuf->Fld.Src = MN_SRC_HOST;
	pBuf->Fld.PktType = isHighPrio ? MN_PKT_TYPE_EXTEND_HIGH
		: MN_PKT_TYPE_EXTEND_LOW;
	pBuf->Fld.Addr = 0;						// Assume first node
	pBuf->Fld.PktLen = 1;
	pBuf->Fld.Mode = pBuf->Fld.Zero1 = 0;	// Fill in unused with 0
	pBuf->Byte.Buffer[2] = (isHighPrio ? (nodechar)MN_CTL_EXT_NOP
		: (nodechar)MN_CTL_EXT_LOW_NOP);
	pBuf->Byte.BufferSize = pBuf->Fld.PktLen + MN_API_PACKET_HDR_LEN;
}
//																			    *
//*******************************************************************************

//*******************************************************************************
//	NAME																	    *
//		infcFlushProc
//
//	DESCRIPTION:
//		Flush network received data and structures. It is assumed the data
//		structures are locked out prior to calling this.
//
//	RETURNS:
//		Number of characters flushed.
//
//	SYNOPSIS:
nodeulong infcFlushProc(netaddr cNum)
{
	nodeulong lostCount = 0;
	nodeushort i;
	respTrackInfo *nextWaiter, *head;
	packetbuf theCmd;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	// Skip if device is not open
	if (!pNCS->pSerialPort)
		return(0);

	#if TRACE_LOW_PRINT&&TRACE_FLUSH
	_RPT1(_CRT_WARN, "%.1f infcFlushProc, starting...\n", infcCoreTime());
	#endif
	if (SysInventory[cNum].Initializing == 0) {
		_RPT2(_CRT_WARN, "%.1f infcFlushProc(%d), online during flush...\n", infcCoreTime(), cNum);
	}
	// Prevent reading of these responses
	pNCS->ReadThread.Stop();
	// Ignore all data past here
	pNCS->pSerialPort->AutoFlush(true);

	// Create a low level NOP to clear out any frags waiting
	makeNopPacket(&theCmd, false);
	infcSendCommand(cNum, &theCmd);

	// Create a high level NOP to clear out any frags waiting
	makeNopPacket(&theCmd, true);
	infcSendCommand(cNum, &theCmd);
	// Let junk accumulate
	infcSleep(50);
	// flush all data out of the serial port
	// Clean up threads waiting
	for (i = 0; i<MN_API_MAX_NODES; i++) {
		// Return all storage for threads waiting past head
		head = pNCS->respNodeState[i].head;
		while (head != NULL) {
			nextWaiter = head->next;
			pNCS->removeThisDBitem(head, &pNCS->respNodeState[i]);
			head = nextWaiter;
		}
	}

	// Clean up the control command buffers
	head = pNCS->controlNodeState.head;
	while (head != NULL) {
		nextWaiter = head->next;
		pNCS->removeThisDBitem(head, &pNCS->controlNodeState);
		// Record the next head
		head = nextWaiter;
	}
	// There are no responses left now
	pNCS->nRespOutstanding = 0;	// This should have already been 0
	infcSleep(0);					// Yield our quantum
									// Get rid of any other detritus
	if (pNCS->pSerialPort) {
		(pNCS->pSerialPort)->Flush();
		// Start getting data past here
		pNCS->pSerialPort->AutoFlush(false);
	}
	// Restart the reader
	pNCS->ReadThread.Start();
	infcSleep(0);					// Yield our quantum
	#if TRACE_LOW_PRINT&&TRACE_FLUSH
	_RPT1(_CRT_WARN, "%.1f infcFlushProc, done...\n", infcCoreTime());
	#endif
	return(lostCount);
}
//																			    *
//*******************************************************************************


//*******************************************************************************
//	NAME																	    *
//		infcFlush
//
//	DESCRIPTION:
//		Flush network received data while locking out structures.
//
//	RETURNS:
//		Number of characters flushed.
//
//	SYNOPSIS:
nodeulong MN_DECL infcFlush(
	netaddr cNum)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	nodeulong lostCount;
	// Lock out other activity
	ENTER_LOCK("infcFlush");
	// Flush NC data
	lostCount = infcFlushProc(cNum);
	EXIT_LOCK("infcFlush");
	return(lostCount);
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		infcGetResponse
//
//	DESCRIPTION:
//		This function will retrieve the response for <theCmd> sent
//		previously. The the current thread is suspended until the result
//		is returned or on error due to lack of response. The results of this
//		transfer are placed in the packetbuf pointed by <theResponse>.
//
//	RETURNS:
//		#cnErrCode of send success
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetResponse(
	netaddr cNum,
	packetbuf *theResp)					// pointer to a command buffer area

{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Interface gone?
	if (!pNCS->pSerialPort)
		return(MN_ERR_CLOSED);
	// Wait and get a response
	if (pNCS->pSerialPort->GetPkt(*theResp)) {
		//_RPT2(_CRT_WARN, "%.1f infcGetResponse bufsize=%d\n",
		//	infcCoreTime(), theResp->Byte.BufferSize);
		pNCS->nPktsRcvd++;
		return(MN_OK);
	}
	else
		return(MN_ERR_RESP_TIMEOUT);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcSendCommand
//
//	DESCRIPTION:
//		This function will send the command specified by the <theCommand>
//		field to the current network controller.
//
//	RETURNS:
//		#cnErrCode of send success
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcSendCommand(
	netaddr cNum,
	packetbuf *theCommand)			// pointer to a command buffer area

{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Interface gone?
	if (!pNCS->pSerialPort)
		return(MN_ERR_CLOSED);

	#if TRACE_SEND_RESP
	theCommand->Fld.StartOfPacket = 1;
	DUMP_PKT(cNum, "SendCommand   ", theCommand);
	#endif

	// Send atomically via the serial port
	if (pNCS->pSerialPort->SendPkt(*theCommand)) {
		pNCS->nPktsSent++;
		return(MN_OK);
	}

	// Something went wrong!
	return(MN_ERR_SEND_FAILED);
}


// Same as infcSendCommand, but log in send log, we are not expecting response.
cnErrCode MN_DECL infcSendCommandLog(
	nodeushort cNum,					// Port Index
	packetbuf *theCommand)			    // Ptr to buffered command
{
	cnErrCode theErr;
	double txStart = infcCoreTime();
	theErr = infcSendCommand(cNum, theCommand);
	// If all OK, log it
	if (cNum < NET_CONTROLLER_MAX && SysInventory[cNum].pNCS)
		SysInventory[cNum].logSend(theCommand, theErr, txStart);
	return(theErr);
}
///																			   *
//******************************************************************************




/*****************************************************************************
*	NAME
*		infcSerialStats
*
*	DESCRIPTION:
*		Get and reset the current character counts for the serial port.
*
*	RETURNS:
*		TRUE if a character is can safely be sent
*
*	SYNOPSIS: 															    */
MN_EXPORT cnErrCode MN_DECL infcSerialStats(
	netaddr cNum,
	nodeulong &rxCnt,
	nodeulong &txCnt,
	nodeulong &rxPktCnt,
	nodeulong &txPktCnt)
{
	// Bounds check arguments
	if (cNum > NET_CONTROLLER_MAX)
		return MN_ERR_BADARG;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS)
		return MN_ERR_FAIL;
	if (!pNCS->pSerialPort)
		return MN_ERR_FAIL;
	rxCnt = pNCS->pSerialPort->ReceiveCount();
	txCnt = pNCS->pSerialPort->TransmitCount();
	rxPktCnt = pNCS->nPktsRcvd;
	txPktCnt = pNCS->nPktsSent;
	// Atomically reduce counts
	pNCS->nPktsRcvd -= rxPktCnt;
	pNCS->nPktsSent -= txPktCnt;
	return MN_OK;
}
//																			 *
//****************************************************************************




//****************************************************************************
//	NAME
//		infcSetInitializeMode
//
//	DESCRIPTION:
//		Adjust the initializing flag and fire the proper set of callbacks
//		when we are completed with all the initializing items. This function
//		is called in a "stacked" manner from all the initializing functions
//		to allow low and high level initialization to produce a single set
//		of "offline" and "online" events for each invokation of the
//		initialization functions such as mnInitializeNet or tgInitialization.
//		mnInitialization calls the low-level functions as a matter of its
//		execution so scheduling of the events would be complicated if this
//		function did not keep track of the "stack depth".
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
cnErrCode MN_DECL infcSetInitializeMode(
	netaddr cNum,
	nodebool initializing,
	cnErrCode lastErr)
{
	enum NetworkChanges state = NODES_OFFLINE;
	#define LCL_INIT_PR 0
	mnNetInvRecords &theNet = SysInventory[cNum];
	//netStateInfo *pNCS = theNet.pNCS;
	if (initializing) {
		theNet.Initializing++;
		if (theNet.Initializing == 1) {
			// If first time, signal we are going offline
			state = NODES_OFFLINE;
			// Prevent polling
			infcBackgroundPollControl(cNum, false);
			infcFireNetEvent(cNum, NODES_OFFLINE);
		}
		#if (TRACE_CALLBACK&&TRACE_LOW_LEVEL) || LCL_INIT_PR
		_RPT4(_CRT_WARN, "%.1f infcSetInitializeMode(%d): Initializing #%d, err=0x%x\n",
			infcCoreTime(), cNum, theNet.Initializing, lastErr);
		#endif
	} // if (initializing)
	else {
		if (theNet.Initializing > 0) {
			theNet.Initializing--;
		}
		else {
			// 12/20/12 DS infcSetQueueLimit when online can cause this
			//// Decrement when the init counter is zero - this shouldn't happen
			//infcErrInfo theErrRec;
			//theErrRec.cNum = cNum;
			//theErrRec.node = MN_API_MAX_NODES;
			//theErrRec.errCode = MN_ERR_RESET_FAILED;
			//theErrRec.response.bufferSize = 0;
			////infcFireErrCallback(&theErrRec);
			//_RPT2(_CRT_WARN, "%.1f infcSetInitializeMode(%d): Init counter already zero\n",
			//				infcCoreTime(), cNum);
		}
		// Last one out fires off the final net state event
		if (theNet.Initializing == 0) {

			//cnErrCode theErr = MN_OK;
			#if (TRACE_CALLBACK&&TRACE_LOW_LEVEL) || LCL_INIT_PR
			_RPT3(_CRT_WARN, "%.1f infcSetInitializeMode(%d): Initialization complete, err=0x%x\n",
				infcCoreTime(), cNum, lastErr);
			#endif
			// Allow next diagnostic to play
			theNet.NetDiagLastResult = MN_OK;
			if (theNet.OpenState == OPENED_SEARCHING) {
				// Save a copy of the inventory in case we go offline
				theNet.InventoryLast = theNet.InventoryNow;
			}
			// Attempt to restore network speed when going back online
			switch (lastErr) {
			case MN_OK:
			case MN_ERR_TEST_INCOMPLETE:
				break;
			default:
				// Ignore other types
				break;
			}
			// Always send some events based on our final operational
			// state.
			switch (lastErr) {
			case MN_OK:
				state = NODES_DRIVER_ONLINE;
				break;
			case MN_ERR_TEST_INCOMPLETE:
				state = NODES_ONLINE_NO_TEST;
				break;
			case MN_ERR_PORT_PROBLEM:
				state = NODES_NO_PORT;
				break;
			case MN_ERR_BAUD_NOT_SUPPORTED:
				state = NODES_BAUD_UNSUPPORTED;
				break;
			default:
				// Start looking for nodes if we could not
				// get online at this time.
				infcStartAutoNetDiscovery(cNum, lastErr);
				state = NODES_OFFLINE;
				break;
			}
			// Clear node error counts when going online, the
			// initialization process leaves nodes with errors due
			// to the break condition sent to set baud rates.
			if (state == NODES_DRIVER_ONLINE) {
				for (unsigned i = 0; i<theNet.InventoryNow.NumOfNodes; i++) {
					mnNetErrStats myDiagResult;
					cnErrCode theErr =
						netGetErrStats(MULTI_ADDR(cNum, i), &myDiagResult);
					// Rescind our ONLINE state!
					if (theErr != MN_OK) {
						_RPT3(_CRT_WARN, "%.1f infcSetInitializeMode(%d): failed to clear net stats, err=0x%x\n",
							infcCoreTime(), cNum, theErr);
						lastErr = theErr;
						break;
					}
				}
				// Rescind advance for port options if we have a non-advance node
				if (theNet.PhysPortSpecifier.PortType == CPM_COMHUB 
				&& (theNet.pNCS)->OurNetConnector == MN_SRC_APP_NET) {
					// Make sure the brake addressing is OK
					for (size_t i = 0; i < mnNetInvRecords::MAX_BRAKES; i++) {
						nodeaddr theAddr = NODE_ADDR(theNet.autoBrake[i].relatedNode);
						// If net smaller now, cancel old brakes
						if (theAddr != MN_UNSET_ADDR
							&& theAddr > theNet.InventoryNow.NumOfNodes) {
							theNet.autoBrake[i].relatedNode = MN_UNSET_ADDR;
						}
						// If unset, make it match the node number
						if (theNet.autoBrake[i].relatedNode == MN_UNSET_ADDR
							&& i < theNet.InventoryNow.NumOfNodes) {
							theNet.autoBrake[i].relatedNode = MULTI_ADDR(cNum, i);
						}
					}

					// Restore the auto-brake state
					theNet.setupBrakes();

					// Restore group shutdowns
					// Skip processing our attentions
					theNet.attnInitializing++;
					for (size_t i = 0; i < theNet.InventoryNow.mnCpScInfo.count; i++) {
						multiaddr theAddr = theNet.InventoryNow.mnCpScInfo.node[i];
						ShutdownInfo &sInfo = theNet.GroupShutdownInfo[NODE_ADDR(theAddr)];
						infcShutdownInfoSet(theAddr, &sInfo);
						// Restore the driver attention masks
						netSetDrvrAttnMask(theAddr, &theNet.drvrAttnMask[NODE_ADDR(theAddr)]);
					}
					// Wait for attentions to propagate
					infcSleep(50);
					// Allow processing again
					theNet.attnInitializing--;
				}
				// Restore the polling, keeping existing error
				cnErrCode pollErr
					= infcBackgroundPollControl(cNum,
						theNet.KeepAlivePollRestoreState);
				lastErr = (lastErr == MN_OK) ? pollErr : lastErr;
			}
			// Go online now so that stats clear can run from user's
			// threads.
			infcFireNetEvent(cNum, state);
			} // if theNet.Initializing == 0 && InitializingGlobal == 0
		else {
			// Still not enough layers of initialization complete yet
			#if (TRACE_CALLBACK&&TRACE_LOW_LEVEL) || LCL_INIT_PR
			_RPT4(_CRT_WARN, "%.1f infcSetInitializeMode: heading online #%d offline=%d, err=0x%x\n",
				infcCoreTime(), theNet.Initializing,
				theNet.OpenState, lastErr);
			#endif
		} // theNet.Initializing != 0 || InitializingGlobal != 0
		} // if !initializing
	#if TRACE_LOW_LEVEL || LCL_INIT_PR
	_RPT2(_CRT_WARN, "infcSetInitializeMode: count=%d, state=%d\n",
		theNet.Initializing, theNet.OpenState);
	#endif
	return(lastErr);
	}
//																			 *
//****************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcGetInitializeMode
//
//	DESCRIPTION:
//		Get the initializing flag.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
cnErrCode MN_DECL infcGetInitializeMode(
	netaddr cNum,
	nodebool *initializing)
{
	// Return TRUE if we are still initializing
	*initializing = (SysInventory[cNum].Initializing > 0);
	return(MN_OK);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcSetCmdQueueLimit
//
//	DESCRIPTION:
//		Set the limit of simultaneous commands in the ring.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcSetCmdQueueLimit(
	netaddr cNum,
	nodeulong nCmds)
{
	cnErrCode theErr = MN_OK;
	// Bounds check arguments
	if (nCmds == 0 || nCmds > 14 || cNum > NET_CONTROLLER_MAX)
		return(MN_ERR_BADARG);
	// See if the network exists
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (pNCS) {
		infcSetInitializeMode(cNum, TRUE, MN_OK);
		// Stop using the network
		theErr = infcStopController(cNum);
		// Note: pNCS is invalid past here!
		// Make the change in our static area
		SysInventory[cNum].NumCmdsInRing = nCmds;
		if (theErr == MN_OK) {
			// Restart the controller to install new command limits if
			// we were running before.
			theErr = infcStartController(cNum);
			if (theErr == MN_OK) {
				theErr = netEnumerate(cNum);
			}
		}
		infcSetInitializeMode(cNum, FALSE, theErr);
	}
	else {
		// Make the change in our static area
		SysInventory[cNum].NumCmdsInRing = nCmds;
	}
	return(theErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcSetTraceEnable
//
//	DESCRIPTION:
//		Manipulate the capture of the trace information.
//
//	RETURNS:
//		Nothing
//
//	SYNOPSIS:
MN_EXPORT void MN_DECL infcSetTraceEnable(
	netaddr cNum,
	nodebool state)
{
	SysInventory[cNum].TraceActiveReq = SysInventory[cNum].TraceActive = state;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME
//		infcUpdateInventory
//
//	DESCRIPTION:
//		Update the last known inventory with this information.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
cnErrCode infcUpdateInventory(netaddr cNum,
	mnNetInvRecords *inventory)
{
	unsigned i;

	SysInventory[cNum].InventoryNow.isValid = false;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS || !pNCS->pSerialPort)
		return(MN_ERR_CLOSED);
	SysInventory[cNum].InventoryLast = inventory->InventoryNow;
	for (i = 0; i<MN_API_MAX_NODES; i++) {
		SysInventory[cNum].InventoryNow.LastIDs[i]
			= SysInventory[cNum].NodeInfo[i].theID;
	}
	SysInventory[cNum].InventoryNow.isValid = true;
	return(MN_OK);
}
//																			 *
//****************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcWaitForNetStop
//
//	DESCRIPTION:
//		This function will wait for the current network commands to finish
//		and their responses to return.
//
//	SYNOPSIS:
void MN_DECL infcWaitForNetStop(netaddr cNum)
{
	cnErrCode theErr;
	packetbuf theCmd, theResp;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS || !pNCS->pSerialPort)
		return;
	// If the channel is offline and can't work return immediatly
	if (SysInventory[cNum].OpenState != OPENED_ONLINE)
		return;
	// If there were nodes at one time, send a probe command to sync up, else
	// return immediately.
	if (SysInventory[cNum].InventoryLast.NumOfNodes == 0)
		return;
	// Generate a command all nodes have: get node ID of first node.
	theErr = netGetParameterFmt(&theCmd, 0, MN_P_NODEID);
	if (theErr != MN_OK)
		return;
	// Run the command to sync this thread's view of the network
	// It doesn't matter if this works or not
	netRunCommand(cNum, &theCmd, &theResp);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcAutoDiscoverRunning
//
//	DESCRIPTION:
//		This function will return true if the network is "offline" and
//		the auto-discover thread is looking for nodes.
//
//	SYNOPSIS:
nodebool infcAutoDiscoverRunning(netaddr cNum)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS || !pNCS->pSerialPort || !pNCS->pAutoDiscover)
		return(false);
	return(pNCS->pAutoDiscover->IsRunning());
}
//																			   *
//******************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcCopyPktToPkt18
//
//	DESCRIPTION:
///		Copy a full packet buffer to truncated one.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
//#if (defined(_WIN32)||defined(_WIN64))
//static void infcCopyPktToPkt18(packetbuf18 *dest, packetbuf *src)
//#else
void infcCopyPktToPkt18(packetbuf18 *dest, packetbuf *src)
//#endif
{
	dest->bufferSize = src->Byte.BufferSize < 18 ? src->Byte.BufferSize : 18;
	memcpy(&dest->buffer[0], &src->Byte.Buffer[0],
		dest->bufferSize);
}
//																			  *
//*****************************************************************************


//******************************************************************************
//	NAME																	   *
//		IsNetClassErr
//
//	DESCRIPTION:
//		Return TRUE if this is a potentially node H/W based error:
//
//	RETURNS:
//		nodebool
//
//	SYNOPSIS:
nodebool IsNetClassErr(
	cnErrCode theErr)
{
	BOOL bErr = FALSE;
	switch (theErr) {
	case MN_ERR_FRAME:				/* 0xe:  command frame error */
	case MN_ERR_OFFLINE:			/* 0x8:  the node is offline */
	case MN_ERR_PKT_ERR:			/* 0x11: Pkt Error */
	case MN_ERR_NET_ERRS_DETECT:	/* 0x2c: Network Errors */
	case MN_ERR_RX_ACCESS:			/* 0x14: RX buffer access/sync problem */
	case MN_ERR_NET_FRAG:			/* 0x104: Fragmented packet detected */
	case MN_ERR_SEND_FAILED:		/* 0x200: Command failed to transfer from NC in time */
	case MN_ERR_SEND_LOCKED:		/* 0x201: Could not acquire NC command interface */
	case MN_ERR_SEND_UNLOCK:		/* 0x202: Could not release NC command interface */
	case MN_ERR_RESP_FAILED:		/* 0x203: Response failed to transfer */
	case MN_ERR_CMD_TIMEOUT:		/* 0x205: Command timeout */
	case MN_ERR_DEF_RESP_TO:		/* 0x20A: Deferred response time-out */
	case MN_ERR_ADDR_RANGE:			/* 0x20D: Response from out of range address */
	case MN_ERR_RESP_TIMEOUT:		/* 0x204: NC response not found in time */

		bErr = TRUE;
		break;
	default:
		// Not sure yet, keep checking
		break;
	}

	// Check for network breaks/offline - network breaks and offline are
	// considered hardware errors
	if (IsNetBreakErr(theErr))
		bErr = TRUE;

	return(bErr);
}


// Returns TRUE if this is a net connectivity type error.
nodebool IsNetBreakErr(
	cnErrCode theErr)
{
	nodebool bErr = (((theErr) >= MN_ERR_NETBREAK_ERR_BASE
		&& (theErr) <= MN_ERR_NETBREAK_ERR_END)
		|| ((theErr) == MN_ERR_RESP_TIMEOUT)
		|| ((theErr) == MN_ERR_SEND_FAILED)
		|| ((theErr) == MN_ERR_RESET_FAILED)
		|| ((theErr) == MN_ERR_CMD_OFFLINE));
	return(bErr);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		processNodeInitiatedPkt
//
//	DESCRIPTION:
/**
Process the node initiated data

\param xxx description
\return description
**/
//	SYNOPSIS:
inline void processNodeInitiatedPkt(netaddr cNum, nodeaddr respAddr,
	netStateInfo *pNCS, packetbuf &readBuf)
{
	infcErrInfo errInfo;		// Error reporting buffer
	mnAttnReqReg attn;
	dacqPacket *dataAcqPacket;
	mnDataAcqPt dataAcqPt[2];
	nodebool lastOverflow;

	nodeaddr changedNode;
	mnNetDiagStats *pStats = (mnNetDiagStats *)(&readBuf.Byte.Buffer[CMD_LOC + 1]);

	// See if we do anything with these6
	mnNetDiagStats *pLog = &SysInventory[cNum].diagStats[readBuf.Fld.Addr];
	switch (readBuf.Fld.PktType) {
	case MN_PKT_TYPE_EXTEND_HIGH:
		switch (readBuf.Byte.Buffer[CMD_LOC]) {
		case MN_CTL_EXT_NET_DIAG_INFO:
			SysInventory[cNum].diagsAvailable[readBuf.Fld.Addr] = TRUE;
			// Fix up new fields found when old firmware in use
			if ((readBuf.Fld.PktLen - 1) / 2U
				<= (unsigned)(&pStats->NetVoltsLowCtr - (Uint16 *)pStats)) {
				pStats->NetVoltsLowCtr = 0;
			}
			// Copy to our state
			*pLog = *pStats;
			_RPT3(_CRT_WARN, "%.1f readThread(%d) node %d reported err: \n",
				infcCoreTime(), cNum, readBuf.Fld.Addr);
			DUMP_PKT(cNum, (char *)"...", &readBuf, true);
			#ifdef _DEBUG
			_RPT2(_CRT_WARN, "Net Diag Counters:\tApp Net\t\t\t\tDiag Net reporting node %d, net %d\n",
				readBuf.Fld.Addr, cNum);
			_RPT0(_CRT_WARN, "\tfrags\tchks\tstray\tover\t\tfrags\tchks\tstray\tover\tlow V\n");
			_RPT4(_CRT_WARN, "%8d%8d%8d%8d",
				pLog->AppNetFragPktCtr, pLog->AppNetBadChksumCtr,
				pLog->AppNetStrayCtr, pLog->AppNetOverrunCtr);
			_RPT5(_CRT_WARN, "\t%8d%8d%8d%8d%8d\n",
				pLog->DiagNetFragPktCtr, pLog->DiagNetBadChksumCtr,
				pLog->DiagNetStrayCtr, pLog->DiagNetOverrunCtr,
				pLog->NetVoltsLowCtr);
			#endif
			// Let others know of the troubles
			errInfo.cNum = cNum;
			errInfo.node = respAddr;
			errInfo.errCode = MN_ERR_NET_ERRS_DETECT;
			infcCopyPktToPkt18(&errInfo.response, &readBuf);
			infcFireErrCallback(&errInfo);
			break;
		case MN_CTL_EXT_NET_CHECK:
			_RPT2(_CRT_WARN, "readThread(%d) detected net integrity check request from node %d\n",
				cNum, readBuf.Fld.Addr);
			break;
		default:
			_RPT2(_CRT_WARN, "readThread(%d) detected unhandled node generated extended high packet %d\n",
				cNum, readBuf.Byte.Buffer[CMD_LOC]);
			break;
		}
		break;
		//case MN_PKT_TYPE_EXTEND_NORMAL:
		//	break;
		//case MN_PKT_TYPE_ERROR:
		//	break;
	case MN_PKT_TYPE_ATTN_IRQ:
		attn.AttentionReg.attnBits = *((Uint32 *)&readBuf.Byte.Buffer[RESP_LOC]);
		// Cleanup the attention bits as required by device
		attn.AttentionReg.attnBits &= SysInventory[cNum].attnCleanupMask[readBuf.Fld.Addr];
		attn.MultiAddr = MULTI_ADDR(cNum, readBuf.Fld.Addr);
		processAttention(attn, pNCS);
		break;
	case MN_PKT_TYPE_EXTEND_LOW:
		switch (readBuf.Byte.Buffer[CMD_LOC]) {
		case MN_CTL_EXT_LOW_NOP:
			break;
		case MN_CTL_EXT_DATA_ACQ:	// Data return originated from the node and is a dataAcq point
			#if 1
									// Record DataAcq in Queue - each data point from the node contains 2 point
			dataAcqPacket = (dacqPacket*)&(readBuf.Byte.Buffer[RESP_LOC + 1]);

			// Replicate the common information
			dataAcqPt[1].Sequence = dataAcqPt[0].Sequence
				= dataAcqPacket->Fld.Sequence;

			dataAcqPt[0].Bool0 = dataAcqPt[1].Bool0 = FALSE;
			dataAcqPt[0].Bool1 = dataAcqPt[1].Bool1 = FALSE;

			dataAcqPt[0].Exception = dataAcqPacket->Fld.Exception0;
			dataAcqPt[1].Exception = dataAcqPacket->Fld.Exception1;

			dataAcqPt[0].MoveState = dataAcqPacket->Fld.MoveState0;
			dataAcqPt[1].MoveState = dataAcqPacket->Fld.MoveState1;
			// Check for alignment issues
			//dataAcqPt[0].Spare[1] = 0x0111;
			//dataAcqPt[1].Spare[1] = 0x1111;
			//dataAcqPt[0].TraceValue[3] = 0.0333;
			//dataAcqPt[1].TraceValue[3] = 0.1333;


			// Calculate fractional (+/-1) first point
			dataAcqPt[0].TraceValue[0]
				= (float)dataAcqPacket->Fld.T0p0
				/ (1 << (P0_BITS - 1));

			dataAcqPt[0].TimeStamp
				= (double)(pNCS->DataAcq[respAddr].SampleCount)
				* pNCS->DataAcq[respAddr].SampRateMilliSec;
			dataAcqPt[1].TimeStamp
				= dataAcqPt[0].TimeStamp
				+ pNCS->DataAcq[respAddr].SampRateMilliSec;

			// We have processed two samples
			pNCS->DataAcq[respAddr].SampleCount += 2;

			// Calculate fractional (+/-1) second point
			dataAcqPt[1].TraceValue[0]
				= (float)dataAcqPacket->Fld.T0p1
				/ (1 << (P1_BITS - 1));

			// If the I/O state was sent in this packet, copy the data to trace 1
			if (readBuf.Byte.BufferSize == 9) {
				dataAcqPt[0].TraceValue[1] = (float)((dataAcqPacket->Fld.Trigger0 << (INPUT_BITS + 1))
					| (dataAcqPacket->Fld.Inputs0 << 1)
					| dataAcqPacket->Fld.Output0);
				dataAcqPt[1].TraceValue[1] = (float)((dataAcqPacket->Fld.Trigger1 << (INPUT_BITS + 1))
					| (dataAcqPacket->Fld.Inputs1 << 1)
					| dataAcqPacket->Fld.Output1);
			}


			// Check for dropped packet by checking sequence, we are OK if
			// the init flag is set
			if (pNCS->DataAcqInit[respAddr]
				|| (dataAcqPt[0].Sequence
					== pNCS->DataAcq[respAddr].SeqCheck + 1)
				|| ((dataAcqPt[0].Sequence == 0)
					&& (pNCS->DataAcq[respAddr].SeqCheck == 3))) {
				// Once we get here, init no longer necessary
				pNCS->DataAcqInit[respAddr] = FALSE;
				// Sequence is valid (it is an increment from before or it wrapped from 3 to 0)
				dataAcqPt[0].Valid = VB_TRUE;
				dataAcqPt[1].Valid = VB_TRUE;
			}
			else {
				// Sequence is invalid
				dataAcqPt[0].Valid = VB_FALSE;
				dataAcqPt[1].Valid = VB_FALSE;
				// 11/4/09 DS Let Reader discover problem, this
				// just leads to more errors if we callback.
				// 11/6/10 DS Restoring for diagnostic purposes. Newer error
				// processing should prevent last year issues

				// Notify that there is an invalid point
				errInfo.cNum = cNum;
				errInfo.node = respAddr;
				errInfo.errCode = MN_ERR_DATAACQ_INVALID;
				_RPT4(_CRT_WARN, "%.1f data acq seq err@node %d: %d->%d\n", 
					infcCoreTime(), respAddr, pNCS->DataAcq[respAddr].SeqCheck,
					dataAcqPt[0].Sequence);
				infcFireErrCallback(&errInfo);
				#if TRACE_DACQ_SEQ
				infcTraceDumpA(cNum, "MeridianSeqErr.dat");
				#endif
		}
			// Save this sequence number in channel state to compare with next sample
			pNCS->DataAcq[respAddr].SeqCheck = dataAcqPt[0].Sequence;
			//_RPT1(_CRT_WARN, "%d", respAddr); // Show activity

			// Write the points to the queue, locking out reader
			pNCS->DataAcq[respAddr].AcqLock.Lock();

			// On overflow deprecate the two oldest points to make room
			// in buffer for 2 new points marking gap at front
			lastOverflow = pNCS->DataAcq[respAddr].Overflow;
			pNCS->DataAcq[respAddr].Overflow
				= pNCS->DataAcq[respAddr].Points.size()
				>= (DATAACQ_OVERFLOW_LVL - 2);
			if (pNCS->DataAcq[respAddr].Overflow) {
				pNCS->DataAcq[respAddr].Points.pop();
				pNCS->DataAcq[respAddr].Points.pop();
				pNCS->DataAcq[respAddr].Points.front().Valid = VB_FALSE;
			}

			// If an overflow occurred, signal back to app
			if (pNCS->DataAcq[respAddr].Overflow && !lastOverflow) {
				// Notify that there was an overflow event
				errInfo.cNum = cNum;
				errInfo.node = respAddr;
				errInfo.errCode = MN_ERR_DATAACQ_INVALID;
				infcCopyPktToPkt18(&errInfo.response, &readBuf);
				//						infcFireErrCallback(&errInfo);
				_RPT2(_CRT_WARN, "%.1f dacq overrun @%d\n",
					infcCoreTime(), respAddr); // Show activity
			}

			// Add the two points on queue
			pNCS->DataAcq[respAddr].Points.push(dataAcqPt[0]);
			pNCS->DataAcq[respAddr].Points.push(dataAcqPt[1]);

			// Allow reader now
			pNCS->DataAcq[respAddr].AcqLock.Unlock();
			#endif
			break;
		case MN_CTL_EXT_PARAM_CHANGED:	//A parameter on the node has changed
										// grab the node id from the packet
			changedNode = readBuf.Fld.Addr;

			// invalidate the local cache so the data gets re-read from the node
			coreInvalidateValCacheByNode(cNum, changedNode);

			// call app's callback associated with this
			if (userInvalCacheFunc != NULL) {
				(*userInvalCacheFunc)(cNum, changedNode);
			}

			// also signal to any non-callback apps that would care
			//   Yes, I'm talking to you, VB/APS!
			pNCS->paramsHaveChanged[changedNode] = TRUE;

			break;
		default:
			_RPT2(_CRT_WARN, "readThread(%d) detected unhandled MN_PKT_TYPE_EXTEND_LOW packet type=%d\n",
				cNum, readBuf.Byte.Buffer[CMD_LOC]);
			break;
	}
		break;
	default:
		_RPT2(_CRT_WARN, "readThread(%d) detected unhandled node generated packet type=%d\n",
			cNum, readBuf.Fld.PktType);
		break;
}
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		processAttention
//
//	DESCRIPTION:
/**
Process the passed attention. This will release any waiters for the
user any of the detected bits are unmasked.

\note The reaction functions within and called by this function
must not use any network functions as the read thread is locked
until this function completes.

\param theAttn A filled attention structure.
\param pNCS Network state
**/
//	SYNOPSIS:
inline void processAttention(
	mnAttnReqReg theAttn,
	netStateInfo *pNCS)
{
	// This is set if this attention should be sent to the user code.
	bool postAttn;
	bool disabling;
	netaddr cNum = pNCS->cNum;
	//cnErrCode theErr;
	#if TRACE_ATTN
	_RPT3(_CRT_WARN, "%.1f attn(%d)=>0x%lx\n", infcCoreTime(), theAttn.MultiAddr, theAttn.AttentionReg.attnBits);
	#endif
	// Prevent re-entry until we process all the items. 
	pNCS->AttnLock.Lock();
	mnNetInvRecords &theNet = SysInventory[pNCS->cNum];
	nodeaddr theAddr = NODE_ADDR(theAttn.MultiAddr);
	disabling = (theAttn.AttentionReg.cpm.Disabled
		| theAttn.AttentionReg.cpm.GoingDisabled) != 0;

	// Look for what happened and cause internal re-actions if required
	// Are these the auto-brake related items
	if ((theAttn.AttentionReg.cpm.Enabled | disabling) != 0
		&& theNet.attnInitializing == 0) {
		// Release any brakes?
		#if TRACE_ATTN
		_RPT5(_CRT_WARN, "%.1f attn(%d) going disabled=%d, disabled=%d, enabled=%d\n",
			infcCoreTime(), theAttn.MultiAddr, theAttn.AttentionReg.cpm.GoingDisabled,
			theAttn.AttentionReg.cpm.Disabled, theAttn.AttentionReg.cpm.Enabled);
		#endif
		for (Uint32 i = 0; i<mnNetInvRecords::MAX_BRAKES; i++) {
			mnNetInvRecords::autoBrakeInfo &brk = theNet.autoBrake[i];
			if (brk.enabled
				&& theAttn.MultiAddr == brk.relatedNode
				&& brk.brakeMode == BRAKE_AUTOCONTROL) {
				#if TRACE_ATTN || TRACE_BRAKE
				_RPT4(_CRT_WARN, "%.1f attn(%d) brake(%d) set=%d\n", infcCoreTime(), theAttn.MultiAddr, i,
					theAttn.AttentionReg.cpm.Disabled);
				#endif
				// We engage the brake when we either about to disable or currently are
				// disabled.
				/*theErr = */infcBrakeSet(pNCS->cNum, i, disabling);
		}
}
	}
	// Was there a status event?
	if (theAttn.AttentionReg.cpm.StatusEvent && theNet.attnInitializing == 0) {
		#if TRACE_ATTN
		_RPT2(_CRT_WARN, "%.1f attn(%d) status event\n", infcCoreTime(), theAttn.MultiAddr);
		#endif
		// See we are enabled to cause shutdowns
		if (theNet.GroupShutdownInfo[theAddr].enabled) {
			theNet.GroupShutdownRequest = true;
			pNCS->GroupShutdownEvent.SetEvent();
	}
	}

	// Apply the attention mask
	theAttn.AttentionReg.attnBits &= theNet.attnMask[NODE_ADDR(theAttn.MultiAddr)].attnBits;
	// Tell the user?
	postAttn = theAttn.AttentionReg.attnBits != 0;
	if (postAttn) {
		// If using the class library methods, do not queue.
		if (SysInventory[cNum].pPortCls->Adv.Attn.Enabled()) {
			if (SysInventory[cNum].pPortCls->Adv.Attn.HasAttnHandler()) {
				#if TRACE_ATTN
				_RPT2(_CRT_WARN, "%.1f attn(%d) invoking user callback\n", infcCoreTime(), theAttn.MultiAddr);
				#endif
				SysInventory[cNum].pPortCls->Adv.Attn.InvokeAttnHandler(theAttn);
			}
			// Try using the by-node signaler
			for (int iNode = 0; iNode < SysInventory[cNum].pPortCls->NodeCount(); iNode++) {
				if (SysInventory[cNum].pPortCls->Nodes(iNode).Info.Ex.Addr() == theAttn.MultiAddr) {
					SysInventory[cNum].pPortCls->Nodes(iNode).Adv.Attn.SignalAttn(theAttn.AttentionReg);
				}
			}
			pNCS->AttnLock.Unlock();
		}
		// Queue and release the lock
		else {
			if (pNCS->AttnBuf.size() < ATTN_OVERFLOW_LVL) {
				pNCS->AttnBuf.push(theAttn);
				pNCS->AttnLock.Unlock();
				pNCS->IrqEvent.SetEvent();
			}
			else {
				infcErrInfo theErr;
				// Post overrun as error
				pNCS->AttnOverrun = TRUE;
				pNCS->AttnLock.Unlock();
				theErr.cNum = pNCS->cNum;
				theErr.node = theAttn.MultiAddr;
				theErr.errCode = MN_ERR_ATTN_OVERRUN;
				theErr.response.bufferSize = 0;
				infcFireErrCallback(&theErr);
			}
		}
	}
	else {
		pNCS->AttnLock.Unlock();
	}
	#if TRACE_ATTN
	_RPT2(_CRT_WARN, "%.1f attn(%d) complete\n", infcCoreTime(), theAttn.MultiAddr);
	#endif
}
/// \endcond																  *
//******************************************************************************

/****************************************************************************/
//						    CALLBACK INTERFACES
/****************************************************************************/

/// \cond INTERNAL_DOC
/*****************************************************************************
*	NAME
*		postErrList
*
*	DESCRIPTION:
*		Log error to the VB error list and .
*
*	SYNOPSIS: 															    */
void postErrList(netStateInfo *pNCS, infcErrInfo *pErrInfo)
{
	pNCS->ErrListLock.Lock();
	size_t nextTail;
	// Too many there now?
	if ((nextTail = ((pNCS->ErrListTailPtr + 1) % ERR_CNT_MAX))
		== pNCS->ErrListHeadPtr) {
		// Eat oldest item off head
		pNCS->ErrListHeadPtr++;
		pNCS->ErrListHeadPtr %= ERR_CNT_MAX;
	}
	// Add to tail
	pNCS->ErrList[pNCS->ErrListTailPtr] = *pErrInfo;
	pNCS->ErrListTailPtr = nextTail;
	pNCS->ErrListLock.Unlock();

}
/// 																  *
//******************************************************************************

/*****************************************************************************
*	NAME
*		infcFireErrCallback
*
*	DESCRIPTION:
*		Fire off the error callback if defined.
*
*	SYNOPSIS: 															    */
void infcFireErrCallback(infcErrInfo *pErrInfo, bool forced)
{
	#define ERR_TRACE 1			// Set to force trace debugs
	infcErrInfo cpyErr;
	unsigned initCntIn;
	BOOL isRecovery;
	netaddr cNum = pErrInfo->cNum;
	mnNetInvRecords &theNet = SysInventory[cNum];
	netStateInfo *pNCS = theNet.pNCS;
	// Port gone
	if (!pNCS)
		return;
	//if (pErrInfo->errCode == MN_ERR_BADARG)
	//	_RPT0(_CRT_WARN, "Ding\n");

	// Prevent recursion here
	if (pNCS->errorRecursePrevent>0) {
		_RPT4(_CRT_WARN, "%.1f infcFireErrCallback(recursion): "
			"err=0x%x, node=%d, net=%d\n",
			infcCoreTime(), pErrInfo->errCode,
			(int)pErrInfo->node, (int)pErrInfo->cNum);
		attemptAutoDiscover(pErrInfo);
		return;
	}

	// we do not want to send an error during shutdown
	if (pNCS && !pNCS->SelfDestruct) {
		pNCS->errorRecursePrevent++;
		// Save the incoming initialization state
		initCntIn = SysInventory[cNum].Initializing;
		// Get a copy of the original error
		cpyErr = *pErrInfo;
		isRecovery = pNCS->isRecoveryThread();
		// Split out handling for specific errors
		switch (cpyErr.errCode) {
		case MN_ERR_CMD_OFFLINE:
			break;
		default:
			// Real user error during online time or a break diagnostic error, skipping
			// redundant items on forced situations.
			if ((((initCntIn == 0) && InitializingGlobal == 0)
				&& ((theNet.OpenState != FLASHING) && !isRecovery))
				|| forced) {
				// Skip if diagnostic same result
				if (IsNetBreakErr(cpyErr.errCode)
					&& (cpyErr.errCode == theNet.NetDiagLastResult)) {
					#if TRACE_CALLBACK
					_RPT4(_CRT_WARN, "%.1f infcFireErrCallback(%d) ignoring dup err: err=0x%x, node=%d\n",
						infcCoreTime(), (int)cpyErr.cNum, cpyErr.errCode, (int)cpyErr.node);
					#endif
					break;
				}
				#if TRACE_CALLBACK
				_RPT4(_CRT_WARN, "%.1f infcFireErrCallback(%d): err=0x%x, node=%d\n",
					infcCoreTime(), (int)cpyErr.cNum, cpyErr.errCode, (int)cpyErr.node);
				#endif
				// Save the error into queue that is extracted from VB app.
				// This will prevent a deadlock on error notification due to single
				// marshalling channel in ActiveX. Are we shutdown now?
				if (!pNCS->SelfDestruct) {
					// No we are not shutdown, post the error in the list
					postErrList(pNCS, &cpyErr);
					// Node rejection errors are non-serious,
					// we create a dump file to record issue.
					if (cpyErr.errCode >= MN_ERR_CMD_ERR_BASE
						&& cpyErr.errCode <= MN_ERR_CMD_ERR_END
						&& SysInventory[cNum].AutoDiscoveryEnable) {
						// Node reject class - just create a trace dump on our dime
						infcTraceDumpNext(cpyErr.cNum);
						// Allow another one here
						theNet.TraceArmed = TRUE;
						#if ERR_TRACE|TRACE_CALLBACK
						_RPT4(_CRT_WARN, "%.1f infcFireErrCallback(net rejected type): err=0x%x, node=%d, net=%d\n",
							infcCoreTime(), pErrInfo->errCode, (int)cpyErr.node, cpyErr.cNum);
						#endif
					}
					else {
						// More serious issue, start diagnostic and fire off recovery
						// efforts using offline threads.
						#if (TRACE_CALLBACK|ERR_TRACE) && defined(_DEBUG)
						_RPT4(_CRT_WARN, "%.1f infcFireErrCallback(other type): err=0x%x, node=%d, net=%d\n",
							infcCoreTime(), pErrInfo->errCode, (int)cpyErr.node, (int)cpyErr.cNum);
						#endif
						// Log lead-up if possible
						infcTraceDumpNext(cpyErr.cNum);
						// If we allow diagnostics and this is not a result of the diagnostics, run them
						// and start auto-discover to watch for network returning to life.
						if (IsNetBreakErr(cpyErr.errCode))
							theNet.NetDiagLastResult = cpyErr.errCode;
						// See if we need to go back online.
						attemptAutoDiscover(&cpyErr);
					}
				}
				// Only report external thread's problems and not
				// initialization errors.

				// Signal user of problem if they registered callback
				if (userErrCallbackFunc != NULL && !pNCS->SelfDestruct
					&& pNCS->pAutoDiscover->ThreadID() != infcThreadID()) {
					(*userErrCallbackFunc)(pErrInfo);
				}
			} // ONLINE detected error
			else {
				if (isRecovery
					&& (cpyErr.errCode >= MN_ERR_OFFLINE_00
						&& cpyErr.errCode <= MN_ERR_OFFLINE_15)) {
					postErrList(pNCS, &cpyErr);
					if (userErrCallbackFunc != NULL && !pNCS->SelfDestruct) {
						(*userErrCallbackFunc)(&cpyErr);
					}
				}
				}
			break;
			}
	}
	#if TRACE_CALLBACK&0
	_RPT0(_CRT_WARN, "infcFireErrCallback: complete\n");
	#endif
	pNCS->errorRecursePrevent--;
}
//																			 *
//****************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcFireNetEvent
//
//	DESCRIPTION:
//		Fire the mnClassEx network event if there is a current reference
//		to a mnClassEx object.
//
//	SYNOPSIS:
void infcFireNetEvent(netaddr cNum, enum NetworkChanges state)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	#if TRACE_CALLBACK//||1
	switch (state) {
	case NODES_OFFLINE:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) OFFLINE\n", infcCoreTime(), cNum);
		break;
	case NODES_ONLINE:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) ONLINE\n", infcCoreTime(), cNum);
		break;
	case NODES_DRIVER_ONLINE:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) DRIVER ONLINE\n", infcCoreTime(), cNum);
		break;
	case NODES_SENDING:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) SENDING\n", infcCoreTime(), cNum);
		break;
	case NODES_STOP_NET_CONFIG:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) STOP_NET_CONFIG\n", infcCoreTime(), cNum);
		break;
	case NODES_CHANGING_NET_CONTROLLER:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) CHANGING_NET_CONFIG\n", infcCoreTime(), cNum);
		break;
	case NODES_RESETTING:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) RESETTING\n", infcCoreTime(), cNum);
		break;
	case NODES_BAUD_UNSUPPORTED:
		_RPT2(_CRT_WARN, "%.1f infcFireNetEvent(%d) BAUD_UNSUPPORTED\n", infcCoreTime(), cNum);
		break;
	default:
		_RPT3(_CRT_WARN, "%.1f infcFireNetEvent(%d)=>%d\n", infcCoreTime(), cNum, state);
		break;
		}
	#endif
	// Prevent crash on dead net
	if (!pNCS)
		return;
	// Queue in the list if state is changing
	if (state != pNCS->LastEvent) {
		pNCS->NetChgListLock.Lock();
		unsigned nextTail;
		// Too many there now?
		if ((nextTail = ((pNCS->NetChgListTailPtr + 1) % NET_EVENTS_MAX))
			== pNCS->NetChgListHeadPtr) {
			// Eat oldest item off head
			pNCS->NetChgListHeadPtr++;
			pNCS->NetChgListHeadPtr %= NET_EVENTS_MAX;
		}
		// Add to tail
		pNCS->NetChgList[pNCS->NetChgListTailPtr] = state;
		pNCS->NetChgListTailPtr = nextTail;
		pNCS->NetChgListLock.Unlock();

		// Implement old online/offline model
		openStates currentState = SysInventory[cNum].OpenState;
		switch (state) {
		case NODES_OFFLINE:
			if (currentState == OPENED_ONLINE) {
				if (pNCS->pSerialPort->IsOpen())
					SysInventory[cNum].OpenStateNext(OPENED_SEARCHING);
				else
					SysInventory[cNum].OpenStateNext(CLOSED);
			}
			break;
		case NODES_DRIVER_ONLINE:
			if (currentState != OPENED_ONLINE) {
				SysInventory[cNum].OpenStateNext(OPENED_ONLINE);
			}
			// Insure auto-brake is reset to current node complement

			break;
		case NODES_FLASH_MODE:
			SysInventory[cNum].OpenStateNext(FLASHING);
			break;
			// Ignore all others
		default:
			break;
		}
	}
	// Signal external watcher our new state
	if (userNetStateFunc != NULL && pNCS->LastEvent != state) {
		(*userNetStateFunc)(cNum, SysInventory[cNum].OpenState == OPENED_ONLINE);
	}
	// Save our current state as the last state
	pNCS->LastEvent = state;
}
//																			   *
//******************************************************************************



//****************************************************************************
//	NAME
//		infcFireParamCallback
//
//	DESCRIPTION:
//		Fire the parameter change callback if it not part of the recovery
//		process.
//
//	SYNOPSIS:
void infcFireParamCallback(paramChangeFunc func, const mnParamChg *pChgItem)
{
	mnParamChg copyChg;
	static unsigned pChgCnt = 0;
	netStateInfo *pNCS = SysInventory[pChgItem->net].pNCS;
	if (!pNCS)
		return;
	// Create working copy on stack
	copyChg = *pChgItem;
	#if 0
	if (copyChg.paramNum == TG_P_POSN_MEAS && copyChg.node == 1) {
		_RPT1(_CRT_WARN, "Reading meas32 %.0f\n", copyChg.newValue);
	}
	#endif
	pChgCnt++;
	if (!pNCS->isRecoveryThread() && !pNCS->SelfDestruct) {
		if (func) {
			(*func)(&copyChg);
		}
		#ifdef USE_ACTIVEX
		// Is this using activex class style interface
		if (mnRef[copyChg.net] != NULL) {
			HRESULT hr;
			// Marshall mnClass
			double tStart = infcCoreTime();
			#if TRACE_CALLBACK&0
			//if (copyChg.node == 4) {
			_RPT4(_CRT_WARN, "%u Fire Param change n=%d p(%d)=%.2f\n",
				pChgCnt, copyChg.node, copyChg.paramNum, copyChg.newValue);
			//}
			#endif
			mn1::_NetAdapterObj *pMN6 = NULL;
			long inCurse = InterlockedIncrement(&ncs[copyChg.net].ChangeRecurse);
			// Marshall the net
			hr = m_pGIT->GetInterfaceFromGlobal(ncs[copyChg.net].mnCookie,
				__uuidof(mn1::_NetAdapterObj),
				(void **)&pMN6);
			if (hr == 0) {
				pMN6->FireParameterChanged((mn1::infcParamChg *)&copyChg);
				pMN6->Release();
			}
			inCurse = InterlockedDecrement(&ncs[copyChg.net].ChangeRecurse);
}
		#endif
	}
}
//																			 *
//****************************************************************************



//****************************************************************************
//	NAME
//		infcGetNextError
//
//	DESCRIPTION:
//		Get the next error from list if it exists.
//
//	RETURNS:
//		nodebool TRUE if there was a new item
//
//	SYNOPSIS:
MN_EXPORT nodebool MN_DECL infcGetNextError(
	netaddr cNum,
	infcErrInfo *pErrInfo)
{
	if (cNum > NET_CONTROLLER_MAX)
		return(FALSE);

	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Anything there?
	if (!pNCS || pNCS->ErrListHeadPtr == pNCS->ErrListTailPtr)
		return(FALSE);
	pNCS->ErrListLock.Lock();
	// Get copy of head and eat
	*pErrInfo = pNCS->ErrList[pNCS->ErrListHeadPtr++];
	// Insure head in bounds
	pNCS->ErrListHeadPtr %= ERR_CNT_MAX;
	pNCS->ErrListLock.Unlock();
	return(TRUE);
}
//																			 *
//****************************************************************************

//****************************************************************************
//	NAME
//		infcGetNetErrorStats
//
//	DESCRIPTION:
//		Grab all the net stats from the node.
//
//	RETURNS:
//		
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetNetErrorStats(
	multiaddr theMultiAddr,
	nodebool *pIsSet,
	mnNetDiagStats *pResult)
{
	Uint16 stat;
	cnErrCode theErr = MN_OK, finalErr = MN_OK;

	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr nodeAddr = NODE_ADDR(theMultiAddr);

	if (pIsSet)
		*pIsSet = FALSE;	//will get set to true if we make it through the whole function

	if (cNum > NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (!pNCS)
		return(MN_ERR_DEV_ADDR);

	switch (SysInventory[cNum].InventoryNow.LastIDs[nodeAddr].fld.devType) {
	case NODEID_MD:
	case NODEID_SC:
	case NODEID_CS:
		break;
	default:
		return(MN_ERR_UNKN_DEVICE);
	}

	if (!pResult)
		return(MN_ERR_BADARG);

	// Dump a trace file
	theErr = infcTraceDumpNext(cNum);
	if (theErr != MN_OK)
		finalErr = theErr;

	// Initialize all the stats
	pResult->AppNetBadChksumCtr = pResult->AppNetFragPktCtr = pResult->AppNetOverrunCtr = pResult->AppNetStrayCtr = -1;
	pResult->DiagNetBadChksumCtr = pResult->DiagNetFragPktCtr = pResult->DiagNetOverrunCtr = pResult->DiagNetStrayCtr = -1;
	pResult->NetVoltsLowCtr = -1;
	// Grab all the data from the node
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_APP_CHKSUM, &stat);
	if (theErr == MN_OK)
		pResult->AppNetBadChksumCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_APP_FRAG, &stat);
	if (theErr == MN_OK)
		pResult->AppNetFragPktCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_APP_OVERRUN, &stat);
	if (theErr == MN_OK)
		pResult->AppNetOverrunCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_APP_STRAY, &stat);
	if (theErr == MN_OK)
		pResult->AppNetStrayCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_DIAG_CHKSUM, &stat);
	if (theErr == MN_OK)
		pResult->DiagNetBadChksumCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_DIAG_FRAG, &stat);
	if (theErr == MN_OK)
		pResult->DiagNetFragPktCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_DIAG_OVERRUN, &stat);
	if (theErr == MN_OK)
		pResult->DiagNetOverrunCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_DIAG_STRAY, &stat);
	if (theErr == MN_OK)
		pResult->DiagNetStrayCtr = stat;
	else
		finalErr = theErr;
	theErr = coreBaseGetParameterInt(cNum, nodeAddr, MN_P_NETERR_LOW_VOLTS, &stat);
	if (theErr == MN_OK)
		pResult->NetVoltsLowCtr = stat;
	else
		finalErr = theErr;

	if (pIsSet)
		*pIsSet = TRUE;
	return(finalErr);
}
//																			 *
//****************************************************************************

//****************************************************************************
//	NAME
//		infcParamsHaveChanged
//
//	DESCRIPTION:
//		Have any params on the given node changed? If so, clear the change bit
//		and return that the node was changed
//
//	RETURNS:
//		nodebool TRUE if there was a change
//
//	SYNOPSIS:
MN_EXPORT nodebool MN_DECL infcParamsHaveChanged(
	multiaddr multiAddr)
{
	netaddr cNum = NET_NUM(multiAddr);
	nodeaddr nodeAddr = NODE_ADDR(multiAddr);

	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (cNum >= NET_CONTROLLER_MAX || !pNCS)
		return FALSE;

	if (pNCS->paramsHaveChanged[nodeAddr]) {
		pNCS->paramsHaveChanged[nodeAddr] = FALSE;
		return TRUE;
	}
	else
		return FALSE;

}
//																			 *
//****************************************************************************



//****************************************************************************
//	NAME
//		infcGetDiagBlock
//
//	DESCRIPTION:
//		Get the new diagnostic block state. When TRUE the network diagnostic
//		is prevented from running.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
cnErrCode MN_DECL infcGetDiagBlock(
	netaddr cNum,
	nodebool *state)
{
	*state = SysInventory[cNum].NetDiagBlock;
	return(MN_OK);
}
//																			 *
//****************************************************************************



//****************************************************************************
//	NAME
//		infcSetDiagBlock
//
//	DESCRIPTION:
//		Set the new diagnostic block state. When TRUE the network diagnostic
//		is prevented from running.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
cnErrCode MN_DECL infcSetDiagBlock(
	netaddr cNum,
	nodebool state)
{
	_RPT2(_CRT_WARN, "infcSetDiagBlock(%d) <= %d\n", cNum, state);
	SysInventory[cNum].NetDiagBlock = state;
	return(MN_OK);
}
//																			 *
//****************************************************************************

//****************************************************************************
//	NAME
//		infcGetOnlineState
//
//	DESCRIPTION:
//		Get the initializing flag.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetOnlineState(
	netaddr cNum,
	nodebool *state)
{
	if (cNum < NET_CONTROLLER_MAX) {
		*state = SysInventory[cNum].OpenState == OPENED_ONLINE;
		return(MN_OK);
	}
	else
		return(MN_ERR_DEV_ADDR);
}
//																			 *
//****************************************************************************

//****************************************************************************
//	NAME
//		infcGetOnlineStateEx
//
//	DESCRIPTION:
//		Get the initializing flag.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetOnlineStateEx(
	netaddr cNum,
	openStates *state)
{
	if (cNum < NET_CONTROLLER_MAX) {
		*state = SysInventory[cNum].OpenState;
		return(MN_OK);
	}
	else
		return(MN_ERR_DEV_ADDR);
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME
//		infcGetNextNetChange
//
//	DESCRIPTION:
//		Retrieve the oldest net change from the net change queue.
//
//	RETURNS:
//		TRUE if there is one
//
//	SYNOPSIS:
MN_EXPORT nodebool MN_DECL infcGetNextNetChange(
	netaddr cNum,
	NetworkChanges *pNetChange)
{
	if (cNum<SysPortCount) {
		netStateInfo *pNCS = SysInventory[cNum].pNCS;
		// Anything there?
		if (!pNCS || pNCS->NetChgListHeadPtr == pNCS->NetChgListTailPtr)
			return(FALSE);
		pNCS->NetChgListLock.Lock();
		// Get copy of head and eat
		*pNetChange = pNCS->NetChgList[pNCS->NetChgListHeadPtr++];
		// Insure head in bounds
		pNCS->NetChgListHeadPtr %= NET_EVENTS_MAX;
		pNCS->NetChgListLock.Unlock();
		return(TRUE);
	}
	return(FALSE);
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME
//		infcGetIOlock
//
//	DESCRIPTION:
//		Acquire the I/O critical section lock. Release with
//		infcReleaseIOlock()
//
//	RETURNS:
//		When Lock is acquired
//
//	SYNOPSIS:
void infcGetIOlock(netaddr cNum)
{
	if (cNum > SysPortCount)
		return;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (pNCS)
		pNCS->IOlock.Lock();
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME
//		infcReleaseIOlock
//
//	DESCRIPTION:
//		Release the I/O critical section lock acquired by infcGetIOlock()
//
//	SYNOPSIS:
void infcReleaseIOlock(netaddr cNum)
{
	if (cNum > SysPortCount)
		return;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (pNCS)
		pNCS->IOlock.Unlock();
}
//																			 *
//****************************************************************************


//****************************************************************************
//	NAME
//		infcProbeForOffline
//
//	DESCRIPTION:
//		Probe the last known network and fire errors off for all the nodes
//		found offline.
//
//	RETURNS:
//		#cnErrCode
//
//	SYNOPSIS:
cnErrCode infcProbeForOffline(netaddr cNum, nodeulong *pNodesNow)
{
	cnErrCode theErr;
	#if TRACE_LOW_PRINT
	_RPT1(_CRT_WARN, "infcProbeForOffline(%d) starting\n", cNum);
	#endif
	// Assume same node count as last at start
	*pNodesNow = SysInventory[cNum].InventoryLast.NumOfNodes;
	// See if we have basic connectivity
	#if TRACE_BREAK
	_RPT3(_CRT_WARN, "%.1f in infcProbeForOffline(%d) calling infcResetNetRate,"
		"thread ID=" THREAD_RADIX_LONG "\n",
		infcCoreTime(), cNum, infcThreadID());
	#endif
	//theErr = infcResetNetRate(cNum);
	//if (theErr == MN_OK) {
		// Yes, probe for crashed nodes by asking for their node
		// IDs. If node crashed it will echo this command and return
		// MN_ERR_OFFLINE.
		unsigned i;
		for (i = 0; i<SysInventory[cNum].InventoryLast.NumOfNodes; i++) {
			packetbuf theResp;
			theErr = netGetParameter(MULTI_ADDR(cNum, i),
				MN_P_NODEID,
				&theResp);
			// Find someone offline?
			//	if (theErr == MN_ERR_OFFLINE)  {
			//		netGetParameter will post errors as it goes
			//	}
		}
		// Restore online state
		*pNodesNow = 0;
		// Restore addressing for next probe
		theErr = netSetAddress(cNum, pNodesNow);
	//}
	//else {
	//	*pNodesNow = 0;
	//}
	#if TRACE_LOW_PRINT
	if (theErr != MN_OK) {
		_RPT2(_CRT_WARN, "infcProbeForOffline(%d) returns 0x%x\n", cNum, theErr);
	}
	#endif
	return(theErr);
}
//																			 *
//****************************************************************************




//*****************************************************************************
//	NAME																	  *
//		infcSetPortSpecifier
//
//	DESCRIPTION:
///		Store the physical port number for this controller in the by
///		controller data base.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
cnErrCode MN_DECL  infcSetPortSpecifier(
	netaddr cNum,					// Controller slot
	const portSpec *portSpec	// x of COMx, LPTx, etc
	)
{
	// Bound problem?
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	SysInventory[cNum].PhysPortSpecifier = *portSpec;
	return(MN_OK);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcGetPortSpecifier
//
//	DESCRIPTION:
///		Get the physical port number for this controller in the by
///		controller data base.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
cnErrCode MN_DECL  infcGetPortSpecifier(
	netaddr cNum,					// Controller slot
	portSpec *portSpec		// Controller specifier
	)
{
	// Bound problem?
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	*portSpec = SysInventory[cNum].PhysPortSpecifier;
	return(MN_OK);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcSetAutoNetDiscovery
//
//	DESCRIPTION:
///		Control the auto-recovery state setting.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcSetAutoNetDiscovery(
	netaddr cNum,
	nodebool newState)
{
	// Bound problem?
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	// Persist across restarts
	//_RPT3(_CRT_WARN, "%.1f infcSetAutoNetDiscovery(%d)=%d\n", 
	//	infcCoreTime(), cNum, newState);
	SysInventory[cNum].AutoDiscoveryEnable = newState;

	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (!pNCS)
		return(MN_ERR_OFFLINE);

	if (!newState)
		infcHaltAutoNetDiscovery(cNum);
	return(MN_OK);
}

MN_EXPORT cnErrCode MN_DECL infcGetAutoNetDiscovery(
	netaddr cNum,
	nodebool *currentState)
{
	// Bound problem?
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	*currentState = SysInventory[cNum].AutoDiscoveryEnable;
	return(MN_OK);
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		infcBackgroundPollControl
//
//	DESCRIPTION:
///		Control the operations of the background poller. This polling thread
///		keeps the watchdog timer from expiring when the driver is loaded.
///
///		This function returns when the thread has entered the desired state.
//
//	SYNOPSIS:
cnErrCode MN_DECL infcBackgroundPollControl(
	netaddr cNum,
	nodebool start)
{
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f infcBackgroundPollControl(%d): start=%d\n",
		infcCoreTime(), cNum, start);
	#endif
	// Bound problem?
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);

	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Persist the request
	SysInventory[cNum].KeepAlivePollEnable = start;
	// If we are currently online, use this as the restore state as well
	if (!SysInventory[cNum].Initializing)
		SysInventory[cNum].KeepAlivePollRestoreState = start;

	if (pNCS) {
		if (start) {
			pNCS->pPollerThread->Start();
		}
		else {
			pNCS->pPollerThread->Stop();
		}
	}
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f infcBackgroundPollControl(%d): exited\n",
		infcCoreTime(), cNum);
	#endif
	return(MN_OK);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcGet/SetOtherNetSnoop
//
//	DESCRIPTION:
///		Control and get the DTR pin state on the serial port. This is used to
///		snoop on traffic from the other net via our serial channel. This
///		feature will disrupt the other channel's data and must be used with
///		care to prevent system errors.
//
//	SYNOPSIS:
void infcSetOtherNetSnoop(netaddr cNum,
	nodebool TheDTRValue)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (pNCS->pSerialPort) {
		if (TheDTRValue)
			pNCS->pSerialPort->SetDTR(true);
		else
			pNCS->pSerialPort->SetDTR(false);
	}
}
void infcGetOtherNetSnoop(netaddr cNum,
	nodebool *pTheDTRValue)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (pTheDTRValue && pNCS->pSerialPort) {
		*pTheDTRValue = (pNCS->pSerialPort->GetDTR());
	}
}

//																			   *
//******************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcFlushDataAcq
//
//	DESCRIPTION:
///		Flush the currently stored data acquisition points.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcFlushDataAcq(multiaddr multiAddr)
{
	netaddr cNum = NET_NUM(multiAddr);
	nodeaddr respAddr = NODE_ADDR(multiAddr);
	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	//_RPT2(_CRT_WARN, "%.1f infcFlushDataAcq(%d)\n", infcCoreTime(), multiAddr);
	if (cNum > SysPortCount || pNCS == NULL)
		return(MN_ERR_BADARG);

	pNCS->DataAcq[respAddr].AcqLock.Lock();

	while (!(pNCS->DataAcq[respAddr].Points.empty())) {
		pNCS->DataAcq[respAddr].Points.pop();
	}

	pNCS->DataAcq[respAddr].Overflow = FALSE;
	pNCS->DataAcq[respAddr].SampleCount = 0;
	pNCS->DataAcq[respAddr].AcqLock.Unlock();

	return MN_OK;
}
//																			   *
//******************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcInitDataAcq
//
//	DESCRIPTION:
///		Record the sample rate for this node.
//
//	SYNOPSIS:
cnErrCode infcInitDataAcq(
	multiaddr multiAddr,
	double SampleRateMicroSec)
{
	netaddr cNum = NET_NUM(multiAddr);
	//_RPT2(_CRT_WARN, "%.1f infcInitDataAcq(%d)\n", infcCoreTime(), cNum);
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	pNCS->DataAcq[NODE_ADDR(multiAddr)].SampRateMilliSec
		= SampleRateMicroSec * 2 / 1000;
	// Restart the sequence number on next packet
	pNCS->DataAcqInit[NODE_ADDR(multiAddr)] = TRUE;
	return MN_OK;
}
//																			   *
//******************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcGetDataAcqPt
//
//	DESCRIPTION:
///		Get up to \e ptsToRead samples in the user supplied \e pTheDataAcqPt
///		buffer. The actual number of samples returned is stored in
///		\e pPtsRead.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetDataAcqPt(
	multiaddr multiAddr,
	nodeulong ptsToRead,
	mnDataAcqPt pTheDataAcqPt[],
	nodeulong *pPtsRead)
{
	cnErrCode errRet = MN_OK;
	netaddr cNum = NET_NUM(multiAddr);
	nodeaddr theAddr = NODE_ADDR(multiAddr);

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	// Hint this is heavily used value
	register netStateInfo *pNCS = SysInventory[cNum].pNCS;

	// Protect against bad arguments
	if (pPtsRead == NULL)
		return MN_ERR_BADARG;

	// Lock out the writer until we are done copying
	pNCS->DataAcq[theAddr].AcqLock.Lock();

	// If empty avoid dequeue error on read
	if (pNCS->DataAcq[theAddr].Points.empty()) {
		pNCS->DataAcq[theAddr].AcqLock.Unlock();
		return MN_ERR_DATAACQ_EMPTY;
	}

	*pPtsRead = 0;
	//_RPT3(_CRT_WARN, "%.1f infcGetDataAcqPt(%d) len=%d\n", infcCoreTime(), multiAddr, ptsToRead);
	// Copy up to the requester's defined buffer length
	while (!(pNCS->DataAcq[theAddr].Points.empty())
		&& *pPtsRead<ptsToRead) {
		pTheDataAcqPt[*pPtsRead] = pNCS->DataAcq[theAddr].Points.front();
		pNCS->DataAcq[theAddr].Points.pop();
		(*pPtsRead)++;
	}
	pNCS->DataAcq[theAddr].Overflow = FALSE;
	// Writer can now run
	pNCS->DataAcq[theAddr].AcqLock.Unlock();

	return errRet;
}
//																			   *
//******************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcGetDataAcqPt
//
//	DESCRIPTION:
///		Return the number of data acquisition points available for this node.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetDataAcqPtCount(
	multiaddr multiAddr,
	nodeulong *pPointCount)
{
	netaddr cNum = NET_NUM(multiAddr);
	netStateInfo *pNCS;
	// Bounds and arg check
	if ((multiAddr == MN_UNSET_ADDR) || cNum > SysPortCount
		|| (pNCS = SysInventory[cNum].pNCS, pNCS == NULL)
		|| pPointCount == NULL)
		return MN_ERR_BADARG;
	*pPointCount
		= (nodeulong)pNCS->DataAcq[NODE_ADDR(multiAddr)].Points.size();
	return MN_OK;
}
//																			   *
//******************************************************************************

//*****************************************************************************
//	NAME
//		infcSetAddressFmt
//
//	DESCRIPTION:
//		Create the re-address the network command using <startingAddr> as
//		the first node.
//
//		NOTE:	The network is addressed starting from <startingAddr>.  For normal
//				operation caller should initialize <startingAddr> with 0.
//
//	RETURNS:
//		MN_OK, else the error condition
//
//	SYNOPSIS:
cnErrCode MN_DECL infcSetAddressFmt(
	nodeulong startingAddr,		// Net address start here
	packetbuf *theCmd)
{
	theCmd->Fld.PktLen = 0;
	theCmd->Fld.Addr = startingAddr;
	theCmd->Fld.PktType = MN_PKT_TYPE_SET_ADDR;
	theCmd->Fld.Mode = theCmd->Fld.Zero1 = 0;	// Fill in unused with 0
	theCmd->Fld.Src = MN_SRC_HOST;
	theCmd->Byte.BufferSize = theCmd->Fld.PktLen + MN_API_PACKET_HDR_LEN;
	return(MN_OK);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcBreakChkFmt
//
//	DESCRIPTION:
//		Format the command buffer with the command that sends a packet
//		around the Meridian ring for a bring-net detection
//
//		NOTE: this command must be sent via the low-level transmission
//		function to preserve the cc_err field.
//
//	RETURNS:
//		standard error return
//
//	SYNOPSIS:
cnErrCode MN_DECL infcBreakChkFmt(
	packetbuf *theCmd)			// ptr to a command buffer
{
	theCmd->Fld.Src = MN_SRC_HOST;
	theCmd->Fld.PktType = MN_PKT_TYPE_EXTEND_HIGH;
	theCmd->Fld.Addr = 0;
	theCmd->Fld.PktLen = 1;
	theCmd->Fld.Zero1 = theCmd->Fld.Mode = 0;
	theCmd->Byte.Buffer[CMD_LOC] = MN_CTL_EXT_NET_CHECK;
	theCmd->Byte.BufferSize = theCmd->Fld.PktLen + MN_API_PACKET_HDR_LEN;
	return(MN_OK);

}
//																   		      *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcBrokenRingDiag
//
//	DESCRIPTION:
//		This diagnostic works for all newer nodes that supports the flow
//		control diagnostic control packet. This diagnostic will disrupt
//		traffic on the other network if ran.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
cnErrCode infcBrokenRingDiag(
	netaddr cNum,
	nodebool hasConnectivity,
	unsigned knownNodeCount)
{
	packetbuf theCmd, theResp;
	//nodebool ChkSuccessful[MN_API_MAX_NODES]={FALSE};
	nodebool CurNetOK = FALSE;
	nodebool CurNetGarbled = FALSE;
	int netBreakLastAddrOK = -1;
	unsigned AltNodeCounted;
	cnErrCode errRet = MN_OK;
	mnNetSrcs otherNet = MN_SRC_UNKNOWN_NET;
	mnNetInvRecords &theNet = SysInventory[cNum];
	netStateInfo *pNCS = theNet.pNCS;
	double startTimeMS, currentTimeMS;

	// --------- Check continuity on the current net ---------
	CurNetOK = FALSE;
	CurNetGarbled = FALSE;

	// Insure we leave with DTR clear
	infcSetOtherNetSnoop(cNum, FALSE);

	// Send probe command if we have connectivity. If not we will try
	// a blind send later of this command and look for results.
	if (hasConnectivity) {
		// Run the probe command
		infcBreakChkFmt(&theCmd);
		errRet = infcRunCommand(cNum, &theCmd, &theResp);
		// If we succeed, we are done. Care must be taken not disturb the
		// other running net.
		if ((CurNetOK = (errRet == MN_OK))) {
			// A successful read happened, make sure the response is correct
			// which is to say the same as what we sent.
			if (theResp.Fld.PktType == theCmd.Fld.PktType					// Right type
				&&  theResp.Fld.Src == theCmd.Fld.Src							// Node originated
				&&  theResp.Byte.Buffer[CMD_LOC] == theCmd.Byte.Buffer[CMD_LOC]	// Same cmd
				&& theResp.Byte.BufferSize == 3) {								// Size OK
																				// OK done.
				return(errRet);
			}
			// We got a response, but it was garbage
			CurNetGarbled = TRUE;
		}
	}
	// We either have no net or got garbage back if <CurNetGarbled> is TRUE.

	// --------- END - Check continuity on the current net ---------

	// Stop the read thread to allow special command cases
	pNCS->ReadThread.Stop();
	pNCS->pSerialPort->AutoFlush(false);
	// Make sure we are in packet mode.
	pNCS->pSerialPort->SetMode(SERIALMODE_MERIDIAN_7BIT_PACKET);

	// Count the number of nodes seen on the alternate net
	// by switching the net adapter to monitor the other ring and wait
	// for responses to appear.

	// Set alternate-net node counted to 0. This variable will be increased as nodes are found
	AltNodeCounted = 0;

	// Insure we have some addresses up to the break.
	infcSetAddressFmt(0, &theCmd);
	// Blind send an addressing packet to give nodes up to the break
	// an address.
	errRet = infcSendCommandLog(cNum, &theCmd);
	if (errRet != MN_OK) {
		goto coreBreakDiagnostic_Exit;
	}

	// Enable the DTR line to get data from other ring to our serial port
	infcSetOtherNetSnoop(cNum, TRUE);

	// Send network break check packet to force traffic onto other net
	infcBreakChkFmt(&theCmd);
	errRet = infcSendCommandLog(cNum, &theCmd);
	if (errRet != MN_OK) {
		// Put serial port back into pass-through
		infcSetOtherNetSnoop(cNum, FALSE);
		goto coreBreakDiagnostic_Exit;
	}

	// If the other net is online, activity on that net can be seen and
	// keep the while loop going - use a timeout to prevent getting stuck
	currentTimeMS = startTimeMS = infcCoreTime();

	// Poll the other network via DTR switch to detect which nodes
	// heard the packet.
	// Add a timeout because the other net could be sending commands
	while (((errRet = infcGetResponse(cNum, &theResp)) == MN_OK) &&
		(currentTimeMS - startTimeMS < 100)) {
		theNet.logReceive(&theResp, errRet, NULL, infcCoreTime());

		// A successful read happened, make sure the response is correct
		if (theResp.Fld.PktType == MN_PKT_TYPE_EXTEND_HIGH		// Right type
			&&  theResp.Fld.Src == MN_SRC_NODE						// Node originated
			&&  theResp.Byte.Buffer[CMD_LOC] == MN_CTL_EXT_NET_CHECK	// Same cmd
			&&  theResp.Byte.BufferSize == 3) {								// Size OK
																			// The other net is set in the mode field
			otherNet = (mnNetSrcs)theResp.Fld.Mode;
			// Update entry in the array
			//ChkSuccessful[theResp.Fld.Addr]=TRUE;
			// Record the highest responding address
			if (netBreakLastAddrOK < (int)theResp.Fld.Addr)
				netBreakLastAddrOK = theResp.Fld.Addr;
			AltNodeCounted++;
		}		// 	if ( correct packet type )

		currentTimeMS = infcCoreTime();
		theNet.logReceive(&theResp, errRet, NULL, infcCoreTime());
	}	// while(infcGetResponse(cNum, &theResp)==MN_OK) {

		// Put serial port back into pass-through
	infcSetOtherNetSnoop(cNum, FALSE);

	// --------- END - Count the number of nodes seen on the alternate net ---------

	// ------ Interpret the values:
	//	CurNetOK - whether the current net is fully connected
	//	AltNodeCounted - How many nodes were counted on the alternate ring
	//	CurNetGarbled - whether there is a garbled response from the current net
	if (CurNetGarbled) {
		errRet = MN_ERR_BRK_LOC_UNKNOWN;
		goto coreBreakDiagnostic_Exit;
	}

	// If there are no nodes found, it could either by alternate ring broken,
	// or the RX of the current ring is broken
	if (netBreakLastAddrOK < 0) {
		switch (pNCS->OurNetConnector) {
		case MN_SRC_APP_NET:
			errRet = MN_ERR_BRK_APP_LOC_UNKNOWN;
			break;
		case MN_SRC_DIAG_NET:
			errRet = MN_ERR_BRK_DIAG_LOC_UNKNOWN;
			break;
		default:
			errRet = MN_ERR_BRK_LOC_UNKNOWN;
			break;
		}
	}
	else {
		// If we had no known nodes, see if alternate net detection
		// got us some information.
		if (knownNodeCount == 0) {
			// We are starting from unknown start due to break somewhere.
			if (AltNodeCounted > 0) {
				// We did get hits from other net via DTR switch, therefore
				// we know the net we are connected. Break is after
				// the highest reported node on the other net. There
				// maybe other nodes past this point, but we can't tell.
				if (otherNet == MN_SRC_APP_NET)
					// Problem somewhere after netBreakLastAddrOK
					errRet = (cnErrCode)(MN_ERR_BRK_DIAG_00_HOST
						+ (cnErrCode)(netBreakLastAddrOK));
				else
					errRet = (cnErrCode)(MN_ERR_BRK_APP_00_HOST
						+ (cnErrCode)(netBreakLastAddrOK));
			}
			else
				// We go no responses, therefore we know nothing new other
				// than its broken,
				errRet = MN_ERR_BRK_LOC_UNKNOWN;
		}
		else {
			// There were nodes at one time, therefore we know location more
			// accurately
			if (netBreakLastAddrOK + 1 >= (int)knownNodeCount) {
				switch (pNCS->OurNetConnector) {
				case MN_SRC_APP_NET:
					errRet = (cnErrCode)(MN_ERR_BRK_APP_00_HOST
						+ (cnErrCode)(netBreakLastAddrOK));
					break;
				case MN_SRC_DIAG_NET:
					errRet = (cnErrCode)(MN_ERR_BRK_DIAG_00_HOST
						+ (cnErrCode)(netBreakLastAddrOK));
					break;
				default:
					errRet = (cnErrCode)(MN_ERR_BRK_00_HOST
						+ (cnErrCode)(netBreakLastAddrOK));
					break;
				}
			}
			else {
				switch (pNCS->OurNetConnector) {
				case MN_SRC_APP_NET:
					errRet = (cnErrCode)(MN_ERR_BRK_APP_00_01
						+ (cnErrCode)(netBreakLastAddrOK));
					break;
				case MN_SRC_DIAG_NET:
					errRet = (cnErrCode)(MN_ERR_BRK_DIAG_00_01
						+ (cnErrCode)(netBreakLastAddrOK));
					break;
				default:
					errRet = (cnErrCode)(MN_ERR_BRK_OTHER_00_01
						+ (cnErrCode)(netBreakLastAddrOK));
					break;
				}
			}
		}
	}
	// ------ END - Interpret the values ---------------------


coreBreakDiagnostic_Exit:
	// Restore read thread
	pNCS->ReadThread.Start();
	return errRet;
}
//																   		      *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcGetNetSource
//
//	DESCRIPTION:
///		Tell drive what net the serial port is attached to.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT mnNetSrcs MN_DECL infcGetNetSource(netaddr cNum)
{
	// Bounds check
	if (cNum>NET_CONTROLLER_MAX)
		return(MN_SRC_UNKNOWN_NET);

	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (!pNCS)
		return(MN_SRC_UNKNOWN_NET);

	return(pNCS->OurNetConnector);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcSetNetSource
//
//	DESCRIPTION:
///		Tell drive what net the serial port is attached to.
///
/// 	\param xxx description
///		\return description
///
/// 	Detailed description.
//
//	SYNOPSIS:
void infcSetNetSource(netaddr cNum, mnNetSrcs theNet)
{
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	pNCS->OurNetConnector = theNet;
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		infcTraceDumpNext
//
//	DESCRIPTION:
//		If trace dump is armed, and we have not made too many of them since
//		the session start, create dump directory in the user's temporary area
//		and then write the next file in sequence there.
//
//	RETURNS:
//		cnErrCode
//
//	SYNOPSIS:
cnErrCode infcTraceDumpNext(netaddr cNum)
{
	char fPath[MAX_PATH], winDir[MAX_PATH];
	cnErrCode theErr = MN_OK;
	mnNetInvRecords &theNet = SysInventory[cNum];
	netStateInfo *pNCS = theNet.pNCS;

	// If pNCS is NULL, exit
	if (!pNCS) {
		_RPT1(_CRT_WARN, "infcTraceDumpNext %d skipped, pNCS = NULL.\n", cNum);
		return(MN_OK);
	}

	// Only perform if armed
	if (!theNet.TraceArmed) {
		//_RPT1(_CRT_WARN, "infcTraceDumpNext %d skipped, not armed.\n", cNum);
		return(MN_OK);
	}
	// Reset dumps until online again
	theNet.TraceArmed = FALSE;
	// Get our temporary location
	infcGetDumpDir(winDir, MAX_PATH);
	// Setup the next number
	InfcLastDumpNumber++;
	InfcLastDumpNumber %= DUMP_WRAP_NUM;				// Wrap to ten files
	theNet.TraceCount++;
	// Insure directory is created, we ignore errors the dump will create
	// an appropriate error.
	#if (defined(_WIN32)||defined(_WIN64))
	mkdir(winDir);
	#else
	mkdir(winDir, 0755);
	#endif
	// Create full name
	sprintf((char*)fPath, "%sMNuserDriver%d-%d.dat", winDir, cNum,
		InfcLastDumpNumber);
	if (theNet.TraceCount < DUMP_WRAP_NUM) {
		//_RPT1(_CRT_WARN, "infcTraceDumpNext dumping %s\n", fPath);
		theErr = infcTraceDumpA(cNum, (nodeANSIstr)fPath);
	}
	//else {
	//	_RPT1(_CRT_WARN, "infcTraceDumpNext skipped, too many @ cnt %d\n",
	//					  theNet.TraceCount);
	//}
	return(theErr);
	}
//																			  *
//*****************************************************************************



//****************************************************************************
//	NAME																	 *
//		infcCmdCompleteCallback
//
//	DESCRIPTION:
/**
This function registers a callback function that is called after the a
node responds to any commands from the host computer.

The callback function template is declared as:
\code
infcCmdCompleteCallback priorHandler;
void MN_DECL myCmdCompletionHandler(mnCompletionInfo *pCmdStats) {
// Signal your application the stats for this command
...
// If prior handler exists, call it.
if (priorHandler)
(*priorHandler)(pCmdStats);
}

...

// Register a callback
priorHandler = infcCmdCompleteCallback(myCmdCompletionHandler);
\endcode

\param[in] newFunc Pointer to the call back function
\return The pointer of the previous callback function that was assigned for this
callback

\see \ref mnCompletionInfo
**/
//	RETURNS:
//		infcAboutRunCallback
//
//	SYNOPSIS:
MN_EXPORT infcCmdCompleteCallback MN_DECL infcSetCmdCompleteFunc(
	infcCmdCompleteCallback newFunc)
{
	infcCmdCompleteCallback oldFunc = userCmdCompleteFunc;
	userCmdCompleteFunc = newFunc;
	return(oldFunc);
}
//																			 *
//****************************************************************************



//****************************************************************************
//	NAME																	 *
//		infcSetInvalCacheFunc
//
//	DESCRIPTION:
//		This function registers a callback function that is called after
//		a node indicates that a param has been changed and the param cache
//		should be invalidated. The callback function template is declared as:
//			void (*funcPtr)(netaddr net, nodeaddr node)
//
//	RETURNS:
//		infcInvalCacheCallback
//
//	SYNOPSIS:
MN_EXPORT infcInvalCacheCallback MN_DECL infcSetInvalCacheFunc(
	infcInvalCacheCallback newFunc)
{
	infcInvalCacheCallback oldFunc = userInvalCacheFunc;
	userInvalCacheFunc = newFunc;
	return(oldFunc);
}


// End of INTERNAL_DOC
/// \endcond




//==============================================================================
// DOCUMENTED API FUNCTIONS GO BELOW THIS LINE
//==============================================================================
/**
\addtogroup LinkGrp
@{
**/
/// \cond ISC_DOC


//******************************************************************************
//	NAME																	   *
//		infcGetBackgroundErrs
//
//	DESCRIPTION:
/**
Get network error statistics detected by the nodes.
The statistics are then cleared as part of the function call.

\param[in] theMultiAddr The address code for this node.
\param[out] pIsSet Ptr boolean result set non-zero if there was a
a statistic item present.
\param[out] pResult Ptr to statistic that was collected.
\return #cnErrCode; MN_OK if \p pResult was updated and \p pIsSet is non-zero
if there was a stored statistic.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetBackgroundErrs(
	multiaddr theMultiAddr,
	nodebool *pIsSet,
	mnNetDiagStats *pResult)
{
	netaddr cNum = NET_NUM(theMultiAddr);
	// Did they spec a location?
	if (!pResult)
		return(MN_ERR_BADARG);
	if (!pIsSet)
		return(MN_ERR_BADARG);
	*pIsSet = false;
	// Is the requested node in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);
	netaddr nodeAddr = NODE_ADDR(theMultiAddr);
	if (nodeAddr >= SysInventory[cNum].InventoryNow.NumOfNodes)
		return(MN_ERR_PARAM_RANGE);

	// Get our net's state
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Anything there?
	if (pNCS && SysInventory[cNum].diagsAvailable[nodeAddr]) {
		*pResult = SysInventory[cNum].diagStats[nodeAddr];
		*pIsSet = VB_TRUE;
		SysInventory[cNum].diagsAvailable[nodeAddr] = FALSE;
		SysInventory[cNum].diagStats[nodeAddr].Clear();
	}
	else {
		*pIsSet = VB_FALSE;
		pResult->Clear();
	}
	return MN_OK;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcGetHostErrStats
//
//	DESCRIPTION:
/**
Get network error statistics detected by the host computer.
The statistics are then cleared as part of the function call.

\param[in] cNum Channel number. The first channel is zero.
\param[out] pIsSet Ptr boolean result set non-zero if there was a
statistic item present.
\param[out] pResult Ptr to statistic that was collected.
\return #cnErrCode; MN_OK if \p pResult was updated and \p pIsSet is non-zero
if there was a stored statistic.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetHostErrStats(
	netaddr cNum,
	nodebool *pIsSet,
	mnNetDiagStats *pResult)
{
	// Is the requested node in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return(MN_ERR_DEV_ADDR);
	// Did they spec a location?
	if (!pResult)
		return(MN_ERR_BADARG);
	if (!pIsSet)
		return(MN_ERR_BADARG);

	// Anything there?
	*pResult = SysInventory[cNum].diagStats[MN_API_MAX_NODES];
	*pIsSet = VB_TRUE;
	SysInventory[cNum].diagsAvailable[MN_API_MAX_NODES] = FALSE;
	SysInventory[cNum].diagStats[MN_API_MAX_NODES].Clear();
	return MN_OK;
}
//																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
//		infcTraceDumpA
//
//	DESCRIPTION:
/**
Dump and reset the trace log with a ANSI string interface.

\param[in] cNum Channel number to for node. The first channel is zero.
\param[in] filePath The ANSI string of the dump file to create from this
channel.
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcTraceDumpA(
	netaddr cNum,
	const char *filePath)
{
	wchar_t uniStr[MAX_PATH];
	mbstowcs(uniStr, filePath, MAX_PATH);
	return(infcTraceDump(cNum, uniStr));
}
// 																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcTraceDump
//
//	DESCRIPTION:
/**
Dump and reset the trace log using the UNICODE interface.

\param cNum Channel number. The first channel is zero.
\param pFilePath A null terminated UNICODE string that represents the
file to store into. The directory path must exist prior to calling this
function.

\return #cnErrCode; MN_OK if dump was succesfully created
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcTraceDump(
	netaddr cNum,
	const wchar_t *pFilePath)
{
	std::ofstream outStream;
	Uint32 len;
	txTraceBuf zero;
	traceHeader theHeader;
	cnErrCode theErr = MN_OK;
	mnNetInvRecords &theNet = SysInventory[cNum];
	netStateInfo *pNCS = theNet.pNCS;

	// Skip if we are gone!
	if (!pNCS)
		return(MN_ERR_CLOSED);
	// Create the file
	#if defined(__linux__)||defined(__GNUC__)
	char utf8Path[MAX_PATH];
	wcstombs(utf8Path, pFilePath, MAX_PATH);
	outStream.open(utf8Path, ios::out | ios::binary);
	#else
	outStream.open(pFilePath, std::ios::out | std::ios::binary);
	#endif
	if (!outStream.is_open()) {
		return(MN_ERR_OS);
	}

	// Send the header size
	len = sizeof(theHeader);
	if (!outStream.write((char *)&len, sizeof(len))) {
		return(MN_ERR_OS);
	}
	// Lock out updates
	theNet.RXlogLock.Lock();
	theNet.TXlogLock.Lock();
	// Send the header
	theHeader.type = DUMP_TYPE_NUM;
	//02-06-2012 DS Not sure why this is off by one?
	theHeader.numOfNodes = SysInventory[cNum].InventoryLast.NumOfNodes - 1;
	theHeader.nextRXrecord = (Uint32)theNet.rxTraceIndx;
	theHeader.nextTXrecord = (Uint32)theNet.txTraceIndx;
	theHeader.version = infcVersion();
	theHeader.kernVersion = 0;
	theHeader.traceLen = SEND_DEPTH;
	theHeader.hdrLen = sizeof(theHeader);
	infcFileNameA(theHeader.filename, TRACE_FILEPATH_LEN);
	#if (defined(_WIN32)||defined(_WIN64))
	_ftime(&theHeader.timestamp);
	#else
	// Convert to 64 bit time buffer
	struct timeb lclTime;
	ftime(&lclTime);
	theHeader.timestamp.time = lclTime.time;
	theHeader.timestamp.millitm = lclTime.millitm;
	theHeader.timestamp.timezone = lclTime.timezone;
	theHeader.timestamp.dstflag = lclTime.dstflag;
	#endif
	if (theNet.respSerNum < SEND_DEPTH) {
		len = theNet.respSerNum;
	}
	else {
		len = SEND_DEPTH;
	}
	theHeader.nRXrecords = len;
	if (theNet.sendSerNum < SEND_DEPTH) {
		len = theNet.sendSerNum;
	}
	else {
		len = SEND_DEPTH;
	}
	theHeader.nTXrecords = len;
	theHeader.diags = NetDiags[cNum];
	//mnNetInvRecords *pInv = &SysInventory[cNum].InventoryLast;
	for (len = 0; len<MN_API_MAX_NODES; len++) {
		theHeader.nodeTypes[len] = SysInventory[cNum].InventoryNow.LastIDs[len];
	}
	if (!outStream.write((char *)&theHeader.type,
		sizeof(theHeader) - sizeof(len))) {
		theErr = MN_ERR_OS;
	}
	// Store the receive information
	if (theErr == MN_OK && !outStream.write((char*)&theNet.rxTraces[0],
		sizeof(rxTraceBuf)*theHeader.nRXrecords)) {
		theErr = MN_ERR_OS;
	}

	// Store the transmit information
	if (theErr == MN_OK && !outStream.write((char*)&theNet.txTraces[0],
		sizeof(txTraceBuf)*theHeader.nTXrecords)) {
		theErr = MN_ERR_OS;
	}
	// Release the lock on the trace updates
	theNet.RXlogLock.Unlock();
	theNet.TXlogLock.Unlock();
	// Got the file open, write out the buffer
	outStream.close();
	return(theErr);
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		infcNetAttnFlush
//
//	DESCRIPTION:
/**
Flush all detected attentions.

\param[in] cNum Channel number. The first channel is zero.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have cleared all past attentions.
- MN_ERR_DEV_ADDR cNum out of range
*/
MN_EXPORT cnErrCode MN_DECL infcNetAttnFlush(
	netaddr cNum)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// No port no storage, we're done.
	if (!pNCS)
		return MN_OK;

	pNCS->AttnLock.Lock();
	// No overruns now
	pNCS->AttnOverrun = false;
	// Zap items
	while (!pNCS->AttnBuf.empty()) {
		pNCS->AttnBuf.pop();
	}
	pNCS->AttnLock.Unlock();
	return (MN_OK);
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcNetGetAttnReq
//
//	DESCRIPTION:
/**
Wait for a new attention to arrive or retrieve the oldest attention
request from the specified network.

Attentions are high priority network packets that implement
an interrupt-like event on detected node events. This feature
reduces the need for host polling on certain events.

Typically this function is called from a loop within a dedicated
"interrupt" thread. When it returns MN_OK the attention is inspected
and the user application will change state or notify some other part of
its code that this event occured.

Before attentions can be received, the desired nodes must unmask status
bits on which notification is required.

\param[in] cNum Channel number. The first channel is zero.
\param[out] pRequest Ptr to result when function returns MN_OK.

\return #cnErrCode; Typical Return Values
- MN_OK A valid attention update exists in the \a pRequest buffer.
- MN_ERR_ATTN_OVERRUN There has been a loss of attentions due to
insufficient service rate or flood generated by a node.	No value is
returned.
- MN_ERR_TIMEOUT There have not been any attentions in the last second. No
value is returned.
.

\see \ref mnAttnsPage
**/
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcNetGetAttnReq(
	netaddr cNum,
	mnAttnReqReg *pRequest)
{
	BOOL waitOK;
	ULONG waitPeriod = 1000;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	if (!pNCS)
		return MN_ERR_CLOSED;

	// Signal a loss occurred
	if (pNCS->AttnOverrun) {
		pNCS->AttnOverrun = FALSE;
		return MN_ERR_ATTN_OVERRUN;
	}

	// If we have one, just return it, otherwise wait
	if (pNCS->AttnBuf.empty()) {
		// Wait for the interrupt to occur or the timeout
		waitOK = pNCS->IrqEvent.WaitFor(waitPeriod);
	}
	else {
		waitOK = TRUE;
	}
	// If this did not time-out get and reset the event
	if (waitOK && !pNCS->AttnBuf.empty()) {
		// Get flags and reset the interrupt event
		#if TRACE_ATTN
		_RPT1(_CRT_WARN, "%.1f ntInfc.WaitForIrq\n", infcCoreTime());
		#endif
		pNCS->AttnLock.Lock();
		// get the attn from the queue
		*pRequest = pNCS->AttnBuf.front();
		// remove the item
		pNCS->AttnBuf.pop();
		// reset the event if there are no more to get
		if (pNCS->AttnBuf.empty())
			pNCS->IrqEvent.ResetEvent();

		pNCS->AttnLock.Unlock();
		return MN_OK;
	}
	return(MN_ERR_TIMEOUT);
	}
//																			  *
//*****************************************************************************



/****************************************************************************/
//						 CALLBACK SETUP INTERFACES
/****************************************************************************/

//*****************************************************************************
//	NAME
//		infcNetStateFunc
//
//	DESCRIPTION:
/**
This function registers a callback function that is called when network
changes state from online to offline or vice versa.

\note There should be no attempt to call any of the S-Foundation library
APIs as deadlock could occur.

An example of a state change function and setup is below:
\code
infcNetStateCallback priorHandler;
void MN_DECL myNetStateHandler(nodeshort cNum, nodebool online) {
// Signal your application something happened
...
// If prior handler exists, call it.
if (priorHandler)
(*priorHandler)(cNum, online);
}

...

// Register a callback
priorHandler = infcNetStateCallback(myNetStateHandler);
\endcode

\param[in] newFunc Pointer to the call back function
\return The pointer of the previous callback function that was
assigned for this callback.	This allows for chained handlers.
**/
//
//	RETURNS:
//		infcNetStateCallback
//
//	SYNOPSIS:
MN_EXPORT infcNetStateCallback MN_DECL infcSetNetStateFunc(
	infcNetStateCallback newFunc)
{
	infcNetStateCallback oldFunc = userNetStateFunc;
	userNetStateFunc = newFunc;
	return(oldFunc);
}
//																			 *
//****************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcSetErrorFunc
//
//	DESCRIPTION:
/**
This function registers a callback function that is called when network
communication errors occurs. When this function is called it
is provided with the network address of the error as well as
an #cnErrCode error number.

\note The function is not called from the main thread and should
not initiate any new network traffic or calls to the
S-Foundation APIs. Deadlock could occur if this is done.

The sample callback function template could be declared as:
\code
infcErrCallback priorHandler;
void MN_DECL myNetworkErrorHandler(infcErrInfo *infcErrInfo) {
// Signal your application something happened
...
// If prior handler exists, call it.
if (priorHandler)
(*priorHandler)(infcErrInfo);
}

...

// Register a callback
priorHandler = infcSetErrorFunc(myNetworkErrorHandler);
\endcode

\param[in] newFunc Pointer to the call back function
\return The pointer of the previous callback function that was
assigned for this callback.

\see \ref infcErrInfo
**/
//
//	RETURNS:
//		Old handler
//
//	SYNOPSIS:
MN_EXPORT infcErrCallback MN_DECL infcSetErrorFunc(
	infcErrCallback newFunc)
{
	infcErrCallback oldFunc;
	oldFunc = userErrCallbackFunc;
	userErrCallbackFunc = newFunc;
	return (oldFunc);
}
/// \endcond																  *
//*****************************************************************************


//******************************************************************************
//******************************************************************************
//						netPollerThread IMPLEMENTATION
//******************************************************************************
///	\cond INTERNAL_DOC

//*****************************************************************************
//	NAME																	  *
//		netPollerThread::netPollerThread construction and destruction
//
//	DESCRIPTION:
/**


\param[in,out] xxx description
\return description

**/
//	SYNOPSIS:
netPollerThread::netPollerThread(netStateInfo *pTheNetInfo)
{
	#if TRACE_THREAD || TRACE_DESTRUCT || TRACE_POLL_THRD
	_RPT1(_CRT_WARN, "%.1f netPollerThread constructing\n",
		infcCoreTime());
	#endif
	#if (defined(_WIN32)||defined(_WIN64))
	SetDLLterm(true);
	#endif
	// Insure we halt at HALTED state
	m_InternalSyncAck.ResetEvent();
	m_halted = true;
	pNCS = pTheNetInfo;
	pNCS->GroupShutdownEvent.ResetEvent();
}

netPollerThread::~netPollerThread() {
	#if TRACE_THREAD || TRACE_DESTRUCT || TRACE_POLL_THRD
	double sTime = infcCoreTime();
	#endif
	// Insure we exit
	Terminate();
	WaitForTerm();
	#if TRACE_THREAD || TRACE_DESTRUCT || TRACE_POLL_THRD
	_RPT2(_CRT_WARN, "%.1f netPollerThread destructed, elapsed=%.1f\n",
		infcCoreTime(), infcCoreTime() - sTime);
	#endif
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		netPollerThread::Run
//
//	DESCRIPTION:
/**
This thread polls the first one on this network to maintain the watchdog
timer. It is halted in offline and in serial port mode.

\param[in,out] xxx description
\return description

**/
//	SYNOPSIS:
int netPollerThread::Run(void *context)
{
	// Create the address for node 0 on our net
	int cntr = 0;

	infcErrInfo errInfo;					// Error reporting buffer
	cnErrCode theErr;
	packetbuf outPkt;
	packetbuf inPkt;
	// Set instance net information
	pNCS = (netStateInfo *)context;
	netaddr cNum = pNCS->cNum;
	multiaddr theNodeAddr = MULTI_ADDR(cNum, 0);
	mnNetInvRecords &theNet = SysInventory[pNCS->cNum];

	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f netPollerThread(%d): id=" THREAD_RADIX " starting\n",
		infcCoreTime(), cNum, UIthreadID());
	#endif
	// Prevent initialize race
	m_critSection.Lock();
	m_halted = true;
	// Ack we have halted
	m_InternalSyncAck.SetEvent();
	// Allow start
	m_critSection.Unlock();
	// Build the out packet
	outPkt.Fld.SetupHdr(MN_PKT_TYPE_EXTEND_LOW, theNodeAddr);
	outPkt.Fld.PktLen = 2;
	outPkt.Byte.Buffer[CMD_LOC] = MN_CTL_EXT_HOST_ALIVE;
	outPkt.Byte.Buffer[CMD_LOC + 1] = cntr;
	outPkt.Byte.BufferSize = outPkt.Fld.PktLen + MN_API_PACKET_HDR_LEN;
	// Loop to read the device ID of the first node.
	while (!Terminating()) {
		// Wait for something to wake us up
		m_RunControl.WaitFor();
		// Wait for start to begin our polling
		#if TRACE_POLL_THRD||TRACE_THREAD
		if (m_halted) {
			_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): Halting!\n",
				infcCoreTime(), cNum);
		}
		#endif
		// Do they want our thread to exit?
		if (Terminating()) {
			#if TRACE_POLL_THRD||TRACE_THREAD
			_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): Terminating!\n",
				infcCoreTime(), cNum);
			#endif
			break;
		}
		// Start to halt
		if (!m_halted) {
			// Driver state allows operations?
			if (SysInventory[cNum].OpenState == OPENED_ONLINE) {
				// Issue node stops if we had detected the need or externally
				// issued.
				if (theNet.GroupShutdownRequest) {
					_RPT1(_CRT_WARN, "%.1f Group Shutdown Request\n", infcCoreTime());
					// Is there group node stop request?
					// Process the ClearPath SC nodes
					for (size_t i = 0; i < theNet.InventoryNow.mnCpScInfo.count; i++) {
						multiaddr theAddr = theNet.InventoryNow.mnCpScInfo.node[i];
						ShutdownInfo &sInfo = theNet.GroupShutdownInfo[NODE_ADDR(theAddr)];
						if (sInfo.enabled) {
							_RPT3(_CRT_WARN, "%.1f Group Shutdown(%d) type %0x\n",
								infcCoreTime(), theAddr,
								sInfo.theStopType.bits);
							theErr = netNodeStop(theAddr, sInfo.theStopType, false);
						}
					}
					// If other node types implement the GroupShutdown feature, process them here

					// Wait for next request
					theNet.GroupShutdownRequest = false;
				}
				else {
					// Runnning. Send low-level get to insure no caching
					theErr = infcRunCommand(cNum, &outPkt, &inPkt);

					// Check that the contents of the return packet match the contents of the sent packet
					bool pktMismatch = outPkt.Byte.BufferSize != inPkt.Byte.BufferSize;
					for (nodeulong iByte = 0; iByte < outPkt.Byte.BufferSize && !pktMismatch; iByte++) {
						pktMismatch = pktMismatch || outPkt.Byte.Buffer[iByte] != inPkt.Byte.Buffer[iByte];
					}
					// If the command failed, it is handled elsewhere
					pktMismatch = pktMismatch && theErr == MN_OK;

					cntr++;
					outPkt.Byte.Buffer[CMD_LOC + 1] = cntr & 0xff;
					#if TRACE_POLL_THRD||TRACE_THREAD
					if ((cntr % 10) == 0) {
						_RPT0(_CRT_WARN, "Polled 10 more\n");
					}
					#endif
					// Ack we have started
					m_InternalSyncAck.SetEvent();
					if (theErr != MN_OK || pktMismatch) {
						#if TRACE_POLL_THRD||TRACE_THREAD
						_RPT3(_CRT_WARN, "%.1f netPollerThread(%d): probe cmd err=0x%x\n",
							infcCoreTime(), cNum, theErr);
						#endif
						if (pktMismatch) {
							// Fire off the error callback
							errInfo.cNum = cNum;
							errInfo.node = theNodeAddr;
							errInfo.errCode = MN_ERR_CMD_OFFLINE;
							infcFireErrCallback(&errInfo);
							_RPT3(_CRT_WARN, "%.1f netPollerThread::Run(%d): response err 0x%x\n",
								infcCoreTime(), cNum, errInfo.errCode);
							// Create dump file on this error
							infcTraceDumpNext(cNum);
						}
						// Stop this until recovery occurs
						m_critSection.Lock();
						m_halted = true;
						// Don't allow loop to run again.
						m_RunControl.ResetEvent();
						m_critSection.Unlock();
						#if TRACE_POLL_THRD||TRACE_THREAD
						_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): Error Halted!\n",
							infcCoreTime(), cNum);
						#endif
					}
					#if 0
					else {
						// Probe was OK, perform additional polling
						_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): probe OK\n",
							infcCoreTime(), cNum);
					}
					#endif				
				} // else not group shutdown
			}
			else {
				// Say we started now, even offline
				m_InternalSyncAck.SetEvent();
			}
			// Wait until we try again
			//CThread::Sleep(pNCS->pollDelayTimeMS);
			pNCS->GroupShutdownEvent.WaitFor(pNCS->pollDelayTimeMS);
			pNCS->GroupShutdownEvent.ResetEvent();
			#if TRACE_POLL_THRD||TRACE_THREAD
			_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): Poll delay complete\n",
				infcCoreTime(), cNum);
			#endif
		} // if (!m_halted)
		// If halted, insure we stop
		m_critSection.Lock();
		if (m_halted && !Terminating()) {
			m_RunControl.ResetEvent();
			m_InternalSyncAck.SetEvent();
		}
		m_critSection.Unlock();
		if (Terminating())
			break;
	}  // while(!Terminating())
	// We are forced halted.
	m_critSection.Lock();
	m_halted = true;
	m_InternalSyncAck.SetEvent();
	m_critSection.Unlock();
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f netPollerThread(%d), ID=" THREAD_RADIX
		" exit via loop termination!\n",
		infcCoreTime(), cNum, UIthreadID());
	#endif
	return(777);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netPollerThread::Start
//
//	DESCRIPTION:
/**
Start the polling thread and return when running.
**/
//	SYNOPSIS:
void netPollerThread::Start()
{
	cnErrCode errRet;
	packetbuf fwVers;
	#define FWVER ((versID_t *)(&fwVers.Byte.Buffer[0]))
	#define CP_P_FW_VERS 1
	#define ISC_P_FW_VERSION 1
	#define CP_MILESTONE_POLLER 0x1109
	#define ISC_MILESTONE_POLLER 0x5600

	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f netPollerThread(%d): starting poll...,term=%d\n",
		infcCoreTime(), pNCS->cNum, Terminating());
	#endif
	// Lock down the read state
	m_critSection.Lock();
	// Already running?
	if (!m_halted || Terminating()) {
		#if TRACE_POLL_THRD||TRACE_THREAD
		_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): already running\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		m_critSection.Unlock();
		return;
	}
	// Make sure Node 0 supports the ping 
	bool pollerSupported = false;
	errRet = MN_OK;		// Assume good, until a real failure occurs
	if (SysInventory[pNCS->cNum].NodeInfo[0].theID.fld.devType == NODEID_CP) {
		errRet = netGetParameter(MULTI_ADDR(pNCS->cNum, 0), CP_P_FW_VERS, &fwVers);
		if (errRet == MN_OK && FWVER->verCode >= CP_MILESTONE_POLLER) {
			pollerSupported = true;
		}
	}
	if (errRet == MN_OK && SysInventory[pNCS->cNum].NodeInfo[0].theID.fld.devType == NODEID_MD) {
		errRet = netGetParameter(MULTI_ADDR(pNCS->cNum, 0), ISC_P_FW_VERSION, &fwVers);
		if (errRet == MN_OK && FWVER->verCode >= ISC_MILESTONE_POLLER) {
			pollerSupported = true;
		}
	}
	if (errRet == MN_OK && SysInventory[pNCS->cNum].NodeInfo[0].theID.fld.devType == NODEID_CS) {
		errRet = netGetParameter(MULTI_ADDR(pNCS->cNum, 0), MN_P_FW_VERSION, &fwVers);
		if (errRet == MN_OK) {
			pollerSupported = true;
		}
	}
	// Not supported or lost contact?
	if (!pollerSupported || errRet != MN_OK) {
		#if TRACE_POLL_THRD||TRACE_THREAD
		_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): not supported\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		m_critSection.Unlock();
		return;
	}

	// Disarm ACK
	m_InternalSyncAck.ResetEvent();
	// Set state before restarting thread
	m_halted = false;
	m_RunControl.SetEvent();
	m_critSection.Unlock();
	m_InternalSyncAck.WaitFor();
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): poll running\n",
		infcCoreTime(), pNCS->cNum);
	#endif
	}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		netPollerThread::Stop
//
//	DESCRIPTION:
/**
Stop the polling thread and return when stopped.
**/
//	SYNOPSIS:
void netPollerThread::Stop()
{
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT3(_CRT_WARN, "%.1f netPollerThread(%d): stopping poll...,term=%d\n",
		infcCoreTime(), pNCS->cNum, Terminating());
	#endif
	// Lock down the read state
	m_critSection.Lock();
	// Already stopped?
	if (m_halted) {
		#if TRACE_POLL_THRD||TRACE_THREAD
		_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): already stopped\n",
			infcCoreTime(), pNCS->cNum);
		#endif
		// We are halted, leave state alone
		m_critSection.Unlock();
		return;
	}
	// We are running, get things to stop
	m_halted = true;
	m_RunControl.SetEvent();
	if (!Terminating())
		m_InternalSyncAck.ResetEvent();
	else
		m_InternalSyncAck.SetEvent();

	// Allow thread to run
	m_critSection.Unlock();
	BOOL waitOK = m_InternalSyncAck.WaitFor();
	if (!waitOK) {
		_RPT2(_CRT_WARN, "%.1f netPollerThread(%d) wait failed\n",
			infcCoreTime(), pNCS->cNum);
	}
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): poll stopped\n",
		infcCoreTime(), pNCS->cNum);
	#endif
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		netPollerThread::Terminate
//
//	DESCRIPTION:
/**
Insure the thread exits in a timely manner.

\param[in,out] xxx description
\return description

**/
//	SYNOPSIS:
void *netPollerThread::Terminate()
{
	#if TRACE_POLL_THRD||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f netPollerThread(%d): terminate set\n",
		infcCoreTime(), pNCS->cNum);
	#endif
	*m_pTermFlag = true;
	m_RunControl.SetEvent();
	pNCS->GroupShutdownEvent.SetEvent();

	// Set the terminate flag
	void *rVal = CThread::Terminate();
	return rVal;
}
//																			  *
//*****************************************************************************
/// \endcond 

/// \cond CPM_CLIB

//******************************************************************************
//	NAME																	   *
//		infcCommUnitPowered
//
//	DESCRIPTION:
/**
Query that Comm Unit is powered up.

\param[in] cNum Channel number. The first channel is zero.
\param[in] brakeNum Brake number. The first brake is zero.
\param[in] engaged Operational state of the brake.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have set the brake override.
- MN_ERR_DEV_ADDR cNum out of range
- MN_ERR_PORT_PROBLEM port not available
- MN_ERR_BADARG Target is not a Comm Unit
*/
MN_EXPORT cnErrCode MN_DECL infcCommUnitPowered(
	netaddr cNum,
	nodebool *pIsPowered)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	// This feature requires ClearPath-SC network
	if (SysInventory[cNum].PhysPortSpecifier.PortType != CPM_COMHUB)
		return MN_ERR_BADARG;
	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (!pNCS || !pNCS->pSerialPort)
		return MN_ERR_PORT_PROBLEM;
	// Does the hub board have power?
	*pIsPowered = pNCS->pSerialPort->GetDSR();
	return(MN_OK);
}
//																			  *
//*****************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcBrakeSet
//
//	DESCRIPTION:
/**
Set the specified brake override bit.

\param[in] cNum Channel number. The first channel is zero.
\param[in] brakeNum Brake number. The first brake is zero.
\param[in] engaged Operational state of the brake.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have set the brake override.
- MN_ERR_DEV_ADDR cNum out of range
- MN_ERR_PORT_PROBLEM port not available or no power
- MN_ERR_PARAM_RANGE brakeNum invalid
- MN_ERR_BADARG Target is not a ClearPath-SC system
*/
cnErrCode infcBrakeSet(
	netaddr cNum,
	size_t brakeNum,
	nodebool engaged,
	nodebool forced)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	if (brakeNum >= mnNetInvRecords::MAX_BRAKES)
		return MN_ERR_PARAM_RANGE;
	// This feature requires ClearPath-SC network
	if (SysInventory[cNum].PhysPortSpecifier.PortType != CPM_COMHUB && !forced)
		return MN_ERR_BADARG;

	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (!pNCS || !pNCS->pSerialPort)
		return MN_ERR_PORT_PROBLEM;

	#if TRACE_BRAKE
	_RPT3(_CRT_WARN, "%.1f brake %d, engaged=%d\n", infcCoreTime(), brakeNum, engaged);
	#endif
	switch (brakeNum) {
	case 0:
		pNCS->pSerialPort->SetDTR(!engaged);
		break;
	case 1:
		pNCS->pSerialPort->SetRTS(!engaged);
		break;
	}
	// Does the hub board have power? We setup lines, but say we failed
	// to notify the actuator is not running properly.
	if (!pNCS->pSerialPort->GetDSR() && !forced) {
		#if TRACE_BRAKE
		_RPT2(_CRT_WARN, "%.1f brake %d, power off fail.\n", infcCoreTime(), brakeNum);
		#endif
		return MN_ERR_24V_OFF;
	}

	return MN_OK;

}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		infcBrakeGet
//
//	DESCRIPTION:
/**
Get the specified brake bit.

\param[in] cNum Channel number. The first channel is zero.
\param[in] brakeNum Brake number. The first brake is zero.
\param[out] pBrakeVal Operational state of the brake.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have returned the brake override.
- MN_ERR_DEV_ADDR cNum out of range
- MN_ERR_PORT_PROBLEM port not available or no power
- MN_ERR_PARAM_RANGE brakeNum invalid
- MN_ERR_BADARG pBrakeVal invalid or target network is not
a ClearPath-SC
*/
cnErrCode infcBrakeGet(
	netaddr cNum,
	size_t brakeNum,
	nodebool *pEngaged,
	nodebool forced)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	if (brakeNum >= mnNetInvRecords::MAX_BRAKES)
		return MN_ERR_PARAM_RANGE;
	// This feature requires ClearPath-SC network
	if (SysInventory[cNum].PhysPortSpecifier.PortType != CPM_COMHUB && !forced)
		return MN_ERR_BADARG;

	netStateInfo *pNCS = SysInventory[cNum].pNCS;

	if (!pNCS || !pNCS->pSerialPort)
		return MN_ERR_PORT_PROBLEM;

	// Does the hub board have power?
	if (!pNCS->pSerialPort->GetDSR() && !forced)
		return MN_ERR_24V_OFF;

	if (!pEngaged)
		return MN_ERR_BADARG;

	switch (brakeNum) {
	case 0:
		*pEngaged = !pNCS->pSerialPort->GetDTR();
		break;
	case 1:
		*pEngaged = !pNCS->pSerialPort->GetRTS();
		break;
	}

	return MN_OK;

}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcBrakeControlSet
//
//	DESCRIPTION:
/**
Set the specified brake control mode.

\param[in] cNum Channel number. The first channel is zero.
\param[in] brakeNum Brake number. The first brake is zero.
\param[in] brakeVal Operational state of the brake.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have set the brake override.
- MN_ERR_DEV_ADDR cNum out of range
- MN_ERR_PORT_PROBLEM port not available
- MN_ERR_PARAM_RANGE brakeNum or brakeVal invalid
*/
MN_EXPORT cnErrCode MN_DECL infcBrakeControlSet(
	netaddr cNum,
	size_t brakeNum,
	BrakeControls brakeVal)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	if (brakeNum >= mnNetInvRecords::MAX_BRAKES)
		return MN_ERR_BADARG;
	// This feature requires ClearPath-SC network
	if (SysInventory[cNum].PhysPortSpecifier.PortType != CPM_COMHUB)
		return MN_ERR_BADARG;

	cnErrCode attnOffErr = MN_OK;
	// If exiting autobrake mode, turn off the autobrake attentions
	if (SysInventory[cNum].autoBrake[brakeNum].brakeMode == BRAKE_AUTOCONTROL
		&& brakeVal != BRAKE_AUTOCONTROL
		&& SysInventory[cNum].autoBrake[brakeNum].enabled) {
		// Save but ignore the error for now
		attnOffErr = infcAutoBrakeSet(cNum, brakeNum, false,
			SysInventory[cNum].autoBrake[brakeNum].relatedNode);
	}
	SysInventory[cNum].autoBrake[brakeNum].brakeMode = brakeVal;
	nodebool engaged;

	switch (brakeVal) {
	case BRAKE_PREVENT_MOTION:
	case GPO_OFF:
		engaged = true;
		break;
	case BRAKE_ALLOW_MOTION:
	case GPO_ON:
		engaged = false;
		break;
	case BRAKE_AUTOCONTROL:
		// Configure the autobrake
		return infcAutoBrakeSet(cNum, brakeNum, true,
			SysInventory[cNum].autoBrake[brakeNum].relatedNode);
	default:
		return MN_ERR_BADARG;
	}
	// Make sure the brake is set to final state if possible.
	cnErrCode setErr = infcBrakeSet(cNum, brakeNum, engaged);
	// Return error that we didn't clean-up OK.
	if (attnOffErr != MN_OK) return attnOffErr;
	return setErr;
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		infcBrakeControlGet
//
//	DESCRIPTION:
/**
Get the specified brake control configuration. This feature requires a
ClearPath-SC system.

\param[in] cNum Channel number. The first channel is zero.
\param[in] brakeNum Brake number. The first brake is zero.
\param[out] pBrakeVal Operational state of the brake.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have returned the brake override.
- MN_ERR_DEV_ADDR cNum out of range
- MN_ERR_BADARG brakeNum or pBrakeVal invalid
*/
MN_EXPORT cnErrCode MN_DECL infcBrakeControlGet(
	netaddr cNum,
	size_t brakeNum,
	BrakeControls *pBrakeVal)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	if (brakeNum >= mnNetInvRecords::MAX_BRAKES || !pBrakeVal)
		return MN_ERR_BADARG;
	// This feature requires ClearPath-SC network
	if (SysInventory[cNum].PhysPortSpecifier.PortType != CPM_COMHUB)
		return MN_ERR_BADARG;

	*pBrakeVal = SysInventory[cNum].autoBrake[brakeNum].brakeMode;

	return MN_OK;

}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcAutoBrakeSet
//
//	DESCRIPTION:
/**
Link the disable state of a node with a given brake.

\param[in] cNum Channel number. The first channel is zero.
\param[in] brakeNum Brake number. The first brake is zero.
\param[in] brakeInfo The node to associate and if the feature is active.

\return #cnErrCode; Various Possible Return Values
- MN_OK We have set the auto brake.
- MN_ERR_DEV_ADDR cNum out of range
- MN_ERR_PARAM_RANGE brakeNum or pBrakeInfo invalid
*/
MN_EXPORT cnErrCode MN_DECL infcAutoBrakeSet(
	netaddr cNum,
	size_t brakeNum,
	nodebool enabled,
	multiaddr theMultiAddr)
{
	// Insure boolean is converted to C-type
	enabled = enabled ? TRUE : FALSE;
	cnErrCode theErr = MN_OK;
	// The address was not set
	if (theMultiAddr == MN_UNSET_ADDR)
		return MN_ERR_BADARG;
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	if (brakeNum >= mnNetInvRecords::MAX_BRAKES)
		return MN_ERR_PARAM_RANGE;

	if (enabled && cNum != NET_NUM(theMultiAddr))
		return MN_ERR_BADARG;

	// This feature requires ClearPath-SC network
	if (SysInventory[cNum].PhysPortSpecifier.PortType != CPM_COMHUB)
		return MN_ERR_BADARG;
	// This feature only works on APP channel
	if (SysInventory[cNum].AccessInfo[0].Fld.NetSource != MN_SRC_APP_NET)
		return MN_ERR_APP_NET_ONLY;


	#if TRACE_BRAKE
	_RPT4(_CRT_WARN, "infcAutoBrakeSet: cNum=%d, brake=%d, enable=%d, addr=%d\n",
		cNum, brakeNum, enabled, theMultiAddr);
	#endif
	// Prevent premature setup from past disabled states
	SysInventory[cNum].attnInitializing++;
	nodeaddr theAddr = NODE_ADDR(theMultiAddr);
	attnReg &newSettings = SysInventory[cNum].drvrAttnMask[theAddr];
	newSettings.cpm.GoingDisabled = enabled;
	newSettings.cpm.Disabled = enabled;
	newSettings.cpm.Enabled = enabled;
	theErr = netSetDrvrAttnMask(theMultiAddr, &newSettings);
	if (theErr != MN_OK) {
		SysInventory[cNum].attnInitializing--;
		return(theErr);
	}
	// See if we need to assert brake now
	if (enabled) {
		mnStatusReg statusNow;
		theErr = netGetStatusRTReg(theMultiAddr, &statusNow);
		if (theErr != MN_OK) {
			SysInventory[cNum].attnInitializing--;
			return(theErr);
		}
		theErr = infcBrakeSet(cNum, brakeNum,
			!statusNow.Fld.LowAttn.DrvFld.Enabled);

	}
	else {
		// Leave the brake control in the current state to avoid flickering
		//theErr = infcBrakeSet(cNum, brakeNum, false);
	}
	// Wait for transmission of attentions to propagate. Ignore errors
	// here, the 24V supply maybe not ready yet so we assume all OK.
	infcSleep(50);
	SysInventory[cNum].autoBrake[brakeNum].enabled = enabled;
	SysInventory[cNum].autoBrake[brakeNum].relatedNode = theMultiAddr;
	SysInventory[cNum].attnInitializing--;
#if TRACE_BRAKE
	if (theErr)
		_RPT2(_CRT_WARN, "%.1f infcAutoBrakeSet return=0x%x.\n",
				infcCoreTime(), theErr);
#endif
	return theErr;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcAutoBrakeGet
//
//	DESCRIPTION:
/**
	Link the disable state of a node with a given brake.

	\param[in] cNum Channel number. The first channel is zero. This is the index
	into the original port selections for \ref mnInitializeSystem.
	\param[in] brakeNum Brake number. The first brake is zero.
	\param[out] pTheMultiAddr The node watched when the feature is active.

	\return #cnErrCode; Various Possible Return Values
	- MN_OK We have returned the brake override.
	- MN_ERR_DEV_ADDR cNum out of range
	- MN_ERR_PARAM_RANGE brakeNum invalid
	- MN_ERR_BADARG pEnabled or pTheMultiAddr invalid
*/
MN_EXPORT cnErrCode MN_DECL infcAutoBrakeGet(
			netaddr cNum,
			size_t brakeNum,
			nodebool *pEnabled,
			multiaddr *pTheMultiAddr)
{
	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	if (brakeNum >= mnNetInvRecords::MAX_BRAKES)
		return MN_ERR_PARAM_RANGE;
	if (!pEnabled || !pTheMultiAddr)
		return MN_ERR_BADARG;

	*pEnabled = SysInventory[cNum].autoBrake[brakeNum].enabled;
	*pTheMultiAddr = SysInventory[cNum].autoBrake[brakeNum].relatedNode;
	return MN_OK;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcShutdownInfoSet
//
//	DESCRIPTION:
/**
	Register the node at \a theMultiAddr to cause a group shutdown if the 
	Status Event condition in the \a pTheInfo condition is detected and
	the feature is enabled.

	\param[in] theMultiAddr The node to include or exclude from the group
			   shutdown feature.
	\param[in] pTheInfo Pointer to #ShutdownInfo buffer that defines how
			   this node is part of the group shutdown feature. This includes
			   the inclusion in the group state and what condition on this
			   node is considered a shutdown condition.

	\return #cnErrCode; Various Possible Return Values
	- MN_OK The feature is has been setup properly.
	- MN_ERR_DEV_ADDR theMultiAddr is illegal or out of range
*/
MN_EXPORT cnErrCode MN_DECL infcShutdownInfoSet(
			multiaddr theMultiAddr,
			const ShutdownInfo *pTheInfo)
{
	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theAddr = NODE_ADDR(theMultiAddr);
	cnErrCode theErr= MN_OK;

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	if (theAddr > MN_API_ADDR_MASK)
		return MN_ERR_DEV_ADDR;

	mnNetInvRecords &inv = SysInventory[cNum];
	ShutdownInfo &info = inv.GroupShutdownInfo[theAddr];

	// Clear GroupShutdownRequest to allow any failure to reissue the nodestop
	inv.GroupShutdownRequest = false;

	// Adjust the polling list as necessary, watch out for VB 
	// booleans for compares.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4800)
#endif
	// Disable attns to eat historic event that maybe pending
	inv.attnInitializing++;
	// Set up the status event mask
	theErr = netSetStatusEventMask(theMultiAddr, &pTheInfo->statusMask);
	if (theErr == MN_OK) {
		inv.drvrAttnMask[theAddr].cpm.StatusEvent = pTheInfo->enabled;
		theErr = netSetDrvrAttnMask(theMultiAddr, &inv.drvrAttnMask[theAddr]);
		// Copy in the new shutdown info and enable if required
		if (theErr == MN_OK) {
			info = *pTheInfo;
		}
	}
	// Wait for attentions to transit net
	infcSleep(100);
	// Allow attention processing
	inv.attnInitializing--;
	return theErr;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcShutdownInfoGet
//
//	DESCRIPTION:
/**
	Get the current state of the Group Shutdown feature at this node.

	\param[in] theMultiAddr The node to include or exclude from the group
			   shutdown feature.
	\param[in] pTheInfo Pointer to #ShutdownInfo buffer that is updated with
			   this node's current Group Shutdown State.

	\return #cnErrCode; Various Possible Return Values
	- MN_OK The feature is has been setup properly.
	- MN_ERR_DEV_ADDR theMultiAddr is illegal or out of range
*/
MN_EXPORT cnErrCode MN_DECL infcShutdownInfoGet(
			multiaddr theMultiAddr,
			ShutdownInfo *pTheInfo)
{
	netaddr cNum = NET_NUM(theMultiAddr);
	nodeaddr theAddr = NODE_ADDR(theMultiAddr);

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;

	if (theAddr > MN_API_ADDR_MASK)
		return MN_ERR_DEV_ADDR;

	mnNetInvRecords &inv = SysInventory[cNum];
	*pTheInfo = inv.GroupShutdownInfo[theAddr];
	return MN_OK;
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		infcShutdownInitiate
//
//	DESCRIPTION:
/**
	Trigger a Group Shutdown to occur via API call on the given channel.

	\param[in] cNum Channel number. The first channel is zero.

	\return #cnErrCode; Various Possible Return Values
	- MN_OK The shutdown event was triggered.
	- MN_ERR_DEV_ADDR cNum is illegal or out of range
*/
MN_EXPORT cnErrCode MN_DECL infcShutdownInitiate(
			netaddr cNum)
{

	// Is the device in our range?
	if (cNum >= NET_CONTROLLER_MAX)
		return MN_ERR_DEV_ADDR;
	
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	
	if (!pNCS)
		return MN_ERR_DEV_ADDR;

	SysInventory[cNum].GroupShutdownRequest = true;
	pNCS->GroupShutdownEvent.SetEvent();

	return MN_OK;
}
// cond CPM_CLIB
/// \endcond																   *
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::mnNetInvRecords constructor
/// \cond INTERNAL_DOC
//
//	DESCRIPTION:
/**
	Construct the network inventory records
*/
mnNetInvRecords::mnNetInvRecords() {
	//_RPT1(_CRT_WARN, "%.1f creating mnNetInvRecords(SysInventory)\n", infcCoreTime());
	for (size_t i=0; i<MN_API_MAX_NODES; i++) {
		pNodes[i] = (sFnd::INode *)NULL;
		AccessInfo[i].bits = 0;
		attnCleanupMask[i] = 0xFFFFFF;
	}
	pNCS = NULL;
	pPortCls = (sFnd::IPort *)NULL;
	brake0Saved = brake1Saved = true;
	// Initialize with serial port closed
	clearNodes(false);
	Initializing = 0;
	NumCmdsInRing = N_CMDS_IN_RING;
	AutoDiscoveryEnable = true;
	KeepAlivePollEnable = true;
	KeepAlivePollRestoreState = true;
	GroupShutdownRequest = false;
	attnInitializing = 0;
	OpenState = UNKNOWN;

	NetDiagLastResult = MN_OK;
	TraceArmed = TRUE;						// Create dump on next error
	NetDiagBlock = FALSE;
	OfflineRootErr = MN_OK;
	OfflineErrNode = MN_UNSET_ADDR;
	TraceActive = TRUE;
	TraceActiveReq = TRUE;
	TraceCount = 0;							// No traces yet
	sendSerNum = respSerNum = 0;			// Start counting from 0
	txTraceIndx = rxTraceIndx = 0;
	txTraces = NULL;
	rxTraces = NULL;
}
//																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::mnNetInvRecords destructor
//
//	DESCRIPTION:
/**
	Construct the network inventory records
*/
mnNetInvRecords::~mnNetInvRecords() {
	//_RPT1(_CRT_WARN, "%.1f deleting mnNetInvRecords\n", infcCoreTime());
	if (txTraces) delete[] txTraces;
	if (rxTraces) delete[] rxTraces;
	txTraces = NULL;
	rxTraces = NULL;
	clearNodes(true);
	delete pPortCls;
	pPortCls = NULL;
	delete pNCS;
	pNCS = NULL;
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::setupBrakeState
//
//	DESCRIPTION:
/**
Set brake outputs based on net type and node states.
*/
void mnNetInvRecords::setupBrakes()
{
	// Return if we don't have brake or can't control one.
	if (!pNCS || !pNCS->pSerialPort) return;
	if (!pNCS->pSerialPort->IsOpen()) return;
	if (AccessInfo[0].Fld.NetSource != MN_SRC_APP_NET) return;
	for (size_t i = 0; i < MAX_BRAKES; i++) {
		// Run ignoring success/failure
		infcAutoBrakeSet(pNCS->cNum, i, autoBrake[i].enabled, autoBrake[i].relatedNode);
	}
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::OpenStateNext
//
//	DESCRIPTION:
/**
	Change the ValidState and record prior state.
*/
void mnNetInvRecords::OpenStateNext(openStates next)
{
#if TRACE_CALLBACK
	_RPT2(_CRT_WARN, "%.1f mnNetInvRecords.OpenStateNext=%d\n", infcCoreTime(), next);
#endif
	//if (next == ONLINE)
	//	_RPT0(_CRT_WARN, "ding\n");
	if (next != OpenState) {
		OpenStateLast = OpenState;
		OpenState = next;
	}
}
//																			   *
//******************************************************************************


//******************************************************************************
//	NAME																	   *
//		mnNetInvRecords::clearNodes
//
//	DESCRIPTION:
/**
	Clear node information from this channel.
*/
void mnNetInvRecords::clearNodes(bool portIsClosed) 
{
	InventoryNow.NumOfNodes = 0;
	InventoryNow.mnDrvInfo.count = 0;
	InventoryNow.mnClearPathInfo.count = 0;
	InventoryNow.mnCpScInfo.count = 0;
	for (size_t i=0; i<MN_API_MAX_NODES; i++) {
		delete pNodes[i];
		pNodes[i] = (sFnd::INode *)NULL;
	}
	if (portIsClosed) {
		OpenStateNext(CLOSED);
	}
	else {
		OpenStateNext(OPENED_SEARCHING);
	}
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		_mnPowerReg::_mnPowerReg
//
//	AUTHOR:
//		Dave Sewhuk - 12/06/2016 17:24
//
//	DESCRIPTION:
/**
	Power Status register for ClearPath/Meridian nodes.
*/
_mnPowerReg::_mnPowerReg(const _mnPowerReg &src)
{
	bits = src.bits;
}

char * _mnPowerReg::StateStr(char *buffer, size_t size)
{
	size_t nowIndx, lastIndx, accumIndex;
	nowIndx = fld.AcPhaseActive >= PWR_CNT ? PWR_CNT-1 : fld.AcPhaseActive;
	lastIndx = fld.AcPhaseActiveLast >= PWR_CNT ? PWR_CNT - 1 : fld.AcPhaseActiveLast;
	accumIndex = fld.AcPhaseActiveAccum >= PWR_CNT ? PWR_CNT - 1 : fld.AcPhaseActiveAccum;

	// Format up a nicely formatted string
	snprintf(buffer, size, "AC Phases Now: %s%s, Last AC Phases: %s%s, AC Phases since reset: %s\n"
						   "Bus loss: %s; Phase loss: %s\n"
						   "Under Operating Voltage: %s; logic supply detected: %s\n",
		pwrTypes[nowIndx], fld.AcPhaseDir ? "(reverse)" : "", pwrTypes[lastIndx], fld.AcPhaseDirLast ? "(reverse)" : "", 
		pwrTypes[accumIndex], fld.InBusLoss ? "yes" : "no", fld.InWiringError ? "yes" : "no",
		fld.InUnderOperV ? "yes" : "no", fld.LogicPwrSupply ? "yes" : "no");
	// Insure buffer is terminated
	buffer[size - 1] = 0;
	return (buffer);
}
// cond INTERNAL_DOC
/// \endcond																   *
//******************************************************************************

/**
	@}
**/
// End of ChanGrp
//==============================================================================
//	END OF FILE lnkAccessCommon.cpp
//==============================================================================


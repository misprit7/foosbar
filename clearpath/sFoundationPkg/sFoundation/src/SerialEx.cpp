//*****************************************************************************
// $Workfile: SerialEx.cpp $
// $Archive: /ClearPath SC-1.0.123/User Driver/src/SerialEx.cpp $
// $Date: 01/19/2017 17:39 $
//
//	SerialEx.cpp - Implementation of the CSerialEx class for QNX Real Time OS
//
//	Copyright (C) 1999-2003 Ramon de Klein (Ramon.de.Klein@ict.nl)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


//////////////////////////////////////////////////////////////////////
// Include the standard header files

#define STRICT


	#include "pubMnNetDef.h"
	#include "lnkAccessCommon.h"
	#if (defined(_WIN32)||defined(_WIN64))
		#include <crtdbg.h>
	#endif


//////////////////////////////////////////////////////////////////////
// Include module headerfile

#include "SerialEx.h"

//////////////////////////////////////////////////////////////////////
// Disable warning C4127: conditional expression is constant, which
// is generated when using the _RPTF and _ASSERTE macros.
#if (defined(_MSC_VER))
#pragma warning(disable: 4127)
#endif
#if defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#ifdef _DEBUG
#define T_ON TRUE
#else
#define T_ON FALSE
#endif
#define T_OFF FALSE

#if !defined(TRACE_THREAD)
#define TRACE_THREAD		T_OFF	// Print thread create/exit IDs
#endif
#define TRACE_HIGH_LEVEL	T_OFF
#define TRACE_LOW_LEVEL		T_OFF
#define TRACE_ERR			T_OFF
#define TRACE_READS			T_OFF
#define TRACE_STRAY			T_OFF
#define TRACE_DESTRUCT		T_OFF	// Print construction/destruction

// Read request time-out. All reads will be protected by this time-out.
// The time-out is a balance of timing between excessive read thread processing
// and the ability to close down the port at the end of port use. This time
// will most likely be added to the destruction time of this object.
#define READ_TIME_OUT_MS  200

// Print ID in platform normal way
#ifdef __unix
#define THREAD_RADIX "%d"
#else
#define THREAD_RADIX "0x%x"
#endif

//*****************************************************************************
//	NAME																	  *
//		DUMP_PKT
//
//	DESCRIPTION:							
//		Write buffer as hex debug string.
//
//	SYNOPSIS:
void DUMP_PKT(const char *msg, packetbuf *buf) {
	unsigned i, bLen;
	//DisplayLock.Lock();
	_RPT1(_CRT_WARN,"%s", msg);
	if (buf->Byte.BufferSize > 20)
		bLen = 20;
	else
		bLen = buf->Byte.BufferSize;
	for (i=0; i<bLen; i++) {
		_RPT1(_CRT_WARN,"%02x ", 0xff&buf->Byte.Buffer[i]);
	}
	_RPT0(_CRT_WARN,"\n");
	//DisplayLock.Unlock();
}
//																			 *
//****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CSerialEx Construction/Destruction 
//
//	DESCRIPTION:							
//		Write buffer as hex debug string.
//
//	SYNOPSIS:
CSerialEx::CSerialEx()
{
	#if TRACE_THREAD || TRACE_DESTRUCT
		_RPT0(_CRT_WARN, "CSerialEx constructing\n");
	#endif
	m_AppPacketAvailable = false;
	m_nCharsRX = m_nCharsTX = 0;

	// Insure all buffers look empty
	m_lowInProcPacket.Byte.BufferSize = 0;
	m_hiInProcPacket.Byte.BufferSize = 0;
	
	// Start up a plain old serial port
	m_packetMode = false;
	
	// Nothing read yet
	#ifdef _DEBUG
		m_maxReadSize = 0;
	#endif
	#if RECORD_PKT_DEPTH
		m_maxDepth = 0;
	#endif

	PacketParseReset();
	m_pUserCommInterrupt = NULL;

	// Create our event engine
	m_pSerialEvts = new CSerialEvt(this);
	
	// ------------------------
	// CThread initializations
	// ------------------------
	
	// This is used in a DLL to implement a special exit strategy as
	// we cannot wait on the thread handle to sync to it's death.
	#if (defined(_WIN32)||defined(_WIN64))
		SetDLLterm(true);
	#endif
}

CSerialEx::~CSerialEx()
{
	// Prevent packet waiters from restarting
	m_packetMode = false;
	// Note: this does not exit on non-Windows system
	if (m_pSerialEvts) {
		delete m_pSerialEvts;
		m_pSerialEvts = NULL;
	}
	// Kill the character read thread
	CThread::Terminate();
	m_responsePacketWaiting.SetEvent();
	// Cancel pending OS I/O 
	CancelCommIo();
	m_rdEvent.SetEvent();
	// Wait for read thread to exit
	CThread::WaitForTerm();
	#if TRACE_THREAD || TRACE_DESTRUCT
		_RPT2(_CRT_WARN,"%.1f CSerialEx (destruction started), id=" THREAD_RADIX "\n",
			infcCoreTime(), ThreadID());
	#endif
	Close();
	// Kill any packets we have outstanding
	m_SendPacketToAppLock.Lock();	
	for (size_t i=0; m_finishedPackets.size(); i++) {
		packetbuf *buf = m_finishedPackets.front();
		m_finishedPackets.pop_front();
		delete buf;
	}
	m_SendPacketToAppLock.Unlock();	
	#if TRACE_THREAD || TRACE_DESTRUCT
		_RPT1(_CRT_WARN,"%.1f CSerialEx (destroyed)\n", infcCoreTime());
	#endif
}
//																			 *
//****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CSerialEx Open/Close  
//
//	DESCRIPTION:							
//		Open and close the serial port hardware.
//
//	SYNOPSIS:
#if (defined(_WIN32)||defined(_WIN64))
CSerial::SERAPI_ERR CSerialEx::OpenComPort (long portNum)
#else
CSerial::SERAPI_ERR CSerialEx::OpenComPort (const char * portName)
#endif
{
	// Call the base class first/ overlap enabled
	#if (defined(_WIN32)||defined(_WIN64))
		CSerial::SERAPI_ERR lLastError = CSerial::Open(portNum, READ_BUF_LEN, 64, true);
	#else
		CSerial::SERAPI_ERR lLastError = CSerial::Open(portName, 0, 0, true);
	#endif
	
	if (lLastError != API_ERROR_SUCCESS)
		return lLastError;
	// Reset OS flags and state
	//6/6/13 Flush();
	// Start the listener thread
	CSerial::EPort pErr;
	pErr = StartListener();
	if (pErr != CSerial::EPortAvailable) {
		// Close the serial port
		Close();

		// Return the error-code
		m_lLastError = API_ERROR_PORT_UNAVAILABLE;
		return API_ERROR_PORT_UNAVAILABLE;
	}
	// Return the error
	m_lLastError = API_ERROR_SUCCESS;
	return API_ERROR_SUCCESS;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *

CSerial::SERAPI_ERR CSerialEx::Close (void)
{
	// Stop listener thread (wait until it ends)
	StopListener();
	// Reset all members
	m_responsePacketWaiting.SetEvent();
	// Call the base class
	return CSerial::Close();
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::TerminateAndWait
//
//	DESCRIPTION:
///		Override CThread termination logic
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
void CSerialEx::TerminateAndWait()
{
	// Set terminate flag and release all waiting objects 
	CThread::Terminate();
	// Cancel any pending I/O
	CancelCommIo();
	// Setup for our specific cleanup, allow any external waiters to go
	m_commEvent.SetEvent();
	// Cancel new I/O that may have leaked through
	CancelCommIo();
	// Do base class work now
	CThread::TerminateAndWait();
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CSerialEx::StartListener
//
//	DESCRIPTION:
///		Start the read listening thread.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
CSerial::EPort CSerialEx::StartListener (void)
{
	// Start our event engine
    #if TRACE_THREAD
        _RPT1(_CRT_WARN, "%.1f CSerialEx::StartListener...\n", infcCoreTime());
    #endif
    if (m_pSerialEvts)
		m_pSerialEvts->Restart();
    #if TRACE_THREAD
        _RPT1(_CRT_WARN, "%.1f CSerialEx::StartListener evt restarted...\n", infcCoreTime());
    #endif
    // Launch with elevated priority to insure responsiveness
	LaunchThread(this, 1);
    #if TRACE_THREAD
        _RPT1(_CRT_WARN, "%.1f CSerialEx::StartListener launched...\n", infcCoreTime());
    #endif
	// Wait until running
	m_ThreadParkedEvent.ResetEvent();
	m_rdAck.WaitFor();
    #if TRACE_THREAD
        _RPT1(_CRT_WARN, "%.1f CSerialEx::StartListener started\n", infcCoreTime());
    #endif
	return(Terminating() ? EPortNotAvailable : EPortAvailable);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::StopListener
//
//	DESCRIPTION:
///		Stop the read listening thread.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
LONG CSerialEx::StopListener (
		DWORD dwTimeout)
{
    #if TRACE_THREAD
        _RPT1(_CRT_WARN, "%.1f CSerial::StopListener...\n", infcCoreTime());
    #endif
	CThread::Terminate();
	// Insure we release external waiters
	m_responsePacketWaiting.SetEvent();
	m_commEvent.SetEvent();
	// Kill our thread
	TerminateAndWait();
    #if TRACE_THREAD
        _RPT1(_CRT_WARN, "%.1f CSerial::StopListener stopped\n", infcCoreTime());
    #endif
	// Insure we release external waiters
	m_responsePacketWaiting.SetEvent();
	m_lLastError = API_ERROR_SUCCESS;
	return API_ERROR_SUCCESS;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::TestAndPushHiPacket
//
//	DESCRIPTION:
///		This will test for embedded high packet in another packet. If we
///		are going to push this this function returns true.
///
///		Typical usage:
///		void someStateFunc(char myChar) {
///			if (TestAndPushHiPacket(myChar))
///				return;
///			// Keep processing
///
/// 	\param xxx description
///		\return true if we are still processing
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
bool CSerialEx::TestAndPushHiPacket(char nextChar) {
	register packetFields *parser = (packetFields *)&nextChar;
	
	// Not start of packet, no problem!
	if (!parser->StartOfPacket)
		return(false);
		
	// We have some start of packet, therefore the first octet's fields
	// are OK.
	
	// Fragmented high-priority packet or low priority start in non-idle
	if (inHighPrioState()  // Any "restart" in high-prio is bad
	|| (!MN_PKT_IS_HIGH_PRIO(parser->PktType) && m_state != READ_STATE_IDLE)) {
		// Create a Fragmentation Error
		SendErrToApp(ND_ERRNET_FRAG, 0,0);
		PacketParseReset();
		// Reset processing and reparse this "start of packet"
		ProcessNextChar(nextChar);
		// Break recursion
		return(true);
	}
	
	// Start of packet / high priority type in regular packet
	if (MN_PKT_IS_HIGH_PRIO(parser->PktType)) {
		// Initialize the high priority packet buffer
		m_hiPrioPktIndx = 0;
		m_hiInProcPacket.Byte.Buffer[m_hiPrioPktIndx++] = nextChar;
		// Assign maximum length until we get the length octet
		m_hiInProcPacket.Fld.PktLen = MN_HDR_LEN_MASK;
		m_hiChecksum = nextChar;
		// We interrupted current state
		m_pushedState = m_state;
		// We get length next
		m_state = READ_STATE_HP_PAYLOAD;
		return(true);
	}
	return(false);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::ProcessNextChar
//
//	DESCRIPTION:
///		This FSM will grab sequential characters from the serial port and 
///		extract	any detected packets and signal the application when there 
///		are some available.
///
/// 	\param xxx description
///		\return true if we are still processing
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
inline void CSerialEx::ProcessNextChar(char nextChar)
{
#if TRACE_LOW_LEVEL
	_RPT2(_CRT_WARN, "ProcessNextChar(%d): char 0x%02x\n",
							m_state, nextChar);
#endif
	// Parser view of this char
	packetFields *parser = (packetFields *)&nextChar;
	// Try and extract the next packet.
	switch(m_state) {
	// --------------------------------------------------
	// WAIT FOR PACKET START (IDLE)
	// --------------------------------------------------
	case READ_STATE_IDLE:
		//we want to fill char 0 first
		m_hiPrioPktIndx = m_lowPrioPktIndx = 0;
		// do we have a start of packet?
		if (parser->StartOfPacket) {
			// if the stray packet count is > 0, send a stray error packet and zero 
			// packet count; the data is contained in location [3] of m_strayCount
			if (m_strayCount>0) {
				// Construct a error packet: Type=2 (stray), Data=StrayOctetCnt
				// Stray Error
				SendErrToApp(ND_ERRNET_STRAY, 0, m_strayCount);
				m_strayCount=0;
				m_pushedState = READ_STATE_IDLE;
			}
			// Push processing 
			if (TestAndPushHiPacket(nextChar))
				break;
			// low priority - cannot interrupt each other
			m_lowInProcPacket.Byte.Buffer[m_lowPrioPktIndx++] = nextChar;
			// Assign maximum length until we get the length octet
			m_lowInProcPacket.Fld.PktLen = MN_HDR_LEN_MASK;
			m_lowChecksum = nextChar;
			m_state = READ_STATE_LP_PAYLOAD;
		} 
		else {
			// Stray octet has been detected!
			// Increment stray count; max count of 127
			if (m_strayCount < 127) {
				#if TRACE_STRAY
					_RPT1(_CRT_WARN, "READ_STATE_IDLE: stray 0x%x\n", 
									 nextChar);
				#endif
				m_strayCount++;
			}
		}
		// we are done with this character - either thrown out or processed
		break;
		
	// --------------------------------------------------
	// PROCESS THE LOW PRIORITY PACKET
	// --------------------------------------------------
	case READ_STATE_LP_PAYLOAD:
		//check to see if we are the start of a high priority packet
		if (TestAndPushHiPacket(nextChar))
			break;
		#define PKT_OVERHEAD_LEN (MN_API_PACKET_HDR_LEN+MN_API_PACKET_TAIL_LEN)
		// Accumulate characters and calculate the checksum 
		m_lowInProcPacket.Byte.Buffer[m_lowPrioPktIndx++] = nextChar;
		m_lowChecksum += nextChar;
		// Have we reached the checksum?
		if (m_lowPrioPktIndx >= m_lowInProcPacket.Fld.PktLen+PKT_OVERHEAD_LEN) {
			#if TRACE_LOW_LEVEL
			//	printf("Checksum calc:%X Checksum received: %X\n", m_checkSumCalc, m_lowInProcPacket.Byte.Buffer[m_lowInProcPacket.Fld.PktLen+2]);
			#endif			
			// Non-zero means corruption					
			if(m_lowChecksum & 0x7F) {	
				// Checksum Error!
				SendErrToApp(ND_ERRNET_CHKSUM, m_lowInProcPacket.Fld.Addr, 0);
			} 
			else {
				// Setup the character length
				m_lowInProcPacket.Byte.BufferSize=m_lowPrioPktIndx;
				#if TRACE_LOW_LEVEL
				DUMP_PKT("LO pkt", &m_lowInProcPacket);
				#endif
				// Send the packet to app and wait until it reads it
				SendPacketToApp(m_lowInProcPacket,true);
			}
			// We are done, go idle
			m_state = READ_STATE_IDLE;
		}
		break;
	// --------------------------------------------------
	// PROCESS THE HIGH PRIORITY PACKET
	// --------------------------------------------------
	case READ_STATE_HP_PAYLOAD:
		if (TestAndPushHiPacket(nextChar))
			break;
		// Accumulate characters and calculate the checksum 
		m_hiInProcPacket.Byte.Buffer[m_hiPrioPktIndx++] = nextChar;
		m_hiChecksum += nextChar;
		// Have we reached the checksum?
		 if (m_hiPrioPktIndx >= m_hiInProcPacket.Fld.PktLen+PKT_OVERHEAD_LEN) {
			// Non-zero means corruption					
			if(m_hiChecksum & 0x7F) {
				// Checksum Error!
				SendErrToApp(ND_ERRNET_CHKSUM, m_hiInProcPacket.Fld.Addr,0 );
			} 
			else {
				// Setup the character length
				m_hiInProcPacket.Byte.BufferSize=m_hiPrioPktIndx;
				#if TRACE_LOW_LEVEL
				DUMP_PKT("HI pkt", &m_hiInProcPacket);
				#endif
				// Send the packet to app and wait until it reads it
				SendPacketToApp(m_hiInProcPacket,true);
			}
			// Return to last processing state
			m_state = m_pushedState;
			m_pushedState = READ_STATE_IDLE;
			m_hiPrioPktIndx = 0;
		}
		break;
	default:
		_RPT1(_CRT_ASSERT, "CSerialEx::ProcessNextChar unknown state %d\n", m_state);
		PacketParseReset();
		// Process the low priority packet
		break;
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::Run
//
//	DESCRIPTION:
///		Background thread to collect characters from the serial port and 
///		packetize them for the upper packet processing thread.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
int CSerialEx::Run(void *context)
{
	// Packet parsing current character
	char thisChar;
	
	// Port event statististics
	#if TRACE_THREAD || TRACE_DESTRUCT
		_RPT2(_CRT_WARN, "%.1f CSerialEx::Run thread id=" THREAD_RADIX " starting\n", 
							infcCoreTime(), CurrentThreadID());
	#endif
	// Initiate the Wait Comm Event waiter on:
	//		 break detect, errors and data reception
	SetEventMask(EEventBreak|EEventError|EEventRecv|EEventCTS);
	// Start the event engine if it exists
	if (m_pSerialEvts) {
		// First let it get to ::Run function
		m_pSerialEvts->WaitForParked();
		m_pSerialEvts->Restart();
	}
	// Signal we are running
	m_rdAck.SetEvent();
#if TRACE_THREAD || TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f CSerialEx::Run thread id=" THREAD_RADIX " looping\n", 
						infcCoreTime(), CurrentThreadID());
#endif
			
	// Keep looping until big error or StopListener kills us
	do {
		// Park here until the event thread wakes us up
#if (defined(_WIN32)||defined(_WIN64))
		m_ThreadParkedEvent.WaitFor();
		//_RPT0(_CRT_WARN,"w");
		// Let event set us
		m_ThreadParkedEvent.ResetEvent();
#endif
		// Stop processing if terminating
		if (Terminating()) {
			#if TRACE_DESTRUCT
				_RPT1(_CRT_WARN, "%.1f CSerialEx::Run terminating post restart\n", infcCoreTime());
			#endif
			break;
		}
		// Read into our empty buffer
		m_rdBuffer.lock();
		#ifdef _DEBUG
			// Save last bucket size for diagnostic purposes
			m_lastReadSize = m_readSize;
		#endif
		while (Read(m_rdBuffer.tailPtr(), m_rdBuffer.charsLeft(),
			&m_readSize, READ_TIME_OUT_MS)) {
			if (Terminating()) {
				m_rdBuffer.unlock();
				goto exit_pt;
			}
		}

		//_RPT1(_CRT_WARN,"r%d ", m_readSize);
		// Store stuff if we had a successful read and not flushing
		if (m_readSize) {
			if (!m_rdAutoFlush) {
				// Make them available in the buffer
				m_rdBuffer.makeAvailable(m_readSize);
				m_nCharsRX += m_readSize;
			}
			else
				m_rdBuffer.flush();
		}
		m_rdBuffer.unlock();
		#ifdef _DEBUG
			// Save peak read size for diagnostic purposes
			if (m_maxReadSize < m_readSize) 
				m_maxReadSize = m_readSize;
		#endif
		// Process any buffer items based on operational mode
		if (m_packetMode) {
			// Process all the characters in our buffer
			while(m_rdBuffer.getChar(thisChar) && !((m_pTermFlag != NULL) && (*m_pTermFlag)) && !m_rdAutoFlush) {
				#ifdef _DEBUG
					static bool dbg=false;
					if (dbg)
						_RPT1(_CRT_WARN, "chr=0x%02x\n", 0xff&thisChar);
				#endif
				ProcessNextChar(thisChar);
			}
			// Reset the buffer for next read
			m_rdBuffer.flush();
		}
	} while (!Terminating());
exit_pt:
	// Kill any I/O pending to prevent issues as port closes
	CancelCommIo();
	// Wait for the cancel to occur
	//m_commEvent.WaitFor();
	#if TRACE_THREAD || TRACE_DESTRUCT
		_RPT2(_CRT_WARN, "%.1f CSerialEx::Run thread id=" THREAD_RADIX
						 " exiting\n",
						infcCoreTime(), ThreadID());
	#endif
	return (API_ERROR_SUCCESS);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::ProcessEvent
//
//	DESCRIPTION:
/**		
	Process this event. This is normally fed from the event detection thread.

 	\param[in,out] xxx description
	\return description
 
**/
//	SYNOPSIS:
void CSerialEx::ProcessEvent(EEvent eEvent)
{
	//#ifdef _DEBUG
	//if (eEvent & 0xfe)
	//	_RPT2(_CRT_WARN, "%.1f CSerialEx::Run evt=" THREAD_RADIX "\n", infcCoreTime(), eEvent);
	//#endif
	// There was a COMM event, which needs handling. We'll call the
	// appropriate event handler(s). 
	if (eEvent) { 
		// We have received some data, restart the read thread
		if((eEvent & CSerial::EEventRecv)) {
			//_RPT1(_CRT_WARN, "%.1f CSerialEx::incr rx char\n", infcCoreTime());
			INCREMENT_ERRORCNT(m_ErrorReport.RXCHARcnt);
			//_RPT0(_CRT_WARN,"*");
			m_ThreadParkedEvent.SetEvent();
		}
		// We received a CTS changed event.
		// We are only interested in the CTS falling edge events
		if((eEvent & CSerial::EEventCTS) && !GetCTS()) {
			//_RPT0(_CRT_WARN, "CSerialEx::Run got CTS fall\n");
			INCREMENT_ERRORCNT(m_ErrorReport.CTScnt);
			m_ThreadParkedEvent.SetEvent();
		}
		// We received the break condition, baud rate changed
		// and note we received one.
		if((eEvent & CSerial::EEventBreak)) {
			//_RPT0(_CRT_WARN, "CSerialEx::Run got break\n");
			INCREMENT_ERRORCNT(m_ErrorReport.BREAKcnt);
		}
		// Some port error was detected, log each one, if not break!
		else if((eEvent & CSerial::EEventError)) {
			// Obtain the error status during this event
			CSerial::EError eError = GetError();
			if (eError & (CSerial::EErrorRxOver | CSerial::EErrorOverrun)) {
				// Port Overrun
				INCREMENT_ERRORCNT(m_ErrorReport.ERR_OVERRUNcnt);
				SendErrToApp(ND_ERRNET_PORT_OVERRUN,0,0);
				_RPT1(_CRT_WARN, "%.1f CSerialEx::RX overrun\n", infcCoreTime());
				Purge();
				eError = (CSerial::EError)(eError & ~(CSerial::EErrorRxOver | CSerial::EErrorOverrun));
			}
			if (eError & (CSerial::EErrorBreak | CSerial::EErrorFrame)) {
				// Ignoring this case as break sometimes appears as this
				// condition
				if (!m_breakSent && !m_rdAutoFlush) {
					_RPT1(_CRT_WARN, "%.1f CSerialEx::Framing Error\n", infcCoreTime());
					INCREMENT_ERRORCNT(m_ErrorReport.ERR_FRAMEcnt);
					SendErrToApp(ND_ERRNET_FRAME, 0, 0);
				}
				#if _DEBUG
				else _RPT1(_CRT_WARN, "%.1f CSerialEx::Framing Error(expected)\n", infcCoreTime());
				#endif

				// Reset filter
				m_breakSent = false;
				eError = (CSerial::EError)(eError & ~(CSerial::EErrorBreak | CSerial::EErrorFrame));
			}
			if (eError) {
				// Skip the rest
				_RPT2(_CRT_WARN, "%.1f CSerialEx::eError %d\n", infcCoreTime(), eError);
			}
		}
	} // if (eEvent)
}
//																			  *
//*****************************************************************************





//*****************************************************************************
//*****************************************************************************
//                          PACKET ORIENTED API
//*****************************************************************************
//*****************************************************************************
//	NAME																	  *
//		CSerialEx::SendPacketToApp
//
//	DESCRIPTION:
///		Send the <thePacket> to the application layer.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
void CSerialEx::SendPacketToApp(packetbuf &thePacket, BOOL Convert7To8Bit)
{

#if TRACE_READS
	{
		_RPT1(_CRT_WARN, "%.1f SendPacketToApp(Start): ", infcCoreTime());
		for (unsigned i=0; i<thePacket.Byte.BufferSize; i++) {
			_RPT1(_CRT_WARN, "%02X+", 0xff&thePacket.Byte.Buffer[i]);
		}
	}
#endif
	if (m_pUserCommInterrupt && !m_rdAutoFlush) {
		// We are going to use this packet, stash it in heap
		packetbuf *pkt = new packetbuf;
		// Copy to internal buffer with proper conversion if required.
		if (Convert7To8Bit) {
			convert7to8(thePacket, *pkt);
		}
		else {
			*pkt = thePacket;
		}
		// Lock GetPkt state until we finish setup
		m_SendPacketToAppLock.Lock();	
		// We want to actually send, queue to back
		m_finishedPackets.push_back(pkt);
		// Tell application layer we have something new
		m_AppPacketAvailable = true;
		m_responsePacketWaiting.SetEvent();
		if (m_pUserCommInterrupt)
			m_pUserCommInterrupt->SetEvent();
		// Allow interactions with m_AppPacketAvailable & m_finishedPackets
		m_SendPacketToAppLock.Unlock();
	}
	else {
		// Flush everything we have
		Flush();
	}
	//_RPT0(_CRT_WARN,"e");
#if TRACE_READS
	_RPT0(_CRT_WARN, "SendPacketToApp(End)\n");
#endif
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::SendErrToApp
//
//	DESCRIPTION:
///		Create and queue an error packet back to application layer.
///
/// 	\param errorType type of error to return
/// 	\param addr address to report error at
/// 	\param data extra information to report
///		\return true if we have data, else was a timeout and no data.
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
void CSerialEx::SendErrToApp(mnNetErrs errorType, unsigned addr, unsigned data)
{
	// Construct a error packet: Type=0 (frag)
	packetbuf errpacket;
	errpacket.Fld.Addr=addr;
	errpacket.Fld.Mode=errpacket.Fld.Zero1=0;
	// Host initiated error
	errpacket.Fld.Src=MN_SRC_HOST;
	errpacket.Fld.PktType = MN_PKT_TYPE_ERROR;
	errpacket.Fld.PktLen = 2;				// Payload size
	// Total packet size (includes header and length fields)
	errpacket.Byte.BufferSize = errpacket.Fld.PktLen + MN_API_PACKET_HDR_LEN;		
	errpacket.Byte.Buffer[RESP_LOC] = (nodechar)errorType;
	errpacket.Byte.Buffer[RESP_LOC+1] = (nodechar)data;
	SendPacketToApp(errpacket, false);
	#if TRACE_ERR
	_RPT3(_CRT_WARN,"CSerialEx::SendErrToApp: errorType = %d ; addr = %d ; data = %d\n",
					 errorType,addr,data);
	#endif
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::GetPkt
//
//	DESCRIPTION:
///		Wait for and get the oldest packet received.
///
/// 	\param buffer buffer to copy data into
///		\return true if we have data, else was a timeout and no data.
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
bool CSerialEx::GetPkt(packetbuf &buffer)
{
#if TRACE_HIGH_LEVEL
	_RPT1(_CRT_WARN, "%.1f CSerialEx::GetPkt...(start)\n", infcCoreTime());
#endif
	if (m_packetMode) {
		// Wait for one to show up or time-out
		if (m_responsePacketWaiting.WaitFor(FRAME_READ_TIMEOUT+500)) {
			//_RPT1(_CRT_WARN, "%.1f CSerialEx::GetPkt...got!\n", infcCoreTime());
			//_RPT0(_CRT_WARN, ".\n");
			// Copy our buffer to the caller
			m_SendPacketToAppLock.Lock();
#if RECORD_PKT_DEPTH
			if (m_finishedPackets.size() > m_maxDepth) {
				m_maxDepth = m_finishedPackets.size();
				//_RPT1(_CRT_WARN, "SerialEx max rx depth=%d pkts\n", m_maxDepth);
			}
#endif
			if (m_finishedPackets.size()) {
				packetbuf *pkt = m_finishedPackets.front();
				#if TRACE_LOW_LEVEL
					DUMP_PKT("SerialEx: <= ", pkt);
				#endif
				buffer = *pkt;
				delete pkt;
				m_finishedPackets.pop_front();
				// Setup app layer for next read if we have them and not terminating
				if ((m_finishedPackets.size()==0) || Terminating()) {
					m_AppPacketAvailable = false;
					m_responsePacketWaiting.ResetEvent();
					if (m_pUserCommInterrupt) 
						m_pUserCommInterrupt->ResetEvent();
				}
				m_SendPacketToAppLock.Unlock();	
				#if TRACE_HIGH_LEVEL
					_RPT1(_CRT_WARN, "%.1f CSerialEx::GetPkt...(OK)!\n", infcCoreTime());
				#endif
				return(true);
			}
			else {
				m_SendPacketToAppLock.Unlock();
			}
		} 
		else {
			//_RPT1(_CRT_WARN, "%.1f CSerialEx::GetPkt...timeout!\n", infcCoreTime());
			buffer.Byte.BufferSize = 0;
			m_responsePacketWaiting.ResetEvent();
			if (m_pUserCommInterrupt) 
				m_pUserCommInterrupt->ResetEvent();
		}
	}
#if TRACE_HIGH_LEVEL
	_RPT1(_CRT_WARN, "%.1f CSerialEx::GetPkt...(fail)!\n", infcCoreTime());
#endif
	// If not processed we failed
	return(false);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::SendPkt
//
//	DESCRIPTION:
///		Send the packet to the serial ring channel in an atomic fashion.
///
/// 	\param xxx description
///		\return TRUE on success
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
bool CSerialEx::SendPkt(packetbuf &buffer)
{
	CSerial::SERAPI_ERR result=API_ERROR_SUCCESS;
	DWORD nWritten;
	packetbuf sendBuf;							// Local thread safe copy
	
	// Clean start of packet
	buffer.Byte.Buffer[0] |= 0x80;			// Set start of packet
	buffer.Byte.Buffer[1] &= ~(0x80);			// Insure length field MSB clear

	convert8to7(buffer, sendBuf);				// Convert to channel format

	// Send link formatted command to the port
	result = Write(sendBuf.Byte.Buffer, sendBuf.Byte.BufferSize,
							&nWritten);
	m_nCharsTX += nWritten;
	if (result!=API_ERROR_SUCCESS) {
		_RPTF1(_CRT_WARN,"CSerial::SendPkt - err 0x%x\n", (unsigned int)result);		
		m_lLastError = result;			
	} 
	return(result==API_ERROR_SUCCESS);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CSerialEx::IsPacketAvailable
//
//	DESCRIPTION:
///		Returns true if a packet is available.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
bool CSerialEx::IsPacketAvailable(void)
{
	bool avail;
	//m_SendPacketToAppLock.Lock();
	avail = m_AppPacketAvailable;
	//m_SendPacketToAppLock.Unlock();	
	return(avail && m_packetMode);
}
//																			  *
//*****************************************************************************

bool CSerialEx::Send_Available()
{
	return true;	// To be modified if we use threaded write
}

//*****************************************************************************
//*****************************************************************************
//                          PLAIN SERIAL PORT API
//*****************************************************************************
//*****************************************************************************
//	NAME																	  *
//		CSerialEx::GetChar
//
//	DESCRIPTION:
///		Get the oldest buffered character. If we have a character, the function
///		returns true.
///
/// 	\param xxx description
///		\return true if we got our character
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
bool CSerialEx::GetChar(char *theChar)
{
	DWORD nRead;
	LONG retErr;
	// This API works only in serial port mode
	if (!m_packetMode) {
		//// getChar returns TRUE if we get char OK
		//return m_rdBuffer.getChar(*theChar);
		retErr = Read(theChar, 1, &nRead, 100);
		m_nCharsRX += nRead;
		return (retErr==0) && (nRead==1);
	}
	return false;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::SendChars
//
//	DESCRIPTION:
///		Atomically send the characters.
///
/// 	\param xxx description
///		\return true if sucessfully queued.
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
bool CSerialEx::SendChars(char *theChars, unsigned int charsLen)
{
	LONG result;
	DWORD nWritten;
	result = Write(theChars, charsLen, &nWritten);
	m_nCharsTX += nWritten;
	if (result != API_ERROR_SUCCESS || charsLen != (unsigned int)nWritten) {
		_RPTF1(_CRT_WARN, "CSerialEx::SendChars failed %d\n", (int)result);
		return(false);
	}
	return (true);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CSerialEx::Set/GetMode
//
//	DESCRIPTION:
///		Set/Get the serial port operational mode: packets or plain old
///		serial port.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
void CSerialEx::SetMode(CSerialMode theNewMode)
{
	switch (theNewMode) {
	case SERIALMODE_MERIDIAN_7BIT_PACKET:
		m_packetMode = true;
		if (m_pSerialEvts)
			m_pSerialEvts->Start();
		break;
	case SERIALMODE_SERIAL:
		m_packetMode = false;
		if (m_pSerialEvts)
			m_pSerialEvts->Stop();
		break;
	default:
		_RPTF1(_CRT_ASSERT, "CSerialEx::SetMode unsupported type %d\n", theNewMode);
		break;
	}
}

CSerialMode CSerialEx::GetMode()
{
	return m_packetMode ? SERIALMODE_MERIDIAN_7BIT_PACKET 
						: SERIALMODE_SERIAL;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::AutoFlush
//
//	DESCRIPTION:
///		Cancel all work in process and flush the characters waiting in the
///		serial port hardware.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
void CSerialEx::AutoFlush(bool mode) {
	m_rdAutoFlush = mode;
	// If flushing, make sure packets waiting are released
	//if (mode) {
	//	
	//}
};

//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::Flush
//
//	DESCRIPTION:
///		Cancel all work in process and flush the characters waiting in the
///		serial port hardware.
///
/// 	\param xxx description
///		\return description
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
void CSerialEx::Flush()
{
#if TRACE_READS
	_RPT0(_CRT_WARN,"CSerialEx::Flush()\n");
#endif
	bool autoIn = m_rdAutoFlush;
	// Start ignoring the data
	AutoFlush(true);

	// Prevent packet waiters from restarting
	m_responsePacketWaiting.ResetEvent();
	// Let stuff pile up
	CThread::Sleep(100);
	// Make sure all the events are cleared
	GetError();
	// Kill serial traffic accumulated
	m_SendPacketToAppLock.Lock();	
	m_rdBuffer.flush();
	for (size_t i=0; m_finishedPackets.size(); i++) {
		packetbuf *buf = m_finishedPackets.front();
		m_finishedPackets.pop_front();
		delete buf;
	}
	m_responsePacketWaiting.ResetEvent();
	// Restart the packet parser
	PacketParseReset();
	m_SendPacketToAppLock.Unlock();
	// Clear OS queue
	Purge();
#if 0
	// Clear pending events
	SetEventMask(0);
	// Start looking again for breaks
	SetEventMask();
#endif
	// Allow our read thread to grab data
	AutoFlush(autoIn);
}
//																			  *
//*****************************************************************************


void CSerialEx::ErrorReportClear()
{
	m_ErrorReport.clear();
}
//																			  *
//*****************************************************************************

void CSerialEx::ErrorReportGet(CSerialExErrReportInfo *pErrorReport)
{
	*pErrorReport = m_ErrorReport;
}
//																			  *
//*****************************************************************************
/*****************************************************************************
 *	!NAME!
 *		cpymem
 *		
 *	DESCRIPTION:
 *		This function will copy nchars of memory from <src> to <dest>.
 *
 *	RETURNS:
 *		cnErrCode error code
 *		
 *	SYNOPSIS: 															    */
__inline void CSerialEx::cpymem(
		nodechar *src, 
		nodechar *dest, 
		nodeushort nchars)
{
	while (nchars--)
		*(dest++) = *(src++);
}
//																			  *
//*****************************************************************************


// This function should allow intentional misconduct by having the
// buffer length not related to packet header length+overhead. If allowed
// this function can be used to test buffer and network proceeing by
// allowing fragmented packets to be transmitted as well as stray data.
void CSerialEx::convert8to7(packetbuf& inBuf, packetbuf& outBuf)
{
	// we copy the first 2 bytes without translation
	outBuf.Byte.Buffer[0] = inBuf.Byte.Buffer[0];
	outBuf.Byte.Buffer[1] = inBuf.Byte.Buffer[1];

	nodechar *s = &inBuf.Byte.Buffer[2];
	nodechar *d = &outBuf.Byte.Buffer[2];
	nodechar *origD = d;
	unsigned long chksum = 0;

	// Allow caller to lie about length to create frag or stray data
	int num8 = inBuf.Fld.PktLen;
	int consumeBytes;
	int outBytes;

	while(num8 > 0){
		consumeBytes = num8 >= 7 ? 7 : num8;
		outBytes = consumeBytes + 1;
		switch(outBytes) {
			case 8: d[7] = 0x7f &				 ((unsigned char)s[6] >> 1);
			// no break OK (special line for Code Analyzer to suppress warning)
			case 7: d[6] = 0x7f & ((s[6] << 6) | ((unsigned char)s[5] >> 2));
			// no break OK (special line for Code Analyzer to suppress warning)
			case 6: d[5] = 0x7f & ((s[5] << 5) | ((unsigned char)s[4] >> 3));
			// no break OK (special line for Code Analyzer to suppress warning)
			case 5: d[4] = 0x7f & ((s[4] << 4) | ((unsigned char)s[3] >> 4));
			// no break OK (special line for Code Analyzer to suppress warning)
			case 4: d[3] = 0x7f & ((s[3] << 3) | ((unsigned char)s[2] >> 5));
			// no break OK (special line for Code Analyzer to suppress warning)
			case 3: d[2] = 0x7f & ((s[2] << 2) | ((unsigned char)s[1] >> 6));
			// no break OK (special line for Code Analyzer to suppress warning)
			case 2: d[1] = 0x7f & ((s[1] << 1) | ((unsigned char)s[0] >> 7));
			// no break OK (special line for Code Analyzer to suppress warning)
			case 1: d[0] = 0x7f &   s[0];
			// no break OK (special line for Code Analyzer to suppress warning)
		}
		d += outBytes;
		s += consumeBytes;
		num8 -= consumeBytes;
	}
	// Adjusts output header for expansion due to 8->7
	outBuf.Fld.PktLen = (d - origD);
	// Adjust packet buffer size for expansion due to 8->7 + checksum append
	// 64-bit OK, buffer small always
	outBuf.Byte.BufferSize = (nodeulong)(inBuf.Byte.BufferSize 
		+ (d - origD)-inBuf.Fld.PktLen
		+ MN_API_PACKET_TAIL_LEN);
	for(unsigned i=0;i<(outBuf.Fld.PktLen+2U);i++) {
		chksum+=outBuf.Byte.Buffer[i];
	}
	chksum = (-1*chksum)&0x7f;
	outBuf.Byte.Buffer[outBuf.Fld.PktLen+2] = (nodechar) chksum;
}
//																			  *
//*****************************************************************************

void CSerialEx::convert7to8(packetbuf& inBuf, packetbuf& outBuf)
{
	// we copy the header without translation
	outBuf.Byte.Buffer[0] = inBuf.Byte.Buffer[0];
	outBuf.Byte.Buffer[1] = inBuf.Byte.Buffer[1];


	int num7 = inBuf.Fld.PktLen;

	nodechar *buf7 = &inBuf.Byte.Buffer[RESP_LOC];
	nodechar *d = &outBuf.Byte.Buffer[RESP_LOC];
	nodechar *origD = d;

	int i, mod8;
	for(i = 0; i < num7; ++i) {
		mod8 = 0x07 & i;
		*d |= 0xff & (buf7[i] << (8 - mod8));
		d += mod8 != 0;			// inc. d 7 out of 8 times
		*d = buf7[i] >> mod8;
	}
	
	outBuf.Fld.PktLen = (num7==1) ? 1: d - origD;
	outBuf.Byte.BufferSize=outBuf.Fld.PktLen+MN_API_PACKET_HDR_LEN;

}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEx::RegisterUserPktCommEvent
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
void CSerialEx::RegisterUserPktCommEvent(CCEvent *pUsersEvent)
{
	m_pUserCommInterrupt = pUsersEvent;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEvt::CSerialEvt
//
//	DESCRIPTION:
/**		
	This class polls for serial port events via a thread and wakes up any
	appropriate threads to handle the event.

 	\param[in,out] xxx description
	\return description
 
**/
//	SYNOPSIS:
CSerialEvt::CSerialEvt(CSerialEx *pOwner)
{
#if TRACE_DESTRUCT
	_RPT1(_CRT_WARN, "%.1f CSerialEvt constructing...\n", infcCoreTime());
#endif
	// Object we interact with for events.
	pPort = pOwner;
	m_paused = false;
	// Pause at startup
	m_ThreadParkedEvent.ResetEvent();
	#if (defined(_WIN32)||defined(_WIN64))
		SetDLLterm(true);
	#endif
	LaunchThread(this,1);
}
CSerialEvt::~CSerialEvt()
{
#if TRACE_DESTRUCT
	_RPT2(_CRT_WARN, "%.1f CSerialEvt destroying...id=" THREAD_RADIX "\n",
			infcCoreTime(), ThreadID());
#endif
	// Insure waiters are released
	m_paused = false;
	m_parkAck.SetEvent();
	Terminate();
	pPort->ForceCommEvent();
	TerminateAndWait();
	pPort = NULL;
#if TRACE_DESTRUCT
	_RPT1(_CRT_WARN, "%.1f CSerialEvt destroyed\n", infcCoreTime());
#endif
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerialEvt::Run
//
//	DESCRIPTION:
/**		
	This thread polls for events detected by the serial port.  When certain
	ones are found the owner serial port is notified.

 	\param[in,out] xxx description
	\return description
 
**/
//	SYNOPSIS:
int CSerialEvt::Run(void *context)
{
	LONG lastError;
	// Signal we are at park state
	m_parkAck.SetEvent();
#if TRACE_DESTRUCT||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f CSerialEvt::Run parked, id=" THREAD_RADIX "...\n",
		infcCoreTime(), ThreadID());
#endif
	// Wait for a signal from read thread to start
	m_ThreadParkedEvent.WaitFor();
#if TRACE_DESTRUCT||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f CSerialEvt::Run restarted, id=" THREAD_RADIX "...\n",
			infcCoreTime(), ThreadID());
#endif
	while (!Terminating()) {
		// Start another wait for event
		if ((lastError = pPort->CommEventWaitInitiate()) != 0) {
#ifdef _DEBUG
			if (pPort->IsOpen() && !Terminating())
				_RPT1(_CRT_WARN, "CSerialEvt::Run CommEventWaitInitiate failed,err=%d\n", lastError);
#endif
			break;
		}
		// We don't care if this fails, causes a periodic poll
		#if (defined(_WIN32)||defined(_WIN64))				// TODO 9/22/10 DS Why not Linux (Rvw? no-events?)!
			/*bool waitOK = */pPort->WaitForCommEvent(INFINITE);
		#endif
		// Exiting?
		if (Terminating())
			break;
		// Determine the event
			CSerial::EEvent eEvent = pPort->GetEventType();
		//_RPT2(_CRT_WARN,"%.1f SerEvt 0x%x\n", infcCoreTime(), eEvent);
		if (!m_paused) 
			pPort->ProcessEvent(eEvent);
		else {
			//_RPT1(_CRT_WARN, "%.1f CSerialEvt::Run paused\n",
			//	 infcCoreTime());
			// Signal we are at park state
			m_parkAck.SetEvent();
			m_ThreadParkedEvent.ResetEvent();
		}
		// Pause control
		m_ThreadParkedEvent.WaitFor();
	}
#if TRACE_DESTRUCT||TRACE_THREAD
	_RPT2(_CRT_WARN, "%.1f CSerialEvt::Run exiting, id=" THREAD_RADIX "...\n",
			infcCoreTime(), ThreadID());
#endif
	return(0);
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CSerialEvt::Start/Stop
//
//	DESCRIPTION:
/**		
	Cause the event loop to pause or restart.

 	\param[in,out] xxx description
	\return description
 
**/
//	SYNOPSIS:
void CSerialEvt::Start() 
{
	m_paused = false;
	m_ThreadParkedEvent.SetEvent();
	//_RPT0(_CRT_WARN, "CSerialEvt::Start\n");
}

void CSerialEvt::Stop() 
{
	m_paused = true;
	//_RPT0(_CRT_WARN, "CSerialEvt::Stop\n");
}
//																			  *
//*****************************************************************************




#if 0
 Comm event handling from CSerialEx::Run we are not using
				if((eEvent & CSerial::EEventCTS)) {
					INCREMENT_ERRORCNT(m_ErrorReport.CTScnt);
				};

				if((eEvent & CSerial::EEventDSR)) {
					INCREMENT_ERRORCNT(m_ErrorReport.DSRcnt);
				};
				if((eEvent & CSerial::EEventRing)) {
					INCREMENT_ERRORCNT(m_ErrorReport.RINGcnt);
				};

				if((eEvent & CSerial::EEventRLSD)) {
					INCREMENT_ERRORCNT(m_ErrorReport.RLSDcnt);
				};

				if((eEvent & CSerial::EEventRcvEv)) {
					INCREMENT_ERRORCNT(m_ErrorReport.RXFLAGcnt);
				};
				// FIFO / buffer @ 80% full (TODO?)
				if((eEvent & CSerial::EEventRx80Full)) {
					INCREMENT_ERRORCNT(m_ErrorReport.RX80FUllcnt);
				}
#endif

//============================================================================= 
//	END OF FILE SerialEx.cpp
//=============================================================================





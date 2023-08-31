//*****************************************************************************
// $Workfile: SerialEx.h $
// $Archive: /ClearPath SC/Win Eclipse/MNuserDriver20/inc-private/SerialEx.h $
// $Date: 01/19/2017 17:39 $
//
//	SerialEx.h - Definition of the CSerialEx class
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


#ifndef __SERIAL_EX_H__
#define __SERIAL_EX_H__


//////////////////////////////////////////////////////////////////////
// Include CSerial base class
#if (defined(_WIN32)||defined(_WIN64))
	#include "SerialWin32.h"
#elif defined __QNX__
	#include "SerialQNX.h"
#else
	#include "SerialLinux.h"
#endif
// Teknic Headers
#include "pubMnNetDef.h"
#include "tekThreads.h"
#include "tekEvents.h"
// StdLib inclusions
#include <deque>

// Maximum polling delay for break detector
#define COMM_EVT_BRK_DLY_MS		100

// Maximum number of characters to read from port at a time
#define READ_BUF_LEN			4096

// Set to 1 to record highest packet depth
#define RECORD_PKT_DEPTH		1

// Saturating incrementor
#define ERRORCNT_MAX			1000000UL
inline void INCREMENT_ERRORCNT(Uint32 &TheVariable)	{
	TheVariable = (TheVariable<ERRORCNT_MAX) ? TheVariable+1
											 : TheVariable;
};

//////////////////////////////////////////////////////////////////////
//
// CSerialEx - Win32 message-based wrapper for serial communications
//
// A lot of MS-Windows GUI based programs use a central message
// loop, so the application cannot block to wait for objects. This
// make serial communication difficult, because it isn't event
// driven using a message queue. This class makes the CSerial based
// classes suitable for use with such a message queue. Whenever
// an event occurs on the serial port, a user-defined message will
// be sent to a user-defined window. It can then use the standard
// message dispatching to handle the event.
//
// Pros:
// -----
//	- Easy to use
//	- Fully ANSI and Unicode aware
//  - Integrates easily in GUI applications and is intuitive to
//    use for GUI application programmers
//
// Cons:
// -----
//  - Uses a thread for each COM-port, which has been opened.
//  - More overhead, due to thread switching and message queues.
//  - Requires a window, but that's probably why you're using
//    this class.
//
// Copyright (C) 1999-2003 Ramon de Klein
//                         (Ramon.de.Klein@ict.nl)

typedef enum _CserialMode
{
	SERIALMODE_SERIAL=0,
	SERIALMODE_MERIDIAN_7BIT_PACKET
}
CSerialMode;

//******************************************************************************
// NAME																		   *
// 	CSerialExErrReportInfo structure
//
// DESCRIPTION
//  Serial Port status information.
//
//
typedef struct  _CSerialExErrReportInfo {
		Uint32 BREAKcnt;
		Uint32 CTScnt;
		Uint32 DSRcnt;
		Uint32 ERR_FRAMEcnt;
		Uint32 ERR_OVERRUNcnt;
		Uint32 ERR_RXPARITYcnt;
		Uint32 RINGcnt;
		Uint32 RLSDcnt;
		Uint32 RXCHARcnt;
		Uint32 RXFLAGcnt;
		Uint32 RX80FUllcnt;
	// Reset value to zero
	void clear() {
		BREAKcnt=0;
		CTScnt=0;
		DSRcnt=0;
		ERR_FRAMEcnt=0;
		ERR_OVERRUNcnt=0;
		ERR_RXPARITYcnt=0;
		RINGcnt=0;
		RLSDcnt=0;
		RXCHARcnt=0;
		RXFLAGcnt=0;
		RX80FUllcnt=0;
	};
} CSerialExErrReportInfo;
//																			   *
//******************************************************************************



//******************************************************************************
// NAME																		   *
// 	rdBuf class
//
// DESCRIPTION
//  A simple serial buffer with optional "ring buffer" debug buffer. It allows
//	"bulk" updates via a block "read" function and sequential access to the
//  items via a thread safe manner.
//
	#ifdef _DEBUG
		#define DBG_RING_BUF 0			// Debug history buffer
	#else
		#define DBG_RING_BUF 0
	#endif

class rdBuf {
public:
	// Place to hold characters
	CCCriticalSection m_lock;
	char m_buffer[READ_BUF_LEN];
	size_t m_head;			// Head of data index
	size_t m_size;			// Size/tail of m_buffer contents
	#if DBG_RING_BUF
		int m_dbgPtr;
		char m_ring[READ_BUF_LEN];
	#endif
	// State lock

	// Kill the content
	void flush() {
		m_size = m_head = 0;
		#ifdef _DEBUG
			//memset(m_buffer, 0xff, READ_BUF_LEN);
		#endif
	};

	// Set the buffer size to <newSize>
	void makeAvailable(int newSize) {
		#if DBG_RING_BUF
		// Make debug copy
		char *cp = &m_buffer[m_size];
		for (int i=0; i<newSize; i++) {
			m_ring[m_dbgPtr++] = *cp++;
			m_dbgPtr &= (READ_BUF_LEN-1);
		}
		#endif
		m_size += newSize;
	};

	// Return true if we have character(s)
	bool hasChars() {
		bool rVal;
		rVal = m_head < m_size;
		return(rVal);
	};

	// Return number of characters in the buffer
	size_t charsInBuf() {
		size_t rVal;
		m_lock.Lock();
		rVal = m_size-m_head;
		m_lock.Unlock();
		return(rVal);
	};

	// Return the number of characters we can put in the buffer
	size_t charsLeft() {
		size_t rVal;
		rVal = READ_BUF_LEN - m_size;
		return(rVal);
	};

	// Get pointer to end buffer
	char *tailPtr() {
		register char *pTheChar;
		// Recycle buffer if we are empty
		pTheChar = &m_buffer[m_size];
		return(pTheChar);
	};

	// Get the next character
	bool getChar(char &theChar) {
		m_lock.Lock();
		if (hasChars()) {
			theChar = m_buffer[m_head++];
			// Reset pointers when we got last group
			if (!hasChars()) {
				flush();
			}
			m_lock.Unlock();
			return(true);
		}
		flush();
		m_lock.Unlock();
		return(false);
	};

	void lock() {
		m_lock.Lock();
	}

	void unlock() {
		m_lock.Unlock();
	}

	// Construct an empty buffer
	rdBuf() {
		flush();
		#if DBG_RING_BUF
		m_dbgPtr = 0;
		#endif
	};
	#if _DEBUG
	void dump() {
		if (m_size>0) {
			_RPT0(_CRT_WARN, "RdBuf: ");
			for (size_t i = 0; i<m_size; i++) {
				_RPT1(_CRT_WARN, "%02X_", m_buffer[i]);
			}
			_RPT0(_CRT_WARN, "\n");
		}
	}
	#endif
};
//																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
// 	CSerialEvt class
//
// DESCRIPTION
//  This is a class that implements the serial port event dispatcher. The
//	embedded thread will wait for the serial port to detect the arrival of
//	data or the receive port break condition.
//
class CSerialEx;
class CSerialEvt : protected CThread {
private:
	CSerialEx *pPort;						// Reference to our main port
	bool m_paused;							// Set if paused
	CCEvent m_parkAck;						// Signaled on initial park
public:
	CSerialEvt(CSerialEx *pOwner);
	virtual ~CSerialEvt();
	// Restart the event thread
	void Restart() {
		m_ThreadParkedEvent.SetEvent();
	}
	// Start/Stop API
	void Start();
	void Stop();
	// Wait for loop to park.
	void WaitForParked() {
		m_parkAck.WaitFor();
	}
protected:
	// Thread function
	int Run(void *context);					// Thread Control function
};

//																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
// 	CSerialEx class
//
// DESCRIPTION
//  Provides a Meridian Packet Based interface to an asynchronous serial port.
//
// 	Each opened COM-port uses its own specific thread, which will
// 	wait for one of the events to happen. When an event happens,
// 	then the client window is send a message informing about the
// 	event.
//
//
class CSerialEx : public CSerial, protected CThread
{
// Construction
public:
	CSerialEx();
	virtual ~CSerialEx();

	typedef enum _ReadStates {
		READ_STATE_IDLE=0,
		READ_STATE_LP_PAYLOAD,
		READ_STATE_HP_PAYLOAD,
	} ReadStates;

// Operations
public:
	// Open the numerically specified port for use
	#if (defined(_WIN32)||defined(_WIN64))
		virtual CSerial::SERAPI_ERR OpenComPort (long portNum);
	#else
		virtual CSerial::SERAPI_ERR OpenComPort (const char * portName);
	#endif


	// Close the serial port.
	virtual CSerial::SERAPI_ERR Close (void);

	// Connect the user's comm event to signal when packet is detected
	void RegisterUserPktCommEvent(CCEvent *pUsersEvent);

	// Operational mode: packets vs plain old serial
	void SetMode(CSerialMode theNewMode);
	CSerialMode GetMode(void);

	// Packet Interface
	bool GetPkt(packetbuf &buffer);
	bool SendPkt(packetbuf &bufferLen);
	bool IsPacketAvailable();

	// Serial Port Interface
	bool GetChar(char *theChar);
	LONG GetCharsAvailable();
	bool SendChars(char *theChars, unsigned int charsLen);
	bool Send_Available();

	// Port statistics
	void ErrorReportClear();
	void ErrorReportGet(CSerialExErrReportInfo *pErrorReport);
private:
	LONG m_nCharsRX;				// Receive character count
	LONG m_nCharsTX;				// Transmit character count
public:

	// Get and clear receive character count
	LONG ReceiveCount() {
		LONG cnt = m_nCharsRX;
		m_nCharsRX -= cnt;			// Atomic reset
		return cnt;
	}
	// Get and clear transmit character count
	LONG TransmitCount() {
		LONG cnt = m_nCharsTX;
		m_nCharsTX -= cnt;			// Atomic reset
		return cnt;
	}


protected:
	// Event handler
	friend class CSerialEvt;
	//virtual void OnEvent (EEvent eEvent, EError eError) = 0;
	virtual __inline void cpymem(nodechar *src, nodechar *dest, nodeushort nchars);
protected:
	// The WaitEvent method is being used by this class internally
	// and shouldn't be used by client applications. Client
	// application should monior the messages.
	using CSerial::CommEventWaitInitiate;

	// The event-type is send in the WPARAM of the message and
	// the GetEventType method returns the wrong data, so we'll
	// hide this method for client applications to avoid problems.
	using CSerial::GetEventType;
	using CSerial::GetError;


protected:
	// - - - - - - - - - - - - - - - - - - - - - -
	// Read Thread states and API
	// - - - - - - - - - - - - - - - - - - - - - -
	// Start the listener thread
	virtual EPort StartListener (void);

	// Stop the listener thread. Because the other thread might be
	// busy processing data it might take a while, so you can specify
	// a time-out.
	virtual LONG StopListener (DWORD dwTimeout = INFINITE);

	// Internal attributes
	CCEvent *m_pUserCommInterrupt;
	// Signalled when there is a packet received by the host.
	// This application layer waits on this.
	CCEvent m_responsePacketWaiting;

	// Packets to send up to application layer
	std::deque<packetbuf *> m_finishedPackets;

	// Overlap structures for writing
	CCEvent  m_evtWrOverlap;
	// Size of last read event
	#if _DEBUG
		DWORD m_lastReadSize, m_maxReadSize;
	#endif
	#if RECORD_PKT_DEPTH
		size_t m_maxDepth;
	#endif
	DWORD m_readSize;
	#if RECORD_PKT_DEPTH
public:
	size_t MaxDepth() {
		size_t maxD;
		maxD = m_maxDepth;
		m_maxDepth = 0;
		return maxD;
	}
	#endif
	// - - - - - - - - - - - - - - - - - - - - - -
	// Virtual overrides of CSerial
	// - - - - - - - - - - - - - - - - - - - - - -
public:
	CSerial::SERAPI_ERR Break (DWORD breakDurationMs)
	{
		m_breakSent = true;
		return(CSerial::Break(breakDurationMs));
	}

	// - - - - - - - - - - - - - - - - - - - - - -
	// Packet parser states
	// - - - - - - - - - - - - - - - - - - - - - -
private:
	bool inHighPrioState() {
		return(m_state == READ_STATE_HP_PAYLOAD);
	}
	// Low priority "next char" location
	unsigned int m_lowPrioPktIndx;
	// Accumulated checksum
	unsigned int m_lowChecksum;
	// High priority "next char" location
	unsigned int m_hiPrioPktIndx;
	// Accumulated checksum
	unsigned int m_hiChecksum;
	// Set when a break is sent, causes next Frame Error to be ignored
	bool m_breakSent;
	// WIP packets
	packetbuf m_lowInProcPacket;
	packetbuf m_hiInProcPacket;

	unsigned m_strayCount;
	bool m_AppPacketAvailable;

	ReadStates m_state;
	ReadStates m_pushedState;

	// Reset the packet parser back to "idle" state.
	void PacketParseReset() {
		m_state = m_pushedState = READ_STATE_IDLE;
		m_lowPrioPktIndx = m_hiPrioPktIndx = 0;
		m_strayCount = 0;
		ErrorReportClear();
	}

	// This function is used to process serial characters out of the
	// m_rdBuffer and signal the upper packet loop we have something.
	bool TestAndPushHiPacket(char nextChar);
	void ProcessNextChar(char nextChar);
public:
	// Reset port items and work in progress
	void Flush();

	// - - - - - - - - - - - - - - - - - - - - - -
	// Internal Packet based API and states
	// - - - - - - - - - - - - - - - - - - - - - -
	CCCriticalSection m_SendPacketToAppLock;

	// Send a packet to the application
	void SendPacketToApp(packetbuf &packet, BOOL convert7To8Bit);
	// Create an error packet and send to the application
	void SendErrToApp(mnNetErrs errorType, unsigned addr, unsigned data);
	void convert7to8(packetbuf& inBuf, packetbuf& outBuf);
	void convert8to7(packetbuf& inBuf, packetbuf& outBuf);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CThread implementation and state
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
private:
	// This event is signalled when the thread is really running
	CCEvent m_rdAck;
	// Read Buffer filled by our thread
	rdBuf m_rdBuffer;
	// Set to have the thread ignore all data
	bool m_rdAutoFlush;
	// Things discovered during operations
	CSerialExErrReportInfo m_ErrorReport;
	// We are running in packet mode, else plain old serial mode
	bool m_packetMode;
	CSerialEvt *m_pSerialEvts;

	// Thread Termination and cleanup
	void TerminateAndWait();
public:
	// Tell read thread to ignore data when mode == true.
	void AutoFlush(bool mode);
	void ProcessEvent(EEvent what);

protected:
	int Run(void *context);					// Thread Control function
};
//																			   *
//******************************************************************************

#endif	// __SERIAL_EX_H

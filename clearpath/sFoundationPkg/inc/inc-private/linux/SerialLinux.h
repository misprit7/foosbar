//*****************************************************************************
// $Workfile: SerialLinux.h $
// $Archive: /ClearPath SC/User Driver/linux/inc/SerialLinux.h $
// $Revision: 6 $ $Date: 8/29/16 3:14p $
//
//	SerialLinux - Definition of the CSerial class for Linux.
//
//	Copyright (C) 1999-2003 Ramon de Klein (Ramon.de.Klein@ict.nl)
//	(C)Copyright 2012  Teknic, Inc.  All rights reserved.
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

#ifndef __SERIALLINUX_H__
#define __SERIALLINUX_H__

#include "tekEvents.h"
#include "tekTypes.h"
#include <termios.h>

/////////////////////////////////////////////////////////////////////
// The SERIAL_DEFAULT_OVERLAPPED defines if the default open mode uses
// overlapped I/O. When overlapped I/O is available (normal Win32
// platforms) it uses overlapped I/O. Windows CE doesn't allow the use
// of overlapped I/O, so it is disabled there by default.

#ifndef SERIAL_DEFAULT_OVERLAPPED
#ifdef __cplusplus

#ifndef SERIAL_NO_OVERLAPPED
#define SERIAL_DEFAULT_OVERLAPPED	true
#else
#define SERIAL_DEFAULT_OVERLAPPED	false
#endif
#endif
#define BREAK_DURATION_MS	40			// # msec for break operation
// Invalid handle return from "open" failures
#define INVALID_HANDLE  -1

// #define SERIAL_FULL_API				// Define for full API


//////////////////////////////////////////////////////////////////////
//
// CSerial - Win32 wrapper for serial communications
//
// Serial communication often causes a lot of problems. This class
// tries to supply an easy to use interface to deal with serial
// devices.
//
// The class is actually pretty ease to use. You only need to open
// the COM-port, where you need to specify the basic serial
// communication parameters. You can also choose to setup handshaking
// and read timeout behaviour.
//
// The following serial classes are available:
//
// CSerial      - Serial communication support.
// CSerialEx    - Serial communication with listener thread for events
// CSerialSync  - Serial communication with synchronized event handler
// CSerialWnd   - Asynchronous serial support, which uses the Win32
//                message queue for event notification.
// CSerialMFC   - Preferred class to use in MFC-based GUI windows.
// 
//
// Pros:
// -----
//	- Easy to use (hides a lot of nasty Win32 stuff)
//	- Fully ANSI and Unicode aware
//
// Cons:
// -----
//  - Little less flexibility then native Win32 API, however you can
//    use this API at the same time for features which are missing
//    from this class.
//  - Incompatible with Windows 95 or Windows NT v3.51 (or earlier),
//    because CancelIo isn't support on these platforms. Define the
//	  SERIAL_NO_CANCELIO macro for support of these platforms as
//	  well. When this macro is defined, then only time-out values of
//	  0 or INFINITE are valid.
//
//
// Copyright (C) 1999-2003 Ramon de Klein
//                         (Ramon.de.Klein@ict.nl)

class CSerial
{
// Class enumerations
public:
	// These are return codes from serial API. They generally map
	// into the host OS's error code list except for the items
	// at the end of the list.
	typedef enum
	{
		API_ERROR_SUCCESS = 0,
		// Between here and API_ERROR_TIMEOUT are mapped
		// to errno.
		API_ERROR_TIMEOUT=0x40000000,
		API_ERROR_PORT_SETUP,
		API_ERROR_INVALID_HANDLE,
		API_ERROR_INVALID_ARG,
		API_ERROR_PORT_UNAVAILABLE
	}
	SERAPI_ERR;
	
	// Communication event. Bit encoded
	typedef enum
	{
		EEventNone = 	0,
		EEventBreak = 	1, 				// A break was detected on input
		EEventError = 	2, 				// A line-status error occurred
		EEventRecv = 	4, 				// Data is received on input
		EEventCTS = 	8				// CTS change
	} 
	EEvent;

	// Data bits (5-8)
	typedef enum
	{
		EDataUnknown = -1,				// Unknown
		EData5       =  CS5,			// 5 bits per byte
		EData6       =  CS6,			// 6 bits per byte
		EData7       =  CS7,			// 7 bits per byte
		EData8       =  CS8				// 8 bits per byte (default)
	}
	EDataBits;

	// Parity scheme
	typedef enum
	{
		EParUnknown = -1,				// Unknown
		EParNone    = 0,				// No parity (default)
		EParOdd     = PARENB | PARODD,	// Odd parity
		EParEven    = PARENB,			// Even parity
//		EParMark    = PARENB | PARMRK,	// Mark parity
//		EParSpace   = PARENB | CMSPAR	// Space parity
	}
	EParity;

	// Stop bits
	typedef enum
	{
		EStopUnknown = -1,				// Unknown
		EStop1       = 0,				// 1 stopbit (default)
		EStop2       = CSTOPB			// 2 stopbits
	} 
	EStopBits;

	// return constant for unknown baud rate
	static const nodeulong EBaudUnknown = 0;

	// Handshaking
	typedef enum
	{
		EHandshakeUnknown		= -1,			// Unknown
		EHandshakeOff			=  0,			// No handshaking
	} 
	EHandshake;

	// DTR
	typedef enum
	{
		EDTRClear		= 0,	// Clear - (Volt DTR = -5)
		EDTRSet			= true,	// Set - (Volt DTR = 5)
	} 
	EDTR;

	// RTS
	typedef enum
	{
		ERTSClear		= 0,	// Clear - (Volt RTS = -5)
		ERTSSet			= true,	// Set - (Volt RTS = 5)
	} 
	ERTS;

	// Timeout settings
	typedef enum
	{
		EReadTimeoutUnknown		= -1,	// Unknown
		EReadTimeoutNonblocking	=  0,	// Always return immediately
		EReadTimeoutBlocking	=  1	// Block until everything is retrieved
	}
	EReadTimeout;

	// Communication errors (bit encoded)
	typedef enum
	{
		EErrorUnknown = 0,			// Unknown
		EErrorFrame = 1,			// Framing error
		EErrorRxOver = 2,			// Input buffer overflow, byte lost
		EErrorMode = 4,
		EErrorOverrun = 8,
		EErrorBreak = 16
	}
	EError;

	// Port availability
	typedef enum
	{
		EPortUnknownError = -1,		// Unknown state
		EPortAvailable    =  0,		// Port is available
		EPortNotAvailable =  1,		// Port is not present
		EPortInUse        =  2		// Port is in use
	} 
	EPort;
	// Text if last failure
	char m_lastErrMsg[500];

// Construction
public:
	CSerial();
	virtual ~CSerial();

// Operations
public:
	// Open the serial communications for a particular COM port. You
	// need to use the full devicename (i.e. "COM1") to open the port.
	// It's possible to specify the size of the input/output queues.
	virtual SERAPI_ERR Open ( const char * portName,
						DWORD dwInQueue = 0,
						DWORD dwOutQueue = 0,
						bool fOverlapped = SERIAL_DEFAULT_OVERLAPPED);

	// Close the serial port.
	virtual SERAPI_ERR Close (void);

	// Setup the communication settings such as baudrate, databits,
	// parity and stopbits. The default settings are applied when the
	// device has been opened. Call this function if these settings do
	// not apply for your application. If you prefer to use integers
	// instead of the enumerated types then just cast the integer to
	// the required type. So the following two initializations are
	// equivalent:
	//
	//   Setup(9600,EDataBits(8),EParity(NOPARITY),EStopBits(ONESTOPBIT))UD_RATE
	//
	// In the latter case, the types are not validated. So make sure
	// that you specify the appropriate values.
	virtual SERAPI_ERR Setup (nodeulong eBaudrate = 9600,
						EDataBits eDataBits = EData8,
						EParity   eParity   = EParNone,
						EStopBits eStopBits = EStop1,
						EDTR eDTRBit = EDTRClear,
						ERTS eRTSBit = ERTSClear);


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Communications Event Interface. Provides signals upon various
	// normal serial communications events such as break condition detect.
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
protected:
	// Signaled when an unmasked event has occurred
	CCEvent m_commEvent;
	// Signaled when read data is available
	CCEvent m_rdEvent;
	// Comms Error(s) detected (bit field)
	EError m_eError;
public:
	// Set the event mask, which indicates what events should be
	// monitored. The WaitForCommEvent method can only monitor events that
	// have been enabled. The default setting only monitors the
	// error events and data events. An application may choose to
	// monitor CTS. DSR, RLSD, etc as well.
	virtual SERAPI_ERR SetEventMask (DWORD dwMask = EEventBreak|EEventError|EEventRecv|EEventCTS);
	DWORD GetEventMask   (void) {return m_dwEventMask;}

	// The WaitEvent method waits for one of the events that are
	// enabled (see SetMask).
	virtual SERAPI_ERR CommEventWaitInitiate();
	// This is the communicatins notification event, must outlive the thread
	bool WaitForCommEvent(unsigned int timeOutMS) {
		return m_commEvent.WaitFor(timeOutMS);
	}
	void ForceCommEvent() {
		m_commEvent.SetEvent();
	}

	// Determine what caused the event to trigger
	EEvent GetEventType (void);
	// Obtain the error and clear
	EError GetError (void) {
		EError errs = m_eError;
		m_eError = EErrorUnknown;
		return(errs);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Basic Port Read/Write of data
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Write ANSI data to the serial port.
	virtual SERAPI_ERR Write (const void* pData,
						size_t iLen,
						DWORD* pdwWritten = 0,
						DWORD dwTimeout = INFINITE);

	// Read data from the serial port. 
	virtual SERAPI_ERR Read (void* pData,
					   size_t iLen,
					   DWORD* pdwRead = 0,
					   DWORD dwTimeout = INFINITE);

	// Check if com-port is opened
	bool IsOpen (void) const		{ return (m_hFile != INVALID_HANDLE); }

	// Obtain last error status
	SERAPI_ERR GetLastError (void) const	{ return m_lLastError; }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Operational State and Control Line API
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Send a break
	virtual SERAPI_ERR Break (DWORD breakDurationMs);
	// Data Terminal Ready output
	bool GetDTR (void);
	void SetDTR(bool EDTRBit);
	// Request to Send output
	bool GetRTS (void);
	void SetRTS(bool ERTSBit);
	// Data Set Ready input
	bool GetDSR (void);
	// Clear-to-send input
	bool GetCTS (void);
	
	// Obtain communication settings
	virtual nodeulong  GetBaudrate    (void);

	
	
protected:

// Attributes
protected:
	SERAPI_ERR	m_lLastError;		// Last serial error
	int		m_hFile;			// File handle
	EEvent	m_eEvent;			// Event type
	DWORD	m_dwEventMask;		// Event mask
	unsigned char readBuf [256];

protected:
	// CancelIo wrapper
	SERAPI_ERR CancelCommIo (void);
	// Purge all buffers
	SERAPI_ERR Purge (void);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	//						FULL API IMPLEMENTATION							  //
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	#ifdef SERIAL_FULL_API
//	virtual BYTE       GetEventChar   (void);
	// Setup the handshaking protocol. There are three forms of
	// handshaking:
	//
	// 1) No handshaking, so data is always send even if the receiver
	//    cannot handle the data anymore. This can lead to data loss,
	//    when the sender is able to transmit data faster then the
	//    receiver can handle.
	// 2) Hardware handshaking, where the RTS/CTS lines are used to
	//    indicate if data can be sent. This mode requires that both
	//    ports and the cable support hardware handshaking. Hardware
	//    handshaking is the most reliable and efficient form of
	//    handshaking available, but is hardware dependant.
	// 3) Software handshaking, where the XON/XOFF characters are used
	//    to throttle the data. A major drawback of this method is that
	//    these characters cannot be used for data anymore.
//	virtual LONG SetupHandshaking (EHandshake eHandshake);
	// Set/clear the event character. When this byte is being received
	// on the serial port then the EEventRcvEv event is signalled,
	// when the mask has been set appropriately. If the fAdjustMask flag
	// has been set, then the event mask is automatically adjusted.
//	virtual LONG SetEventChar (BYTE bEventChar, bool fAdjustMask = true);

	// Obtain communication settings
//	virtual EDataBits  GetDataBits    (void);
//	virtual EParity    GetParity      (void);
//	virtual EStopBits  GetStopBits    (void);
//	virtual EHandshake GetHandshaking (void);
	// Obtain CTS/DSR/RING/RLSD settings
	bool GetCTS (void);
	bool GetDSR (void);
//	bool GetRing (void);
//	bool GetRLSD (void);
	#endif
};
#endif	// Cplusplus

#endif	// __SERIAL_H


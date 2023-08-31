//*****************************************************************************
// $Workfile: SerialLinux.cpp $
// $Archive: /ClearPath SC-1.0.123/User Driver/linux/src/SerialLinux.cpp $
// $Revision: 13 $ $Date: 12/09/16 4:43p $
//
//	SerialLinux.cpp - Implementation of the CSerial class for QNX.
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


//////////////////////////////////////////////////////////////////////
// Include module headerfile

#include "SerialEx.h"
#include "SerialLinux.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>

#ifdef _DEBUG
#define T_ON TRUE
#else
#define T_ON FALSE
#endif
#define T_OFF FALSE
#if !defined(TRACE_THREAD)
#define TRACE_THREAD		T_OFF	// Print thread create/exit IDs
#endif
#define TRACE_HIGH_LEVEL	T_OFF	// Print high level debug text
#define TRACE_LOW_LEVEL		T_OFF	// Print low level debug test

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif



// *************** Prototype *********
// Returns the baud rate to set baud rate via OS function calls
// Argument 1 is a long that specifies the bit/second (e.g. 9600)
bool OSBaudRate(nodeulong eBaudrate, speed_t *theSpeed);

// Time stamper
extern "C" MN_EXPORT double MN_DECL infcCoreTime(void);

//*****************************************************************************
//	NAME																	  *
//		CSerial::breakSigHandler
//
//	DESCRIPTION:
/**
 *	Register the detection of a serial line break condition.
 *
 *	\return TRUE on success
**/
//	SYNOPSIS:
void sigHandler(int signo, siginfo_t *info, void *context) {
	switch (signo) {
	case SIGINT:
		break;
	default:
		// We don't care about unprocessed signals
		break;
	}
}

//*****************************************************************************
//	NAME																	  *
//		CSerial::CancelCommIo
//
//	AUTHOR:
//		Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Cancel pending serial I/O and flush all current data.
 *
 *	\return TRUE on success
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::CancelCommIo (void)
{
	SERAPI_ERR retErr;
	// Cancel pending I/O
	// ... TODO?
	// Kill buffers
	retErr = Purge();
	return (retErr);
}
//
//****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerial::Purge
//
//	AUTHOR:
//		Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Purge all of our hardware and operating system buffers.
 *
 *	\return TRUE on success
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Purge (void)
{
	if (tcflush(m_hFile, TCIOFLUSH))
		return (CSerial::SERAPI_ERR(errno));
	return(API_ERROR_SUCCESS);
}
//
//****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CSerial::CSerial
//
//	AUTHOR:
//		Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Generic asynchronous threaded serial port access for use on various
 *	operating systems.
**/
//	SYNOPSIS:
CSerial::CSerial ()
	: m_lLastError(API_ERROR_SUCCESS)
	, m_hFile(INVALID_HANDLE)
	, m_eEvent(EEventNone)
	, m_dwEventMask(0)
{
	#if TRACE_THREAD
		_RPTF0(_CRT_WARN,"CSerial::CSerial (created)\n");
	#endif
}

CSerial::~CSerial ()
{
	// Close implicitly
	if (m_hFile != INVALID_HANDLE)
		Close();
	#if TRACE_THREAD
		_RPTF0(_CRT_WARN,"CSerial::~CSerial (destroyed)\n");
	#endif
}
//
//****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CSerial::CSerial
//
//	AUTHOR:
//		Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Generic asynchronous threaded serial port access for use on various
 *	operating systems.
 *
 *	\param portName[in] Pointer to serial port path.
 *	\param dwInQueue[in] Suggested input queue size
 *	\param dwOutQueue[in] Suggested output queue size
 *	\param fOverlapped[in] Performed overlapped I/O if possible
 *	\return API_ERROR_SUCCESS(0) on success
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Open (
	const char * portName,
	DWORD dwInQueue,
	DWORD dwOutQueue,
	bool fOverlapped) {

	struct termios tio;
	speed_t theSpeed;
	int theErr;
#if TRACE_HIGH_LEVEL
	_RPT1(_CRT_WARN, "CSerial::Open serial port %s\n", portName);
#endif
	// definition of signal action
#if 0
	struct sigaction saio;
	saio.sa_sigaction = sigHandler;
	saio.sa_flags = SA_SIGINFO;
	// saio.sa_mask = 0;
	saio.sa_flags = 0;
//MD_TODO	saio.sa_restorer = NULL;
//	sigaction(SIGINT, &saio, NULL);	  // 5/29/12 DS ?? TODO will this catch break if we can cancel action
#endif

	//set the user console port up
	//m_hFile = open( portName, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
	m_hFile = open( portName, O_RDWR | O_NOCTTY | O_NONBLOCK );
	if (m_hFile == INVALID_HANDLE) {
		m_lLastError = (CSerial::SERAPI_ERR)errno;
		snprintf(m_lastErrMsg, sizeof(m_lastErrMsg),
				"open %s failed, errno=%d\n", portName, m_lLastError);
		fputs(m_lastErrMsg, stderr);
		return(API_ERROR_INVALID_HANDLE);
	}
	//_RPT1(_CRT_WARN, "opened m_hFile: %d\n", m_hFile);
	// set new port settings for non-canonical input processing  //must be NOCTTY
    tcgetattr(m_hFile, &tio);
    cfmakeraw(&tio);
//    // No RTS/CTS handshake, no hang-up on break, 8 bit, no parity
//    tio.c_cflag &= ~(CRTSCTS);
//    tio.c_cflag |= CS8 | CLOCAL | CREAD;
//
//    // No output processing
//	tio.c_oflag = 0;
//
//	// Setup raw
//	tio.c_lflag = 0;		//ICANON;
//
//	// Input options: flow control off, no parity check, ignore break
//	tio.c_iflag &= ~(IXON | IXOFF | IXANY | BRKINT | INPCK);
//	tio.c_iflag |= IGNBRK;

    theErr=OSBaudRate(9600, &theSpeed);
    if(!theErr) {                           // If invalid baud rate, return error
        m_lLastError=API_ERROR_PORT_SETUP;
        return API_ERROR_PORT_SETUP;
    }
	cfsetispeed(&tio, theSpeed);  // Input BAUD Rate = 9600
	cfsetospeed(&tio, theSpeed);  // Output BAUD Rate = 9600
	// Wait for one character minimally to wake up
	tio.c_cc[VMIN]= 1;
	// No delay until we see char
	tio.c_cc[VTIME]= 0;

	if (tcsetattr( m_hFile, TCSANOW, &tio ) == -1) {
		_RPT0(_CRT_WARN, "serial port setup issue!\n" );
		printf("tcsetattr failure, code %d\n",errno);
		return(API_ERROR_PORT_SETUP);
	}
//    // Make sure flow control is defeated
//    if(tcflow(m_hFile, TCOON | TCION)) {
//    	fprintf(stderr, "Serial::Open flow control off failed, %d\n", errno);
//    }
	// Reset CON-MOD loopback
	SetDTR( false );
	SetRTS( false );
	usleep( 100000 );
	// Clear any character that leaked in before changes
	Purge();
	// OK, return
	return(API_ERROR_SUCCESS);
}
//
//****************************************************************************

//****************************************************************************
//	NAME																	 *
//		CSerial::Close
//
//	AUTHOR:
//		Refactored from Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
	Shutdown the serial port and release to the operating system.

	\return API_ERROR_SUCCESS(0) on success
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Close (void) {

	// Reset error state
	m_lLastError = API_ERROR_SUCCESS;

	// If the device is already closed,
	// then we don't need to do anything.
	if (m_hFile == INVALID_HANDLE)
	{
		// Display a warning
		//_RPTF0(_CRT_WARN,"CSerial::Close - Method called when device is not open\n");
		return API_ERROR_SUCCESS;
	}
	// Insure all open work has stopped
	CancelCommIo();
	// Yield quanta now to let it happen
	usleep(10000);

	// Close COM port
//	#if TRACE_HIGH_LEVEL
//		_RPTF1(_CRT_WARN, "CSerial::Close port = hndl=0x%x\n", m_hFile);
//	#endif
	close( m_hFile );
	m_hFile = INVALID_HANDLE;

	// Return successful
	return API_ERROR_SUCCESS;
}
//
//****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CSerial::Setup
//
//	DESCRIPTION:
/**
 *	Setup the serial port's speed in bits/second (baud rate)
 *
 *	\param eBaudrate[in] Send/Receive speed in bits/second.
 *
 *	\return API_ERROR_SUCCESS(0) on success
**/
//	SYNOPSIS:
bool OSBaudRate(nodeulong eBaudrate, speed_t *theSpeed) {

    switch(eBaudrate){
        case 9600:      // Common Rate
            *theSpeed=B9600;
            break;
        case 19200:
            *theSpeed=B19200;
            break;
        case 38400:
            *theSpeed=B38400;
            break;
        case 57600:
            *theSpeed=B57600;
            break;
        case 115200:
            *theSpeed=B115200;    // Common
            break;
        case 230400:
            *theSpeed=B230400;   // Common
            break;
        case 460800:
            *theSpeed=B460800;
            break;
        case 921600:
            *theSpeed=B921600;
            break;
        case 1152000:
            *theSpeed=B1152000;
            break;
        default:
            return false;
            break;
    }

    // Speed was set successfully
    return true;

}


//*****************************************************************************
//	NAME																	  *
//		CSerial::Setup
//
//	AUTHOR:
//		Refactored from Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Setup the serial port's speed and other attributes.
 *
 *	\param eBaudrate[in] Send/Receive speed in bits/second.
 *	\param eDataBits[in] Data size, usually EData8
 *	\param eParity[in] Parity generation/checking, usually EParNone
 *	\param eStopBits[in] Stop bits, usually EStop1
 *	\param eDTRBit[in] DTR output assertion, usually EDTRSet
 *
 *	\return API_ERROR_SUCCESS(0) on success
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Setup (
		nodeulong eBaudrate,
		EDataBits eDataBits,
		EParity eParity,
		EStopBits eStopBits,
		EDTR eDTRBit,
		ERTS eRTSBit) {
    speed_t theSpeed;
    int theErr;

	// Reset error state
	m_lLastError = API_ERROR_SUCCESS;

	// Check if the device is open
	if (m_hFile == INVALID_HANDLE)
	{
		// Set the internal error code
		m_lLastError = API_ERROR_INVALID_HANDLE;

		// Issue an error and quit
		_RPTF0(_CRT_WARN,"CSerial::Setup - Device is not opened\n");
		return API_ERROR_INVALID_HANDLE;
	}
#if TRACE_HIGH_LEVEL
	_RPT1(_CRT_WARN, "CSerial::Setup baud=%d\n", eBaudrate);
#endif
	struct termios my_termios;

	m_lLastError = (CSerial::SERAPI_ERR)tcgetattr( m_hFile, &my_termios );
	if (m_lLastError)
		return API_ERROR_PORT_SETUP;
	my_termios.c_cflag &= ~CSIZE;
	my_termios.c_cflag |= eDataBits;
	my_termios.c_cflag &= ~EParOdd;
	my_termios.c_cflag |= eParity;
	my_termios.c_cflag &= ~CSTOPB;
	my_termios.c_cflag |= eStopBits;
	my_termios.c_iflag |= BRKINT;

    theErr=OSBaudRate(eBaudrate, &theSpeed);
    if(!theErr) {                           // If invalid baud rate, return error
        m_lLastError=API_ERROR_PORT_SETUP;
        return API_ERROR_PORT_SETUP;
    }

    theErr=cfsetispeed(&my_termios, theSpeed);
    if(theErr!=0){
        m_lLastError=API_ERROR_PORT_SETUP;
    }

	theErr=cfsetospeed(&my_termios, theSpeed);
    if(theErr!=0){
        m_lLastError=API_ERROR_PORT_SETUP;
    }

	m_lLastError = (CSerial::SERAPI_ERR)tcsetattr( m_hFile, TCSANOW, &my_termios );
	if (m_lLastError)
		return API_ERROR_PORT_SETUP;
	
	// Make sure DTR is set properly
	SetDTR(eDTRBit);
	// Make sure DTR is set properly
	SetRTS(eRTSBit);
	// Return successful
	return API_ERROR_SUCCESS;
}
//
//****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CSerial::SetEventMask
//
//	AUTHOR:
//		Refactored from Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Setup the serial port's speed and other attributes.
 *
 *	\param dwEventMask[in] Bit mask of events to watch for.
 *
 *	\return API_ERROR_SUCCESS(0) on success
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::SetEventMask (DWORD dwEventMask) {

	// Reset error state
	m_lLastError = API_ERROR_SUCCESS;

	// Check if the device is open
	if (m_hFile == INVALID_HANDLE) {
		// Set the internal error code
		m_lLastError = API_ERROR_INVALID_HANDLE;

		// Issue an error and quit
		_RPTF0(_CRT_WARN,"CSerial::SetEventMask - Device is not opened\n");
		return API_ERROR_INVALID_HANDLE;
	}

	// Set the new mask. Note that this will generate an EEventNone
	// if there is an asynchronous WaitCommEvent pending.




	// Save event mask and return successful
	m_dwEventMask = dwEventMask;
	return API_ERROR_SUCCESS;
}
//
//****************************************************************************


//*****************************************************************************
//	NAME
//		CSerial::CommWaitEventInitiate
//
//	DESCRIPTION:
/**
	Initiate the wait for the specified serial communications event(s).

	This could be implemented via polling or other operating system
	event support for responsive reactions.

	Note: For POSIX style platforms, only the break is detected (TODO)

	\return API_ERROR_SUCCESS(0) of OK, else an error code.
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::CommEventWaitInitiate() {
	// TODO: Find and post events in m_eEvent
	usleep(100000);
	return API_ERROR_SUCCESS;
}
//
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CSerial::GetEventType
//
//	AUTHOR:
//		Refactored from Ramon de Klein (Ramon.de.Klein@ict.nl)
//
//	DESCRIPTION:
/**
 *	Return the events detected since last inquiry.
 *
  *	\return EEventNone if none, else a bit encoded result.
**/
CSerial::EEvent CSerial::GetEventType (void)
{

	// Obtain the event (mask unwanted events out)
	EEvent eEvent = m_eEvent;

	// Reset internal event type
	m_eEvent = EEventNone;

	// Return the current cause
	return eEvent;
}
//
//****************************************************************************


//*****************************************************************************
//	NAME
//		CSerial Write
//
//	DESCRIPTION:
///		Write data to port.
//
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Write (const void* pData, size_t iLen, DWORD* pdwWritten, DWORD dwTimeoutMS) {
	// Reset error state
	m_lLastError = API_ERROR_SUCCESS;

	// Reset the number of bytes written
	*pdwWritten = 0;

	// Check if the device is open
	if (m_hFile == INVALID_HANDLE) {
		// Set the internal error code
		m_lLastError = API_ERROR_INVALID_HANDLE;

		// Issue an error and quit
		_RPTF0(_CRT_WARN,"CSerial::Write - Device is not opened\n");
		return API_ERROR_INVALID_HANDLE;
	}
	ssize_t ret = write( m_hFile, pData, iLen );
	if ( ret < 0 ) {
		// Set the internal error code
	 	m_lLastError = (CSerial::SERAPI_ERR)errno;
	 	snprintf(m_lastErrMsg, sizeof(m_lastErrMsg),
		// Issue an error and quit
	 			"CSerial::Write - Unable to write the data err=%d\n", (int)m_lLastError);
	 	_RPTF1(_CRT_WARN, "%s", m_lastErrMsg);
		return (CSerial::SERAPI_ERR)errno;
	}
	else {
		*pdwWritten = ret;
//		unsigned char * prt = (unsigned char *)pData;
//		printf( "write: " );
//		for ( int ix = 0; ix < iLen; ++ix ) printf( "%X ", prt[ix] );
//		printf( "\n" );
	}

	// Return successfully
	return API_ERROR_SUCCESS;
}
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerialEx::Read
//
//	DESCRIPTION:
/**
	Read the current unprocessed characters from the serial port with
	time-out.

 	\param pData[out] Buffer to write the octets received by the serial
 	port to.
 	\param iLen[in] Maximum number of characters to read this call.
 	\param pdwRead[out] The number of characters actually read into
 	\a pData.
 	\param dwTimeoutMS[in] Maximum time to wait for characters to
 	arrive.

	\return API_ERROR_SUCCESS(0) if read was successful and \a pData was
	filled with \a pdwRead characters.
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Read (
	void* pData,
	size_t iLen,
	DWORD* pdwRead,
	DWORD dwTimeoutMS)
{
	// Reset error state
	m_lLastError = API_ERROR_SUCCESS;

	// Reset the number of bytes read
	*pdwRead = 0;

	// Check if the device is open
	if (m_hFile == INVALID_HANDLE) {
		// Set the internal error code
		m_lLastError = API_ERROR_INVALID_HANDLE;
		// Issue an error and quit
		_RPTF0(_CRT_WARN,"CSerial::Read - Device is not opened\n");
		return API_ERROR_INVALID_HANDLE;
	}

//	printf("about to read serial port\n");
    fd_set r_fds;
	FD_ZERO(&r_fds);
	FD_SET( m_hFile, &r_fds );
	ssize_t ret = -1;
	if ( dwTimeoutMS == INFINITE ) {
		ret = select( m_hFile + 1, &r_fds, NULL, NULL, NULL );
	} else {
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = dwTimeoutMS * 1000;
		ret = select( m_hFile + 1, &r_fds, NULL, NULL, &tv );
	}

	if ( ret && FD_ISSET( m_hFile, &r_fds ) ) {
		ret = read( m_hFile, pData, iLen );
		if ( ret == -1 ) {
		 	m_lLastError = (CSerial::SERAPI_ERR)errno; 		// Set the internal error code
//			_RPTF1(_CRT_WARN, "CSerial::Read - Unable to read the data err=%d\n", (int)m_lLastError);
			return (CSerial::SERAPI_ERR)errno;
		} else if ( ret >= 0 ) {
			*pdwRead = ret;
		}
	} else if ( ret == 0 ) {
		m_lLastError = API_ERROR_TIMEOUT;
		return API_ERROR_TIMEOUT;
		// Allow release of waiters
		m_rdEvent.SetEvent();	//5/29/12 DS Do we really need this?
	} else {
	 	m_lLastError = (CSerial::SERAPI_ERR)errno; 		// Set the internal error code
	 	snprintf(m_lastErrMsg, sizeof(m_lastErrMsg),
	 			"CSerial::Read - Unable to read the data err=%d\n", (int)m_lLastError);
		fputs(m_lastErrMsg, stderr);
		return (CSerial::SERAPI_ERR)errno;
	}
	return API_ERROR_SUCCESS;
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::Break
//
//	DESCRIPTION:
/**
	Send the break condition for the specified time.

	\param breakDurationMs[in] Duration in milliseconds.

	\return API_ERROR_SUCCESS(0) if performed.
**/
//	SYNOPSIS:
CSerial::SERAPI_ERR CSerial::Break ( DWORD breakDurationMs)
{
	// Reset error state
	m_lLastError = API_ERROR_SUCCESS;

	// Check if the device is open
	if (m_hFile == INVALID_HANDLE)
	{
		// Set the internal error code
		m_lLastError = API_ERROR_INVALID_HANDLE;

		// Issue an error and quit
		_RPTF0(_CRT_WARN,"CSerial::Break - Device is not opened\n");
		return API_ERROR_INVALID_HANDLE;
	}
#if TRACE_HIGH_LEVEL
	fprintf(stderr, "%.1f CSerial::Break about to send\n", infcCoreTime() );
#endif
	// Set the RS-232 port in break mode for a little while
	if (tcsendbreak(m_hFile, breakDurationMs)) {
		m_lLastError = (CSerial::SERAPI_ERR)errno;
		fprintf(stderr, "%.1f CSerial::Break failed, err=%d\n",
				infcCoreTime(), m_lLastError);
		return (CSerial::SERAPI_ERR)m_lLastError;
	}
	// TODO: Assume we get our break back (no good POSIX way
	// to detect in C++). The subsequent time-out will keep us
	// offline. Uses serial event thread to signal back.
	m_eEvent = EEvent(EEventBreak|EEventError);
	m_eError = EErrorBreak;
	//struct termios options;
	//tcgetattr(m_hFile, &options);
	//tcsetattr(m_hFile, TCSADRAIN, &options);
#if TRACE_HIGH_LEVEL
	fprintf(stderr, "%.1f CSerial::Break finished\n", infcCoreTime() );
#endif
	ForceCommEvent();

	// Return successfully
	return API_ERROR_SUCCESS;
}
//
//*****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::GetDSR
//
//	DESCRIPTION:
/**
	Return the last known state of the DSR line.

	\return true if asserted
**/
//	SYNOPSIS:
bool CSerial::GetDSR(void)
{
	unsigned long int status;
	if (ioctl(m_hFile, TIOCMGET, &status) == -1)
		return false;
	return (status & TIOCM_DSR) != 0;
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::GetDTR
//
//	DESCRIPTION:
/**
	Return the last known state of the DTR line.

	\return true if asserted
**/
//	SYNOPSIS:
bool CSerial::GetDTR(void)
{
	unsigned long int status;
	if (ioctl(m_hFile, TIOCMGET, &status) == -1)
		return false;
	return (status & TIOCM_DTR) != 0;
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::SetDTR
//
//	DESCRIPTION:
/**
	Return the last known state of the DTR line.

	\param EDTRBit state to assert.
**/
//	SYNOPSIS:
void CSerial::SetDTR(bool EDTRBit)
{
	int iFlags;
	if (ioctl (m_hFile, TIOCMGET, &iFlags) == -1)
		return;
	if(EDTRBit) {
		iFlags |= TIOCM_DTR;
	}
	else {
		iFlags &= ~TIOCM_DTR;
	}
	ioctl( m_hFile, TIOCMSET, &iFlags );
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::GetRTS
//
//	DESCRIPTION:
/**
	Return the last known state of the RTS line.

	\return true if asserted
**/
//	SYNOPSIS:
bool CSerial::GetRTS(void)
{
	int iFlags;
	if (ioctl (m_hFile, TIOCMGET, &iFlags) == -1)
		return false;

	return (iFlags & TIOCM_RTS) != 0;
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::SetRTS
//
//	DESCRIPTION:
/**
	Return the last known state of the RTS line.

	\param ERTSBit state to assert.
**/
//	SYNOPSIS:
void CSerial::SetRTS(bool ERTSBit)
{
	int iFlags;
	if (ioctl (m_hFile, TIOCMGET, &iFlags) == -1)
		return;

	if(ERTSBit) {
		iFlags |= TIOCM_RTS;
	}
	else {
		iFlags &= ~TIOCM_RTS;
	}
	ioctl( m_hFile, TIOCMSET, &iFlags );
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerial::GetCTS
//
//	DESCRIPTION:
/**
	Return the state of the CTS line.

	\return true if asserted
**/
//	SYNOPSIS:
bool CSerial::GetCTS(void)
{
	int iFlags;
	if (ioctl (m_hFile, TIOCMGET, &iFlags) == -1)
		return false;
	
	return (iFlags & TIOCM_CTS) != 0;
}
//
//****************************************************************************

//*****************************************************************************
//	NAME
//		CSerialEx::GetBaudrate
//
//	DESCRIPTION:
/**
	Get the port's current baud rate in bits/seconds.

	\return Port speed in bits/second. EBaudUnknown if unknown.
**/
//	SYNOPSIS:
nodeulong  CSerial::GetBaudrate(void)
{
	struct termios my_termios;

	if (tcgetattr( m_hFile, &my_termios )) {
		m_lLastError = (CSerial::SERAPI_ERR)errno;
		return(EBaudUnknown);
	}
	// Note: ispeed and ospeed are always identical in our driver
	return(nodeulong(my_termios.c_ispeed));
}
//
//****************************************************************************


//*****************************************************************************
//*****************************************************************************
//                       Platform Dependent CSerialEx
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CSerialEx::GetCharsAvailable
//
//	DESCRIPTION:
///		Return the count of characters available for reading.
///
///		\return Number of unprocessed characters.
///
/// 	Detailed description.
//
//	SYNOPSIS:
LONG CSerialEx::GetCharsAvailable(void)
{
	int bytes = 0;
	if (m_hFile != INVALID_HANDLE)
		ioctl(m_hFile, FIONREAD, &bytes);
	return(LONG(bytes));
}

//																			  *
//*****************************************************************************




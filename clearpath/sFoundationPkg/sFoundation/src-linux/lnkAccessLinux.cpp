//******************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/linux/src/lnkAccessLinux.cpp $
// $Revision: 8 $ $Date: 12/09/16 5:00p $
// $Workfile: lnkAccessLinux.cpp $
//
// DESCRIPTION:
/**
	\file
	\defgroup ChanGrp Channel Access Functions
	\brief Channel Access API for Linux

	This implements the function library portion of the driver suite for
	operating system dependent features.

**/
// CREATION DATE:
//	02/11/1998 16:34:00
//  06/09/2009 17:39 Refactored from ControlPoint implementation
//
// COPYRIGHT NOTICE:
//	(C)Copyright 1998-2012  Teknic, Inc.  All rights reserved.
//
//	This copyright notice must be reproduced in any copy, modification,
//	or portion thereof merged into another program. A copy of the
//	copyright notice must be included in the object library of a user
//	program.
// 																			   *
//******************************************************************************



//******************************************************************************
// NAME																		   *
// 	lnkAccessLinux.cpp headers
//
	// Our driver headers
	#include "lnkAccessCommon.h"
	#include "mnErrors.h"
	#include "sFoundResource.h"
	#include "SerialEx.h"

	#include <errno.h>
	#include <stdio.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/syscall.h>
	#include <time.h>
	#include <inttypes.h>
	#include <wchar.h>
	#include "ErrCodeStr.hpp"

	#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
	#endif
	#include <dlfcn.h>
	#include <libgen.h>
// 																			   *
//******************************************************************************



//******************************************************************************
// NAME																		   *
// 	lnkAccessLinux.cpp constants
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
// 	lnkAccessLinux.cpp imported information
//
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
// 	lnkAccessLinux.cpp function prototypes
//
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
// 	lnkAccessLinux.cpp static variables
//
ErrCodeStr ErrorLookups;
// 																			   *
//******************************************************************************



//******************************************************************************
//	NAME																	   *
//		infcCoreTime
//
//	CREATION DATE:
//		11/17/1998 21:13:16
//
//	DESCRIPTION:
///		Return the high precision time value with at least tenth of millisecond
///		resolution.	This is used to time stamp the log files and provide
///		time-out facilities.
///
///		\return double count in milliseconds
//
//	SYNOPSIS:
MN_EXPORT double MN_DECL infcCoreTime(void)
{
#if 1
	static double initial = 0;
	struct timespec now;
	if (clock_gettime(CLOCK_REALTIME, &now) == -1)
		return(0);
	double tNow = double(now.tv_sec)*1000 + double(now.tv_nsec)*1e-6-initial;
	if (!initial)
		initial = tNow;

	//clock_t now = clock();
	//double tNow = double(now/(CLOCKS_PER_SEC*.001));
	return(tNow);
#else
	static Uint64 initial = 0;
	Uint64 tNow = ClockCycles()-initial;
	// Reset time at first request
	if (!initial)
		initial = tNow;
	return(1000*double(tNow)/ SYSPAGE_ENTRY(qtime)->cycles_per_sec);
#endif
}
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
//		infcVersion
//
//	CREATION DATE:
//		01/26/2001 10:38:58
//
//	DESCRIPTION:
/**
	Return the DLL driver revision code in 8.8.16 format.

	\return	unsigned 8.8.16 format. \b major . \b minor . \b revision
**/
//	SYNOPSIS:
MN_EXPORT nodeulong MN_DECL infcVersion(void)
{
	return(VER_FILEVERSION_DW);
}
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
//		infcGetDumpDir
//
//	CREATION DATE:
//		03/10/2010 10:15:25
//
//	DESCRIPTION:
/**
	Return the directory where automatic dump files are created with
	trailing directory delimitor.  This will be "%TEMP%\Teknic\" for Windows.

 	\param[out] pStr Ptr to string to be returned.
	\param[in] maxLen Size of \e pStr area.
**/
//	SYNOPSIS:
void infcGetDumpDir(
	char *pStr,						// Ptr to string area
	nodelong maxLen)
{
	// Create a dump file in the temp directory
	if (mkdir("/tmp/Teknic", 0777) != 0){
        if(errno != EEXIST)
            fprintf(stderr, "ERROR %d: unable to mkdir; %s\n",
                    errno, strerror(errno));
	}
	strncpy(pStr, "/tmp/Teknic/", maxLen);
}
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
//		infcErrCodeStrA
//
// 	CREATION DATE:
//      01/15/2004 14:49:41
//
// 	DESCRIPTION:
/**
    Return the descriptive ANSI string for the selected cnErrCode.

	\param[in] lookupCode Error code number to lookup.
	\param[in] maxLen Maximum size of return string
	\param[out] resultStr Ptr to the string result to return

	\return MN_OK if succeed and \p resultStr updated.
**/
//      SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcErrCodeStrA(
        cnErrCode lookupCode,
        Uint16 maxLen,
        char resultStr[])
{
	int nChars;
	switch(lookupCode) {
	case MN_OK:
		strncpy(resultStr, "OK", maxLen);
		// Insure null
		resultStr[maxLen-1] = 0;
		nChars = strlen(resultStr);
		break;
	case MN_ERR_OS:
		break;
	default:
		if (lookupCode >= MN_ERR_BASE && lookupCode < MN_ERR_BASE+0xfff) {
			char toStringBuf[10];
			// Create "key"
			snprintf(toStringBuf, sizeof(toStringBuf), "%ld",
					lookupCode - MN_ERR_BASE + STR_ERR_BASE);
			char *errString = ErrorLookups.lookup(toStringBuf);
			// Safe copy to user
			strncpy(resultStr, errString, maxLen);
			nChars = strlen(resultStr);
		}
		else {
			nChars = snprintf(resultStr, maxLen, "Error: 0x%x", lookupCode);
		}
		break;
	}
	// Warn if the buffer is too small
	return(nChars<maxLen ? MN_OK : MN_ERR_BADARG);
}
// 																			   *
//******************************************************************************

//******************************************************************************
// NAME																		   *
//		infcHeapCheck
//
// 	CREATION DATE:
//      03-26-2012
//
// 	DESCRIPTION:
/**
    Debugging aid for checking heap corruption at various points within
    the driver.

**/
//      SYNOPSIS:
#pragma GCC diagnostic ignored "-Wunused-parameter"
MN_EXPORT void infcHeapCheck(const char *msg)
{
	// TODO
}
// 																			   *
//******************************************************************************


//****************************************************************************
//	NAME																	 *
//		infcFilenameA
//
//	DESCRIPTION:
//		Update the ANSI <fname> string up to <len> chars with the file name of
//		this DLL.
//
//	CREATION DATE:
//		09/23/2004 16:02:36
//
//	RETURNS:
//		fname updated
//
//	SYNOPSIS:
MN_EXPORT void MN_DECL infcFileNameA(char *fname, long len)
{
	Dl_info dl_info;
    dladdr((void *)infcFileNameA, &dl_info);
    strncpy(fname, dl_info.dli_fname, len);
}
//																			 *
//****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		infcGetHubPorts
//
//	DESCRIPTION:
///		return the SC Hub USB port adapter name(s)
///
/// 
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetHubPorts(std::vector<std::string> &comHubPorts)
{

	comHubPorts.clear();
	FILE *pfd = popen( "ls /dev/ttyXRUSB*", "r" );

    if ( pfd <= 0 )
    {
      throw std::runtime_error( "Command or process could not be executed." );
    }

    while ( !feof(pfd) )
    {
      char buf[ 1024 ] = {0};

      if ( fgets(buf, sizeof(buf), pfd) > 0 )
      {
        std::string str(buf);
        // TODO: check the VID/PID of the device using udevadm/libudev to
        // verify that this is a Teknic SC4-Hub (vid=2890, pid=0213)
        comHubPorts.push_back(str.substr(0, str.length()-1));
      }
    }
    pclose( pfd );

    return MN_OK;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcGetPortInfo
//
//	CREATION DATE:
//		2/8/2011 08:45:35
//
//	DESCRIPTION:
///		return the port adapter name and manufacturer from the registry
///
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT cnErrCode MN_DECL infcGetPortInfo(
		const char *portName,
		serPortInfo *portInfo)
{
	return(MN_ERR_NOT_IMPL);
}
//																			 *
//****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcDbgDepth
//
//	CREATION DATE:
//		2/8/2011 08:45:35
//
//	DESCRIPTION:
///		return the port adapter name and manufacturer from the registry
///
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT nodelong MN_DECL infcDbgDepth(netaddr cNum) {
	extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];
	if (cNum > NET_CONTROLLER_MAX)
		return(0);
	netStateInfo *pNCS = SysInventory[cNum].pNCS;
	if (pNCS)
		return(pNCS->pSerialPort->MaxDepth());
	return(0);
}
//																			 *
//****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		infcThreadID
//
//	CREATION DATE:
//		6/23/2016 12:04
//
//	DESCRIPTION:
///		Return the thread ID of the currently running thread.
///
///
/// 	Detailed description.
//
//	SYNOPSIS:
MN_EXPORT Uint64 MN_DECL infcThreadID() {
	return (Uint64)syscall(SYS_gettid);
}
//																			 *
//****************************************************************************


//******************************************************************************
// NAME																		   *
//		_controllerSpec::_controllerSpec
//
// 	CREATION DATE:
//      03-26-2012
//
// 	DESCRIPTION:
/**
    Create a controller specification record.

    \param[in] Ptr to serial port device path.
    \param[in] Speed to run port at.

**/
//      SYNOPSIS:
_controllerSpec::_controllerSpec(const char * name, netRates rate) {
	strncpy(PortName, name, MAX_PATH);
	PortRate = rate;
}
// 																			   *
//******************************************************************************


//******************************************************************************
// NAME																		   *
//		ErrCodeStr::ErrCodeStr()
//
// 	CREATION DATE:
//      01-25-2017
//
// 	DESCRIPTION:
/**
    Load the XML error code list at construction time. It will be
    located were this shared object file is located.

**/
//      SYNOPSIS:
ErrCodeStr::ErrCodeStr() {
	char pathLoc[MAX_PATH];
	infcFileNameA(pathLoc, sizeof(pathLoc));
	dirname(pathLoc);
	strncat(pathLoc, LNK_ACCESS_XML_ERR_TXT, sizeof(pathLoc));
	//fprintf(stderr, "In ErrCodeStr constructor err XML=%s\n", pathLoc);
	this->load(pathLoc);
}
// 																			   *
//******************************************************************************


//==============================================================================
//	END OF FILE lnkAccessLinux.cpp
//==============================================================================

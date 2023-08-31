//*****************************************************************************
// $Archive: /ClearPath SC/LinuxPrjEclipse/sFoundation20/linux/src/tekEventsLinux.cpp $
// $Revision: 4 $ $Date: 9/19/16 12:11 $
// $Workfile: tekEventsLinux.cpp $
//
// DESCRIPTION:
/**
	\file
	\brief  Implementation module for a portable event based library for
	the Linux platform.
**/
// PRINCIPLE AUTHORS:
//		Dave Sewhuk
//
// CREATION DATE:
//		2012-03-27 13:38:27
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2012  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	tekEventsLinux.cpp headers
//
	#include "tekEventsLinux.h"
	#include <assert.h>
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	tekEventsLinux.cpp function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	tekEventsLinux.cpp constants
//
//
const size_t MAXIMUM_WAIT_OBJECTS = 64;
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	tekEventsLinux.cpp static variables
//
//

//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CCMTMultiLock::CCMTMultiLock
//
//	AUTHOR:
//		Dave Sewhuk - created on 2012-03-27 13:42:31
//
//	DESCRIPTION:
/**
	Wait until all the events in the list signal when unlocking.

	This version can only work with CCEvents and provides a subset
	of the Windows functionality of the WaitForMultipleObjects
	API function.

 	\param[in,out] xxx description
	\return description

**/
//	SYNOPSIS:
CCMTMultiLock::CCMTMultiLock(CCEvent *pObjects[],Uint32 dwCount,
		          Uint32 UnlockCount, bool bWaitForAll,
				  Uint32 dwTimeout, Uint32 dwWakeMask)
{
	//dwWakeMask=dwWakeMask;	// Suppress compiler warning
	//dwTimeout=dwTimeout;	// Suppress compiler warning

	m_dwUnlockCount = UnlockCount;
	assert(dwCount > 0 && dwCount <= MAXIMUM_WAIT_OBJECTS);
	assert(pObjects != NULL);
	// Linux can't really do this
	assert(bWaitForAll);

	m_ppObjectArray = pObjects;
	m_dwCount = dwCount;

	// as an optimization, skip allocating array if
	// we can use a small, pre-eallocated bunch of handles
	if (m_dwCount > (sizeof(m_hPreallocated)/sizeof(m_hPreallocated[0]))) {
		m_pHandleArray = new CCEvent *[m_dwCount];
		m_bLockedArray = new bool[m_dwCount];
	}
	else {
		m_pHandleArray = m_hPreallocated;
		m_bLockedArray = m_bPreallocated;
	}

	// get list of handles from array of objects passed
	for (Uint32 i = 0; i <m_dwCount; i++) {
		assert(pObjects[i]);

		m_pHandleArray[i] = pObjects[i];
		m_bLockedArray[i] = false;
	}
	////Lock(dwTimeout,bWaitForAll,dwWakeMask);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CCMTMultiLock::~CCMTMultiLock
//
//	AUTHOR:
//		Dave Sewhuk - created on 2012-03-27 13:43:39
//
//	DESCRIPTION:
/**


 	\param[in,out] xxx description
	\return description

**/
//	SYNOPSIS:
CCMTMultiLock::~CCMTMultiLock()
{
	for (Uint32 i=0; i < m_dwCount; i++)
	if (m_bLockedArray[i])
	  	m_ppObjectArray[i]->SetEvent();

	if (m_pHandleArray != m_hPreallocated) {
		delete[] m_bLockedArray;
		delete[] m_pHandleArray;
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CCMTMultiLock::Lock
//
//	AUTHOR:
//		Dave Sewhuk - created on 2012-03-27 13:47:05
//
//	DESCRIPTION:
/**


 	\param[in,out] xxx description
	\return description

**/
//	SYNOPSIS:
Uint32 CCMTMultiLock::Lock(Uint32 dwTimeOut,
		     bool bWaitForAll,Uint32 dwWakeMask)
{
	Uint32 dwResult = 0;
	//dwWakeMask=dwWakeMask;	// Suppress Compiler Warnings
	if (bWaitForAll){
		// Wait for all of our events to signal
		for (Uint32 i = 0; i < m_dwCount; i++) {
			m_bLockedArray[i] = m_pHandleArray[i]->WaitFor(dwTimeOut);
			dwResult |= m_bLockedArray[i]<<i;
		}
	}
	else {
		assert(0);
	}
	return dwResult;
}
//																			  *
//*****************************************************************************





//=============================================================================
//	END OF FILE tekEventsLinux.cpp
//=============================================================================

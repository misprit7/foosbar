//***************************************************************************
// $Header:: /ClearPath SC-1.0.123/User Driver/inc-private/tekThreads.h 26   $
//
// NAME
//		Thread.h
//
// DESCRIPTION:
//		Thread virtual class template.
//
// CREATION DATE:
//		03/10/2004 14:18:41
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2004-2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//
//																			  *
//*****************************************************************************
#ifndef _THREAD_H_
#define _THREAD_H_

//***************************************************************************
// NAME																        *
// 	Thread.h headers
//																			*/
	#include "tekEvents.h"
	#ifdef THREAD_SLOTS
		#include "sigslot.h"
	#endif
	#include <cstdlib>
//																			  *
//*****************************************************************************

#if !(defined(_WIN32)||defined(_WIN64))
        #define THREAD_PRIORITY_BELOW_NORMAL -1
#endif

//***************************************************************************
// NAME																	    *
// 	Thread.h function prototypes
//
//

/***************************************************************************/



//*****************************************************************************
// NAME																		  *
// 	Thread.h constants
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																		  *
// 	CThread Class
//
// DESCRIPTION:
///	This pure virtual class allows for the easy cross-platform construction
/// and use of threads. The <Run> function will start and should continue to
/// run until it's completion or the detection of the <*m_pTermFlag>. To
/// prevent application deadlock the termination flag should be checked
/// regularly and/or make sure to implement a destructor to cause the
/// thread to terminate.
///
/// To create a thread:
//		1) Derive a new class inheriting this class
///		2) Implement the function int Run(void *context). The context
///		   pointer will point to any data passed in on the <LaunchThread>
///		   function.
///		3) Optionally, setup a common termination flag via <SetTerminateFlag>.
///		   A common flag in your application may then set this flag to
///		   initiate a shutdown.
///		4) For Windows based systems, an ActiveX apartment initialization
///		   may be executed via the <UseWithCOM()>.
///
///		5) Once setup, call <LaunchThread()> to start the <Run> function
///		   in a new thread.
///
///	You may call <TerminateAndWait()> to force a thread to terminate. This
/// function returns when the thread has stopped.
//
class CThread
{
private:
	#if (defined(_WIN32)||defined(_WIN64))
		static unsigned __stdcall ThreadEntry(void* pArgs);
		// TRUE if CoInitialize required
		nodebool m_isCOM;
		// Use DLL terminate ACK strategy
		nodebool m_DLLterm;
		// Thread handle
		HANDLE m_hThread;
	#else
public:
		// This is the control function run in the thread
		static void *ThreadEntry(void *pArgs);
		// Thread handle
		pthread_t m_hThread;
	#endif

protected:

#define THREAD_RET_TYPE
	nodebool *m_pTermFlag;					// TRUE to terminate thread
	nodebool m_lclOwnTerm;					// TRUE if we allocated m_pTermFlag
	nodebool m_running;						// TRUE during thread function execution
	// Function to run in the new thread
	void constructInit(nodebool isCom=false);
	CCCriticalSection m_exitSection;		// Critical section for access to isRunning
	// Seed that was used to init random # generator
	unsigned int m_seed;
	// Status return of exiting thread control function
	void *m_status;
	// Last error code
	nodeulong m_lastErr;

public:
	CCEvent m_TermEvent;					// Termination ACK event
	// (optional) Event prior to running. Can be used to wait for thread
	// function launch and initialization complete.
	CCEvent m_ThreadParkedEvent;

	void *m_context;						// Thread context pointer
	virtual int Run(void *context)=0;
	static void Sleep(Uint32 milliseconds);
	// Create the thread
	HANDLE LaunchThread(void *context = NULL, int priority = 0);
#ifdef THREAD_SLOTS
	signal0<> ShuttingDown;					// Signal at termination time
	signal0<> Shutdown;						// Signal as terminating
#endif
	// NOTE: these unsigned int are required for WIN32/WIN64 
	unsigned int m_idThread;				// Thread ID
	unsigned int m_idThreadSaved;			// Saved m_idThread for debug
	nodeulong m_uiID;						// Saved UI thread ID
	// Wait for thread to terminate
	nodebool WaitForTerm();
	// Terminate the thread via term flag
	virtual HANDLE Terminate();
	// Terminate and wait for exit
	virtual void TerminateAndWait();
	// Detect if thread is terminating or has terminated
	nodebool Terminating()
	{
		nodebool returnValue = (m_hThread == 0);
		if (m_pTermFlag != NULL)
		{
			returnValue = (*m_pTermFlag || (m_hThread == 0));
		}
		return(returnValue);
	}
	// Setup a common terminate flag
	void SetTerminateFlag(nodebool *flag);

	// Returns the OS specific thread ID for this instance. This
	// ID is useful when debugging to identify a thread using
	// the operating system's debugger ability to show threads
	// associated with the current process.
	nodeulong ThreadID() {
		return(nodeulong(m_idThreadSaved));
	}

	// Return the UI thread ID for the current thread
	nodeulong UIthreadID();

	// Returns the seed used for the random number generator
	unsigned int Seed(){
		return m_seed;
	}

	// Sets the seed to use for the random number generator
	void SetSeed(unsigned int seed){
		// Save the seed
		m_seed = seed;
		// Reinit the random # generator
		srand(1);	// reset random # gen
		srand(seed);	// set the seed
	}

	// Returns the operation system's current thread ID.
	static nodeulong CurrentThreadID();

	// Construction
	CThread();
	#if (defined(_WIN32)||defined(_WIN64))
		CThread(nodebool isCOM);
		CThread(void *context, int priority, nodebool isCOM = false);
	// Set to perform ActiveX init/un-inits
	void UseWithCOM() {
		m_isCOM = TRUE;
	}

	// Set DLL terminate strategy, wait for event vs thread handle signal
	// This is necessary as the handle cannot signal within the DLL coding.
	void SetDLLterm(nodebool DLLterm) {
		m_DLLterm = DLLterm;
	}
	#endif
	virtual ~CThread();
};
//																			  *
//*****************************************************************************

#endif

//=============================================================================
//	END OF FILE Thread.h
//=============================================================================

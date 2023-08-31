//*****************************************************************************
// $Workfile: tekThreadsLinux.cpp $
// $Archive: /ClearPath SC-1.0.123/User Driver/linux/src/tekThreadsLinux.cpp $
// $Revision: 10 $ $Date: 12/09/16 4:45p $
//
// DESCRIPTION:
/**
	\file
	\defgroup PortGrp Porting Abstraction Functions
	\brief Linux Threading Implementation

	This module implements the CThread virtual class for the Linux platform.
**/
//
// CREATION DATE:
//		01/18/2011 14:24:32
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
// 	tekThreadsLinux.cpp headers
//
	#include <stdio.h>
	#include <cassert>
	#include <iostream>
	#include <unistd.h>
	#include <pthread.h>
	#include <stdlib.h>
	#include "tekThreads.h"
	#include <assert.h>
	#include <sys/types.h>
    #include <sys/syscall.h>
    #include <errno.h>
    #include <asm-generic/errno-base.h>
	using namespace std;

//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	tekThreadsLinux.cpp function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	tekThreadsLinux.cpp constants
//
//
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	// - - - - - - - - - - - - - - - - - - -
	// DEBUG TRACING
	// - - - - - - - - - - - - - - - - - - -
#if !defined(TRACE_THREAD0)
	#if defined( _DEBUG)
	#define TRACE_THREAD0 0
	#else
	#define TRACE_THREAD0 0
	#endif
#endif
	#define RPT_OUT _CRT_WARN

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	tekThreadsLinux.cpp static variables
//
//

//																			  *
//*****************************************************************************





//*****************************************************************************
// NAME	  																	  *
//		CThread::CThread
//
// DESCRIPTION:
//		Construction/Destruction
//
// SYNOPSIS:
CThread::CThread()
{
#if TRACE_THREAD0
	_RPT0(RPT_OUT, "CThread::constructing\n");
#endif
	m_uiID = m_idThread = m_idThreadSaved = 0;
	m_pTermFlag = new nodebool;
	// We are not done yet
	*m_pTermFlag = false;
	m_lclOwnTerm = true;
	m_context = NULL;
	m_hThread = pthread_t(NULL);
	m_running = false;
}

CThread::~CThread( )
{
	if (m_lclOwnTerm)
		*m_pTermFlag = true;
	if (m_hThread)	{
		try {
			TerminateAndWait();
			#if TRACE_THREAD0
				_RPT1(_CRT_WARN, "CThread destructor handle closed thrd=%d\n",
						UIthreadID() );
			#endif
		}
		catch(...) {
			_RPT0(_CRT_WARN, "CThread destructor failed\n");
		}
	}
	if (m_lclOwnTerm)
		delete m_pTermFlag;

	m_hThread = pthread_t(NULL);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		ThreadEntry
//
//	DESCRIPTION:
/**
	This is the static function the operating system will call on the
	newly created thread. It is passed the object's \e this pointer to
	allow access to the instance's state and member functions. This
	method is a friend of CThread.

 	\param[in,out] arg Pointer to our class's this pointer to get access
					   to the member functions and state.
	\return it's argument

**/
//	SYNOPSIS:
void *CThread::ThreadEntry( void * arg )
{
	CThread * t = static_cast< CThread * > ( arg );

	t->m_idThreadSaved = nodeulong(t->m_idThread = pthread_self());
	t->m_uiID = nodeulong(syscall(SYS_gettid));
	// Setup random number generator
	srand(1);				// reset random # gen
	srand(t->m_seed);		// set the seed
#if TRACE_THREAD0
	_RPT1(RPT_OUT, "CThread::ThreadEntry, id=%d\n",
			t->UIthreadID());
#endif
	t->Run( t->m_context );
	// Signal completion event
	if (t->m_lclOwnTerm)
		*t->m_pTermFlag = true;
	t->m_TermEvent.SetEvent();
 	//pthread_exit(arg);
#if TRACE_THREAD0
	_RPT1(_CRT_WARN,"CThread::Thread %d signaled and is exiting\n",
				 t->UIthreadID());
#endif
	return (HANDLE)arg;
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CThread::LaunchThread
//
//	DESCRIPTION:
/**
	Call this to start the thread,

 	\param[in,out] context Some \i context the
	\return description

**/
//	SYNOPSIS:
HANDLE CThread::LaunchThread( void *context, int priority )
{
	// This needs to get set right away so that nothing else thinks it's
	// available.
	int theErr;
	m_context = context;
	m_exitSection.Lock();
#if TRACE_THREAD0
	_RPT0(RPT_OUT, "CThread::LaunchThread\n");
#endif

    pthread_attr_t threadAttr;
    struct sched_param  param;             // for setting priority
    int32_t policy;

    assert((theErr = pthread_getschedparam( pthread_self(), &policy, &param))==EOK);
 	assert((theErr = pthread_attr_init(&threadAttr))==EOK);
 	// Assign lighter weight stack
 	const int STACK_SIZE = 250000;
 	assert((theErr = pthread_attr_setstacksize(&threadAttr, STACK_SIZE))==EOK);
#if 0
    // TODO: Work out priority adjuster
	assert((theErr = pthread_attr_setschedpolicy (&threadAttr, SCHED_RR))==EOK);
    if (priority!=0) {
    	_RPT1(RPT_OUT, "CThread::LaunchThread, prio adj=%d\n", priority);
    	// TODO: changing this leads to dead-lock in netStateInfo construction
		//theErr = pthread_attr_setinheritsched (&threadAttr, PTHREAD_EXPLICIT_SCHED);
		assert((theErr = pthread_attr_setinheritsched (&threadAttr, PTHREAD_INHERIT_SCHED))==EOK);
		// Adjust thread base priority
		param.sched_priority += priority;
		assert((theErr = pthread_attr_setschedparam (&threadAttr, &param))==EOK);
		//theErr = pthread_attr_setschedpolicy (&threadAttr, SCHED_OTHER);
    }
#endif
    theErr= pthread_create(&m_hThread, &threadAttr,
                                ThreadEntry, this);
 //   theErr= pthread_create(&m_hThread, NULL,
 //                               ThreadEntry, this);
    if (theErr != EOK) {
        _RPT1(_CRT_WARN, "CThread::LaunchThread create err=%d\n", theErr);
        assert(theErr==EOK);
    }
//assert((theErr = pthread_create(&m_hThread, &threadAttr,
//							ThreadEntry, this))==EOK);
    pthread_attr_destroy(&threadAttr);
    m_exitSection.Unlock();

	return (HANDLE)m_hThread;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CThread::Sleep
//
//	DESCRIPTION:
/**
	Suspend the current thread for \e milliseconds.

 	\param[in] milliseconds Suspension time.
	\return description

**/
//	SYNOPSIS:
void CThread::Sleep(Uint32 milliseconds)
{
	// Convert to microseconds
	usleep(milliseconds*1000);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CThread::WaitForTerm
//
//	DESCRIPTION:
/**
	This function will wait for the thread function to return, signalling
	the end of the the thread's Run function execution. The classes's
	Run function dispatcher will still be running after this returns and
	all the application's thread activities are guaranteed to be complete
	when this returns TRUE.

**/
//	SYNOPSIS:
nodebool CThread::WaitForTerm()
{
	nodebool exitState = FALSE;
	// Lock state from thread
	m_exitSection.Lock();
	// Thread already dead!
	if (!m_running) {
		#if TRACE_THREAD0
			_RPT1(RPT_OUT, "CThread::WaitForTerm already died %d\n",
								UIthreadID());
		#endif
		exitState = TRUE;
	}
	// The thread is still "running", wait for it to signal the
	// end of its control function.
	else {
		#if TRACE_THREAD0
			_RPT1(RPT_OUT, "CThread::WaitForTerm waiting for thread %d\n",
							 UIthreadID());
		#endif
		// Wait for it to terminate on its own
		exitState = m_TermEvent.WaitFor(30000);
		if (!exitState) {
			// Failed to timely die, force kill it.
			//TODO_LINUX pthread_cancel(m_hThread);
			_RPT2(RPT_OUT, "CThread::WaitForTerm failed Run terminate, id=%d, exitState=%d\n",
						m_idThreadSaved, exitState);
		}
		#if TRACE_THREAD0
			_RPT2(RPT_OUT, "CThread::WaitForTerm terminated, id=%d, exitState=%d\n",
						UIthreadID(), exitState);
		#endif
        // We are really gone, now, set state to reflect this
		m_idThread = 0;
		m_running = false;
	}
	// Unlock state from thread
	m_exitSection.Unlock();
	return(exitState);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CThread::Terminate
//
//	DESCRIPTION:
/**
	Call this function to terminate the thread.

	\return handle/ptr to thread
**/
//	SYNOPSIS:
void *CThread::Terminate()
{
#if TRACE_THREAD0
	_RPT1(RPT_OUT, "CThread::Terminate terminating thread %d\n",
					 UIthreadID());
#endif
	// Thread still OK?
	if (m_hThread) {
		if (m_pTermFlag) {
			// Own m_running to prevent exit race
			m_exitSection.Lock();
			*m_pTermFlag = true;
			// Insure we break through the parking event
			m_ThreadParkedEvent.SetEvent();
			#ifdef THREAD_SLOTS
				ShuttingDown.emit();
			#endif
			// Join with the thread until it terminates.
			if (m_hThread) {
				m_lastErr = pthread_join( m_hThread, &m_status );
				if (m_lastErr)
					_RPT2(RPT_OUT, "CThread::Terminate join failed id=%d, join ret'd=%d\n",
													 m_idThreadSaved, m_lastErr);
#if TRACE_THREAD0
				_RPT2(RPT_OUT, "CThread::Terminate id=%d, join ret'd=%d\n",
								 UIthreadID(), m_lastErr);
#endif
			}
            // Return TLS and internal junk, else leak
            if (m_hThread) {
                // Note this usually returns ESRCH, but the leak is gone
                /*int theDetachErr = */pthread_detach(m_hThread);
               // if (theDetachErr)
                //    _RPT1(_CRT_WARN, "CThread::Terminate detach err=%d\n", theDetachErr);
                //assert(theDetachErr!=EOK);
            }
			// One join per terminate!
			m_hThread = pthread_t(NULL);
			m_running = false;
			m_idThread = 0;
			// Release m_running lock
			m_exitSection.Unlock();
		}
		else {
			// Insure we break through the parking event
			m_ThreadParkedEvent.SetEvent();
		}
	}
	return((void *)m_hThread);
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CThread::TerminateAndWait
//
//	DESCRIPTION:
/**
	Signal the thread function to terminate and wait for it to occur.
**/
//	SYNOPSIS:
void CThread::TerminateAndWait()
{
#if TRACE_THREAD0
	_RPT1(_CRT_WARN, "CThread::TerminateAndWait thrd=%d\n",
			UIthreadID() );
#endif
	Terminate();
	WaitForTerm();
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CThread::SetTerminateFlag
//
//	DESCRIPTION:
/**
	This function is useful for tying many thread instances together to a
	common termination flag.

 	\param[in] flag Pointer to the shared termination flag.
**/
//	SYNOPSIS:
void CThread::SetTerminateFlag(nodebool *pFlag)
{
	// If we own our terminate flag, release and let owner
	// control it.

	// Own m_running to prevent exit race
//    printf("CThread::SetTerminateFlag Start\n");
	m_exitSection.Lock();
	if (m_lclOwnTerm) {
		delete m_pTermFlag;
		m_lclOwnTerm = false;
	}
	m_pTermFlag = pFlag;
	m_exitSection.Unlock();
//    printf("CThread::SetTerminateFlag Stop\n");
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		CThread::CurrentThreadID
//
//	DESCRIPTION:
/**
	Utility function to return this instance's thread ID. This
	may or may not match the internal OS identifier depending
	on the platform.

	\see UIthreadID

	\return OS specific thread ID for current thread

**/
//	SYNOPSIS:
nodeulong CThread::CurrentThreadID()
{
	return(nodeulong(pthread_self()));
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CThread::UIthreadID
//
//	DESCRIPTION:
/**
	Utility function to return this instance's thread ID. It is useful for
	debugging and various low level reports as it should match
	the debugger's identifiers.

	\return OS specific thread ID for current thread

**/
//	SYNOPSIS:
nodeulong CThread::UIthreadID()
{
    return(m_uiID);
}
//																			  *
//*****************************************************************************


//=============================================================================
//	END OF FILE tekThreadsLinux.cpp
//=============================================================================

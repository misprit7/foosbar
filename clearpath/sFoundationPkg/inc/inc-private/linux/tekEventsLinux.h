//*****************************************************************************
// $Workfile: tekEventsLinux.h $
// $Archive:: /ClearPath SC/LinuxPrjEclipse/sFoundation20/linux/inc/tekEvents $
// $Revision: 12 $ $Date: 12/22/16 12:42 $
//
// DESCRIPTION:
//		Thin layer to create synchronization objects for Linux.
//
// CREATION DATE:
//		05/14/2004 13:03:56
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2004-2012  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//																			   *
//******************************************************************************
#ifndef __TEKEVENTS_H__
#define	__TEKEVENTS_H__



//******************************************************************************
// NAME																           *
// 	tekEvents.h headers
//
	#include <errno.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <stdio.h>
	#include <string>
    #include <sys/time.h>

	#include <assert.h>
	#include "tekTypes.h"
	using namespace std;
	#define EOK 0
//																			   *
//******************************************************************************


//******************************************************************************
// NAME																           *
// 	tekEvents.h constants
//
//

#ifndef NULL
#define NULL 0
#endif

#define SYNC_INFINITE  0xFFFFFFFFU
#define INFINITE  DWORD(0xFFFFFFFF)
#define LPSECURITY_ATTRIBUTES void *
typedef void * HANDLE;


//*****************************************************************************
//   NAME
//           CCaddAbsTime
//
//   CREATION DATE:
//           03/29/2012 13:33
//
//   DESCRIPTION:
/**
          Add the required milliseconds from now and update
          \a abs_time.

          \param[in] incrMilliseconds time increment
          \param[out] abs_time resulting time from now
 **/
//   SYNOPSIS:
inline void CCaddAbsTime(timespec &abs_time, double incrMilliseconds)
{
	// Try better clock, if not fallback on common
	//if(clock_gettime( CLOCK_MONOTONIC, &abs_time )) {
		assert(clock_gettime( CLOCK_REALTIME, &abs_time ) == 0);
	//}
	nodeulong secs = nodeulong(incrMilliseconds/1000);
	abs_time.tv_sec += secs;
	// Prevent overflow on longer time-outs
	abs_time.tv_nsec += (incrMilliseconds-secs*1000) * 1000000L;
	if ( abs_time.tv_nsec > 1000000000L) {
		abs_time.tv_nsec -= 1000000000L;
		abs_time.tv_sec += 1;
	}
}
//
//*****************************************************************************


//*****************************************************************************
//   NAME
//           class CCSemaphore
//
//   CREATION DATE:
//           02/08/2010 11:37:08
//
//   DESCRIPTION:
///          The classic resource counting mutual exclusion device.  A request
///          to Lock is blocked via the scheduler until some other thread
///          Unlocks the resource. Optionally multiple "counts" can be made
///          available if required.
//
//   SYNOPSIS:
class CCSemaphore
{
public:
	CCSemaphore (long lInitialCount = 0,
			  long lMaxCount = 1,
			  int lpsaAttributes = 0)
	{
		assert(sem_init( &sem, lpsaAttributes, unsigned(lMaxCount) )==EOK);
	}
	virtual ~CCSemaphore() {
	    assert(sem_destroy(&sem)==EOK);
	}

	// Return TRUE if we own resource
	virtual bool Lock (Uint32 timeoutMsec = SYNC_INFINITE)
	{
		int r = -1;
		if ( timeoutMsec == SYNC_INFINITE ) {
			r = sem_wait( &sem );
		} else {
			struct timespec abs_time;
			CCaddAbsTime(abs_time, timeoutMsec);
			r = sem_timedwait( &sem, &abs_time );
		}
	  	return r == -1 ? false : true;
	}

	// Release one item, return TRUE if succeeded
	virtual int Unlock ()
	{
		return sem_post( &sem ) == 0 ? 1 : 0;
	}

	// Release multiple items and return the count that was left.
	virtual bool Unlock (long lCount, long *lPrevCount = NULL)
	{
		for ( int ix = 0; ix < lCount; ++ix )
			sem_post( &sem );
		if ( lPrevCount != NULL )
			sem_getvalue( &sem, (int *) lPrevCount );
		return 1;
	}

protected:
	sem_t sem;
};
//
//*****************************************************************************



//*****************************************************************************
//	NAME
//		class CCEvent
//
//  CREATION DATE:
//		02/08/2010 11:37:08
//
//	DESCRIPTION:
/**
	A scheduler based blocking event. Multiple threads can wait for the
	underlying event to "signal" via SetEvent to restart
	their execution. There is an optional time-out for the wait.

 	Detailed description.
**/
//	SYNOPSIS:
class CCEvent
{
private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int manual_reset;
	bool m_set;
public:
	CCEvent (bool bInitiallyOwn = false,
			bool bManualReset = true,
			LPSECURITY_ATTRIBUTES lpsaAttribute = NULL) :
			manual_reset( bManualReset ),
			m_set( bInitiallyOwn ) {
		assert(pthread_mutex_init( &mutex, NULL )==EOK);
		assert(pthread_cond_init( &cond, NULL )==EOK);
 	}
	~CCEvent() {
		int theErr;
		assert((theErr=pthread_mutex_destroy(&mutex))==EOK);
		assert((theErr=pthread_cond_destroy(&cond))==EOK);
	}

	// in Linux the semaphore always exists
	bool isOK ()
	{
		return true;
	}

	// Signal and leave signaled our event
	bool SetEvent ( bool do_mutex = true )
	{
//		_RPT1(_CRT_WARN, "Evt Set 0x%lx\n", this);
		int r = EOK;
		if ( do_mutex ) assert(pthread_mutex_lock( &mutex )==EOK);

		if (!m_set) {
			m_set = true;
			r = pthread_cond_broadcast( &cond );
		}
		if ( do_mutex ) assert(pthread_mutex_unlock( &mutex )==EOK);
	  	return r==EOK;
	}

	// Un-signal and block waiters
	bool ResetEvent ()
	{
//		_RPT1(_CRT_WARN, "Evt Reset 0x%lx\n", this);
		m_set = false;
	  	return true;
	}


	// Cause the calling thread to block until "signaled" or TimeOut
	// occurs. Returns TRUE if wait was signaled normally.
	bool WaitFor ( unsigned timeoutMsec=SYNC_INFINITE )
	{
		assert(pthread_mutex_lock( &mutex )==EOK);
		int r=EOK;
		// Return if already signaled
		if ( ! m_set ) {
			if ( timeoutMsec == SYNC_INFINITE ) {
				// Prevent spurious wakeup
				do {
					r = pthread_cond_wait( &cond, &mutex );
				} while (r!=EOK && !m_set);
			}
			else {
				// Create our time-out
				struct timespec abs_time;
				CCaddAbsTime(abs_time, timeoutMsec);
				do {
					r = pthread_cond_timedwait( &cond, &mutex, &abs_time );
				} while (r!=EOK && r!=ETIMEDOUT && !m_set);
			}
		}
		assert(pthread_mutex_unlock( &mutex )==EOK);
	  	return (r == EOK);
	}

	HANDLE GetHandle() {
		return(&cond);
	}

};
//
//*****************************************************************************


//*****************************************************************************
//	NAME
//		class CCCriticalSection
//
//  CREATION DATE:
//		02/08/2010 11:37:08
//
//	DESCRIPTION:
///		Create a critical section lockout.
//
//	SYNOPSIS:
class CCCriticalSection
{
protected:
	 pthread_mutex_t mutex;
public:
  	CCCriticalSection ()
	{
  		//pthread_mutexattr_t psharedm;
  		//(void) pthread_mutexattr_init(&psharedm);
  		//(void) pthread_mutexattr_settype(&psharedm, PTHREAD_MUTEX_RECURSIVE);
  		//assert(pthread_mutex_init( &mutex, &psharedm )==EOK);
  		assert(pthread_mutex_init( &mutex, NULL )==EOK);
 		// might need to set PTHREAD_PROCESS_SHARED
	}

  	~CCCriticalSection ()
	{
		pthread_mutex_destroy( &mutex );
	}

	HANDLE GetHandle ()
	{
	  return &mutex;
	}
	// Return TRUE if we released critical section
	bool Unlock ()
	{
		return(pthread_mutex_unlock( &mutex )==EOK);
	}

	// Return TRUE if we own critical section
	bool Lock ( Uint32 timeoutMsec=SYNC_INFINITE )
	{
		int r = 0;
		if ( timeoutMsec == SYNC_INFINITE ) {
			r = pthread_mutex_lock( &mutex );
		} else {
			struct timespec abs_time;
			CCaddAbsTime(abs_time, timeoutMsec);
			r = pthread_mutex_timedlock( &mutex, &abs_time );
		}
		//if ( r ) throw ;
	  	return r==EOK;
	}
};
//
//*****************************************************************************


//*****************************************************************************
//	NAME
//		class CCMutex
//
//  CREATION DATE:
//		02/08/2010 11:37:08
//
//	DESCRIPTION:
///		Create a critical section lockout.
//
//	SYNOPSIS:
class CCMutex
{
protected:
	 pthread_mutex_t mutex;
public:
	 CCMutex ()
	{
  		//pthread_mutexattr_t psharedm;
  		//(void) pthread_mutexattr_init(&psharedm);
  		//(void) pthread_mutexattr_settype(&psharedm, PTHREAD_MUTEX_RECURSIVE);
  		//assert(pthread_mutex_init( &mutex, &psharedm )==EOK);
  		assert(pthread_mutex_init( &mutex, NULL )==EOK);
 		// might need to set PTHREAD_PROCESS_SHARED
	}

  	~CCMutex ()
	{
		pthread_mutex_destroy( &mutex );
	}

	HANDLE GetHandle ()
	{
	  return &mutex;
	}
	// Return TRUE if we released critical section
	bool Unlock ()
	{
		return(pthread_mutex_unlock( &mutex )==EOK);
	}

	// Return TRUE if we own critical section
	bool Lock ( Uint32 timeoutMsec=SYNC_INFINITE )
	{
		int r = 0;
		if ( timeoutMsec == SYNC_INFINITE ) {
			r = pthread_mutex_lock( &mutex );
		} else {
			struct timespec abs_time;
			CCaddAbsTime(abs_time, timeoutMsec);
			r = pthread_mutex_timedlock( &mutex, &abs_time );
		}
		//if ( r ) throw ;
	  	return r==EOK;
	}
};
//
//*****************************************************************************




//*****************************************************************************
//	NAME																	  *
//		CCMTMultiLock Class
//
//  CREATION DATE:
//		2012-03-27 13:40:33
//
//	DESCRIPTION:
/**
		Wait for one or all events to signal.

**/
class CCMTMultiLock
{
protected:
	Uint32            m_dwUnlockCount;
	Uint32            m_dwCount;
	CCEvent           *m_hPreallocated[8];
	bool              m_bPreallocated[8];
	CCEvent * const *
	                 m_ppObjectArray;
	CCEvent         ** m_pHandleArray;
	bool           * m_bLockedArray;

public:
	CCMTMultiLock(CCEvent *pObjects[], Uint32 dwCount,
		          Uint32 UnlockCount=1, bool bWaitForAll=true,
				  Uint32 dwTimeout=SYNC_INFINITE, Uint32 dwWakeMask=0);

	~CCMTMultiLock();

public:
	Uint32 Lock(Uint32 dwTimeOut=SYNC_INFINITE,
		     bool bWaitForAll=true,Uint32 dwWakeMask=0);

};
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		class CCatomicUpdate
//
//	DESCRIPTION:
/**
	Atomic increment and decrement / test object.
**/
//	SYNOPSIS:
class CCatomicUpdate
{
private:
	LONG m_value;
public:
	CCatomicUpdate() {
		m_value = 0;
	}

	LONG Incr() {
		return __sync_add_and_fetch(&m_value,1);
	}

	LONG Decr() {
		return __sync_sub_and_fetch(&m_value,1);
	}
};
//																			  *
//*****************************************************************************


#endif
//=============================================================================
//	END OF FILE tekEvents.h
//=============================================================================

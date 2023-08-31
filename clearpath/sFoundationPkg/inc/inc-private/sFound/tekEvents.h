//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc-private/tekEvents.h $
// $Date: 01/19/2017 17:39 $
// $Workfile: tekEvents.h $
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	\brief Brief Module Description

	This file selects the proper platform specific events, mutexes, and other
	primatives header file based on the project defined macros.
	
**/
//
// CREATION DATE:
//		01/18/2011 13:54:56
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2011-2012 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************

#ifndef _tekEvents_h
#define _tekEvents_h

//*****************************************************************************
// !NAME!																      *
// 	tekEvents.h headers included
//
	#if (defined(_WIN32)||defined(_WIN64))	// MS Windows
        #include "tekEventsWin32.h"
	#elif defined __QNX__					// QNX
        #include "tekEventsQNX.h"
	#else									// Linux
        #include "tekEventsLinux.h"
	#endif
//																			  *
//*****************************************************************************


#endif


//============================================================================= 
//	END OF FILE tekEvents.h
// ============================================================================

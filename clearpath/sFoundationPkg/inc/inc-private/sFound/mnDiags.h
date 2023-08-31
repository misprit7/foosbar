//***************************************************************************
// $Header:: /ClearPath SC-1.0.123/User Driver/inc/mnDiags.h 10    12/09/16  $
//
// NAME
//		mnDiags.h
//
// DESCRIPTION:
//		MachineNet diagnostic API elements
//
// CREATION DATE:
//		12/10/2003 20:14:37
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2003  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//
/***************************************************************************/

#ifndef _MNDIAGS_H
#define _MNDIAGS_H


//***************************************************************************
// NAME																        *
// 	mnDiags.h headers
//																			*/
	#include "pubMnNetDef.h"
	#include "pubNetAPI.h"
	#include "mnErrors.h"
/***************************************************************************/




//***************************************************************************
// NAME																	    *
// 	mnDiags.h function prototypes
//
//

/***************************************************************************/



//***************************************************************************
// NAME																	    *   
// 	mnDiags.h constants
//
//

/***************************************************************************/



//***************************************************************************
// NAME																	    *
// 	mnDiags.h static variables
//
// 

// Flow control diagnostic information
//typedef struct _mnFlowStats {
//	double maxFCin;					// Maximum time in flow control in
//	double maxFCout;				// Maximum time flow control asserted
//} mnFlowStats;

/// Network error statistics for a single port on a single node
typedef struct _mnNetErrStatsSinglePort {
	nodeulong nFragPkts;			// Number of fragmented packets
	nodeulong nChecksumBad;			// Number of badly checksumed packets
	nodeulong nStrayChars;			// Number of stray data characters	
	nodeulong nOverflows;			// Number of overflow events
	void clear() {
		nFragPkts = nChecksumBad = nStrayChars = nOverflows = 0;
	}
	_mnNetErrStatsSinglePort() {
		clear();
	}
} mnNetErrStatsSinglePort;

/// Network error statistics for a single network
typedef struct _mnNetErrStats {
	/// Errors detected on the application channel.
	mnNetErrStatsSinglePort ApplicationPort;
	/// Errors detected on the diagnostic channel.
	mnNetErrStatsSinglePort DiagnosticPort;
	/// Count of network brown-outs events detected.
	nodeulong LowPowerEvents;		
	/// Clear the statistics
	void clear() {
		ApplicationPort.clear();
		DiagnosticPort.clear();
		LowPowerEvents = 0;
	}
	/// Construct a cleared instance
	_mnNetErrStats() {
		clear();
	}
} mnNetErrStats;


typedef enum _mnTestResult {
	NET_TEST_UNTESTED,				// 0 Test not run (state unknown)
	NET_TEST_PASSED,				// 1 Test passed
	NET_TEST_FAILED					// 2 Test failed
} mnTestResult;

// Network Diagnostic Result information
typedef struct _mnNetDiagResult {
	// Raw Data Area
	mnNetErrStats lastErrs;			// Last error report
	cnErrCode collectCode;			// Error return of last collection cycle
	// Accumulated Data
	mnNetErrStats accumErrs;		// Last error report
	// Results Area
	mnTestResult netOK;				// Network test result
#ifdef __cplusplus
	_mnNetDiagResult() {
		collectCode = MN_OK;
		lastErrs.clear();
		accumErrs.clear();
		netOK = NET_TEST_UNTESTED;
	}
#endif
} mnNetDiagResult;

// Network Diagnostic Result information   (NOTE: This must match VB version)
#pragma pack(push,1)
typedef struct _mnNetDiagResults {
	// Result area
	mnNetDiagResult node[MN_API_MAX_NODES];
	// Time Stamps
	double startTime;				// Time @ last clear/initialization
	double sampleTime;				// Time last sample occurred
	nodeulong nSamples;				// Number of samples accumulated
	// Network break diagnostic results
	cnErrCode lastBreakDiag;		// Result of the last break diagnostic
	cnErrCode breakDiag;			// Last non-MN_OK error
} mnNetDiagResults;
#pragma pack(pop)

#endif
//=========================================================================== 
//	END OF FILE mnDiags.h
//===========================================================================

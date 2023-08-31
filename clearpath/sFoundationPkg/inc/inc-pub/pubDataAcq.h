//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubDataAcq.h $
// $Revision: 21 $ $Date: 12/09/16 3:46p $
// $Workfile: pubDataAcq.h $
//
/// \file
/// \brief Link Data Acquisition Interface and types
//
// NAME
//		pubDataAcq.h
//
// DESCRIPTION:
///		
//
// CREATION DATE:
//		9/3/2009 3:53:24 PM
//
// COPYRIGHT NOTICE:
//		(C)Copyright 2009 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//
//*****************************************************************************

#ifndef __PUBDATAACQ_H__
#define __PUBDATAACQ_H__

//*****************************************************************************
// NAME																	      *
// 	pubDataAcq.h headers
	#include "tekTypes.h"
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	pubDataAcq.h constants
//
// scope mode definitions
typedef enum {
 	SCP_OFF  = 0,				 // off
 	SCP_MXN  = 1,				 // max/min mode
 	SCP_PDR  = 2,				 // point/derivative mode
 	SCP_DRATE  = 3				 // double sample rate mode
} scopemode;

typedef enum _dacqSTATES {
	DACQ_STATE_IDLE_DONE=0,
	DACQ_STATE_NEG_IN	 =1,
	DACQ_STATE_SETTLING =2,
	DACQ_STATE_POS_IN	 =3,
	DACQ_STATE_NEG_OUT	 =5,
	DACQ_STATE_OUT_RANGE=6,
	DACQ_STATE_POS_OUT	 =7
} dacqSTATES;

typedef enum _dacqEXCPS {
	DACQ_EXCP_ENABLED		=	0,      // 00 Drive Enabled
	DACQ_EXCP_VOLT_SAT		=	2,      // 02 Voltage Saturation
	DACQ_EXCP_TRQ_SAT		=	3,      // 03 Torque Saturation
	DACQ_EXCP_TRQ_VOLT_SAT	=	4,      // 04 Torque and Voltage Sat
	DACQ_EXCP_OVER_SPEED	=	6,      // 06 Over-Speed
	DACQ_EXCP_HARD_STOP		=	7,      // 07 Hard Stop asserted
	DACQ_EXCP_POS_LIMIT		=	8,      // 08 Positive Limit asserted
	DACQ_EXCP_NEG_LIMIT		=	9,      // 09 Negative Limit asserted
	DACQ_EXCP_SHUTDOWN_ENBL	=	13,     // 13 Shutdown/enable asserted
	DACQ_EXCP_DISABLE		=	14,     // 14 Disabled
	DACQ_EXCP_LPB			=	15     	// 15 In LPB / Low Bus Voltage
} dacqEXCPS;
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	pubDataAcq.h types 
//
// 4 octet return packet via the communications link
#define P0_BITS		11			// # of bits in channel 0
#define P1_BITS		11			// # of bits in channel 1
#define P0LSB_BITS	6			// # of bits in channel 0 LSB field
#define P1LSB_BITS	3			// # of bits in channel 1 LSB field
#define INPUT_BITS	3			// # of bits of input

typedef union _dacqPacket {
	nodechar bits[6];				// Uses 6*8 bits of this buffer
	struct {
#ifdef __TI_COMPILER_VERSION__
		unsigned Sequence	: 2;	// Sequence indicator
		int		 T0p0LSBs	:P0LSB_BITS; // LSBs of Trace 0, point 0
		int	nc0				: 8;	// Force word alignment

		int		 T0p0MSBs	: 5;	// MSBs of Trace 0, point 0
		int		 T0p1LSBs	:P1LSB_BITS; // LSBs of Trace 0, point 1
		int	nc1				: 8;	// Force word alignment

		int		 T0p1MSBs	: 8;	// MSBs of Trace 0, point 1
		int	nc2				: 8;	// Force word alignment

		unsigned MoveState0 : 3;	// Move state code
		unsigned Trigger0	: 1;
		unsigned Exception0	: 4;	// Exception code
		int	nc4				: 8;		// Force word alignment

		unsigned MoveState1 : 3;	// Move state code
		unsigned Trigger1	: 1;
		unsigned Exception1	: 4;	// Exception code
		int	nc6				: 8;	// Force word alignment

		unsigned Inputs0	: INPUT_BITS;	// Inputs - sample 0
		unsigned Output0	: 1;	// Output - sample 0
		unsigned Inputs1	: INPUT_BITS;	// Inputs - sample 1
		unsigned Output1	: 1;	// Output - sample 1
		int	nc8				: 8;	// Force word alignment
#else
		unsigned Sequence	: 2;	// Sequence indicator
		int		 T0p0		:P0_BITS;	// Trace 0, point 0
		int		 T0p1		:P1_BITS;	// Trace 0, point 1

		unsigned MoveState0  : 3;	// Move state code
		unsigned Trigger0	: 1;	// Trigger seen?
		unsigned Exception0	: 4;	// Exception code

		unsigned MoveState1 : 3;	// Move state code
		unsigned Trigger1	: 1;	// Trigger seen?
		unsigned Exception1	: 4;	// Exception code

		unsigned Inputs0	: INPUT_BITS;	// Inputs - sample 0
		unsigned Output0	: 1;	// Output - sample 0
		unsigned Inputs1	: INPUT_BITS;	// Inputs - sample 1
		unsigned Output1	: 1;	// Output - sample 1
#endif

//		unsigned nc			: 2;	// Move state code
//		int		 T1p0		:DP0_BITS;	// Trace 1, point 0
//		int		 T1p1		:DP1_BITS;	// Trace 1, point 1
	} Fld;
} dacqPacket;

//																			  *
//*****************************************************************************


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// User/sw driver accessible point structure
//  NOTE: The ordering of the members must remain this way or the TekNetLib
//        will cause crashes.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct _mnDataAcqPt {
    double TimeStamp;			// Time stamp (msec.)
    float TraceValue[4];		// Normalized trace values (-1 .. 0.9999)
    nodeulong MoveState;		// State information (dacqSTATES type)
    nodeulong Exception;        // Exception state (dacqEXCPS type)
	nodeushort Sequence;		// Sequencing check
	nodebool Bool0;				// Spare
    nodebool Bool1;				// Spare
	nodebool Valid;				// Data gap
    nodelong Spare[2];			// (unused) Future expansion/padding
#ifdef __cplusplus
    _mnDataAcqPt() {
    	TimeStamp = 0;
    	TraceValue[0] = TraceValue[1] = TraceValue[2] = TraceValue[3] = 0;
    	MoveState = Exception = Sequence = 0;
    	Bool0 = Bool1 = Valid = false;
    	Spare[0] = Spare[1] = 0;
    }
#endif
} mnDataAcqPt;

#endif // __PUBDATAACQ_H__

//=============================================================================
//	END OF FILE mnDataAcq.h
//=============================================================================

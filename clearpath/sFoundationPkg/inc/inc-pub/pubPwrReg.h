//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/inc/pubPwrReg.h $
// $Revision: 3 $ $Date: 12/06/16 19:09 $
// $Workfile: pubPwrReg.h $
//
// DESCRIPTION:
/**
 * 	\file
 * 	sFoundation definition of the power status register.
**/
// CREATION DATE:
//		2016-11-28
//
// COPYRIGHT NOTICE:-
//		(C)Copyright 2016 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//
//*****************************************************************************
#ifndef __PUBPWRREG_H__
#define __PUBPWRREG_H__

//*****************************************************************************
// NAME																          *
// 	pubCPM_API.h headers
//
	#include "tekTypes.h"
//																			  *
//*****************************************************************************

// Set byte packing
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

//*****************************************************************************
// NAME                                                                       *
//  mnPowerFlds structure
//
// DESCRIPTION
/** \cond ExpertDoc **/
/**
\brief Fields of the Power Status Register.

This status defines a container used to access a node's power status.
**/
typedef struct PACK_BYTES _mnPowerFlds {

	typedef enum _acConnectionTypes {
		AC_PHASES_NONE = 0,
		AC_PHASES_X = 1,
		AC_PHASES_Y = 2,
		AC_PHASES_Z = 4,
		AC_PHASES_XY = AC_PHASES_X | AC_PHASES_Y,
		AC_PHASES_XZ = AC_PHASES_X | AC_PHASES_Z,
		AC_PHASES_YZ = AC_PHASES_Y | AC_PHASES_Z,
		AC_PHASES_XYZ = AC_PHASES_X | AC_PHASES_Y | AC_PHASES_Z,
	} acConnectionTypes;
	/**
		Active Phases

		This field shows which AC phases have a signal present
	**/
	acConnectionTypes AcPhaseActive			: 3;	// 0-2

	/**
		AC Phase Direction

		This field shows that AC is applied in a manner to imply the reverse direction.
	**/
	unsigned AcPhaseDir						: 1;	// 3

	/**
		Last Active Phases

		This field shows which AC phases have a signal present
	**/
	acConnectionTypes AcPhaseActiveLast		: 3;	// 4-6

	/**
		Last AC Phase Direction

		This field shows that AC is applied in a manner to imply the reverse direction.
	**/
	unsigned AcPhaseDirLast					: 1;	// 7
	
	/**
		Active Phases Accumulating

		This field shows which AC phases have had a signal present
	**/
	
	unsigned AcPhaseActiveAccum				: 3;	// 8-10

	/**
		AC Phase Direction Accumulating

		This field shows that AC had been applied in a manner to imply the reverse direction.
	**/
	unsigned AcPhaseDirAccum				: 1;	// 11

	/**
		Bus Power Loss is detected.

		This field will be set when bus voltage source has been disconnected.
		Motion will be canceled and new motion will be blocked when this
		field is set.

		\note If 24V Backup power is not present, communications will be
		interrupted after a short duration. The duration depends on the
		charge remaining in the motor's power supply.
	**/
	unsigned InBusLoss						: 1;	// 12

	/**
		AC wiring error is detected.

		This error is detected when the configuration is set for 3phase
		and an AC source other than 3phase is encountered, or when the
		configuration is set for single phase and an AC source other
		than single phase is encountered.

	**/
	unsigned InWiringError					: 1;	// 13

	/**
		24V Power Present.

		This field will be set when 24 volt backup power is detected.
	**/
	unsigned LogicPwrSupply					: 1;	// 14

	/**
	 	 Under Operating Voltage

	 	 This field will be set when the bus power is below the user-
	 	 defined minimum operating voltage.
	**/
	unsigned InUnderOperV					: 1;	// 15

} mnPowerFlds;

/** \endcond **/
//                                                                            *
//*****************************************************************************

//*****************************************************************************
// NAME                                                                       *
//  mnPowerReg union
//
// DESCRIPTION
/** \cond ExpertDoc **/
/**
\brief Universal Power Status register.

This union defines a container used to access a node's power status.
**/
typedef union MN_EXPORT PACK_BYTES _mnPowerReg {
	/// Bit-wise generic access
	uint32_t	bits;
	/// Field access to the power information.
	mnPowerFlds	fld;

	/** \cond INTERNAL_DOC **/
	// Construction
	_mnPowerReg() {
		bits = 0;
	}
	/**
	\brief Update a string with the set fields in the Power Status Register.

	\param[in,out] buffer  Point to string buffer to update.
	\param[in] size  The size of the buffer in characters including the null
	character.
	\return the parameter \a buffer.

	\CODE_SAMPLE_HDR
	char myBuffer[500];
	cout << myNode.Status.Power.Value.StateStr(myBuffer, sizeof(myBuffer));
	\endcode
	**/
	char * StateStr(char *buffer, size_t size);
	// Copy constructor
	_mnPowerReg(const _mnPowerReg &src);
	/** \endcond **/
} mnPowerReg;
/** \endcond **/
//                                                                            *
//*****************************************************************************
// Restore byte packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif
//===========================================================================
//	END OF FILE pubPwrReg.h
//===========================================================================

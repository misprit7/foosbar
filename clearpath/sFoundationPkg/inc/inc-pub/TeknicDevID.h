//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/TeknicDevID.h $
// $Revision: 37 $ $Date: 12/09/16 4:07p $
// $Workfile: TeknicDevID.h $
//
// DESCRIPTION:
/**
	\file
	\brief Node Type Identifiers and Node Type Container types.

	This header defines the standard device ID container and constants.	 
	
	\note This file defines constants for all Teknic product families.
	Not all node types are available within a product family.
**/
// CREATION DATE:
//		9/13/2006 21:46:33
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2006-2010 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************

#ifndef __TEKNICDEVID_H__
#define __TEKNICDEVID_H__


//*****************************************************************************
// NAME																          *
// 	TeknicDevId.h headers

	#include "tekTypes.h"
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																          *
// 	TeknicDevId.h constants
//
/**
	\brief Node class identifiers.	
	\ingroup LinkGrp
	@{

	\note Not all devices are available within a product line.
**/
typedef enum _nodeIDs {
	NODEID_UNK,						///< 0 Unknown
#ifndef OMIT_LEGACY_PRODUCTS
	NODEID_NC,						// 1 Network Controller
	NODEID_TG,						// 2 Trajectory Generator
	NODEID_SC,						// 3 ControlPoint Integrated Servo Controller
	NODEID_IO,						// 4 IO Cluster
	NODEID_DA,						// 5 SSt and Eclipse Drives
	NODEID_NS,						// 6 eFoundation NC
	NODEID_PS,						// 7 Amazon Power Supply
#endif
	NODEID_MD = 8,					///< 8 Meridian Integrated Servo Controller
	NODEID_CP = 11,					//11 ClearPath Motor
	NODEID_CS						///<12 ClearPath-SC Motor
} nodeIDs;							///< Node Identifier Type
/// @}

/// \cond INTERNAL_DOC

///  Processor Types
typedef enum {
	PROC_TARG_UNKNOWN,				//	0 Older type (F240, F2407)
	PROC_TARG_F240,					//	1 F240
	PROC_TARG_F2407,				//	2				; F2407
	PROC_TARG_F2808,				//	3				; F2808
	PROC_TARG_F2806,				//	4				; F2806
	PROC_TARG_F28015,				//  5				; F28015
	PROC_TARG_LM3S628,				//  6				; Stellaris
	PROC_TARG_F28031,				//  7				; Piccolo
	PROC_TARG_F28034,				//  8				; Piccolo
	PROC_TARG_F28062,				//  9				; Piccolo
	PROC_TARG_F28069,				//  10				; Piccolo
	PROC_TARG_F28067				//  11				; Piccolo
} processorTargets;

// Magic number to qualify processor type
#define ROMINFO_MAGIC 0xa5			 
/// \endcond 
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	TeknicDevId.h types
//
// 
/**
	\brief Firmware Version container
**/
typedef union _versID {
	/// Accessor to the bit-orient fields.
	struct {
		unsigned buildVers		: 8;	//!< Build version (x .y .\b z)
		unsigned minorVers		: 4;	//!< Minor version (x. \b y .z)
		unsigned majorVers		: 4;	//!< Major version (\b x .y .z)
	} fld;								//!< Field access to version parts.
	/// Version code as an integer.
	Uint16 verCode;
} versID_t;

/**
	\brief Device ID container
**/
typedef union _devID {
	/// Accessor to the bit-oriented fields.
	struct {
		/// Model number.
		unsigned devModel		: 8;
		/// Device type from nodeID enumeration
		nodeIDs devType			: 8;
	} fld;								//!< Field access to identifier fields.
	/// Device code as an integer.
	Uint16 devCode;
} devID_t;

/**
	\brief Hardware Version Number
**/
typedef union _hwVers {
	struct {
		unsigned char Minor;		//!< Minor version
		unsigned char Major;		//!< Major version: 0=A. 1=B, etc
	} fld;							//!< Field access to the hardware version.
	Uint16 revCode;					//!< Bitwise access to the version code.
} hwVers_t;

/// \cond INTERNAL_DOC
typedef union _procID {
	struct {
		unsigned procType		: 8;
		unsigned magic			: 8;// Should be ROMINFO_MAGIC
	} fld;
	Uint16 procCode;
} procID_t;
// Logical device identifier
typedef struct _TEKDEVID {
	versID_t versID; 
	devID_t devID; 
	procID_t procID; 
	int16 spare;
	char copywright[60];
} tekDevID;
// Packed version of tekDevID
typedef struct _TEKDEVIDACKED {
	// WORD 0: Firmware Version
	unsigned buildVers		: 8;	//!< Build version (x .y .\b z)
	unsigned minorVers		: 4;	//!< Minor version (x. \b y .z)
	unsigned majorVers		: 4;	//!< Major version (\b x .y .z)
	// WORD 1: Model Version
	unsigned devModel		: 8;	//!< Model Number
	/// Device type from nodeID enumeration
	nodeIDs devType			: 8;	// Device type: TG, NC, etc.
	// WORD 2: Processor Target
	unsigned procType		: 8;	// Processor type
	unsigned magic			: 8;	// Should be ROMINFO_MAGIC
	// WORD 3: Spare
	unsigned				: 16;	// Padding/spare
	// Copyright string
	char copywright[60];
} tekDevPacked;
/// \endcond 

//																			  *
//*****************************************************************************

#endif
//============================================================================= 
//	END OF FILE TeknicDevId.h
//=============================================================================

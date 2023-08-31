//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/meridianHdrs.h $
// $Revision: 45 $ $Date: 12/09/16 1:47p $
// $Workfile: meridianHdrs.h $
//
// DESCRIPTION:
/**
	\file
	\brief Master C/C++ Header File for Meridian Projects.

	This header file defines Meridian technology globally relevant
	constants and functions. Include just this file in your project
	using the Meridian library.
**/
//
// CREATION DATE:
//		10/29/1998 14:04:19
//
// COPYRIGHT NOTICE:
//		(C)Copyright 1998-2010  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification,
//		or portion thereof merged into another program. A copy of the
//		copyright notice must be included in the object library of a user
//		program.
//                                                                             *
//******************************************************************************

#ifndef __MERIDIANHDRS_H__
#define __MERIDIANHDRS_H__

//******************************************************************************
// NAME                                                                        *
// 	meridianHdrs.h headers
//
#include "tekTypes.h"
#include "pubMnNetDef.h"
#include "pubCoreRegs.h"
#include "pubDataAcq.h"
#include "pubMotion.h"

#include "lnkAccessAPI.h"
#include "netCmdAPI.h"
#include "pubIscRegs.h"
#include "pubIscAPI.h"
#include "pubIex.h"
#include "pubCpmAdvAPI.h"
//                                                                             *
//******************************************************************************


//******************************************************************************
// NAME                                                                        *
// 	meridianHdrs.h function prototypes
//
#ifdef __cplusplus
extern "C" {
#endif

//||||||||||||||||||||||||||||||||||||
// MULTINET INITIALIZATION INTERFACE
//||||||||||||||||||||||||||||||||||||

// Main System initialization
MN_EXPORT cnErrCode MN_DECL mnInitializeNets(
			nodebool resetNodes,		// Set to reset nodes on init
			nodeulong netCount,			// Number of nets to initialize (1..2)
			const controllerSpec controllers[]); // Ptr to controller list

// Return the node count of <device> type nodes on all initialized nets.
MN_EXPORT cnErrCode MN_DECL mnSysInventoryCount(
			nodeIDs deviceType,		// Which device type
			nodeulong *pCount);		// Ptr to result

// Return the node addresses of <device> type nodes on all initialized nets.
MN_EXPORT cnErrCode MN_DECL mnSysInventoryRecord(
			nodeIDs deviceType,		// Which device type
			nodeulong maxDevices,	// Maximum number of devices in list
			multiaddr devices[]);	// Ptr to result list

// Return the count of all nodes on the system.
MN_EXPORT cnErrCode MN_DECL mnSysNodeCount(
			nodeulong *pCount);		// Ptr to result area

// Return the node count of the specified <device> on the specified net.
MN_EXPORT cnErrCode MN_DECL mnNetInventoryCount(
			netaddr cNum,			// Network number
			nodeIDs deviceType,		// Which device type
			nodeulong *pCount);

// Return the node addresses of <device> type nodes on the specified net.
MN_EXPORT cnErrCode MN_DECL mnNetInventoryRecord(
			netaddr cNum,			// Network number
			nodeIDs deviceType,		// Which device type
			nodeulong maxDevices,	// Maximum number of devices in list
			multiaddr devices[]);	// Ptr to result list

// Return the count of all nodes on the specified net.
MN_EXPORT cnErrCode MN_DECL mnNetNodeCount(
			netaddr cNum,			// Net to inquire about
			nodeulong *pCount);		// Ptr to result area

// Release all network ports and resources
MN_EXPORT cnErrCode MN_DECL mnShutdown();

// Attempt to restart the specified network
MN_EXPORT cnErrCode MN_DECL mnRestartNet(
	netaddr cNum, 					// Controller to restart
	nodebool restartNodes = FALSE);

// Attempt to restart the specified node
MN_EXPORT cnErrCode MN_DECL mnRestartNode(
	multiaddr addr);

#ifdef __cplusplus
}
#endif
//                                                                             *
//******************************************************************************

#endif
//==============================================================================
//	END OF FILE meridianHdrs.h
//==============================================================================

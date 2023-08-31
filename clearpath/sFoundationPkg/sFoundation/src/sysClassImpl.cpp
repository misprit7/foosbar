//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/src/sysClassImpl.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: sysClassImpl.cpp $
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	\brief Implements the integration of the system and port classes into
	the sFoundation function library.
**/
// PRINCIPLE AUTHORS:
//		DWS
//
// CREATION DATE:
//		2015-12-08 18:24:15
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2015-2016  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	sysClassImpl.cpp headers
//
	#include "pubNetAPI.h"
	#include "lnkAccessAPI.h"
	#include "pubSysCls.h"
	#include "pubCpmCls.h"
	#include "lnkAccessCommon.h"
	#include "meridianHdrs.h"
	#include "netCmdPrivate.h"
	#include "mnParamDefs.h"
	#include <stdarg.h>
	#include <stdio.h>
	#include <math.h>

// Disable windows warning about 'this' in constructor.
#ifdef _MSC_VER
#pragma warning(disable:4355)
#if _MSC_VER<=1800
#define snprintf sprintf_s
#endif
#endif


//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	sysClassImpl.cpp function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	sysClassImpl.cpp constants
//
//
#define LIB_ERR_MSG_SIZE 256
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	sysClassImpl.cpp static variables
//
// 
	extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];
	extern unsigned SysPortCount;

	// Variables for debug thread locking
	extern bool inDebugging;
	extern nodeulong lockedThreadID;
	extern CCEvent debugThreadLockResponseGate;
	extern CCEvent debugGate;
	extern CCMutex threadLockMutex;

	// Use our sFoundation namespace
	using namespace sFnd;

	// Static hack for attention handling by class lib
	static CCCriticalSection AttnMutex;

	class UseAttnMutex {
	private:
	public:
		UseAttnMutex() {
			AttnMutex.Lock();
		}
		~UseAttnMutex() {
			AttnMutex.Unlock();
		}
	};
//																		      *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Helper function to fill in an error object.
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


//*****************************************************************************
//	NAME																	  *
//		fillInErrs
//
//	AUTHOR:
//		DWS - created on 2016-01-20
//
//	DESCRIPTION:
/**
Fill in an mnErr structure with standardized error information.

\param [out] eInfo Buffer to fill in
\param [in] pNode, if non-NULL a node reference to use
\param [in] funcName, the function name to report as the source
\param [in] fmtStr, if non-null a printf control string to add to message as
			details.
**/
void vfillInErrs(mnErr &eInfo, INode *pNode,
		cnErrCode theErr, std::string funcName,
		const char *fmtStr, va_list vl)
{
		char sfndErrMsg[LIB_ERR_MSG_SIZE];
		char lclMsg[LIB_ERR_MSG_SIZE];
		char boilerPlate[256];
		// Get our error message from C driver resources
		infcErrCodeStrA(theErr, sizeof(sfndErrMsg), sfndErrMsg);
		// Fill in address if a node defined.
		if (pNode) {
			// Format their message
			snprintf(boilerPlate, sizeof(boilerPlate),
				"Node @ %d error. Reported by function: %s.\n",
				pNode->Info.Ex.Addr(), funcName.c_str());
			eInfo.TheAddr = pNode->Info.Ex.Addr();
		}
		else {
			snprintf(boilerPlate, sizeof(boilerPlate),
				"Serial Port Problem. Reported by function: %s.\n",
				funcName.c_str());
			eInfo.TheAddr = MN_UNSET_ADDR;
		}
		eInfo.ErrorCode = theErr;
		// They have a message to add?
		if (fmtStr && *fmtStr) {
			vsnprintf(lclMsg, sizeof(lclMsg), fmtStr, vl);
			snprintf(eInfo.ErrorMsg, sizeof(eInfo.ErrorMsg),
				"%s\nError: %s\nDetails: %s\n", boilerPlate, sfndErrMsg, lclMsg);
		}
		else {
			snprintf(eInfo.ErrorMsg, sizeof(eInfo.ErrorMsg),
				"%s\nError: %s\n", boilerPlate, sfndErrMsg);
		}
}


void fillInErrs(mnErr &eInfo,
				cnErrCode theErr, std::string funcName,
			    const char *fmtStr, ...) 
{
	va_list vl;
	va_start(vl, fmtStr);
	vfillInErrs(eInfo, NULL, theErr, funcName, fmtStr, vl);
	va_end(vl);
}

void fillInErrs(mnErr &eInfo, INode *pNode,
				cnErrCode theErr, std::string funcName,
				const char *fmtStr, ...)
{
	va_list vl;
	va_start(vl, fmtStr);
	vfillInErrs(eInfo, pNode, theErr, funcName,
		fmtStr, vl);
	va_end(vl);
}

//																			  *
//*****************************************************************************

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysManager implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Windows port opener

//*****************************************************************************
//	NAME																	  *
//		SysManager::FindComHubPorts
//
//	AUTHOR:
//		SM - created on 2017-07-21
//
//	DESCRIPTION:
/**		
	Get a reference to the port setup object.

 	\param[in] comHubPorts Vector of strings to be filled with port numbers
**/
//	SYNOPSIS:
void SysManager::FindComHubPorts(std::vector<std::string>& comHubPorts)
{
	infcGetHubPorts(comHubPorts);
}

//																			  *
//*****************************************************************************

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysManager implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Windows port opener

#if defined(_WIN32)||defined(_WIN64)
//*****************************************************************************
//	NAME																	  *
//		SysManager::PortSetup
//
//	AUTHOR:
//		DWS - created on 2015-12-28
//
//	DESCRIPTION:
/**		
	Get a reference to the port setup object.

 	\param[in] index Port index to specify [0..NET_CONTROLLER_MAX-1]
**/
//	SYNOPSIS:
void SysManager::ComHubPort(
		size_t netNumber, 
		int portNumber, 
		netRates portRate)
{
	if (netNumber >= NET_CONTROLLER_MAX) {
		mnErr eInfo;
		fillInErrs(eInfo, MN_ERR_PARAM_RANGE, _TEK_FUNC_SIG_,
			"Port Index %d should be less than %d", netNumber, NET_CONTROLLER_MAX);
		throw eInfo;
	}
	m_ports[netNumber].PortNumber = portNumber;
	m_ports[netNumber].PortType	= CPM_COMHUB;
	m_ports[netNumber].PortRate = portRate;
}
#endif
//*****************************************************************************
//	NAME																	  *
//		SysManager::PortSetup
//
//	AUTHOR:
//		DWS - created on 2015-12-28
//
//	DESCRIPTION:
/**
	Get a reference to the port setup object.

 	\param[in] index Port index to specify [0..NET_CONTROLLER_MAX-1]
 	\param[in] portPath Path to the serial port device.
 	\param[in] portRate Operational speed desired
**/
//	SYNOPSIS:
void SysManager::ComHubPort(
		size_t netNumber,
		const char *portPath,
		netRates portRate)
{
	if (netNumber >= NET_CONTROLLER_MAX) {
		mnErr eInfo;
		fillInErrs(eInfo, MN_ERR_PARAM_RANGE, _TEK_FUNC_SIG_,
			"Port Index %d should be less than %d", netNumber, NET_CONTROLLER_MAX);
		throw eInfo;
	}
#if defined(_WIN32)||defined(_WIN64)

	if (strstr(portPath, "COM") == portPath) {
		int portNumber = strtol(portPath + 3, nullptr, 10);
		m_ports[netNumber].PortNumber = portNumber;
		m_ports[netNumber].PortType	= CPM_COMHUB;
		m_ports[netNumber].PortRate = portRate;
	}
	else {
		mnErr eInfo;
		fillInErrs(eInfo, MN_ERR_BADARG, _TEK_FUNC_SIG_,
			"Port Path format should be \"COMXX\"");
		throw eInfo;
	}
#else
	strncpy(m_ports[netNumber].PortName, portPath, MAX_PATH);
	m_ports[netNumber].PortType	= CPM_COMHUB;
	m_ports[netNumber].PortRate = portRate;
#endif
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::PortSetup
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Get a reference to the port setup object.

 	\param[in] netNumber Port index to specify [0..NET_CONTROLLER_MAX-1]
**/
//	SYNOPSIS:
portSpec &SysManager::PortSetup(size_t netNumber)
{
	if (netNumber < NET_CONTROLLER_MAX) {
		return(m_ports[netNumber]);
	}
	mnErr eInfo;
	fillInErrs(eInfo, MN_ERR_PARAM_RANGE, _TEK_FUNC_SIG_,
		"Port Index %d should be less than %d", netNumber, NET_CONTROLLER_MAX);
	throw eInfo;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::PortsOpen
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Open port(s) specified by the earlier setup SysManager::PortSetup 

 	\param[in] portCount Number of ports to open.
**/
//	SYNOPSIS:

void SysManager::PortsOpen(size_t portCount)
{
	PortsOpen(int(portCount));
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		SysManager::PortsOpen
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**
Open port(s) specified by the earlier setup SysManager::PortSetup

\param[in] portCount Number of ports to open.
**/
//	SYNOPSIS:

void SysManager::PortsOpen(int portCount)
{
	cnErrCode theErr = mnInitializeSystem(false, portCount, m_ports);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
	// Save for restarts
	m_portOpenCount = portCount;
}
//																			  *
//*****************************************************************************



//*****************************************************************************
//	NAME																	  *
//		SysManager::Ports
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Get reference to our port object.

 	\param[in] index Port index to specify [0..NET_CONTROLLER_MAX-1]
	\return Reference to the port object for this system.
**/
IPort &SysManager::Ports(size_t netNumber)
{ 
	if (netNumber < NET_CONTROLLER_MAX) {
		return *SysInventory[netNumber].pPortCls;
	}
	// Bad call
	mnErr eInfo;
	fillInErrs(eInfo, NULL, MN_ERR_PARAM_RANGE, _TEK_FUNC_SIG_, 
		"Port Index %d should be less than %d", netNumber, NET_CONTROLLER_MAX);
	throw eInfo;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::Shutdown
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Close operations all the ports.
**/
void SysManager::PortsClose() 
{
	mnShutdown();
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::RestartCold
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Close and re-open port, restart nodes to powered-up	state and re-inventory 
	for nodes.
**/
void SysManager::RestartCold() 
{
	cnErrCode theErr = mnInitializeSystem(true, nodeulong(m_portOpenCount), m_ports);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::TimeStampMs
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Get time stamp with millisecond resolution.
**/
double SysManager::TimeStampMsec()
{
	return infcCoreTime();
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::TimeStampMsecStr
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**
Get time stamp with millisecond resolution.
**/
std::string &SysManager::TimeStampMsecStr(std::string &toUpdString)
{
	double tNow = infcCoreTime();
	char buffer[40];
	snprintf(buffer, sizeof(buffer), "%.1f", tNow);
	toUpdString = buffer;
	return toUpdString;
}
std::string SysManager::TimeStampMsecStr() 
{
	double tNow = infcCoreTime();
	std::string rStr;
	char buffer[40];
	snprintf(buffer, sizeof(buffer), "%.1f", tNow);
	rStr = buffer;
	return rStr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::RestartWarm
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**
Close and re-open all the ports then detect attached nodes.
**/
void SysManager::RestartWarm()
{
	cnErrCode theErr = mnInitializeSystem(false, nodeulong(m_portOpenCount), m_ports);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::Delay
//
//	AUTHOR:
//		DWS - created on 2016-01-21
//
//	DESCRIPTION:
/**
Suspend thread for specified number of milliseconds.

\param [in] msec Delay in milliseconds.
**/
void SysManager::Delay(uint32_t msec)
{
	infcSleep(msec);
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::Delay
//
//	AUTHOR:
//		DWS - created on 2016-03-16
//
//	DESCRIPTION:
/**
Return an INode object reference for the node at \a theMultiAddr.

\param [in] theMultiAddr Address of the desired node.
**/
INode &SysManager::NodeGet(multiaddr theMultiAddr)
{
	mnErr eInfo;
	if (theMultiAddr == MN_UNSET_ADDR) {
		fillInErrs(eInfo, MN_ERR_BADARG, _TEK_FUNC_SIG_, "Unset address");
		throw eInfo;
	}
	netaddr theNetAddr = NET_NUM(theMultiAddr);
	nodeaddr theNodeAddr = NODE_ADDR(theMultiAddr);
	// Is the request in range of port count
	if (theNetAddr > SysPortCount || theNetAddr > NET_CONTROLLER_MAX) {
		fillInErrs(eInfo, MN_ERR_BADARG, _TEK_FUNC_SIG_, "Net Number out of range");
		throw eInfo;
	}
	// Is there a node here?
	if (theNodeAddr > SysInventory[theNetAddr].InventoryNow.NumOfNodes
		|| !SysInventory[theNetAddr].pNodes[theNodeAddr]) {
		fillInErrs(eInfo, MN_ERR_BADARG, _TEK_FUNC_SIG_, 
				"No nodes at multi-address %d", theMultiAddr);
		throw eInfo;
	}
	return *SysInventory[theNetAddr].pNodes[theNodeAddr];
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		SysManager::SysManager
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Create the management object for interacting with ClearPath-SC nodes
	attached to serial ports.
**/
SysManager::SysManager()
{
	m_portOpenCount = 0;
}

SysManager::~SysManager()
{
	PortsClose();
}

//																			  *
//*****************************************************************************

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IPort Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

//*****************************************************************************
//	NAME																	  *
//		IPort::Nodes
//
//	AUTHOR:
//		DWS - created on 2015-12-16
//
//	DESCRIPTION:
/**		
	Return a reference to the indexed node on this port.

 	\param[in] index Node index to specify [0..(MN_API_MAX_NODES-1)]
	/return INode reference
**/
INode &IPort::Nodes(size_t index)
{
	if (index < MN_API_MAX_NODES)
		return(*SysInventory[m_netNumber].pNodes[index]);
	mnErr tErr;
	fillInErrs(tErr, MN_ERR_BADARG, _TEK_FUNC_SIG_, "");
	throw tErr;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//			INTERNAL DOCUMENTED BELOW HERE
//
/// \cond INTERNAL_DOC
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
IPort::IPort(netaddr index, IBrakeControl &brake, IGrpShutdown &shtDwn, 
			 IPortAdv &adv)
	: m_netNumber(index), GrpShutdown(shtDwn), BrakeControl(brake), Adv(adv)
{
}

IPort::~IPort()
{
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IObj Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
IObj::IObj() 
{ 
	m_supported = false; 
}

IObj::IObj(bool support) 
{ 
	m_supported = support; 
}

/// Returns true if this object is supported by this node
bool IObj::Supported() 
{ 
	return m_supported; 
}

void IObj::Supported(bool newState) 
{ 
	m_supported = newState; 
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IObjWithNode/Port Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
IObjWithNode::IObjWithNode(INode &ourNode) 
{
	m_pNode = &ourNode;
}

IObjWithPort::IObjWithPort(IPort &ourPort) 
{
	m_pPort = &ourPort;
}



//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Base class constructors for our virtual classes with port references.
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
IBrakeControl::IBrakeControl(IPort &ourPort) 
	: IObjWithPort(ourPort)
{
}

IGrpShutdown::IGrpShutdown(IPort &ourPort) 
	: IObjWithPort(ourPort)
{
}

SysCPMbrakeControl::SysCPMbrakeControl(IPort &ourPort)
	: IBrakeControl(ourPort) 
{
}

SysCPMgrpShutdown::SysCPMgrpShutdown(IPort &ourPort)
	: IGrpShutdown(ourPort) 
{
}

SysCPMattnPort::SysCPMattnPort(IPort &ourPort)
	: IAttnPort(ourPort)
{
}

IAttnPort::IAttnPort(IPort &ourPort)
	: IObjWithPort(ourPort),
	AttnsEnabled(false),
	AttnCallback(NULL)
{
}

IAttnPort::~IAttnPort()
{
}

IPortAdv::IPortAdv(IPort &ourPort, IAttnPort &attnPort)
	: IObjWithPort(ourPort), Attn(attnPort)
{

}

bool IPortAdv::Supported()
{
	bool isAdv = true;
	// If any node is not advanced, port functions are not advanced
	for (nodeaddr i=0; i<m_pPort->NodeCount(); i++) {
		isAdv &= m_pPort->Nodes(i).Adv.Supported();
	}
	return(isAdv);
}


SysCPMportAdv::SysCPMportAdv(IPort &ourPort)
	: IPortAdv(ourPort, m_attnPort), m_attnPort(ourPort)
{

}

SysCPMport::SysCPMport(netaddr index)
	: IPort(index, m_brakeControl, m_groupShutdown,  m_advPort),
	m_brakeControl(*this), m_groupShutdown(*this), 
	m_advPort(*this)
{
	bool isComHub = SysInventory[index].PhysPortSpecifier.PortType == CPM_COMHUB;
	m_brakeControl.Supported(isComHub);
	m_groupShutdown.Supported(isComHub);
}

SysCPMport::~SysCPMport()
{
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Base class constructors for our virtual classes with node references
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
IAttnNode::IAttnNode(INode &ourNode,
	nodeparam attnMaskPnum, nodeparam warnMaskPnum,
	nodeparam alertMaskPnum, nodeparam statusMaskPnum)
	: IObjWithNode(ourNode),
	Mask(ourNode, attnMaskPnum, false),
	WarnMask(ourNode, warnMaskPnum, false),
	AlertMask(ourNode, alertMaskPnum, false),
	StatusMask(ourNode, statusMaskPnum, false)
{

}


IHoming::IHoming(INode &ourNode, nodeparam accPnum, nodeparam velLimPnum,
			     nodeparam clampPnum, nodeparam offsPnum, nodeparam postMvDlyPnum)
	: IObjWithNode(ourNode)
{
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#endif
IInfo::IInfo(INode &ourNode,
			 IInfoAdv &adv, IInfoEx &ex,
			 nodeparam snPnum, nodeparam fwPnum, nodeparam usrRamPnum, 
			 ValueString &userID, ValueString &hwVer, ValueString &fwStr,
			 ValueString &model, nodeparam resPnum)
	: IObjWithNode(ourNode),
	SerialNumber(ourNode, snPnum),
	FirmwareVersion(fwStr),
	FirmwareVersionCode(ourNode, fwPnum),
	HardwareVersion(hwVer),
	Model(model),
	UserID(userID),
	UserRAM(ourNode, usrRamPnum),
	PositioningResolution(ourNode, resPnum),
	Ex(ex), Adv(adv)

{
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

IInfoAdv::IInfoAdv(INode &ourNode) 
	: IObjWithNode(ourNode)
{
}

IInfoEx::IInfoEx(INode &ourNode)
	: IObjWithNode(ourNode), m_addr(ourNode.m_addr)
{
}

multiaddr IInfoEx::Addr()
{
	return m_addr;
}

nodeaddr IInfoEx::NodeIndex()
{
	return NODE_ADDR(m_addr);
}

ILimits::ILimits(INode &ourNode, ILimitsAdv &adv, 
				nodeparam globTrqPnum, nodeparam trkLimPnum,
				nodeparam softLim1Pnum, nodeparam softLim2Pnum,
				nodeparam mtrSpdLimPnum)
	: IObjWithNode(ourNode),
	TrqGlobal(ourNode, globTrqPnum), 
	SoftLimit1(ourNode, softLim1Pnum),
	SoftLimit2(ourNode, softLim2Pnum),
	PosnTrackingLimit(ourNode, trkLimPnum),
	MotorSpeedLimit(ourNode, mtrSpdLimPnum),
	Adv(adv)
{
}

ILimitsAdv::ILimitsAdv(INode &ourNode,
	nodeparam posFldkPnum, nodeparam posFldkTCpNum,
	nodeparam negFldkPnum, nodeparam negFldkTCpNum)
	: IObjWithNode(ourNode),
	PositiveTrq(ourNode, posFldkPnum),
	PositiveRelaxTCmsec(ourNode, posFldkTCpNum),
	NegativeTrq(ourNode, negFldkPnum),
	NegativeRelaxTCmsec(ourNode, negFldkTCpNum)
{
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#endif
INodeAdv::INodeAdv(INode &ourNode, IMotionAudit &audit, IAttnNode &attnNode)
	: IObjWithNode(ourNode), MotionAudit(audit), Attn(attnNode)
{
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

INodeEx::INodeEx(INode &ourNode)
	: IObjWithNode(ourNode)
{
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IAttnNode Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
void IAttnNode::SignalAttn(mnStatusReg theAttn)
{
	INode::MyEvent myEvent(Node());
	m_attn.attnBits |= theAttn.attnBits;

	myEvent.Signal();

}

void IAttnNode::ClearAttn(mnStatusReg attnClr)
{
	m_attn.attnBits &= ~attnClr.attnBits;
}

mnStatusReg IAttnNode::WaitForAttn(mnStatusReg theAttn, int32_t timeoutMs, bool autoClear)
{
	INode::MyEvent myEvent(Node());
	mnStatusReg retVal;
	double endTime = infcCoreTime() + timeoutMs;
	double timeRemaining = timeoutMs;

	myEvent.Register();

	while (timeoutMs < 0 || timeRemaining > 0) {
		retVal.attnBits = m_attn.attnBits & theAttn.attnBits;
		if (retVal.attnBits){
			break;
		}
		myEvent.Wait((DWORD)timeRemaining);

		timeRemaining = endTime - infcCoreTime();
	}

	// Timed out, do one last refresh 
	retVal.attnBits = m_attn.attnBits & theAttn.attnBits;
	if (autoClear)
		ClearAttn(retVal);
	
	myEvent.Unregister();

	return retVal;
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IAttnPort Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
bool IAttnPort::HasAttnHandler()
{
	return AttnCallback != NULL;
}

void IAttnPort::InvokeAttnHandler(const mnAttnReqReg &detected)
{
	// Serialize the calls from each port to avoid re-entrancy
	UseAttnMutex lock;
	if (AttnCallback) {
		(*AttnCallback)(detected);
	}
}

void IAttnPort::AttnHandler(mnAttnCallback theNewHandler)
{
	AttnCallback = theNewHandler;
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IMotion Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
IMotion::IMotion(INode &ourNode, IMotionAdv &adv, IHoming &homing,
	nodeparam accLimPnum, nodeparam velLimPnum, nodeparam stopDecLimPnum,
	nodeparam posnMeasPnum, nodeparam posnCmdPnum, nodeparam posnTrkPnum,
	nodeparam velMeasPnum, nodeparam velCmdPnum,
	nodeparam trqMeasPnum, nodeparam trqCmdPnum, nodeparam jrkLimPnum, 
	nodeparam dwellPnum, nodeparam jrkDelayPnum)
	: IObjWithNode(ourNode),
	AccLimit(ourNode, accLimPnum),
	VelLimit(ourNode, velLimPnum),
	JrkLimit(ourNode, jrkLimPnum),
	JrkLimitDelay(ourNode, jrkDelayPnum),
	DwellMs(ourNode, dwellPnum),
	NodeStopDecelLim(ourNode, stopDecLimPnum),
	PosnMeasured(ourNode, posnMeasPnum),
	PosnCommanded(ourNode, posnCmdPnum),
	PosnTracking(ourNode, posnTrkPnum),
	VelMeasured(ourNode, velMeasPnum),
	VelCommanded(ourNode, velCmdPnum),
	TrqMeasured(ourNode, trqMeasPnum),
	TrqCommanded(ourNode, trqCmdPnum), Adv(adv), Homing(homing)
{
}

double IMotion::MovePosnDurationMsec(int32_t target, bool targetIsAbsolute)
{
	double retVal = 0;
	int32_t relativeDist;
	double accelTime, slewTime;
	double accelDecelCnts, slewCnts;
	double velLimCntPerSec;
	double accLimCntPerSec2;


	if (targetIsAbsolute){
		PosnCommanded.Refresh();
		relativeDist = abs(target - (int32_t)PosnCommanded.Value());
	}
	else {
		relativeDist = abs(target);
	}

	switch (Node().VelUnit()){
		case INode::RPM:
			velLimCntPerSec = VelLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
			break;
		case INode::COUNTS_PER_SEC:
			velLimCntPerSec = VelLimit.Value();
			break;
	}

	switch (Node().AccUnit()){
		case INode::RPM_PER_SEC:
			accLimCntPerSec2 = AccLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
			break;
		case INode::COUNTS_PER_SEC2:
			accLimCntPerSec2 = AccLimit.Value();
			break;
	}

	// We are calculating the time for a symmetric move so accel info 
	// will be used to calculate both accel and decel times and distances
	accelTime = velLimCntPerSec / accLimCntPerSec2;
	accelDecelCnts = accelTime * velLimCntPerSec;

	//Check if the distance required to accelerate to full speed is greater than the move distance
	if (accelDecelCnts >= relativeDist){
		// The triangle move to peak velocity exceeds the move distance
		// The actual move will be a smaller similar triangle
		// Distance traveled scales at the square of the time, 
		// so rescale the time by the sqrt of the distance ratio
		accelTime = accelTime * sqrt(relativeDist / accelDecelCnts);
		accelDecelCnts = relativeDist;
		slewTime = slewCnts = 0;
	}
	else {
		// Distance requested is greater than the triangle move distance
		// Calculate the constant velocity time
		slewCnts = relativeDist - accelDecelCnts;
		slewTime = slewCnts / velLimCntPerSec;
	}

	// The move duration is the sum of the accel, slew, and decel times
	retVal = accelTime * 2 + slewTime;

	// Convert from sec to ms
	retVal *= 1000;

	// Account for the RAS time
	retVal += JrkLimitDelay.Value();

	return retVal;
}

double IMotion::MoveVelDurationMsec(double target)
{
	double retVal = 0;
	double velChange;
	double accLimCntPerSec2;

	switch (Node().AccUnit()){
		case INode::RPM_PER_SEC:
			accLimCntPerSec2 = AccLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
			break;
		case INode::COUNTS_PER_SEC2:
			accLimCntPerSec2 = AccLimit.Value();
			break;
	}

	// Refresh the current velocity so the difference can be calculated
	VelCommanded.Refresh();
	velChange = fabs(target - VelCommanded.Value());

	// Translate to cnts/sec
	if (Node().VelUnit() == INode::RPM) {
		velChange = velChange / 60 * Node().Info.PositioningResolution.Value();
	}

	// Calculate the time to change velocity by the given amount
	retVal = velChange / accLimCntPerSec2;

	// Convert from sec to ms
	retVal *= 1000;

	// Account for the RAS time
	retVal += JrkLimitDelay.Value();

	return retVal;
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// IMotionAdv Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
IMotionAdv::IMotionAdv(INode &ourNode,
	nodeparam decLimPnum, nodeparam headTailVlimPnum, nodeparam headDxPnum,
	nodeparam tailDxPnum, nodeparam aDxPnum, nodeparam bDxPnum)
	: IObjWithNode(ourNode),
	DecelLimit(ourNode,decLimPnum),
	HeadTailVelLimit(ourNode, headTailVlimPnum),
	HeadDistance(ourNode, headDxPnum),
	TailDistance(ourNode, tailDxPnum),
	AfterStartDistance(ourNode, aDxPnum),
	BeforeEndDistance(ourNode, bDxPnum)
{
}

double IMotionAdv::MovePosnDurationMsec(int32_t target, 
	bool targetIsAbsolute){
	return Node().Motion.MovePosnDurationMsec(target, targetIsAbsolute);
}

double IMotionAdv::MoveVelDurationMsec(double target){
	return Node().Motion.MoveVelDurationMsec(target);
}

double IMotionAdv::MovePosnHeadTailDurationMsec(int32_t target, 
							bool targetIsAbsolute,
							bool hasHead, bool hasTail)
{
	double retVal = 0;
	int32_t relativeDist;
	double htAccelTime, htSlewTime = 0;
	double htAccelCnts, htSlewCnts = 0;
	double htVelLimCntPerSec;
	double accLimCntPerSec2;

	if (targetIsAbsolute){
		Node().Motion.PosnCommanded.Refresh();
		relativeDist = abs(target - (int32_t)Node().Motion.PosnCommanded.Value());
	}
	else {
		relativeDist = abs(target);
	}

	// If HTVelLim > VelLim treat this as a normal move
	if (HeadTailVelLimit.Value() < Node().Motion.VelLimit.Value() ){

		switch (Node().VelUnit()){
			case INode::RPM:
				htVelLimCntPerSec = HeadTailVelLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
				break;
			case INode::COUNTS_PER_SEC:
				htVelLimCntPerSec = HeadTailVelLimit.Value();
				break;
		}

		switch (Node().AccUnit()){
			case INode::RPM_PER_SEC:
				accLimCntPerSec2 = Node().Motion.AccLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
				break;
			case INode::COUNTS_PER_SEC2:
				accLimCntPerSec2 = Node().Motion.AccLimit.Value();
				break;
		}

		htAccelTime = htVelLimCntPerSec / accLimCntPerSec2;
		htAccelCnts = htAccelTime * htVelLimCntPerSec / 2;
		
		htSlewCnts = 0;
		if (hasHead){
			htSlewCnts += max(0., HeadDistance.Value() - htAccelCnts);
		}
		if (hasTail){
			htSlewCnts += max(0., TailDistance.Value() - htAccelCnts);
		}

		htSlewCnts = min(htSlewCnts, relativeDist - (2 * htAccelCnts));
		htSlewCnts = max(htSlewCnts, 0.);
		htSlewTime = htSlewCnts / htVelLimCntPerSec;
	}

	retVal = MovePosnDurationMsec(relativeDist - (int32_t)htSlewCnts) + (htSlewTime * 1000);

	return retVal;
}

double IMotionAdv::MovePosnAsymDurationMsec(int32_t target, 
							bool targetIsAbsolute)
{
	double retVal = 0;
	int32_t relativeDist;
	double accelTime, decelTime, slewTime;
	double accelCnts, decelCnts, slewCnts;
	double velLimCntPerSec;
	double accLimCntPerSec2;
	double decLimCntPerSec2;


	if (targetIsAbsolute){
		Node().Motion.PosnCommanded.Refresh();
		relativeDist = abs(target - (int32_t)Node().Motion.PosnCommanded.Value());
	}
	else {
		relativeDist = abs(target);
	}

	switch (Node().VelUnit()){
		case INode::RPM:
			velLimCntPerSec = Node().Motion.VelLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
			break;
		case INode::COUNTS_PER_SEC:
			velLimCntPerSec = Node().Motion.VelLimit.Value();
			break;
	}

	switch (Node().AccUnit()){
		case INode::RPM_PER_SEC:
			accLimCntPerSec2 = Node().Motion.AccLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
			decLimCntPerSec2 = DecelLimit.Value() / 60 * Node().Info.PositioningResolution.Value();
			break;
		case INode::COUNTS_PER_SEC2:
			accLimCntPerSec2 = Node().Motion.AccLimit.Value();
			decLimCntPerSec2 = DecelLimit.Value();
			break;
	}

	// Calculate accel and decel times and distances
	accelTime = velLimCntPerSec / accLimCntPerSec2;
	accelCnts = accelTime * velLimCntPerSec / 2;
	decelTime = velLimCntPerSec / decLimCntPerSec2;
	decelCnts = decelTime * velLimCntPerSec / 2;

	//Check if the distance required to accelerate to full speed is greater than the move distance
	if ((accelCnts + decelCnts) >= relativeDist){
		// The triangle move to peak velocity exceeds the move distance
		// The actual move will be a smaller similar triangle
		// Distance traveled scales at the square of the time, 
		// so rescale the time by the sqrt of the distance ratio
		double scaleFactor = sqrt(relativeDist / (accelCnts + decelCnts));
		accelTime *= scaleFactor;
		decelTime *= scaleFactor;

		scaleFactor = relativeDist / (accelCnts + decelCnts);
		accelCnts *= scaleFactor;
		decelCnts *= scaleFactor;
		slewTime = slewCnts = 0;
	}
	else {
		// Distance requested is greater than the triangle move distance
		// Calculate the constant velocity time
		slewCnts = relativeDist - (accelCnts + decelCnts);
		slewTime = slewCnts / velLimCntPerSec;
	}

	// The move duration is the sum of the accel, slew, and decel times
	retVal = accelTime + decelTime + slewTime;

	// Convert from sec to ms
	retVal *= 1000;

	// Account for the RAS time
	retVal += Node().Motion.JrkLimitDelay.Value();

	return retVal;
}


IMotionAudit::IMotionAudit(INode &ourNode) 
	: IObjWithNode(ourNode)
{
}

IOuts::IOuts(INode &ourNode,
			nodeparam usrOutPnum, nodeparam outPnum) 
	: IObjWithNode(ourNode),
	User(ourNode, usrOutPnum, false, true),
	Out(ourNode, outPnum, false, false)
{
}


ISetupEx::ISetupEx(INode &ourNode, 
					nodeparam appPnum, nodeparam hwPnum, nodeparam netWdPnum)
	: IObjWithNode(ourNode),
	NetWatchdogMsec(ourNode, netWdPnum),
	App(ourNode, appPnum), 
	HW(ourNode, hwPnum)
{

}
ISetup::ISetup(INode &ourNode, 
				nodeparam appPnum, nodeparam hwPnum, nodeparam netWdPnum,
				nodeparam delayPnum)
	: IObjWithNode(ourNode),
	DelayToDisableMsecs(ourNode, delayPnum),
	Ex(ourNode, appPnum, hwPnum, netWdPnum)
{

}

IStatus::IStatus(INode &ourNode, IStatusAdv &adv,
	nodeparam rtPnum, nodeparam risePnum, nodeparam fallPnum, nodeparam accumPnum,
	nodeparam warnPnum, nodeparam alertPnum, nodeparam rmsLvlPnum,
	nodeparam powerPnum)
	: IObjWithNode(ourNode),
	RT(ourNode, rtPnum, false), Rise(ourNode, risePnum, true),
	Fall(ourNode, fallPnum, true), Accum(ourNode, accumPnum, true),
	Warnings(ourNode, warnPnum, true), Alerts(ourNode, alertPnum, false),
	RMSlevel(ourNode, rmsLvlPnum), Power(ourNode, powerPnum, true), Adv(adv)
{
}

IStatusAdv::IStatusAdv(INode &ourNode, nodeparam hiResCapPnum, nodeparam capPnum)
	: IObjWithNode(ourNode),
	CapturedHiResPosn(ourNode, hiResCapPnum),
	CapturedPos(ourNode, capPnum)
{
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// INode Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
INode::INode(IPort &port, multiaddr theAddr,
			 IInfo &info, IMotion &motion, IStatus &status, 
			 ILimits &limits, IOuts &outs, INodeAdv &adv, ISetup &setup,
			 INodeEx &ex) 
	: m_addr(theAddr), Port(port), Info(info), Motion(motion), Status(status),
	  Limits(limits), Outs(outs), Setup(setup),
	  Ex(ex), Adv(adv)

{ 
	m_AccUnit = INode::RPM_PER_SEC;
	m_VelUnit = INode::RPM;
	m_TrqUnit = INode::PCT_MAX;
}
/// END INTERNAL_DOC
/// \endcond

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ValueBase Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueBase::ValueBase(INode &node, nodeparam pNum)
: IObjWithNode(node), m_paramNum(pNum), m_valid(false)
{
	m_exists = true;
	m_scaleToUser = 1.0;
	m_refreshOnAccess = false;
	m_isVolatile = false;
}


void ValueBase::Exists(bool newState)
{
	m_exists = newState;
}

void ValueBase::ParamNum(nodeparam newParamNum)
{
	m_valid = false;
	m_paramNum = newParamNum;
}

void ValueBase::Scale(double newScale)
{
	m_scaleToUser = newScale;
	m_valid = false;
}

void ValueBase::Valid(bool newState) 
{
	m_valid = false;
}
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamDouble Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueDouble::ValueDouble(INode &node, nodeparam pNum, enum _srcs src)
	:ValueBase(node, pNum), m_src(src)
	
{
	m_currentValue = 0;
	m_lastValue = 0;
}

void ValueDouble::Value(int32_t newValue, bool makeNonVolatile)
{
	Value(double(newValue), makeNonVolatile);
}

void ValueDouble::Value(double newValue, bool makeNonVolatile)
{
	// Write out new value
	Node().Info.Ex.Parameter(ParamNum(), newValue/m_scaleToUser);
	// Save new power-on default as well
	if (makeNonVolatile)
		Node().Info.Ex.Parameter(ParamNum()+PARAM_OPT_MASK, 
								 newValue/m_scaleToUser);
	// Save last value
	m_lastValue = m_currentValue;
	// Account for truncations
	m_currentValue = Node().Info.Ex.Parameter(ParamNum())*m_scaleToUser;
	m_valid = true;
}

double ValueDouble::Value(bool getNonVolatile)
{
	return getNonVolatile ? Node().Info.Ex.Parameter(ParamNum()+PARAM_OPT_MASK)
						  : operator double();
}

double ValueDouble::operator=(double newValue) 
{
	Value(newValue);
	return m_currentValue;
}

/**
	Update a numeric parameter using a floating point value.
**/
ValueDouble::operator double() 
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(m_currentValue);
}
/**
Update a numeric parameter using a 32-bit integer value.
**/
ValueDouble::operator int32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(int32_t(m_currentValue));
}

ValueDouble::operator uint32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(uint32_t(m_currentValue));
}


void ValueDouble::Refresh()
{
	double rdVal = Node().Info.Ex.Parameter(ParamNum());
	m_lastValue = m_currentValue;
	m_currentValue = rdVal*m_scaleToUser;
	m_valid = true;
}



//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamSigned Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueSigned::ValueSigned(INode &node, nodeparam pNum)
	:ValueBase(node, pNum)

{
	m_lastValue = 0;
	m_currentValue = 0;
}

void ValueSigned::Value(int32_t newValue, bool makeNonVolatile)
{
	// Write out new value
	Node().Info.Ex.Parameter(ParamNum(), newValue);
	// Save new power-on default as well
	if (makeNonVolatile)
		Node().Info.Ex.Parameter(ParamNum() + PARAM_OPT_MASK, newValue);
	// Save last value
	m_lastValue = m_currentValue;
	// Account for truncations
	m_currentValue = int32_t(Node().Info.Ex.Parameter(ParamNum()));
	m_valid = true;
}

int32_t ValueSigned::Value(bool getNonVolatile)
{
	return int32_t(
		getNonVolatile ? Node().Info.Ex.Parameter(ParamNum() + PARAM_OPT_MASK)
		: operator int32_t());
}

int32_t ValueSigned::operator=(int32_t newValue)
{
	Value(newValue);
	return m_currentValue;
}

ValueSigned::operator double()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(double(m_currentValue));
}
ValueSigned::operator int32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(m_currentValue);
}

ValueSigned::operator uint32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(uint32_t(m_currentValue));
}


void ValueSigned::Refresh()
{
	int32_t rdVal = int32_t(Node().Info.Ex.Parameter(ParamNum()));
	m_lastValue = m_currentValue;
	m_currentValue = rdVal;
	m_valid = true;
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamUnsigned Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueUnsigned::ValueUnsigned(INode &node, nodeparam pNum)
	:ValueBase(node, pNum)

{
	m_lastValue = 0;
	m_currentValue = 0;
}

void ValueUnsigned::Value(uint32_t newValue, bool makeNonVolatile)
{
	// Write out new value
	Node().Info.Ex.Parameter(ParamNum(), newValue);
	// Save new power-on default as well
	if (makeNonVolatile)
		Node().Info.Ex.Parameter(ParamNum() + PARAM_OPT_MASK, newValue);
	// Save last value
	m_lastValue = m_currentValue;
	// Account for truncations
	m_currentValue = uint32_t(Node().Info.Ex.Parameter(ParamNum()));
	m_valid = true;
}

uint32_t ValueUnsigned::Value(bool getNonVolatile)
{
	return uint32_t(
		getNonVolatile ? Node().Info.Ex.Parameter(ParamNum() + PARAM_OPT_MASK)
		: operator uint32_t());
}

uint32_t ValueUnsigned::operator=(uint32_t newValue)
{
	Value(newValue);
	return m_currentValue;
}

ValueUnsigned::operator double()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(double(m_currentValue));
}

ValueUnsigned::operator uint32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(m_currentValue);
}


void ValueUnsigned::Refresh()
{
	uint32_t rdVal = uint32_t(Node().Info.Ex.Parameter(ParamNum()));
	m_lastValue = m_currentValue;
	m_currentValue = rdVal;
	m_valid = true;
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ValueString Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueString::ValueString(INode &node, nodeparam pNum, enum _srcs location)
	: ValueBase(node, pNum), m_location(location)
{
	m_currentValue[0] = 0;
	m_lastValue[0] = 0;
}

const char *ValueString::operator=(const char *newValue) 
{
	Value(newValue);
	return newValue;
}

ValueString::operator const char *()
{
	return m_currentValue;
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamOutReg Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueOutReg::ValueOutReg(INode &node, nodeparam pNum, 
						 bool clearOnReadType, bool writable)
	:ValueBase(node, pNum), m_clearOnRead(clearOnReadType), m_writeable(writable)

{

}

mnOutReg ValueOutReg::Value()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return m_currentValue;
}

void ValueOutReg::Value(const mnOutReg &newValue)
{
	cnErrCode theErr;
	// Take the mutex
	INode::UseMutex myLock(Node());
	theErr = netSetParameterDbl(Node().Info.Ex.Addr(),
		mnParams(ParamNum()),
		newValue.bits);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue.bits = m_currentValue.bits;
		m_currentValue.bits = newValue.bits;
		m_valid = true;
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}

mnOutReg &ValueOutReg::operator=(mnOutReg &newValue)
{
	Value(newValue);
	return m_currentValue;
}

ValueOutReg::operator mnOutReg()
{
	return Value();
}

ValueOutReg::operator double()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(double(m_currentValue.bits));
}
ValueOutReg::operator int32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(int32_t(m_currentValue.bits));
}

ValueOutReg::operator uint32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(uint32_t(m_currentValue.bits));
}

/**
\brief Update the local values from the node.

If this status is an accumulating type, the local copy is updated with
any new bits.

\see ParamOutReg::Clear to clear any accumulations.
**/
void ValueOutReg::Refresh()
{
	cnErrCode theErr;
	paramValue val;
	// Take the mutex
	INode::UseMutex myLock(Node());
	// Get current bits
	theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
								mnParams(ParamNum()), NULL, &val);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue = m_currentValue;
		// Convert to integer equivalent
		m_currentValue.bits = Uint16(val.value);
		// If clear-on-read type, OR accumulate reading
		if (m_clearOnRead) {
			m_currentValue.bits |= m_lastValue.bits;
		}
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
Set bits in thread safe way.
**/
void ValueOutReg::Set(const mnOutReg &mask)
{
	paramValue val;
	cnErrCode theErr = MN_OK;
	if (m_writeable) {
		// Take the mutex
		INode::UseMutex myLock(Node());
		theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
									mnParams(ParamNum()), NULL, &val);
		m_valid = theErr == MN_OK;
		if (m_valid) {
			m_lastValue.bits = m_currentValue.bits;
			m_currentValue.bits |= mask.bits;
			Node().Info.Ex.Parameter(ParamNum(), m_currentValue.bits);
			return;
		}
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
Clear bits in thread safe way.
**/
void ValueOutReg::Clear(const mnOutReg &mask)
{
	paramValue val;
	cnErrCode theErr = MN_OK;
	if (m_writeable) {
		// Take the mutex
		INode::UseMutex myLock(Node());
		theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
									mnParams(ParamNum()), NULL, &val);
		m_valid = theErr == MN_OK;
		if (m_valid) {
			m_lastValue.bits = m_currentValue.bits;
			m_currentValue.bits &= ~mask.bits;
			Node().Info.Ex.Parameter(ParamNum(), m_currentValue.bits);
			return;
		}
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ValuePowerReg Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValuePowerReg::ValuePowerReg(INode &node, nodeparam pNum, bool clearOnReadType)
	:ValueBase(node, pNum), m_clearOnRead(clearOnReadType)

{

}

/**
\brief Get a reference to the current alert register state and optionally
refresh the status register if ValueBase::AutoRefresh is set.
**/
mnPowerReg ValuePowerReg::Value()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return m_currentValue;
}


/**
\brief Update the local values from the node.

If this status is an accumulating type, the local copy is updated with
any new bits.

\see ParamAlert::Clear to clear any accumulations.
**/
void ValuePowerReg::Refresh()
{
	cnErrCode theErr;
	double val;
	// Take the lock
	INode::UseMutex myLock(Node());
	// Get current bits
	theErr = netGetParameterDbl(Node().Info.Ex.Addr(), 
								mnParams(ParamNum()), &val);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue = m_currentValue;
		// Convert to integer equivalent
		m_currentValue.bits = uint32_t(val);
		// If clear-on-read type, OR accumulate reading
		if (m_clearOnRead) {
			m_currentValue.bits |= m_lastValue.bits;
		}
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
Clear accumulated status
**/
void ValuePowerReg::Clear()
{
	// Take the lock
	INode::UseMutex myLock(Node());
	m_currentValue.bits = 0;
	m_lastValue.bits = 0;
}

/**
\brief Read the register, test and clear accumulated status in thread-safe manner
using the mask. The set bits are returned

\return true if result is non-zero.
**/
bool ValuePowerReg::TestAndClear(const mnPowerReg &mask, mnPowerReg &result)
{
	Refresh();
	// Take the lock
	INode::UseMutex myLock(Node());
	result.bits = m_currentValue.bits & mask.bits;
	m_currentValue.bits &= ~mask.bits;
	return result.bits != 0;
}
//																			  *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamAppConfigReg Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueAppConfigReg::ValueAppConfigReg(INode &node, nodeparam pNum)
	:ValueBase(node, pNum)
{
	m_lastValue.bits = 0;
	m_currentValue.bits = 0;
	m_testResult.bits = 0;
}

mnAppConfigReg ValueAppConfigReg::Value()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return m_currentValue;
}

void ValueAppConfigReg::Value(const mnAppConfigReg &newValue)
{
	cnErrCode theErr;
	theErr = netSetParameterDbl(Node().Info.Ex.Addr(),
		mnParams(ParamNum()),
		newValue.bits);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		setNewVal(newValue);
		m_valid = true;
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}

mnAppConfigReg &ValueAppConfigReg::operator=(mnAppConfigReg &newValue)
{
	Value(newValue);
	return(m_currentValue);
}

ValueAppConfigReg::operator double()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(double(m_currentValue.bits));
}
ValueAppConfigReg::operator int32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(int32_t(m_currentValue.bits));
}

ValueAppConfigReg::operator uint32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(uint32_t(m_currentValue.bits));
}

void ValueAppConfigReg::setNewVal(const mnAppConfigReg &newValue) {
	m_lastValue.bits = m_currentValue.bits;
	m_currentValue.bits = newValue.bits;
}

/**
\brief Update the local values from the node.

If this status is an accumulating type, the local copy is updated with
any new bits.

\see ParamAppConfigReg::Clear to clear any accumulations.
**/
void ValueAppConfigReg::Refresh()
{
	cnErrCode theErr;
	paramValue val;
	// Take the mutex
	INode::UseMutex myLock(Node());
	// Get current bits
	theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
		mnParams(ParamNum()), NULL, &val);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue = m_currentValue;
		// Convert to integer equivalent
		m_currentValue.bits = Uint32(val.value);
		// If clear-on-read type, OR accumulate reading
		m_currentValue.bits |= m_lastValue.bits;
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
Set bits in thread safe way.
**/
void ValueAppConfigReg::Set(const mnAppConfigReg &mask)
{
	// Take the mutex
	INode::UseMutex myLock(Node());
	m_lastValue.bits = m_currentValue.bits;
	m_currentValue.bits |= mask.bits;
	Node().Info.Ex.Parameter(ParamNum(), m_currentValue.bits);
}
/**
Clear bits in thread safe way.
**/
void ValueAppConfigReg::Clear(const mnAppConfigReg &mask)
{
	// Take the mutex
	INode::UseMutex myLock(Node());
	m_lastValue.bits = m_currentValue.bits;
	m_currentValue.bits &= ~mask.bits;
	Node().Info.Ex.Parameter(ParamNum(), m_currentValue.bits);
}
//																			  *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamHwConfigReg Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueHwConfigReg::ValueHwConfigReg(INode &node, nodeparam pNum)
	:ValueBase(node, pNum)
{
	m_lastValue.bits = 0;
	m_currentValue.bits = 0;
	m_testResult.bits = 0;
}

mnHwConfigReg ValueHwConfigReg::Value()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return m_currentValue;
}

void ValueHwConfigReg::Value(const mnHwConfigReg &newValue)
{
	cnErrCode theErr;
	theErr = netSetParameterDbl(Node().Info.Ex.Addr(),
		mnParams(ParamNum()),
		newValue.bits);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue.bits = m_currentValue.bits;
		m_currentValue.bits = newValue.bits;
		m_valid = true;
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}

mnHwConfigReg &ValueHwConfigReg::operator=(mnHwConfigReg &newValue)
{
	Value(newValue);
	return(m_currentValue);
}

ValueHwConfigReg::operator mnHwConfigReg()
{
	return m_currentValue;
}

ValueHwConfigReg::operator double()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(double(m_currentValue.bits));
}
ValueHwConfigReg::operator int32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(int32_t(m_currentValue.bits));
}

ValueHwConfigReg::operator uint32_t()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return(uint32_t(m_currentValue.bits));
}

/**
\brief Update the local values from the node.

If this status is an accumulating type, the local copy is updated with
any new bits.

\see ParamHwConfigReg::Clear to clear any accumulations.
**/
void ValueHwConfigReg::Refresh()
{
	cnErrCode theErr;
	paramValue val;
	// Take the mutex
	INode::UseMutex myLock(Node());
	// Get current bits
	theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
		mnParams(ParamNum()), NULL, &val);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue = m_currentValue;
		// Convert to integer equivalent
		m_currentValue.bits = Uint32(val.value);
		// If clear-on-read type, OR accumulate reading
		m_currentValue.bits |= m_lastValue.bits;
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
Set bits in thread safe way.
**/
void ValueHwConfigReg::Set(const mnHwConfigReg &mask)
{
	// Take the mutex
	INode::UseMutex myLock(Node());
	m_lastValue.bits = m_currentValue.bits;
	m_currentValue.bits |= mask.bits;
	Node().Info.Ex.Parameter(ParamNum(), m_currentValue.bits);
}
/**
Clear bits in thread safe way.
**/
void ValueHwConfigReg::Clear(const mnHwConfigReg &mask)
{
	// Take the mutex
	INode::UseMutex myLock(Node());
	m_lastValue.bits = m_currentValue.bits;
	m_currentValue.bits &= ~mask.bits;
	Node().Info.Ex.Parameter(ParamNum(), m_currentValue.bits);
}
//																			  *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ParamStatus Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueStatus::ValueStatus(INode &node, nodeparam pNum, bool clearOnReadType)
	:ValueBase(node, pNum), m_clearOnRead(clearOnReadType)

{

}
/**
	\brief Get a reference to the current status register state and optionally
	refresh the status register if ValueBase::AutoRefresh is set.
**/
mnStatusReg ValueStatus::Value()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return m_currentValue;
}

void ValueStatus::Value(const mnStatusReg &newValue)
{
	cnErrCode theErr;
	packetbuf nVal;
	switch (ParamNum()) {
	case CPM_P_ATTN_MASK:
		theErr = netSetParameterDbl(Node().Info.Ex.Addr(),
									mnParams(ParamNum()), newValue.attnBits);
		break;
	default:
		memcpy(nVal.Byte.Buffer, newValue.bits, sizeof(newValue.bits));
		nVal.Byte.BufferSize = sizeof(newValue.bits);
		theErr = netSetParameterEx(Node().Info.Ex.Addr(),
			mnParams(ParamNum()), &nVal);
		break;
	}
	m_valid = theErr == MN_OK;
	if (m_valid) {
		memcpy(m_lastValue.bits, m_currentValue.bits, sizeof(m_lastValue.bits));
		memcpy(m_currentValue.bits, newValue.bits, sizeof(m_currentValue.bits));
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}

mnStatusReg &ValueStatus::operator=(mnStatusReg &newValue)
{
	Value(newValue);
	return(m_currentValue);
}

ValueStatus::operator mnStatusReg()
{
	return Value();
}

mnStatusReg ValueStatus::Last()
{
	return m_lastValue;
}


/**
	\brief Update the local values from the node.

	If this status is an accumulating type, the local copy is updated with
	any new bits.

	\see ParamStatus::Clear to clear any accumulations.
**/
void ValueStatus::Refresh()
{
	cnErrCode theErr;
	paramValue val;
	// Take the mutex
	INode::UseMutex myLock(Node());
	// Get current bits
	theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
								 mnParams(ParamNum()), NULL, &val);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue = m_currentValue;
		// Convert to integer equivalent
		memcpy(&m_currentValue, val.raw.Byte.Buffer, sizeof(mnStatusReg));
		// If clear-on-read type, OR accumulate reading
		if (m_clearOnRead) {
			for (size_t i = 0; i < mnStatusReg::N_BITS; i++) {
				m_currentValue.bits[i] |= m_lastValue.bits[i];
			}
		}
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
	Clear accumulated status
**/
void ValueStatus::Clear() 
{
	// Take the mutex
	INode::UseMutex myLock(Node());
	m_currentValue.clear();
	m_lastValue.clear();
}

/**
	\brief Read the register, test and clear accumulated status in thread-safe manner
	using the mask. The set bits are returned

	\return true if result is non-zero.
**/
bool ValueStatus::TestAndClear(const mnStatusReg &mask, mnStatusReg &result)
{
	Refresh();
	// Take the mutex
	INode::UseMutex myLock(Node());
	for (size_t i = 0; i < mnStatusReg::N_BITS; i++) {
		result.bits[i] = m_currentValue.bits[i] & mask.bits[i];
		m_currentValue.bits[i] &= ~mask.bits[i];
	}
	return !result.isClear();
}

/**
\brief Read the register, test and clear accumulated status in thread-safe manner
using the mask. The set bits are returned

\return true if result is non-zero.
**/
bool ValueStatus::TestAndClear(const mnStatusReg &mask)
{
	mnStatusReg result;
	return TestAndClear(mask, result);
}
//																			  *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ValueAlert Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
ValueAlert::ValueAlert(INode &node, nodeparam pNum, bool clearOnReadType)
	:ValueBase(node, pNum), m_clearOnRead(clearOnReadType)

{

}

/**
\brief Get a reference to the current alert register state and optionally
refresh the status register if ValueBase::AutoRefresh is set.
**/
alertReg ValueAlert::Value()
{
	if (!m_valid || m_refreshOnAccess) {
		Refresh();
	}
	return m_currentValue;
}

void ValueAlert::Value(const alertReg &newValue)
{
	cnErrCode theErr;
	packetbuf nVal;
	memcpy(nVal.Byte.Buffer, newValue.bits, sizeof(newValue));
	nVal.Byte.BufferSize = sizeof(newValue);
	theErr = netSetParameterEx(Node().Info.Ex.Addr(),
		mnParams(ParamNum()), &nVal);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		memcpy(m_lastValue.bits, m_currentValue.bits, sizeof(m_lastValue.bits));
		memcpy(m_currentValue.bits, newValue.bits, sizeof(m_currentValue.bits));
		m_valid = true;
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}

alertReg &ValueAlert::operator=(alertReg &newValue)
{
	Value(newValue);
	return(m_currentValue);
}


/**
\brief Update the local values from the node.

If this status is an accumulating type, the local copy is updated with
any new bits.

\see ParamAlert::Clear to clear any accumulations.
**/
void ValueAlert::Refresh()
{
	cnErrCode theErr;
	paramValue val;
	// Take the lock
	INode::UseMutex myLock(Node());
	// Get current bits
	theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
		mnParams(ParamNum()), NULL, &val);
	m_valid = theErr == MN_OK;
	if (m_valid) {
		m_lastValue = m_currentValue;
		// Convert to integer equivalent
		memcpy(&m_currentValue, val.raw.Byte.Buffer, sizeof(alertReg));
		// If clear-on-read type, OR accumulate reading
		if (m_clearOnRead) {
			for (size_t i = 0; i < alertReg::N_BITS; i++) {
				m_currentValue.bits[i] |= m_lastValue.bits[i];
			}
		}
		return;
	}
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "Parameter=%d", ParamNum());
	throw eInfo;
}
/**
Clear accumulated status
**/
void ValueAlert::Clear()
{
	// Take the lock
	INode::UseMutex myLock(Node());
	m_currentValue.clear();
	m_lastValue.clear();
}

/**
\brief Read the register, test and clear accumulated status in thread-safe manner
using the mask. The set bits are returned

\return true if result is non-zero.
**/
bool ValueAlert::TestAndClear(const alertReg &mask, alertReg &result)
{
	Refresh();
	// Take the lock
	INode::UseMutex myLock(Node());
	for (size_t i = 0; i < alertReg::N_BITS; i++) {
		result.bits[i] = m_currentValue.bits[i] & mask.bits[i];
		m_currentValue.bits[i] &= ~mask.bits[i];
	}
	return !result.isClear();
}
//																			  *
//*****************************************************************************

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// UseMutex Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
INode::UseMutex::UseMutex(INode &ourNode) : m_node(ourNode)
{
	ourNode.Ex.MutexTake();
}


INode::UseMutex::~UseMutex()
{
	m_node.Ex.MutexRelease();
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// MyEvent Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
INode::MyEvent::MyEvent(INode &ourNode) : m_node(ourNode)
{
	m_eventCount = 0;
}


void INode::MyEvent::Register()
{
	m_node.Adv.Attn.AttnRegister();
}

void INode::MyEvent::Unregister()
{
	m_node.Adv.Attn.AttnUnregister();
}

void INode::MyEvent::Signal()
{
	m_node.Adv.Attn.AttnSignal();
}

bool INode::MyEvent::Wait(size_t timeoutMs)
{
	return m_node.Adv.Attn.AttnWait(timeoutMs);
}

/// END INTERNAL_DOC
/// \endcond
//																			  *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// ThreadLock Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

SysManager::ThreadLock::ThreadLock()
{
	threadLockMutex.Lock();	
	// Set that we're debugging and store the debugging thread ID
	inDebugging = true;
	lockedThreadID = CThread::CurrentThreadID();
	// Set the gate to wait, it will be realeased when all commands finish
	debugThreadLockResponseGate.ResetEvent();
	debugThreadLockResponseGate.WaitFor(INFINITE);
}

SysManager::ThreadLock::~ThreadLock()
{
	// Release the mutex and debugging gate
	inDebugging = false;
	debugGate.SetEvent();
	threadLockMutex.Unlock();
}

/// END INTERNAL_DOC
/// \endcond
//																			  *
//*****************************************************************************


//============================================================================= 
//	END OF FILE sysClassImpl.cpp
//=============================================================================

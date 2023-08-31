//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/src/cpmClassImpl.cpp $
// $Date: 01/19/2017 17:39 $
// $Workfile: cpmClassImpl.cpp $
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	\brief Implementations of the ClearPath-SC node object and dependencies.

	
**/
//
// CREATION DATE:
//		2015-12-08 18:22:51
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2015  Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																          *
// 	cpmClassImpl.cpp headers
//
#include "pubCpmCls.h"
#include "pubCpmAPI.h"
#include "tekEvents.h"
#include "lnkAccessAPI.h"
#include "lnkAccessCommon.h"
#include "meridianHdrs.h"
#include "netCmdPrivate.h"

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
// 	cpmClassImpl.cpp function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	cpmClassImpl.cpp constants
//
//
#define FW_MILESTONE_PWR 0x1504
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	cpmClassImpl.cpp static variables
//
// 
	extern mnNetInvRecords SysInventory[NET_CONTROLLER_MAX];
	// Read-modify-write mutex for node parameters. This is declared
	// here to avoid including events headers to make these opaque to 
	// the user.
	CCCriticalSection ParamLockMutex[NET_CONTROLLER_MAX][MN_API_MAX_NODES];
	// Events objects to signal Attention packets received from the node
	CCEvent AttnEvent[NET_CONTROLLER_MAX][MN_API_MAX_NODES];
	// Read-modify-write mutex for node parameters. This is declared
	// here to avoid including events headers to make these opaque to 
	// the user.
	CCCriticalSection AttnEvtMutex[NET_CONTROLLER_MAX][MN_API_MAX_NODES];
	CCCriticalSection AttnCntMutex[NET_CONTROLLER_MAX][MN_API_MAX_NODES];
	int32_t AttnListenerCnt[NET_CONTROLLER_MAX][MN_API_MAX_NODES];

//																			  *
//*****************************************************************************
using namespace sFnd;

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Helper function to fill in an error object.
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMstring Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
CPMstring::CPMstring(INode &theNode, nodeparam pNum, enum _srcs location)
	: ValueString(theNode, pNum, location)
{

}

void CPMstring::Refresh()
{
	cnErrCode theErr = MN_ERR_FAIL;
	mnErr eInfo;
	char tempStr[MAX_STR];
	double theDblVal;
	paramInfo theInfo;
	paramValue theVal;
	uint16_t romSum;

	switch (m_location) {
	case STR_MODEL_STR:
		theErr = netGetPartNumber(Node().Info.Ex.Addr(), tempStr, MAX_STR);
		break;
	case STR_USERID:
		theErr = netGetUserID(Node().Info.Ex.Addr(),
							  tempStr, MAX_STR);
		break;
	case STR_HW_VERSION:
		theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
									MN_P_HW_VERSION, &theInfo, &theVal);
		if (theErr == MN_OK) {
			strncpy(tempStr, (char *)theVal.raw.Byte.Buffer, MAX_STR);
		}
		break;
	case STR_FW_VERSION:
		theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
			MN_P_FW_VERSION, &theInfo, &theVal);
		if (theErr == MN_OK) {
			theErr = netGetParameterDbl(Node().Info.Ex.Addr(), 
									MN_P_ROM_SUM, &theDblVal);
			if (theErr == MN_OK) {
				romSum = uint16_t(theDblVal);
				snprintf(tempStr, MAX_STR, "%s %04X",
					(char *)theVal.raw.Byte.Buffer, romSum);
			}
		}
		break;
	default:
		// Implement me
		break;
	}
	// Ensure that the buffer is null-terminated
	tempStr[MAX_STR-1] = 0;

	// If OK tempStr has our string
	if (theErr == MN_OK) {
		memcpy(m_lastValue, m_currentValue, MAX_STR);
		memcpy(m_currentValue, tempStr, MAX_STR);
		Valid(true);
		return;
	}
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
	throw eInfo;
}


void CPMstring::Value(const char *newValue)
{
	cnErrCode theErr=MN_ERR_FAIL;
	mnErr eInfo;

	switch (m_location) {
	case STR_USERID:
		theErr = netSetUserID(Node().Info.Ex.Addr(), newValue);
		if (theErr == MN_OK) {
			memcpy(m_lastValue, m_currentValue, MAX_STR);
			memcpy(m_currentValue, newValue, MAX_STR);
			Valid(true);
			return;
		}
		break;
		
	// Read-only parameters, cause errors
	case STR_MODEL_STR:
	case STR_HW_VERSION:
	case STR_FW_VERSION:
		theErr = MN_ERR_CMD_WR_FAIL;
		break;
	default:
		// We don't handle these yet
		break;
	}
	// Show as busted
	fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
	throw eInfo;
}


const char *CPMstring::Value()
{
	if (!m_valid) {
		Refresh();
	}
	return m_currentValue;
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMlimitsAdv Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
void CPMlimitsAdv::StartPosFoldback(bool engage)
{
	mnOutReg fld;
	fld.cpm.StartPosTrqFldBk = true;
	if (engage)
		Node().Outs.User.Set(fld);
	else
		Node().Outs.User.Clear(fld);
}


bool CPMlimitsAdv::StartPosFoldback()
{
	return Node().Outs.User.Value().cpm.StartPosTrqFldBk;
}

void CPMlimitsAdv::StartNegFoldback(bool engage)
{
	mnOutReg fld;
	fld.cpm.StartNegTrqFldBk = true;
	if (engage)
		Node().Outs.User.Set(fld);
	else
		Node().Outs.User.Clear(fld);
}


bool CPMlimitsAdv::StartNegFoldback()
{
	return Node().Outs.User.Value().cpm.StartNegTrqFldBk;
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMhoming Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


/**		
	\copydoc IHoming::Initiate()
**/
void CPMhoming::Initiate()
{
	cpmSetFlagsReg flgs;
	flgs.fld.HomingReq = 1;
	// Initiate homing (see drvSetFlagsReg in ioHAL for this constant)
	Node().Info.Ex.Parameter(CPM_P_DRV_SET_FLAGS, flgs.bits);
}

/**
	\copydoc IHoming::IsComplete
**/
bool CPMhoming::IsHoming()
{
	Node().Status.RT.Refresh();
	return Node().Status.RT.Value().cpm.Homing;
}

/**
	\copydoc IHoming::WasHomed
**/
bool CPMhoming::WasHomed()
{
	Node().Status.RT.Refresh();
	return Node().Status.RT.Value().cpm.WasHomed;
}

/**
\copydoc IHoming::SignalComplete
**/
void CPMhoming::SignalComplete()
{
	cpmSetFlagsReg flgs;
	flgs.fld.HomingComplete = 1;
	// Complete homing (see drvSetFlagsReg in ioHAL for this constant)
	Node().Info.Ex.Parameter(CPM_P_DRV_SET_FLAGS, flgs.bits);
}

/**
\copydoc IHoming::SignalInvalid()
**/
void CPMhoming::SignalInvalid()
{
	cpmSetFlagsReg flgs;
	flgs.fld.HomingInvalid = 1;
	// Complete homing (see drvSetFlagsReg in ioHAL for this constant)
	Node().Info.Ex.Parameter(CPM_P_DRV_SET_FLAGS, flgs.bits);
}

/**
\copydoc IHoming::HomingValid()
**/
bool CPMhoming::HomingValid()
{
	bool isValid = true;
	paramValue* val = new paramValue();
	netGetParameterInfo(Node().Info.Ex.Addr(), CPM_P_XPS_FEAT_AT_HOME, NULL, val);
	
	if (Node().Setup.Ex.HW.Value().cpm.HomingStyle == _cpmHwConfigFlds::HOME_TO_SWITCH &&
		val->value == 0) {
		isValid = false;
	}
	return isValid;
}
//																			  *
//*****************************************************************************


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMinfoEx Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


//*****************************************************************************
//	NAME																	  *
//		CPMinfoEx::Parameter
//
//	DESCRIPTION:
/**
\copydoc IInfoEx::Parameter
**/
double CPMinfoEx::Parameter(nodeparam index)
{
	double paramVal;
	cnErrCode theErr = netGetParameterDbl(Node().Info.Ex.Addr(), mnParams(index),
										  &paramVal);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, 
			"Parameter(%d) failed to read,", index);
		throw eInfo;
	}
	return paramVal;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//	NAME																	  *
//		CPMinfoEx::Parameter
//
//	DESCRIPTION:
/**
\copydoc IInfoEx::Parameter
**/
void CPMinfoEx::Parameter(nodeparam index, double newValue)
{
	cnErrCode theErr = netSetParameterDbl(Node().Info.Ex.Addr(), mnParams(index),
										  newValue);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, 
			"Parameter(%d) failed to write %f", index, newValue);
		throw eInfo;
	}
}
//																			  *
//*****************************************************************************



//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMinfo Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


void CPMinfo::NodeType(nodeTypes newType) {
	m_nodeType = newType;
	bool isAdvanced = newType == IInfo::CLEARPATH_SC_ADV;
	INode &theNode = Node();
	IPort &thePort = Node().Port;
	theNode.Adv.Supported(isAdvanced);
	theNode.Info.Adv.Supported(isAdvanced);
	theNode.Limits.Adv.Supported(isAdvanced);
	theNode.Motion.Adv.Supported(isAdvanced);
	theNode.Status.Adv.Supported(isAdvanced);
}

//*****************************************************************************
//	NAME																	  *
//		CPMinfo::UserData
//
//	DESCRIPTION:
/**		
	\copydoc IInfo::UserData(size_t, nodeuchar[])
**/
void CPMinfo::UserData(size_t bank, const uint8_t theData[MN_USER_NV_SIZE])
{
	cnErrCode theErr;
	packetbuf rBuf;
	if (bank < 4) {
		memcpy(rBuf.Byte.Buffer, theData, MN_USER_NV_SIZE);
		rBuf.Byte.BufferSize = MN_USER_NV_SIZE;
		theErr = netSetParameterEx(Node().Info.Ex.Addr(),
									nodeparam(bank + CPM_P_USER_DATA_NV0),
									&rBuf);
		if (theErr != MN_OK) {
			mnErr eInfo;
			fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_, "");
			throw eInfo;
		}
	}
	else {
		mnErr eInfo;
		fillInErrs(eInfo, MN_ERR_BADARG, _TEK_FUNC_SIG_,
			"Bank number must be less then 4.");
		throw eInfo;
	}
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CPMinfo::UserData
//
//	DESCRIPTION:
/**		
	\copydoc IInfo::UserData(size_t)
**/
std::vector<uint8_t> CPMinfo::UserData(size_t bank)
{
	std::vector<uint8_t> rVal;
	paramValue rBuf;
	// Args OK
	if (bank < 4) {
		rVal.reserve(MN_USER_NV_SIZE);
		cnErrCode theErr;
		theErr = netGetParameterInfo(Node().Info.Ex.Addr(),
			nodeparam(bank + CPM_P_USER_DATA_NV0), NULL, &rBuf);
		if (theErr != MN_OK) {
			mnErr eInfo;
			fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_, "");
			throw eInfo;
		}
	}
	else {
		mnErr eInfo;
		fillInErrs(eInfo, MN_ERR_BADARG, _TEK_FUNC_SIG_, 
			"Bank number must be less then 4.");
		throw eInfo;
	}
	// Update our vector with returned data
	for (size_t i = 0; i < MN_USER_NV_SIZE; i++) {
		rVal.push_back(uint8_t(rBuf.raw.Byte.Buffer[i]));
	}
	return rVal;
}
//																			  *
//*****************************************************************************





//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//						CPMlimits class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************



//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//					  CPMlimitsAdv class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************



//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//					    CPMmotion Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Kinematic Constraint Group
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/**		
	\copydoc IMotion::MovePosnDurationMsec
**/
double CPMmotion::MovePosnDurationMsec(int32_t target, bool targetIsAbsolute)
{
	double retVal = IMotion::MovePosnDurationMsec(target, targetIsAbsolute);
	retVal += Node().Info.Ex.Parameter(CPM_P_DRV_MV_DN_TC);
	return retVal;
}

/**		
	\copydoc IMotion::MoveVelDurationMsec
**/
double CPMmotion::MoveVelDurationMsec(double target)
{
	double retVal = IMotion::MoveVelDurationMsec(target);
	retVal += Node().Info.Ex.Parameter(CPM_P_DRV_MV_DN_TC);
	return retVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Motion Initiating Group
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/**		
	\copydoc IMotion::MovePosnStart
**/
size_t CPMmotion::MovePosnStart(int32_t target, 
						   bool targetIsAbsolute,
						   bool hasDwell)
{
	int32_t bufsLeft = 0;
	mgPosnStyle theStyle;
	theStyle.fld.wait = false;
	theStyle.fld.head = false;
	theStyle.fld.tail = false;
	theStyle.fld.relative = !targetIsAbsolute;
	theStyle.fld.dwell = hasDwell;
	cnErrCode theErr = cpmForkPosnMove(Node().Info.Ex.Addr(), target,
		theStyle, &bufsLeft);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
	return bufsLeft;
}

/**		
	\copydoc IMotion::MoveVelStart(double)
**/
size_t CPMmotion::MoveVelStart(double target)
{
	int32_t bufsLeft = 0;

	// This is safe as CPMmotion always back point to a CPMnode instance
	CPMnode &myNode = static_cast<CPMnode &>(Node());
	cnErrCode theErr = cpmForkVelMove(Node().Info.Ex.Addr(),
		target/ myNode.velScale(),	false, &bufsLeft);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
	return bufsLeft;
}


bool CPMmotion::VelocityReachedTarget()
{
	mnStatusReg fld;
	fld.cpm.AtTargetVel = true;
	return Node().Status.Rise.TestAndClear(fld);
}


bool CPMmotion::VelocityAtTarget()
{
	Node().Status.RT.Refresh();
	return Node().Status.RT.Value().cpm.AtTargetVel;
}

bool CPMmotion::MoveWentDone()
{
	mnStatusReg fld;
	fld.cpm.MoveDone = true;
	return Node().Status.Rise.TestAndClear(fld);
}


bool CPMmotion::MoveIsDone()
{
	Node().Status.RT.Refresh();
	return Node().Status.RT.Value().cpm.MoveDone;
}

bool CPMmotion::WentNotReady()
{
	mnStatusReg fld, result;
	fld.cpm.NotReady = true;

	return Node().Status.Rise.TestAndClear(fld);
}

bool CPMmotion::IsReady()
{
	Node().Status.RT.Refresh();
	return !Node().Status.RT.Value().cpm.NotReady;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Motion Canceling Group
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
\copydoc IMotion::NodeStop(nodeStopCodes)
**/
void CPMmotion::NodeStop(nodeStopCodes howToStop)
{
	cnErrCode theErr = netNodeStop(Node().Info.Ex.Addr(), howToStop, false);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}

/**
\copydoc IMotion::NodeStop(mgNodeStopReg)
**/
void CPMmotion::NodeStop(mgNodeStopReg &howToStop)
{
	cnErrCode theErr = netNodeStop(Node().Info.Ex.Addr(), howToStop.bits, false);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}

/**
\copydoc IMotion::GroupNodeStop(nodeStopCodes)
**/
void CPMmotion::GroupNodeStop(nodeStopCodes howToStop)
{
	for (Uint16 iNode = 0; iNode < m_pNode->Port.NodeCount(); iNode++) {
		cnErrCode theErr = netNodeStop(m_pNode->Port.Nodes(iNode).Info.Ex.Addr(), howToStop, false);
		if (theErr != MN_OK) {
			mnErr eInfo;
			fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
			throw eInfo;
		}
	}
}

/**
\copydoc IMotion::NodeStopClearC
**/
void CPMmotion::NodeStopClear()
{
	cnErrCode theErr = netNodeStop(Node().Info.Ex.Addr(), 
						STOP_TYPE_CLR_ALL, false);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}

/**		
	\copydoc IMotion::AddToPosition
**/
void CPMmotion::AddToPosition(double adjAmount)
{
	cnErrCode theErr = cpmAddToPosition(Node().Info.Ex.Addr(), adjAmount);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}



//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMmotionAdv Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************


double CPMmotionAdv::MovePosnHeadTailDurationMsec(int32_t target,
	bool targetIsAbsolute,
	bool hasHead, bool hasTail)
{
	double retVal = IMotionAdv::MovePosnHeadTailDurationMsec(target, 
		targetIsAbsolute, hasHead, hasTail);
	retVal += Node().Info.Ex.Parameter(CPM_P_DRV_MV_DN_TC);
	return retVal;
}

double CPMmotionAdv::MovePosnAsymDurationMsec(int32_t target,
	bool targetIsAbsolute)
{
	double retVal = IMotionAdv::MovePosnAsymDurationMsec(target, 
		targetIsAbsolute);
	retVal += Node().Info.Ex.Parameter(CPM_P_DRV_MV_DN_TC);
	return retVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Motion Initiating Group
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
/**
**/

/**		
	\copydoc IMotionAdv::MovePosnHeadTailStart
**/
size_t CPMmotionAdv::MovePosnStart(int32_t target,
	bool targetIsAbsolute,
	bool isTriggered, bool hasDwell)
{
	return MovePosnHeadTailStart(target, targetIsAbsolute, isTriggered, 
							 false, false, hasDwell);
}

size_t CPMmotionAdv::MovePosnHeadTailStart(int32_t target, 
							bool targetIsAbsolute,
							bool isTriggered,
						    bool hasHead, bool hasTail, bool hasDwell)
{
	int32_t bufsLeft = 0;
	mgPosnStyle theStyle;
	theStyle.fld.wait = isTriggered;
	theStyle.fld.head = hasHead;
	theStyle.fld.tail = hasTail;
	theStyle.fld.relative = !targetIsAbsolute;
	theStyle.fld.dwell = hasDwell;
	cnErrCode theErr = cpmForkPosnMove(Node().Info.Ex.Addr(), target,
									  theStyle, &bufsLeft);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
	return bufsLeft;
}


/**		
	\copydoc IMotionAdv::MovePosnAsymStart
**/
size_t CPMmotionAdv::MovePosnAsymStart(int32_t target, 
							bool targetIsAbsolute,
							bool isTriggered,
							bool hasDwell)
{
	int32_t bufsLeft = 0;
	mgPosnStyle theStyle;
	theStyle.fld.wait = isTriggered;
	theStyle.fld.asym = true;
	theStyle.fld.relative = !targetIsAbsolute;
	theStyle.fld.dwell = hasDwell;
	cnErrCode theErr = cpmForkPosnMove(Node().Info.Ex.Addr(), target,
									   theStyle, &bufsLeft);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
	return bufsLeft;
}

/**		
	\copydoc IMotionAdv::MoveVelStart
**/
size_t CPMmotionAdv::MoveVelStart(double target, 
								 bool isTriggered)
{
	int32_t bufsLeft = 0;
	// This is safe as CPMmotion always back point to a CPMnode instance
	CPMnode &myNode = static_cast<CPMnode &>(Node());
	cnErrCode theErr = cpmForkVelMove(Node().Info.Ex.Addr(),
									  target/myNode.velScale(),
									  isTriggered, &bufsLeft);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr,  _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
	return bufsLeft;
}

// Triggering waiting moves
/**		
	\copydoc IMotionAdv::TriggerMove
**/
void CPMmotionAdv::TriggerMove()
{
	cnErrCode theErr = netTrigger(Node().Port.NetNumber(),
								  nodeaddr(NODE_ADDR(Node().Info.Ex.Addr())),
								  false);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}

/**		
	\copydoc IMotionAdv::TriggerMovesInMyGroup
**/
void CPMmotionAdv::TriggerMovesInMyGroup()
{
	mnAppConfigReg app = Node().Setup.Ex.App.Value();
	cnErrCode theErr = netTrigger(Node().Port.NetNumber(),
								  nodeaddr(app.cpm.TriggerGroup),
								  true);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}

/**
	\copydoc IMotionAdv::TriggerGroup(size_t)
**/
void CPMmotionAdv::TriggerGroup(size_t groupNumber)
{
	if (groupNumber <= 3) {
		cnErrCode theErr;
		INode::UseMutex myMutex(Node());
		double theVal;
		theErr = netGetParameterDbl(Node().Info.Ex.Addr(),
									mnParams(CPM_P_APP_CONFIG_REG), &theVal);
		if (theErr == MN_OK) {
			mnAppConfigReg app;
			app.bits = nodeulong(theVal);
			app.cpm.TriggerGroup = groupNumber;
			theErr = netSetParameterDbl(Node().Info.Ex.Addr(),
									mnParams(CPM_P_APP_CONFIG_REG),
									double(app.bits));
			if (theErr == MN_OK) {
				Node().Setup.Ex.App.setNewVal(app);
			}
		}
		return;
	}
	// Argument out of range
	mnErr eInfo;
	fillInErrs(eInfo, m_pNode, MN_ERR_BADARG, _TEK_FUNC_SIG_, "", "");
}

/**
\copydoc IMotionAdv::TriggerGroup()
**/
size_t CPMmotionAdv::TriggerGroup()
{
	mnAppConfigReg app = Node().Setup.Ex.App.Value();
	return app.cpm.TriggerGroup;
}


//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMmotionAudit Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
/**		
	\copydoc IMotionAudit::Refresh()
**/
void CPMmotionAudit::Refresh()
{
	cnErrCode theErr;
	theErr = netGetDataCollected(Node().Info.Ex.Addr(), &Results);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}
/**
\copydoc IMotionAudit::SelectTestPoint()
**/

void CPMmotionAudit::SelectTestPoint(enum _testPoints testPoint,
								     double fullScale, double filterTCmsec)
{
	iscMonState setup;
	setup.filterTC = filterTCmsec;
	setup.gain = fullScale;
	// The motion audit class should map the indices to the monTestPoints.
	setup.var = monTestPoints(testPoint);
	cnErrCode theErr;
	theErr = iscSetMonitor(Node().Info.Ex.Addr(), 0, &setup);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}

}


//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//						CPMnode Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
void CPMnode::VelUnit(enum _velUnits newUnits)
{
	mnErr eInfo;
	double effDens;

	switch (newUnits) {
	case INode::COUNTS_PER_SEC:
		m_velScaleToUser = 1;
		break;
	case INode::RPM:
		effDens = Info.Ex.Parameter(CPM_P_CMD_CNTS_PER_REV);
		m_velScaleToUser = 60. / effDens;
		break;
	default:
		fillInErrs(eInfo, this, MN_ERR_BADARG, _TEK_FUNC_SIG_,
			"Unknown velocity unit %d", newUnits);
		throw eInfo;
	}
	// Adjust the list of know values
	std::vector<ValueDouble *>::iterator i;
	for (i = m_velValues.begin(); i < m_velValues.end(); ++i) {
		ValueDouble &v = **i;
		v.Scale(m_velScaleToUser);
	}
	m_VelUnit = newUnits;
}

void CPMnode::AccUnit(enum _accUnits newUnits)
{
	mnErr eInfo;
	double effDens;
	switch (newUnits) {
	case INode::COUNTS_PER_SEC2:
		m_accScaleToUser = 1;
		break;
	case INode::RPM_PER_SEC:
		effDens = Info.Ex.Parameter(CPM_P_CMD_CNTS_PER_REV);
		m_accScaleToUser = 60. / effDens;
		break;
	default:
		fillInErrs(eInfo, this, MN_ERR_BADARG, _TEK_FUNC_SIG_,
			"Unknown acceleration unit %d", newUnits);
		throw eInfo;
	}
	// Adjust the list of know values
	std::vector<ValueDouble *>::iterator i;
	for (i = m_accValues.begin(); i < m_accValues.end(); ++i) {
		ValueDouble &v = **i;
		v.Scale(m_accScaleToUser);
	}
	m_AccUnit = newUnits;
}


void CPMnode::TrqUnit(enum _trqUnits newUnits)
{
	mnErr eInfo;
	double limDbl;
	switch (newUnits) {
	case AMPS:
		m_trqScaleToUser = 1;
		break;
	case PCT_MAX:
		// Get scale, if basis is bad zero out scale to force 
		// bad values.
		limDbl = Info.Ex.Parameter(CPM_P_DRV_I_MAX);
		if (limDbl > 0)
			m_trqScaleToUser = 100. / limDbl;
		else
			m_trqScaleToUser = 0;

		limDbl = Info.Ex.Parameter(CPM_P_DRV_ADC_MAX);
		if (limDbl>0)
			m_adcTrqScaleToUser = 100. / limDbl;
		else
			m_adcTrqScaleToUser = 0;

		limDbl = Info.Ex.Parameter(CPM_P_DRV_RMS_LIM);
		if (limDbl>0)
			m_rmsTrqScaleToUser = 100. / limDbl;
		else
			m_rmsTrqScaleToUser = 0;
		break;
	default:
		fillInErrs(eInfo, this, MN_ERR_BADARG, _TEK_FUNC_SIG_,
			"Unknown torque unit %d", newUnits);
		throw eInfo;
	}
	// Adjust the list of know values
	std::vector<ValueDouble *>::iterator i;
	for (i = m_trqValues.begin(); i < m_trqValues.end(); ++i) {
		ValueDouble &v = **i;
		v.Scale(m_trqScaleToUser);
	}
	for (i = m_adcTrqValues.begin(); i < m_adcTrqValues.end(); ++i) {
		ValueDouble &v = **i;
		v.Scale(m_adcTrqScaleToUser);
	}
	for (i = m_rmsTrqValues.begin(); i < m_rmsTrqValues.end(); ++i) {
		ValueDouble &v = **i;
		v.Scale(m_rmsTrqScaleToUser);
	}
	m_TrqUnit = newUnits;
}



//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//						CPMouts Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
// Easy access to enable request
/**
\copydoc IOuts::EnableReq(bool)
**/
void CPMouts::EnableReq(bool newState)
{
	mnOutReg enbl;
	enbl.cpm.EnableReq = 1;
	if (newState)
		User.Set(enbl);
	else
		User.Clear(enbl);
}
/**
\copydoc IOuts::EnableReq()
**/
bool CPMouts::EnableReq()
{
	bool enbl = User.Value().cpm.EnableReq;
	return enbl;
}


//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMsetup Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


//*****************************************************************************
//	NAME																	  *
//		CPMsetup::ConfigLoad
//
//	DESCRIPTION:
/**
\copydoc ISetup::ConfigLoad
**/
void CPMsetup::ConfigLoad(const char *filePath,
						  bool doReset)
{
	cnErrCode theErr;
	configFmts cfgFmt;
	cfgFmt = doReset ? CLASSIC : CLASSIC_NO_RESET;
	theErr = netConfigLoad(Node().Info.Ex.Addr(),
							cfgFmt, filePath);
	if (theErr == MN_OK)
		return;
	mnErr eInfo;
	fillInErrs(eInfo, &Node(), theErr, _TEK_FUNC_SIG_, "");
	throw eInfo;
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CPMsetup::ConfigSave
//
//	DESCRIPTION:
/**
\copydoc ISetup::ConfigSave
**/
void CPMsetup::ConfigSave(const char *filePath)
{
	cnErrCode theErr;
	theErr = netConfigSave(Node().Info.Ex.Addr(),
							CLASSIC, filePath);
	if (theErr == MN_OK) return;
	mnErr eInfo;
	fillInErrs(eInfo, &Node(), theErr, _TEK_FUNC_SIG_, "");
	throw eInfo;
}
//																			  *
//*****************************************************************************

//*****************************************************************************
//	NAME																	  *
//		CPMsetup::AccessLevelIsFull
//
//	DESCRIPTION:
/**
\copydoc ISetup::AccessLevelIsFull
**/
bool CPMsetup::AccessLevelIsFull()
{
	mnNetStatus accessLvl = SysInventory->AccessInfo[Node().Info.Ex.Addr()];
	if ( (accessLvl.Fld.NetSource == MN_SRC_APP_NET && 
		  accessLvl.Fld.NetAccessApp == MN_ACCESS_LVL_FULL) ||
	     (accessLvl.Fld.NetSource == MN_SRC_DIAG_NET && 
		  accessLvl.Fld.NetAccessDiag == MN_ACCESS_LVL_FULL) ){
		return true;
	}
	else
		return false;
}
//																			  *
//*****************************************************************************


//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMstatusAdv Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// CPMstatus Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************


/**		
	\copydoc IStatus::AlertsClear
**/
void CPMstatus::AlertsClear()
{
	cnErrCode theErr = netAlertClear(Node().Info.Ex.Addr());
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, m_pNode, theErr, _TEK_FUNC_SIG_, "");
		throw eInfo;
	}
}

/**
	\copydoc IStatus::HadTorqueSaturation
**/
bool CPMstatus::HadTorqueSaturation()
{
	Warnings.Refresh();
	alertReg satFld, result;
	satFld.cpm.TrqSat = 1;
	return (Warnings.TestAndClear(satFld, result));
}
//																			  *
//*****************************************************************************





//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Constructor Implementations
/// \cond INTERNAL_DOC	
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
CPMattnNode::CPMattnNode(INode &ourNode)
	: IAttnNode(ourNode,
		   CPM_P_ATTN_MASK, CPM_P_WARN_MASK, CPM_P_ALERT_MASK, 
		   CPM_P_STATUS_EVENT_MASK)
{

}

CPMnodeAdv::CPMnodeAdv(INode &ourNode) 
	: INodeAdv(ourNode, m_audit, m_attnNode),
	  m_audit(ourNode), m_attnNode(ourNode)
{
	
}

void CPMnodeAdv::Supported(bool newState) 
{
	IObj::Supported(newState);
	m_audit.Supported(newState);
	m_attnNode.Supported(newState);
}

CPMhoming::CPMhoming(INode &ourNode)
	: IHoming(ourNode, 
		CPM_P_APP_HOMING_ACCEL, CPM_P_APP_HOMING_VEL,
		CPM_P_DRV_HARDSTOP_TRQ_LIM, CPM_P_APP_HOMING_OFFSET, 
		CPM_P_APP_HOMING_DELAY)
{
	Supported(true);
}

CPMinfo::CPMinfo(INode &ourNode) 
	: IInfo(ourNode, m_advInfo, m_exInfo,
		CPM_P_SER_NUM, CPM_P_FW_VERS, CPM_P_USER_RAM0,
		m_userID, m_hwVersion, m_fwVersion, m_model, CPM_P_CMD_CNTS_PER_REV),
	m_advInfo(ourNode), m_exInfo(ourNode),
	m_userID(ourNode, 0, ValueString::STR_USERID),
	m_fwVersion(ourNode, 0, ValueString::STR_FW_VERSION),
	m_hwVersion(ourNode, 0, ValueString::STR_HW_VERSION),
	m_model(ourNode, 0, ValueString::STR_MODEL_STR)
{
	m_nodeType = UNKNOWN;
	Supported(true);
}

CPMinfoAdv::CPMinfoAdv(INode &ourNode) 
	: IInfoAdv(ourNode)
{
	Supported(true);
}

CPMinfoEx::CPMinfoEx(INode &ourNode)
	: IInfoEx(ourNode)
{
	Supported(true);
}

CPMlimits::CPMlimits(INode &ourNode)
	: ILimits(ourNode, m_adv,
		CPM_P_DRV_TRQ_LIM, CPM_P_POSN_TRK_ERR_LIM,
		CPM_P_USER_SOFT_LIM_POSN_1, CPM_P_USER_SOFT_LIM_POSN_2,
		CPM_P_DRV_MTR_SPEED_LIM),
	m_adv(ourNode)
{
	Supported(true);
}

CPMlimitsAdv::CPMlimitsAdv(INode &ourNode)
	: ILimitsAdv(ourNode,
		CPM_P_DRV_POS_FLDBK_TRQ, CPM_P_DRV_POS_FLDBK_TRQ_TC,
		CPM_P_DRV_NEG_FLDBK_TRQ, CPM_P_DRV_NEG_FLDBK_TRQ_TC)
{
	
}

CPMmotion::CPMmotion(INode &ourNode) 
	: IMotion(ourNode, m_adv, m_homing,
				CPM_P_ACC_LIM, CPM_P_VEL_LIM, CPM_P_STOP_ACC_LIM, 
				CPM_P_POSN_MEAS, CPM_P_POSN_CMD, CPM_P_POSN_TRK,
				CPM_P_VEL_MEAS, CPM_P_VEL_CMD, 
				CPM_P_DRV_TRQ_MEAS, CPM_P_DRV_TRQ_CMD,
				CPM_P_JERK_LIM, CPM_P_MOVE_DWELL, CPM_P_APP_RAS_DELAY),
	m_adv(ourNode), 
	m_homing(ourNode)
{
	Supported(true);
	VelCommanded.IsVolatile(true);
	VelMeasured.IsVolatile(true);
	PosnCommanded.IsVolatile(true);
	PosnMeasured.IsVolatile(true);
	PosnTracking.IsVolatile(true);
	TrqCommanded.IsVolatile(true);
	TrqMeasured.IsVolatile(true);
	JrkLimitDelay.IsVolatile(true);
}

CPMmotionAdv::CPMmotionAdv(INode &ourNode) 
	: IMotionAdv(ourNode,
		CPM_P_DEC_LIM, CPM_P_HEADTAIL_VEL, 
		CPM_P_HEAD_DISTANCE, CPM_P_TAIL_DISTANCE, CPM_P_A_START,
		CPM_P_B_END)
{
	
}


CPMmotionAudit::CPMmotionAudit(INode &ourNode) 
	: IMotionAudit(ourNode)
{
	
}


CPMnodeEx::CPMnodeEx(INode &ourNode)
	: INodeEx(ourNode)
{

}


CPMouts::CPMouts(INode &ourNode) 
	: IOuts(ourNode,
		CPM_P_USER_OUT_REG, CPM_P_OUT_REG)
{
	Supported(true);
	Out.IsVolatile(true);
}

CPMouts::~CPMouts()
{
}

CPMsetup::CPMsetup(INode &ourNode)
	: ISetup(ourNode, CPM_P_APP_CONFIG_REG, CPM_P_HW_CONFIG_REG, 
			CPM_P_WATCHDOG_TIME, CPM_P_DLY_TO_DISABLE)
{

}

CPMstatus::CPMstatus(INode &ourNode) 
	: IStatus(ourNode, m_adv, 
			  CPM_P_STATUS_RT_REG, CPM_P_STATUS_RISE_REG, 
			  CPM_P_STATUS_FALL_REG, CPM_P_STATUS_ACCUM_REG,
			  CPM_P_WARN_REG, CPM_P_ALERT_REG, CPM_P_DRV_RMS_LVL,
			  CPM_P_POWER_REG), 
	  m_adv(ourNode)
{
	Supported(true);
	RT.IsVolatile(true);
	Rise.IsVolatile(true);
	Fall.IsVolatile(true);
	Accum.IsVolatile(true);
	Alerts.IsVolatile(true);
	Warnings.IsVolatile(true);
	RMSlevel.IsVolatile(true);
	Power.IsVolatile(true);
	Power.Supported(ourNode.Info.FirmwareVersionCode.Value() >= FW_MILESTONE_PWR);
}

CPMstatusAdv::CPMstatusAdv(INode &ourNode) 
	: IStatusAdv(ourNode, CPM_P_POSN_CAP_INA_HI_SPD, CPM_P_POSN_CAP_INB)
{
	CapturedHiResPosn.IsVolatile(true);
	CapturedPos.IsVolatile(true);
}

//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//						CPMnodeEx Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
/**
\copydoc INodeEx::MutexTake
**/
void CPMnodeEx::MutexTake()
{
	ParamLockMutex[Node().Port.NetNumber()]
				  [NODE_ADDR(Node().Info.Ex.Addr())].Lock();
}

/**
\copydoc INodeEx::MutexRelease
**/
void CPMnodeEx::MutexRelease()
{
	ParamLockMutex[Node().Port.NetNumber()]
				  [NODE_ADDR(Node().Info.Ex.Addr())].Unlock();
}

//*****************************************************************************
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//						CPMattnNode Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
//*****************************************************************************
/**
\copydoc IAttnNode::AttnRegister
**/
void CPMattnNode::AttnRegister()
{
	netaddr netNum = Node().Port.NetNumber();
	int nodeNum = NODE_ADDR(Node().Info.Ex.Addr());

	infcSleep(0);
	AttnEvtMutex[netNum][nodeNum].Lock();

}

/**
\copydoc IAttnNode::AttnUnregister
**/
void CPMattnNode::AttnUnregister()
{
	netaddr netNum = Node().Port.NetNumber();
	int nodeNum = NODE_ADDR(Node().Info.Ex.Addr());

	AttnEvtMutex[netNum][nodeNum].Unlock();
}

/**
\copydoc IAttnNode::AttnWait
**/
bool CPMattnNode::AttnWait(size_t timeoutMs)
{
	bool retVal;
	netaddr netNum = Node().Port.NetNumber();
	int nodeNum = NODE_ADDR(Node().Info.Ex.Addr());
	

	AttnCntMutex[netNum][nodeNum].Lock();
	AttnListenerCnt[netNum][nodeNum]++;
	AttnCntMutex[netNum][nodeNum].Unlock();

	AttnEvtMutex[netNum][nodeNum].Unlock();

	retVal = AttnEvent[netNum][nodeNum].WaitFor(unsigned(timeoutMs));

	AttnCntMutex[netNum][nodeNum].Lock();

	AttnListenerCnt[netNum][nodeNum]--;
	if (retVal && AttnListenerCnt[netNum][nodeNum] == 0){
		AttnEvent[netNum][nodeNum].ResetEvent();
	}
	AttnCntMutex[netNum][nodeNum].Unlock();
	infcSleep(0);
	AttnEvtMutex[netNum][nodeNum].Lock();

	return retVal;
}

/**
\copydoc IAttnNode::AttnSignal
**/
void CPMattnNode::AttnSignal()
{

	netaddr netNum = Node().Port.NetNumber();
	int nodeNum = NODE_ADDR(Node().Info.Ex.Addr());
	
	AttnCntMutex[netNum][nodeNum].Lock();

	if (AttnListenerCnt[netNum][nodeNum]){
		AttnEvent[netNum][nodeNum].SetEvent();
	}
	AttnCntMutex[netNum][nodeNum].Unlock();
}


// NODE 
CPMnode::CPMnode(IPort &port, multiaddr theAddr)
	: INode(port, theAddr, m_info, m_motion, m_status, m_limits, m_outs, m_adv, 
			m_setup, m_ex),
	m_motion(*this),
	m_status(*this), 
	m_limits(*this),
	m_outs(*this),
	m_setup(*this),
	m_adv(*this),
	m_ex(*this),
	m_info(*this)
{
	// Setup advanced API items
	optionReg myOptions;
	myOptions.bits = nodeulong(m_info.Ex.Parameter(MN_P_OPTION_REG));
	if (myOptions.cpm.Advanced) {
		m_info.NodeType(IInfo::CLEARPATH_SC_ADV);
	}
	else {
		m_info.NodeType(IInfo::CLEARPATH_SC);
	}
	
	m_adv.Supported(myOptions.cpm.Advanced);
	m_limits.Adv.Supported(myOptions.cpm.Advanced);

	AttnListenerCnt[port.NetNumber()][NODE_ADDR(theAddr)] = 0;

	// Build up our list of acceleration items.
	m_accValues.push_back(&m_motion.AccLimit);
	m_accValues.push_back(&m_motion.NodeStopDecelLim);
	m_accValues.push_back(&m_motion.Adv.DecelLimit);
	
	// Build up our list of velocity items.
	m_velValues.push_back(&m_motion.VelLimit);
	m_velValues.push_back(&m_motion.VelCommanded);
	m_velValues.push_back(&m_motion.VelMeasured);
	m_velValues.push_back(&m_motion.Adv.HeadTailVelLimit);
	m_velValues.push_back(&m_limits.MotorSpeedLimit);

	// Build up our list of torque items.
	m_trqValues.push_back(&m_motion.TrqCommanded);
	m_trqValues.push_back(&m_motion.TrqMeasured);
	m_trqValues.push_back(&m_limits.TrqGlobal);
	m_trqValues.push_back(&m_limits.Adv.NegativeTrq);
	m_trqValues.push_back(&m_limits.Adv.PositiveTrq);

	// RMS level / RMS limit setting
	m_rmsTrqValues.push_back(&m_status.RMSlevel);


	// Establish unit scalings
	AccUnit(INode::RPM_PER_SEC);
	VelUnit(INode::RPM);
	TrqUnit(INode::PCT_MAX);
	// Get the user ID for potential error messages later while node 
	// appears to be online.
	m_info.UserID.Refresh();
}

// END OF INTERNAL_DOC
/// \endcond
//																			  *
//*****************************************************************************




//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysCPMport Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

/**
	\copydoc IPort::NodeCount
**/
Uint16 SysCPMport::NodeCount() 
{
	return SysInventory[m_netNumber].InventoryNow.NumOfNodes;
}

/**
	\copydoc IPort::OpenState
**/
openStates SysCPMport::OpenState() 
{
	return SysInventory[m_netNumber].OpenState;
}

/**
	\copydoc IPort::WaitForOnline
**/
bool SysCPMport::WaitForOnline(int32_t timeoutMs) 
{
	double endTime = infcCoreTime() + timeoutMs;
	
	while (infcCoreTime() < endTime) {
		if (SysInventory[m_netNumber].OpenState == OPENED_ONLINE){
			return true;
		}
		infcSleep(100);
	}

	return (SysInventory[m_netNumber].OpenState == OPENED_ONLINE);
}

/**
	\copydoc IPort::RestartCold
**/
void SysCPMport::RestartCold() 
{
	cnErrCode theErr = mnRestartNet(NetNumber(), true);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
			"Failure to restart network %d", NetNumber());
		throw eInfo;
	}
}


/**
	\copydoc IPort::NodeStop
**/
void SysCPMport::NodeStop(mgNodeStopReg stopType)
{
	// This is the network form of high-priority note stops, 
	// create fake address to allow function to extract controller number.
	cnErrCode theErr = netNodeStop(MULTI_ADDR(NetNumber(), 0), stopType, true);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
			"Failure to issue network %d node stop type 0x%x", 
			NetNumber(), stopType.bits);
		throw eInfo;
	}
}

/**
	\copydoc IPort::CommandTraceSave
**/
void SysCPMport::CommandTraceSave(const char *filePath)
{
	cnErrCode theErr = infcTraceDumpA(NetNumber(), filePath);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
			"Failure to create dump file %s for net %d", 
			filePath, NetNumber());
		throw eInfo;
	}
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysCPMportAdv Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
/**
\copydoc IPortAdv::TriggerMovesInGroup
**/
void SysCPMportAdv::TriggerMovesInGroup(size_t groupNumber)
{
	cnErrCode theErr = netTrigger(m_pPort->NetNumber(), 
								  nodeaddr(groupNumber), true);
	if (theErr != MN_OK) {
		mnErr eInfo;
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_, 
			"Failure to trigger moves in group %d on network %d",
			groupNumber, m_pPort->NetNumber());
		throw eInfo;
	}
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysCPMattnPort Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
void SysCPMattnPort::Enable(bool newState)
{
	AttnsEnabled = newState;
}

bool SysCPMattnPort::Enabled()
{
	return AttnsEnabled;
}

IAttnPort::attnState SysCPMattnPort::WaitForAttn(mnAttnReqReg &attnRecvd)
{
	cnErrCode theErr;
	mnErr eInfo;

	// They have disabled our processing of attentions
	if (!AttnsEnabled) {
		// Spin-out prevention as well as fielding any in route
		// attentions. 
		infcSleep(100);
		// Keep killing buffer
		theErr = infcNetAttnFlush(m_pPort->NetNumber());
		if (theErr != MN_OK) {
			fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
				"Attention flush critically failed");
			throw eInfo;
		}
		return IAttnPort::DISABLED;
	}

	// Wait for next attention or time-out
	theErr = infcNetGetAttnReq(m_pPort->NetNumber(), &attnRecvd);
	switch (theErr) {
	case MN_OK:
		return IAttnPort::HAS_ATTN;
	case MN_ERR_TIMEOUT:
		return IAttnPort::TIMEOUT;
	case MN_ERR_ATTN_OVERRUN:
	default:
		fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
			"Wait for attention critically failing");
		throw eInfo;
		break;
	};
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysCPMgrpShutdown Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
void SysCPMgrpShutdown::ShutdownWhen(size_t nodeIndex, const ShutdownInfo &theInfo)
{
	multiaddr theAddr = MULTI_ADDR(m_pPort->NetNumber(), nodeIndex);
	cnErrCode theErr = infcShutdownInfoSet(theAddr, &theInfo);
	if (theErr == MN_OK) return;
	mnErr eInfo;
	fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
		"Failed to setup group shutdown for node %d on port %d",
		nodeIndex, m_pPort->NetNumber());
	throw eInfo;
}

void SysCPMgrpShutdown::ShutdownWhenGet(size_t nodeIndex, ShutdownInfo &theInfo)
{
	multiaddr theAddr = MULTI_ADDR(m_pPort->NetNumber(), nodeIndex);
	cnErrCode theErr = infcShutdownInfoGet(theAddr, &theInfo);
	if (theErr == MN_OK) return;
	mnErr eInfo;
	fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
		"Failed to get group shutdown information for node %d on port %d",
		nodeIndex, m_pPort->NetNumber());
	throw eInfo;
}

void SysCPMgrpShutdown::ShutdownInitiate()
{
	cnErrCode theErr = infcShutdownInitiate(m_pPort->NetNumber());
	if (theErr == MN_OK) return;
	mnErr eInfo;
	fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
		"Failed to initiate group shutdown on port %d",
		m_pPort->NetNumber());
	throw eInfo;
}

//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// SysCPMbrakeControl Class Implementations
//= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

/**
	\copydoc IBrakeControl::BrakeSetting(size_t, BrakeControls)
**/
void SysCPMbrakeControl::BrakeSetting(size_t brakeNum, BrakeControls brakeMode)
{
	cnErrCode theErr = infcBrakeControlSet(
			m_pPort->NetNumber(), brakeNum, brakeMode);
	if (theErr == MN_OK) return;
	mnErr eInfo;
	fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
		"Failed override brake %d on port %d",
		brakeNum, m_pPort->NetNumber());
	throw eInfo;
}
/**
	\copydoc IBrakeControl::BrakeControl(size_t)
**/
BrakeControls SysCPMbrakeControl::BrakeSetting(size_t brakeNum)
{
	BrakeControls theState;
	cnErrCode theErr = infcBrakeControlGet(
			m_pPort->NetNumber(), brakeNum, &theState);
	if (theErr == MN_OK) return theState;
	mnErr eInfo;
	fillInErrs(eInfo, theErr, _TEK_FUNC_SIG_,
		"Failed get override state for brake %d on port %d",
		brakeNum, m_pPort->NetNumber());
	throw eInfo;
}

/**
	\copydoc IBrakeControl::BrakeEngaged(size_t)
**/
bool SysCPMbrakeControl::BrakeEngaged(size_t brakeNum) 
{
	nodebool nBool;
	/*cnErrCode theErr =*/ infcBrakeGet(
		m_pPort->NetNumber(), brakeNum, &nBool);
	return nBool != 0;
}
//============================================================================= 
//	END OF FILE cpmClassImpl.cpp
//=============================================================================


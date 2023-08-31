//*****************************************************************************
// $Archive: /ClearPath SC-1.0.123/User Driver/inc/pubCpmCls.h $
// $Revision: 50 $ $Date: 12/09/16 3:10p $
// $Workfile: pubCpmCls.h $
//
// DESCRIPTION:
/**
	\file	
	\defgroup grpCode grpDescription
	Concrete implementation of ClearPath-SC objects
**/
// CREATION DATE:	  
//		2015-12-08 15:13:51
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
#ifndef __PUBCPMCLS_H__
#define __PUBCPMCLS_H__


//*****************************************************************************
// NAME																          *
// 	pubCpmCls.h headers
//
#include "tekTypes.h"
#include "pubSysCls.h"
#include <vector>
//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	pubCpmCls.h function prototypes
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	pubCpmCls.h constants
//
//

//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	CPMouts class
//
// 
namespace sFnd
{
	// Forward declare for friends
	class CPMnode;
	class CPMnodeAdv;
	class CPMinfo;
	class CPMinfoAdv;
	class CPMlimits;
	class CPMmotion;
	class CPMstatus;

//*****************************************************************************
//  NAME                                                                      *
//      CPMstring class
//
// CREATION DATE:
//      01/15/2016
//
//  DESCRIPTION:
/**
\brief Class to access and modify string items within a ClearPath-SC node.
**/
class MN_EXPORT CPMstring : public ValueString
{
friend class CPMnode;
friend class CPMinfo;

private:
	void Refresh();
	void Value(const char *newValue);
	const char *Value();
protected:
	CPMstring(INode &theNode, nodeparam pNum, enum _srcs location);
};
//																			  *
//*****************************************************************************

//*****************************************************************************
//  NAME                                                                      *
//      CPMattnNode class
//
// CREATION DATE:
//      05/23/2016
//
//  DESCRIPTION:
/**
This class implements the ClearPath-SC attention feature.
**/
class MN_EXPORT CPMattnNode : public IAttnNode
{
friend class CPMnodeAdv;

private:
	bool AttnWait(size_t timeoutMs);
	void AttnSignal();
	void AttnRegister();
	void AttnUnregister();
	// Constructors
protected:
														/// \cond INTERNAL_DOC
	CPMattnNode(INode &ourNode);
														/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMlimitsAdv class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
This class implements the advanced ClearPath-SC limits feature.
**/
class MN_EXPORT CPMlimitsAdv : public ILimitsAdv
{
	friend class CPMlimits;
	friend class CPMnode;
private:
	void StartPosFoldback(bool engage);
	bool StartPosFoldback();

	void StartNegFoldback(bool engage);
	bool StartNegFoldback();

	// Constructors
protected:
														/// \cond INTERNAL_DOC
	CPMlimitsAdv(INode &ourNode);
														/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMmotionAudit class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	\brief Motion audit feature access.

	This class implements a ClearPath-SC advanced feature for collecting
	data from moves.
**/
class MN_EXPORT CPMmotionAudit : public IMotionAudit
{
	friend class CPMnode;
	friend class CPMnodeAdv;
	friend class CPMinfoAdv;

private:
	void Refresh();
	void SelectTestPoint(enum _testPoints testPoint,
		double fullScale, double filterTCmsec);
	// Constructors
	/// \cond INTERNAL_DOC
protected:
	CPMmotionAudit(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************

	
//*****************************************************************************
//  NAME                                                                      *
//      CPMnodeAdv class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	\brief Advanced features access

	This class implements a ClearPath-SC advanced features accessor.
**/
class MN_EXPORT CPMnodeAdv : public INodeAdv
{
	friend class CPMnode;
protected:
private:
	CPMmotionAudit m_audit;
	CPMattnNode m_attnNode;
	/// \cond INTERNAL_DOC
	void Supported(bool newState);
	/// \endcond
	// Constructors
protected:
	/// \cond INTERNAL_DOC
	CPMnodeAdv(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMhoming class
//
// CREATION DATE:
//      12/21/2015
//
//  DESCRIPTION:
/**
	This class implements the homing feature of the ClearPath-SC.

**/
class MN_EXPORT CPMhoming : public IHoming
{
	friend class CPMmotion;
private:
	// Initiator
	void Initiate();
	// Test for execution
	bool IsHoming();
	// Test for homing complete
	bool WasHomed();
	// Signal homing complete
	void SignalComplete();
	// Signal home invalid
	void SignalInvalid();
	// Test homing settings
	bool HomingValid();
protected:
	// Constructor
	CPMhoming(INode &ourNode);
};
//																			  *
//*****************************************************************************

	
	
//*****************************************************************************
//  NAME                                                                      *
//      CPMinfoAdv class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	\brief Advanced information features access

	This class implements a ClearPath-SC advanced information accessor.
**/
class MN_EXPORT CPMinfoAdv : public IInfoAdv
{
	friend class CPMinfo;
protected:
	// Constructors
	/// \cond INTERNAL_DOC
	CPMinfoAdv(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMinfoEx class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
\brief Expert information features access

This class implements a ClearPath-SC advanced information accessor.
**/
class MN_EXPORT CPMinfoEx : public IInfoEx
{
	friend class CPMinfo;
private:
	// Parameter access
	double Parameter(nodeparam index);
	void Parameter(nodeparam index, double newValue);
protected:
	// Constructors
	/// \cond INTERNAL_DOC
	CPMinfoEx(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMinfo class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements the Info object for the INode virtual class.

	This includes common information such as:
	- Firmware version
	- Node types
	- Serial Number
	- User ID

	In addition to specific information, the configuration file can be
	saved and loaded. This file is typically generated by the ClearView
	setup application.
**/
class MN_EXPORT CPMinfo : public IInfo
{
	friend class CPMnode;
protected:
	nodeTypes m_nodeType;
	void NodeType(nodeTypes);
private:
	// Access to node type
	nodeTypes NodeType() { return m_nodeType; }
	// User Data
	void UserData(size_t bank, const uint8_t theData[MN_USER_NV_SIZE]);
	std::vector<uint8_t> UserData(size_t bank);
	// Construction
	// Advanced info access
	CPMinfoAdv m_advInfo;
	// Expert info access
	CPMinfoEx m_exInfo;
	// Actual instances for string "parameters"
	CPMstring m_userID;
	CPMstring m_fwVersion;
	CPMstring m_hwVersion;
	CPMstring m_model;
	/// \cond INTERNAL_DOC
protected:
	CPMinfo(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMlimits class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements a ClearPath-SC limits feature.
**/
class MN_EXPORT CPMlimits  : public ILimits
{
	friend class CPMnode;
protected:
	/**
		\cond INTERNAL_DOC
		ClearPath-SC implementation instance for advanced limits.
	**/
	CPMlimitsAdv m_adv;
	/// \endcond
private:

public:

	// Constructors
	/// \cond INTERNAL_DOC
protected:
	CPMlimits(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMmotionAdv class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements the motion interface object for advanced optioned 
	INode virtual class.

	This interface can:
	- query and setup kinematic limits for advanced motion 
	- initiate advanced motion
**/
class MN_EXPORT CPMmotionAdv : public IMotionAdv
{
	friend class CPMmotion;
public:
	 
	/**		
		\copydoc IMotion::MovePosnHeadTailDurationMsec
	**/
	double MovePosnHeadTailDurationMsec(int32_t targetPosn,
		bool targetIsAbsolute = false,
		bool hasHead = true,
		bool hasTail = true);
	
	/**		
		\copydoc IMotion::MovePosnAsymDurationMsec
	**/
	double MovePosnAsymDurationMsec(int32_t targetPosn,
		bool targetIsAbsolute = false);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Motion Initiating Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	size_t MovePosnStart(int32_t target,
						bool targetIsAbsolute,
						bool isTriggered,
						bool hasDwell);

	size_t MovePosnHeadTailStart(int32_t target, 
								bool targetIsAbsolute,
								bool isTriggered,
								bool hasHead, bool hasTail, 
								bool hasDwell);
	size_t MovePosnAsymStart(int32_t target, 
						bool targetIsAbsolute,
						bool isTriggered, bool hasDwell);
	size_t MoveVelStart(double target,
					   bool isTriggered);

	// Triggering waiting moves
	void TriggerMove();
	void TriggerMovesInMyGroup();
	void TriggerGroup(size_t groupNumber);
	size_t TriggerGroup();

	// Construction
	/// \cond INTERNAL_DOC
protected:
	CPMmotionAdv(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMmotion class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements the motion interface object for the 
	INode virtual class.

	This interface can:
	- query and setup kinematic limits for motion 
	- initiate motion
	- check for motion completion and status
**/
class MN_EXPORT CPMmotion : public IMotion
{
	friend class CPMnode;
protected:
	/**
		\cond INTERNAL_DOC
		ClearPath-SC implementation instance for advanced motion.
	**/
	CPMmotionAdv m_adv;
	/**
		ClearPath-SC implementation instance for homing.
	**/
	CPMhoming m_homing;
	/// \endcond
public:
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Kinematic Constraint Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**		
		\copydoc IMotion::MovePosnDurationMsec
	**/
	double MovePosnDurationMsec(int32_t target, bool targetIsAbsolute);
	/**		
		\copydoc IMotion::MoveVelDurationMsec
	**/
	double MoveVelDurationMsec(double target);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Motion Initiating Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	size_t MovePosnStart(int32_t target, 
					     bool targetIsAbsolute,
					     bool hasDwell);

	size_t MoveVelStart(double target);
	
	/**
		\copydoc IMotion::MoveWentDone
	**/
	virtual bool MoveWentDone();

	/**
		\copydoc IMotion::MoveIsDone
	**/
	virtual bool MoveIsDone();

	/**
		\copydoc IMotion::VelocityReachedTarget
	**/
	virtual bool VelocityReachedTarget();

	/**
		\copydoc IMotion::VelocityAtTarget
	**/
	virtual bool VelocityAtTarget();

	/**
		\copydoc IMotion::WentNotReady
	**/
	virtual bool WentNotReady();

	/**
		\copydoc IMotion::IsReady
	**/
	virtual bool IsReady();

	void AddToPosition(double adjAmount);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Motion Canceling Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void NodeStop(nodeStopCodes howToStop);
	void NodeStop(mgNodeStopReg &howToStop);
	void GroupNodeStop(nodeStopCodes howToStop);
	void NodeStopClear();
	// Construction
	/// \cond INTERNAL_DOC
protected:
	CPMmotion(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMouts class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements access to the outputs object.

	This allow access to features controlled via the output register.
**/
class MN_EXPORT CPMouts : public IOuts
{
	friend class CPMnode;
private:
public:
	// Easy access to enable request
	void EnableReq(bool newState);
	bool EnableReq();
	// Constructor
	/// \cond INTERNAL_DOC
protected:
	CPMouts(INode &ourNode);
	~CPMouts();
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMsetup class
//
// CREATION DATE:
//      12/21/2015
//
//  DESCRIPTION:
/**
This class implements access to the setup information and methods for
a ClearPath-SC node.
**/
class MN_EXPORT CPMsetup : public ISetup
{
	friend class CPMnode;
protected:
	void ConfigLoad(const char *filePath, bool doReset = false);
	void ConfigSave(const char *filePath);
	bool AccessLevelIsFull();
public:
	// Construction
	/// \cond INTERNAL_DOC
protected:
	CPMsetup(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMstatusAdv class
//
// CREATION DATE:
//      12/21/2015
//
//  DESCRIPTION:
/**
	This class implements access to the advanced status information for 
	a ClearPath-SC node.
**/
class MN_EXPORT CPMstatusAdv  : public IStatusAdv
{
	friend class CPMstatus;
protected:
public:
	// Construction
	/// \cond INTERNAL_DOC
protected:
	CPMstatusAdv(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


	
//*****************************************************************************
//  NAME                                                                      *
//      CPMstatus class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements access to status information for a ClearPath-SC
	node.
**/
class MN_EXPORT CPMstatus  : public IStatus
{
	friend class CPMnode;
protected:
	/**
		\cond INTERNAL_DOC
		ClearPath-SC implementation instance for advanced motion.
	**/
	CPMstatusAdv m_adv;
	/// \endcond
public:
	void AlertsClear();
	bool IsReady() { return Node().Motion.IsReady(); }
	bool HadTorqueSaturation();
public:
	// Construction
	/// \cond INTERNAL_DOC
protected:
	CPMstatus(INode &ourNode);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMnodeEx class
//
// CREATION DATE:
//      01/18/2016
//
//  DESCRIPTION:
/**
This class implements a ClearPath-SC node expert feature accessors.
**/
class MN_EXPORT CPMnodeEx : public INodeEx
{
	friend class CPMnode;
protected:
private:
	void MutexTake();
	void MutexRelease();
	/// \cond INTERNAL_DOC	
protected:
	CPMnodeEx(INode &node);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
//  NAME                                                                      *
//      CPMnode class
//
// CREATION DATE:
//      12/16/2015
//
//  DESCRIPTION:
/**
	This class implements a ClearPath-SC node accessors.
**/
class CPMnode : public INode
{
	friend class CPMmotion;
	friend class CPMmotionAdv;
private:
	// Common node info access
	CPMinfo m_info;
	CPMmotion m_motion;
    CPMstatus m_status;
	CPMlimits m_limits;
	CPMouts m_outs;
	CPMsetup m_setup;
	// Advanced features
	CPMnodeAdv m_adv;
	CPMnodeEx m_ex;
	// Units interface
	void VelUnit(enum _velUnits newUnits);
	void TrqUnit(enum _trqUnits newUnits);
	void AccUnit(enum _accUnits newUnits);

	// Scaling 'constants'
	double m_accScaleToUser;
	// Scale factor to use for amps based on I_MAX
	double m_trqScaleToUser;
	// Scale factor to use for amps based on ADC_MAX
	double m_adcTrqScaleToUser;
	// Scale factor to use for amps to shutdown amps
	double m_rmsTrqScaleToUser;
	double m_velScaleToUser;
protected:
	double velScale() { return m_velScaleToUser;  }
	// Handy lists of various values to allow quick unit changes.
	std::vector<ValueDouble *> m_accValues;
	std::vector<ValueDouble *> m_velValues;
	std::vector<ValueDouble *> m_trqValues;
	std::vector<ValueDouble *> m_adcTrqValues;
	std::vector<ValueDouble *> m_rmsTrqValues;

/// \cond INTERNAL_DOC	
public:
	CPMnode(IPort &port, multiaddr theAddr);
	~CPMnode() {};
/// \endcond
};
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																	      *
// 	SysCPMbrakeControl class 
/**
	\brief ClearPath COM Hub brake control feature implementation.

	This is the brake control feature that either is driven by the enabled
	state of the associated node or manually controlled by the user.
**/
class SysCPMbrakeControl : public IBrakeControl
{
	friend class SysCPMport;
private:
	void BrakeSetting(size_t brakeNum, BrakeControls brakeMode);
	BrakeControls BrakeSetting(size_t brakeNum);
	bool BrakeEngaged(size_t brakeNum);

	/// \cond INTERNAL_DOC
	/// Construct our ClearPath COM Hub brake implementation
protected:
	SysCPMbrakeControl(IPort &ourPort);
	/// \endcond
};
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																	      *
// 	SysCPMgrpShutdown class 
/**
	\brief ClearPath-SC Group Shutdown feature.

	This is group shutdown feature for ClearPath-SC nodes.
**/
class SysCPMgrpShutdown : public IGrpShutdown
{
	friend class IGrpShutdown;
	friend class SysCPMport;
private:
	void ShutdownWhen(size_t nodeIndex, const ShutdownInfo &theInfo);
	void ShutdownWhenGet(size_t nodeIndex, ShutdownInfo &theInfo);
	void ShutdownInitiate();
	/// \cond INTERNAL_DOC
	/// Construct our ClearPath COM Hub brake implementation
protected:
	SysCPMgrpShutdown(IPort &ourPort);
	/// \endcond
};
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																	      *
// 	SysCPMattnPort class 
/**
\brief ClearPath-SC Group Shutdown feature.

This is group shutdown feature for ClearPath-SC nodes.
**/
class SysCPMattnPort : public IAttnPort
{
	friend class IPort;
	friend class SysCPMportAdv;
private:
	void Enable(bool newState);
	bool Enabled();
	
	IAttnPort::attnState WaitForAttn(mnAttnReqReg &attn);

	/// \cond INTERNAL_DOC
	/// Construct our ClearPath COM Hub brake implementation
protected: 
	SysCPMattnPort(IPort &ourPort);
	/// \endcond
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	SysCPMportAdv class
/**
\brief ClearPath COM Hub port implementation for advanced features.

This is the IPortAdv implementation for the serial port attached to the
ClearPath Communications hub.

\note This is not exported to prevent end-user from creating
instances in DLL.
**/
class SysCPMportAdv : public IPortAdv
{
	friend class IPort;
	friend class SysCPMport;

private:
	SysCPMattnPort m_attnPort;
	void TriggerMovesInGroup(size_t groupNumber);
protected:
	SysCPMportAdv(IPort &ourPort);
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	SysCPMport class 
/**
	\brief ClearPath COM Hub port implementation.

	This is the IPort implementation for the serial port attached to the
	ClearPath Communications hub.

	\note This is not exported to prevent end-user from creating
	instances in DLL.
**/
class SysCPMport : public IPort
{
private:
	SysCPMbrakeControl m_brakeControl;
	SysCPMgrpShutdown m_groupShutdown;
	SysCPMportAdv m_advPort;
	/**
		\copydoc IPort::NodeCount
	**/
	Uint16 NodeCount();
	/**
		\copydoc IPort::OpenState
	**/
	openStates OpenState();
	/**
		\copydoc IPort::WaitForOnline
	**/
	bool WaitForOnline(int32_t timeoutMs = 15000);
	/**
		\copydoc IPort::RestartCold
	**/
	void RestartCold();
	/**
		\copydoc IPort::NodeStop
	**/
	void NodeStop(mgNodeStopReg stopType);
	/**
		\copydoc IPort::CommandTraceSave
	**/
	void CommandTraceSave(const char *filePath);
	/// \cond INTERNAL_DOC
	// Construction
public: 
	SysCPMport(netaddr index);
	~SysCPMport();
	/// \endcond
};
//																			  *
//*****************************************************************************

//*****************************************************************************
}  // namespace sFnd
//*****************************************************************************

#endif


//============================================================================= 
//	END OF FILE pubCpmCls.h
//=============================================================================

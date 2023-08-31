///*******************************************************************************
//
// NAME
//		Axis
//
// DESCRIPTION:
//		This class holds the state machine and handles communication
//		directly with the ClearPath-SC node
//
//																			   *
//******************************************************************************

//******************************************************************************
// NAME																		   *
// 	Axis.cpp headers
//
#include <stdio.h>
#include "Axis.h"
#include "Supervisor.h"
#if defined(_WIN32)||defined(_WIN64)
#include <windows.h>
#endif
//																			   *
//******************************************************************************

//******************************************************************************
// NAME																		   *
// 	Axis.cpp defines
//
//																			   *
//******************************************************************************
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

//******************************************************************************
//	NAME																	   *
//		Axis::Axis
//
//	DESCRIPTION:
//		Constructor for the axis state
//
Axis::Axis(INode *node) : 
	m_node(node), 
	m_moveCount(0), 
	m_quitting(false) {

	// Save the config file before starting
	// This would typically be performed right after tuning each axis.
	// Chosing a file name that contains the address and serial number
	// can ensure that the system does not get miswired.
	// Using the UserID as a filename key can allow swapping motors with 
	// minimal setup required. This requires that no invalid characters 
	// are present in the UserID.
	char tempPath[MAX_CONFIG_PATH_LENGTH];
	#if defined(_WIN32)||defined(_WIN64)
		GetTempPathA(MAX_CONFIG_PATH_LENGTH, tempPath);
	#else
		strncpy(tempPath, "/tmp", sizeof(tempPath));
	#endif	
	snprintf(m_configFile, sizeof(m_configFile), "%s/config-%02d-%d.mtr", tempPath, 
		m_node->Info.Ex.Addr(), m_node->Info.SerialNumber.Value());
	// sprintf(m_configFile, "%s\config-%s.mtr", tempPath, m_node->Info.UserID.Value());

	if (m_node->Setup.AccessLevelIsFull())
		m_node->Setup.ConfigSave(m_configFile);

};
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::~Axis
//
//	DESCRIPTION:
//		Destructor for the axis state
//
Axis::~Axis(){
	// Print out the move statistics one last time
	PrintStats();

	// If we don't have full access, there's nothing to do here
	if (!m_node->Setup.AccessLevelIsFull()){
		return;
	}

	// Disable the node and restore the config file
	attnReg attnReq;
	attnReq.cpm.Disabled = 1;
	m_node->Adv.Attn.ClearAttn(attnReq);
	m_node->EnableReq(false);

	m_node->Status.RT.Refresh();
	if (m_node->Status.RT.Value().cpm.Enabled){
		attnReg theAttn = m_node->Adv.Attn.WaitForAttn(attnReq, 3000);
		if (theAttn.cpm.Enabled){
			printf("Error: Timed out waiting for disable\n");
			return;
		}
	}
	// Restore the original config file
	// This would typically be performed when setting up the machine.
	m_node->Setup.ConfigLoad(m_configFile);
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::Enable
//
//	DESCRIPTION:
//		Clear alerts and get the node ready to roll
//
void Axis::Enable(){
	if (m_node != NULL){
		// Clear alerts and node stops
		m_node->Status.AlertsClear();
		m_node->Motion.NodeStop(STOP_TYPE_ABRUPT);
		m_node->Motion.NodeStopClear();
		
		// Wait for node to be ready to take commands
		attnReg attnReq;
		attnReq.cpm.Ready = 1;
		m_node->Adv.Attn.ClearAttn(attnReq);

		m_node->EnableReq(true);

		// If the node is not currently ready, wait for it to get there
		if (!m_node->Status.IsReady()){
			attnReg theAttn = m_node->Adv.Attn.WaitForAttn(attnReq, 3000);
			if (!theAttn.cpm.Ready){
				mnErr eInfo;
				eInfo.ErrorCode = MN_ERR_TIMEOUT;
				eInfo.TheAddr = m_node->Info.Ex.Addr();
				snprintf(eInfo.ErrorMsg, sizeof(eInfo.ErrorMsg),
					"Error: Timed out waiting for enable\n");
				throw eInfo;
			}
		}
		attnReq.cpm.Ready = 0;
		attnReq.cpm.WasHomed = 1;

		m_node->Adv.Attn.ClearAttn(attnReq);
		//If node is set up to home, start homing
		if (m_node->Motion.Homing.HomingValid()) {
			m_node->Motion.Homing.Initiate();

				attnReg theAttn = m_node->Adv.Attn.WaitForAttn(attnReq, 30000);
				if (!theAttn.cpm.WasHomed) {
					mnErr eInfo;
					eInfo.ErrorCode = MN_ERR_TIMEOUT;
					eInfo.TheAddr = m_node->Info.Ex.Addr();
					snprintf(eInfo.ErrorMsg, sizeof(eInfo.ErrorMsg),
						"Error: Timed out waiting for home\n");
					throw eInfo;
				}
		}

	}
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::InitMotionParams
//
//	DESCRIPTION:
//		Initialize accLim, velLim, etc
//
void Axis::InitMotionParams(){
	// Set the user units to RPM and RPM/s
	m_node->VelUnit(INode::RPM);
	m_node->AccUnit(INode::RPM_PER_SEC);
	m_node->Motion.VelLimit = VEL_LIM_RPM;
	m_node->Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
	m_node->Motion.JrkLimit = RAS_CODE;
	m_node->Limits.PosnTrackingLimit.Value(TRACKING_LIM);
	m_node->Limits.TrqGlobal.Value(TRQ_PCT);
	// Set up the move itself
	::int32_t moveDuration;
#define USE_HEAD_TAIL_MOVES 1
#if USE_HEAD_TAIL_MOVES
	m_move.type = MG_MOVE_STYLE_HEADTAIL_TRIG;
	// Create a reference to advanced motion object to save typing
	IMotionAdv &motion = m_node->Motion.Adv;
	motion.HeadDistance = m_move.value/4;
	motion.TailDistance = m_move.value/4;
	motion.HeadTailVelLimit = m_node->Motion.VelLimit.Value() / 8;
	_mgPosnStyle theStyle(m_move.type);
	moveDuration = (::int32_t)motion.MovePosnHeadTailDurationMsec(m_move.value,
							theStyle.fld.relative == 0, 
							motion.HeadDistance.Value() > 0,
							motion.TailDistance.Value() > 0);
#else
	m_move.type = MG_MOVE_STYLE_NORM_TRIG;
	_mgPosnStyle theStyle(m_move.type);
	moveDuration = (::int32_t)m_node->Motion.Adv.MovePosnDurationMsec(m_move.value,
							theStyle.fld.relative == 0);
#endif
	// Timeout for move: expected move duration + some time for synch between the nodes
	m_moveTimeoutMs = moveDuration + 20;
	//printf("  [%2d]: moveCounts=%d moveTimeout=%d\n", m_node->Info.Ex.Addr, m_move.value, m_moveTimeoutMs);
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::Move
//
//	DESCRIPTION:
//		Issue the move
//
void Axis::Move(){
	// Clear out any lingering move done attentions
	attnReg attnMask;
	attnMask.cpm.MoveDone = 1;
	attnMask.cpm.Disabled = 1;
	attnMask.cpm.NotReady = 1;
	m_node->Adv.Attn.ClearAttn(attnMask);

	// Uncomment the following line to enable breakpoint debugging. The lock 
	// allows the application to break without having timeout errors be thrown.
	// WARNING: While stepping through, leaving the scope this lock is 
	// declared in (ie this function) will disengage the lock,
	// and may result in timeouts.
	//SysManager::ThreadLock lock;

	_mgPosnStyle theStyle(m_move.type);
#if USE_HEAD_TAIL_MOVES
	if (m_moveCount % 2 == 0){
		m_node->Motion.Adv.MovePosnStart(0, true);
	} else {
		m_node->Motion.Adv.MovePosnHeadTailStart(m_move.value,
			theStyle.fld.relative == 0, theStyle.fld.wait == 1,
			m_node->Motion.Adv.HeadDistance.Value() > 0,
			m_node->Motion.Adv.TailDistance.Value() > 0,
			theStyle.fld.dwell == 1);
	}
#else
	m_node->Motion.Adv.MovePosnStart(m_move.value, 
		theStyle.fld.relative == 0, theStyle.fld.wait == 1, 
		theStyle.fld.dwell == 1);
#endif
	//printf("  Start move axis %d: %f\n", m_node->Info.Ex.Addr(), infcCoreTime());
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::WaitForMove
//
//	DESCRIPTION:
//		Wait for attention that move has completed
//
bool Axis::WaitForMove(::int32_t timeoutMs){
	if (m_node == NULL){
		return false;
	}
	attnReg attnMask;
	attnReg attnSeen;
	attnMask.cpm.MoveDone = 1;
	attnMask.cpm.Disabled = 1;
	attnMask.cpm.NotReady = 1;
	attnSeen = m_node->Adv.Attn.WaitForAttn(attnMask, timeoutMs, false);
	//printf("  End move axis %d: %f\n", m_node->Info.Ex.Addr(), infcCoreTime());
	if (attnSeen.cpm.Disabled){
		printf("  ERROR: [%d] disabled during move\n", m_node->Info.Ex.Addr());
		return false;
	}
	if (attnSeen.cpm.NotReady){
		printf("  ERROR: [%d] went NotReady during move\n", m_node->Info.Ex.Addr());
		return false;
	}
	return attnSeen.cpm.MoveDone;
};
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::AxisMain
//
//	DESCRIPTION:
//		The main work of the axis class - initialize the node and run
//		the state machine
//
void Axis::AxisMain(Supervisor *theSuper){
	bool moveDone;

	m_super = theSuper;

	// If we don't have nodes, exit
	if (m_node == NULL) {
		printf("Thread: No node, exiting\n");
		return;
	}

	try {
		attnReg attnMask;
		attnMask.cpm.MoveDone = 1;
		attnMask.cpm.Disabled = 1;
		attnMask.cpm.Enabled = 1;
		attnMask.cpm.Ready = 1;
		attnMask.cpm.WasHomed = 1;
		m_node->Adv.Attn.Mask = attnMask;

		// Get the node ready to go
		Enable();
		// Initialize the motion values
		InitMotionParams();
	}
	catch (mnErr &err) {
		fprintf(stderr, "Thread Setup link error: %s\n", err.ErrorMsg);
		m_super->Quit();
	}
	catch(...) {
		fprintf(stderr, "Thread setup failed generically.\n");
		m_super->Quit();
	}

	if (!m_quitting){
		// Perform homing here??
		
		// Zero out the measured position
		m_node->Motion.PosnMeasured.Refresh();
		double posn = m_node->Motion.PosnMeasured.Value();
		m_node->Motion.AddToPosition(-posn);
		m_node->Motion.PosnMeasured.Refresh();
		PrintStats(true);
	}

	m_state = STATE_IDLE;

	m_super->SignalReady(m_node->Info.Ex.Addr());

	// Start the machine cycling
	while (!m_quitting)  {
		try {

			// Save last state for debugging purposes
			m_lastState = m_state;
			
			switch (m_state) {
				case STATE_IDLE:
					// Update idle state
					m_super->SignalIdle(m_node->Info.Ex.Addr());
					// Quitting?
					if (m_quitting) 
						continue;
					m_state = STATE_WAIT_FOR_KICK;
					break;
				case STATE_WAIT_FOR_KICK:
					m_super->WaitForKick();
					// Out of here?
					if (m_quitting)
						continue;
					// Supervisor restarted us
					m_state = STATE_SEND_MOVE;
					break;
				case STATE_SEND_MOVE:
					// Initiate the motion
					Move();
					m_state = STATE_SIGNAL_MOVE_SENT;
					break;
				case STATE_SIGNAL_MOVE_SENT:
					m_super->SignalMoveSent(m_node->Info.Ex.Addr());
					m_state = STATE_WAIT_FOR_MOVE_DONE;
					break;
				case STATE_WAIT_FOR_MOVE_DONE:
					moveDone = WaitForMove(m_moveTimeoutMs);
					m_super->SignalMoveDone(m_node->Info.Ex.Addr());
					if (!moveDone){
						printf("ERROR: [%2d] timed out waiting for move done\n", m_node->Info.Ex.Addr());
						m_super->Quit();
						return;
					}
					else{
						m_moveCount++;
						m_state = STATE_IDLE;
					}
					if (m_quitting)
						continue;
					break;
				default:
					fprintf(stderr, "ThreadFunction: Illegal state");
					return;
			}
		} // Try block
		catch (mnErr &err) {
			fprintf(stderr, "Thread link error: %s\n", err.ErrorMsg);
			m_super->Quit();
			// Send any signals that the supervisor might be waiting for
			m_super->SignalMoveSent(m_node->Info.Ex.Addr());
			m_super->SignalMoveDone(m_node->Info.Ex.Addr());
			ResetToIdle();
			continue;
		}
		catch(...) {
			fprintf(stderr, "Thread failed generically.\n");
			m_super->Quit();
			// Send any signals that the supervisor might be waiting for
			m_super->SignalMoveSent(m_node->Info.Ex.Addr());
			m_super->SignalMoveDone(m_node->Info.Ex.Addr());
			ResetToIdle();
		}
	} // while (!m_quitting) 

	m_state = STATE_EXITED;
	ResetToIdle();
	m_super->SignalIdle(m_node->Info.Ex.Addr());
	printf("[%s] Thread Exited...\n", m_node->Info.UserID.Value());
	return;
};
//																			   *
//******************************************************************************

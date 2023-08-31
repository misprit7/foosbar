/****************************************************************************
 * $Source: $
 * $Revision: 7 $ $State: $ $Date: 6/02/16 5:32p $
 *
 * !NAME!
 *		Axis
 *
 * DESCRIPTION:
 *		This class provides an interface to the ClearPath SC node 
 *		in our example program
 *
 * PRINCIPLE AUTHORS:
 *		JS
 *
 * CREATION DATE:
 *		03/30/2016
 *
 * !end!																    *
 ****************************************************************************/

#ifndef __AXIS_H__
#define __AXIS_H__

/****************************************************************************
 * !NAME!																    *
 * 	Axis.h headers
 *																		!0! */
#include <thread>
#include <condition_variable>
#include <mutex>
#include "pubSysCls.h"
using namespace sFnd;
using namespace std;
/* 																	   !end!*
 ****************************************************************************/

/****************************************************************************
 * !NAME!																	*
 * 	Axis.h constants
 * 																		!0!	*/
#define RAS_CODE			3 
#define TRQ_PCT				100
#define ACC_LIM_RPM_PER_SEC	8000
#define VEL_LIM_RPM			2000
#define TRACKING_LIM		(m_node->Info.PositioningResolution.Value() / 4)
/* !end!																	*
 ****************************************************************************/

// This class needs to have a reference to its Supervisor
class Supervisor;

//******************************************************************************
// NAME																		   *
// 	Axis class
//
// DESCRIPTION
//	This is the interface to the ClearPath SC. It includes a thread 
//	which runs the state machine for the node.
//
class Axis {
private:
	Supervisor *m_super;		// The supervisor
	INode *m_node;				// The ClearPath-SC for this axis
	thread m_thread;			// Handle to this Axis's thread

	// Filename for the config file stored at the start
#define MAX_CONFIG_PATH_LENGTH 256
#define MAX_CONFIG_FILENAME_LENGTH 384
	char m_configFile[MAX_CONFIG_FILENAME_LENGTH];

	// State machine information
	enum StateEnum {
		STATE_IDLE,
		STATE_WAIT_FOR_KICK,
		STATE_SEND_MOVE,
		STATE_SIGNAL_MOVE_SENT,
		STATE_WAIT_FOR_MOVE_DONE,
		STATE_EXITED
	};
	StateEnum m_state;
	StateEnum m_lastState;

	// Move information
	mgMoveProfiledInfo m_move;
	::uint32_t m_moveCount;
	::int32_t m_moveTimeoutMs;

	bool m_quitting;			// Axis quitting

	// Synchronization
	condition_variable m_cond;
	mutex m_mutex;

	// Enable the node and get it ready to go
	void Enable();

	// Initialize accLim, velLim, etc
	void InitMotionParams();

	// Initiate a move
	void Move();

	// Wait for attention that move has completed
	bool WaitForMove(::int32_t timeoutMs);
	
	// The state machine
	void AxisMain(Supervisor *theSuper);

public:
	// Constructor/Destructor
	Axis(INode *node);
	~Axis();
	
	// Return a reference to our node
	INode* MyNode() {
		return(m_node);
	};

	// Print the current stats (number of moves performed and
	// current measured position)
	void PrintStats(bool onInit = false){
		m_node->Motion.PosnMeasured.Refresh();
		if (onInit){
			printf("  [%2d]:\t\t**at startup**\t\t%8.0f\n", m_node->Info.Ex.Addr(),
				m_node->Motion.PosnMeasured.Value());
		}
		else{
			printf("  [%2d]:\t\t%8d\t\t%8.0f\n", m_node->Info.Ex.Addr(), m_moveCount,
				m_node->Motion.PosnMeasured.Value());
		}
	}

	// Set the move distance to the given number of revolutions
	void SetMoveRevs(int numRevs){
		m_move.value = (long)(m_node->Info.PositioningResolution.Value())*numRevs;
		printf("[%s] m_move.value=%d\n", m_node->Info.UserID.Value(), m_move.value);
		m_node->Motion.Adv.HeadDistance = m_move.value/4;
		m_node->Motion.Adv.TailDistance = m_move.value/4;
	}

	// Create the thread and get it going
	void CreateThread(Supervisor *theSuper){
		m_thread = thread(&Axis::AxisMain, this, theSuper);
	};

	// Reset sequencer to "idle" state
	void ResetToIdle(){
		if (m_state != STATE_IDLE)
			m_state = m_lastState = STATE_IDLE;
	};

	// Time to quit
	void Quit(){
		m_quitting = true;
	}

	// Park here to wait for the thread to exit
	void Join(){
		m_thread.join();
	}
};
//																			   *
//******************************************************************************

#endif
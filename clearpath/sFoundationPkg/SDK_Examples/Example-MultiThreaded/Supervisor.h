/****************************************************************************
 * $Source: $
 * $Revision: 7 $ $State: $ $Date: 12/09/16 13:13 $
 *
 * !NAME!
 *		Supervisor
 *
 * DESCRIPTION:
 *		This class handles the synchronization between all the
 *		nodes in the system
 *
 * !end!																    *
 ****************************************************************************/

/****************************************************************************
 * !NAME!																    *
 * 	Supervisor.h headers
 *																		!0! */
#include "Axis.h"
#include "pubSysCls.h"
/* 																	   !end!*
 ****************************************************************************/

//******************************************************************************
// NAME																		   *
// 	Supervisor class
//
// DESCRIPTION
//	The supervisory thread that sets up and handles the synchronization
//	between all the nodes.
//
class Supervisor {
private:
	thread m_thread;		// Supervisor's thread
	SysManager &m_sysMgr;	// sFoundation manager

	vector<Axis *> m_axes;		// the list of nodes

	bool m_quitting;		// are we quitting?

	// synchronization variables
	mutex m_mutex;
	condition_variable m_cond;
	// Flags to indicate that all nodes have reported back 
	// for various conditions
	bool m_nodesIdle, m_nodesMoved, m_nodesDone, m_nodesKicked, m_nodesReady;
	// Flags to keep track of which nodes have reported back
	// for various conditions
	vector<bool> m_axisReady;
	vector<bool> m_axisIdle;
	vector<bool> m_axisSentMove;
	vector<bool> m_axisDone;

	// State machine information
	enum SuperStateEnum {
		SUPER_IDLE,
		SUPER_WAIT_FOR_IDLE,
		SUPER_WAIT_FOR_MOVES_SENT,
		SUPER_WAIT_FOR_MOVES_DONE,
		SUPER_EXITED
	};

	SuperStateEnum m_state;
	SuperStateEnum m_lastState;
	
public:
	// Constructor
	Supervisor(vector<Axis*> theAxes, SysManager &myMgr);

	void CreateThread(){
		m_thread = thread(&Supervisor::SuperMain, this);
	}

	// Print stats for all the nodes
	void PrintStats(){
		printf("\nStats:\t\tnumMoves\t\tPosition\n");
		for (Uint16 iAxis = 0; iAxis < m_axes.size(); iAxis++){
			m_axes.at(iAxis)->PrintStats();
		}
	}

	// Signals and waits by the nodes
	void SignalIdle(Uint16 nodeNum);		// Nodes tell the supervisor when they are idle
	void WaitForKick();						// Nodes need to wait for the supervisor to kick them
	void SignalMoveSent(Uint16 nodeNum);	// Nodes tell the supervisor when they've sent their move
	void SignalMoveDone(Uint16 nodeNum);	// Nodes tell the supervisor whne their move is done
	void SignalReady(Uint16 nodeNum);		// Nodes signal when they are initialized

	void SuperMain();		// the work for this thread

	// sFoundation supervisor for access to foundation support
	SysManager &Mgr() { return m_sysMgr; }

	// Quit and tell the nodes to quit, too
	void Quit(){
		printf("  Supervisor quitting\n");
		m_quitting = true;
		for (Uint16 iAxis = 0; iAxis < m_axes.size(); iAxis++)
			m_axes.at(iAxis)->Quit();
	}

	// Wait for the thread to finish
	void Terminate(){
		m_thread.join();
	}

private:
	// Mark that a particular node has signalled back for the given condition
	void SignalPerNode(vector<bool> &nodeFlags, 
					   bool &condition, 
					   Uint16 nodeNum){
		unique_lock<mutex> lock(m_mutex);
		nodeFlags.at(nodeNum) = true;
		bool gotAll = true;
		for (Uint16 iAxis = 0; iAxis < nodeFlags.size(); iAxis++)
			gotAll = gotAll && nodeFlags.at(iAxis);
		if (gotAll){
			condition = true;
			m_cond.notify_all();
		}
	}

	// Wait for a given condition
	void WaitForCondition(bool &condition){
		unique_lock<mutex> lock(m_mutex);
		while (!condition && !m_quitting)
			m_cond.wait(lock);
	}

	// Clear the indicators for a particular condition
	void ResetCondition(vector<bool> &nodeFlags, bool &condition){
		unique_lock<mutex> lock(m_mutex);
		for (Uint16 iAxis = 0; iAxis < nodeFlags.size(); iAxis++)
			nodeFlags.at(iAxis) = false;
		condition = false;
	}

	// Set a condition that doesn't rely on all the nodes to do something
	void SetCondition(bool &condition){
		unique_lock<mutex> lock(m_mutex);
		condition = true;
		m_cond.notify_all();
	}

	// Clear a condition that doesn't relay on all the nodes
	void ResetCondition(bool &condition){
		unique_lock<mutex> lock(m_mutex);
		condition = false;
	}
};

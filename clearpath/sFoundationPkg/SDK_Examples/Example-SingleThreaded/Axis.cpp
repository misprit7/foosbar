///*******************************************************************************
//
// NAME
//		Axis
//
// DESCRIPTION:
//		This class holds the state machine and handles communication
//		directly with the ClearPath-SC node
//																			   *
//******************************************************************************

//******************************************************************************
// NAME																		   *
// 	Axis.cpp headers
//
#include <stdio.h>
#include "Axis.h"
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
Axis::Axis(SysManager& SysMgr, INode *node) :
	m_node(node),
	m_moveCount(0),
	m_positionWrapCount(0),
	m_quitting(false),
	m_sysMgr (SysMgr) {

	// OPTIONAL - Save the config file before starting
	// This would typically be performed right after tuning each axis.
	// Chosing a file name that contains the address and serial number
	// can ensure that the system does not get miswired.
	// Using the UserID as a filename key can allow swapping motors with 
	// minimal setup required. This requires that no invalid characters 
	// are present in the UserID.
#if defined(_WIN32) || defined(_WIN64)
	char tempPath[MAX_CONFIG_PATH_LENGTH];
	GetTempPathA(MAX_CONFIG_PATH_LENGTH, tempPath);
	snprintf(m_configFile, sizeof(m_configFile), "%s\\config-%02d-%d.mtr", tempPath,
		m_node->Info.Ex.Addr(), m_node->Info.SerialNumber.Value());
#else
	snprintf(m_configFile, sizeof(m_configFile), "/tmp/config-%02d-%d.mtr",
			m_node->Info.Ex.Addr(), m_node->Info.SerialNumber.Value());
#endif

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
//		Destructor for the axis state; disable the node and restore the saved
//		config file
//
Axis::~Axis() {
	// Print out the move statistics one last time
	PrintStats();

	// If we don't have full access, there's nothing to do here
	if (!m_node->Setup.AccessLevelIsFull()) {
		return;
	}

	// Disable the node and wait for it to disable
	m_node->EnableReq(false);

	// Poll the status register to confirm the node's disable
	time_t timeout;
	timeout = time(NULL) + 3;
	m_node->Status.RT.Refresh();
	while (m_node->Status.RT.Value().cpm.Enabled) {
		if (time(NULL) > timeout) {
			printf("Error: Timed out waiting for disable\n");
			return;
		}
		m_node->Status.RT.Refresh();
	};

	// Restore the original config file
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
void Axis::Enable() {
	if (m_node != NULL) {
		// Clear alerts and node stops
		m_node->Status.AlertsClear();
		m_node->Motion.NodeStop(STOP_TYPE_ABRUPT);
		m_node->Motion.NodeStop(STOP_TYPE_CLR_ALL);

		// Enable the node
		m_node->EnableReq(true);

		// If the node is not currently ready, wait for it to get there
		time_t timeout;
		timeout = time(NULL) + 3;
		// Basic mode - Poll until disabled
		while (!m_node->Status.IsReady()) {
			if (time(NULL) > timeout) {
				printf("Error: Timed out waiting for enable\n");
				return;
			}
		};

		//If node is set up to home, start homing
		if (m_node->Motion.Homing.HomingValid()) {
			m_node->Motion.Homing.Initiate();
			printf("Homing...");
			time_t timeout;
			timeout = time(NULL) + 10000;
			// Basic mode - Poll until disabled
			while (!m_node->Motion.Homing.WasHomed()) {
				if (time(NULL) > timeout) {
					printf("Error: Timed out waiting for homing\n");
					return;
				}
			};
			printf("Completed\n");
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
void Axis::InitMotionParams() {
	// Set the user units to RPM and RPM/s
	m_node->VelUnit(INode::RPM);
	m_node->AccUnit(INode::RPM_PER_SEC);

	// Set the motion's limits
	m_node->Motion.VelLimit = VEL_LIM_RPM;
	m_node->Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
	m_node->Motion.JrkLimit = RAS_CODE;
	m_node->Limits.PosnTrackingLimit.Value(TRACKING_LIM);
	m_node->Limits.TrqGlobal.Value(TRQ_PCT);

	// Set the dwell used by the motor when applicable
	m_node->Motion.DwellMs = 5;
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::ChangeMotionParams
//
//	DESCRIPTION:
//		Modify velLimit, AccLimit, etc.
//
void Axis::ChangeMotionParams(double newVelLimit,
	double newAccelLimit,
	uint32_t newJrkLimit,
	uint32_t newTrackingLimit,
	int32_t newTrqGlobal) {
	// Set the motion's limits
	m_node->Motion.VelLimit = newVelLimit;
	m_node->Motion.AccLimit = newAccelLimit;
	m_node->Motion.JrkLimit = newJrkLimit;
	m_node->Limits.PosnTrackingLimit.Value(newTrackingLimit);
	m_node->Limits.TrqGlobal.Value(newTrqGlobal);
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::PosnMove
//
//	DESCRIPTION:
//		Issue a position move
//
void Axis::PosnMove(int32_t targetPosn, bool isAbsolute, bool addDwell) {
	// Save the move's calculated time for timeout handling
	m_moveTimeoutMs = (int32_t)m_node->Motion.MovePosnDurationMsec(targetPosn,
		isAbsolute);
	m_moveTimeoutMs += 20;

	// Check for number-space wrapping. With successive moves, the position may
	// pass the 32-bit number cap (+ or -). The check below looks for that and
	// updates our counter to be used with GetPosition
	if (!isAbsolute) {
		if (GetPosition(false) + targetPosn > INT32_MAX) {
			m_positionWrapCount++;
		}
		else if (GetPosition(false) + targetPosn < INT32_MIN) {
			m_positionWrapCount--;
		}
	}

	// Issue the move to the node
	m_node->Motion.MovePosnStart(targetPosn, isAbsolute, addDwell);
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::VelMove
//
//	DESCRIPTION:
//		Issue a velocity move
//
void Axis::VelMove(double targetVel) {
	// Issue the move to the node
	m_node->Motion.MoveVelStart(targetVel);

	// Move at the set velocity for half a second
	m_sysMgr.Delay(500);

	// Issue a node stop, forcing any movement (like a 
	// velocity move) to stop. There are several 
	// different types of node stops
	m_node->Motion.NodeStop(STOP_TYPE_ABRUPT);
	//m_node->Motion.NodeStop(STOP_TYPE_RAMP);

	// Configure the timeout for the node stop to settle
	m_moveTimeoutMs = 100;
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
bool Axis::WaitForMove(::int32_t timeoutMs, bool secondCall) {
	if (m_node == NULL) {
		return false;
	}

	// Wait for the proper calculated move time
	time_t timeout;
	timeout = time(NULL) + timeoutMs;

	m_node->Status.RT.Refresh();
	// Basic mode - Poll until move done
	while (!m_node->Status.RT.Value().cpm.MoveDone) {
		m_node->Status.RT.Refresh();
		if (time(NULL) > timeout) {
			printf("Error: Timed out during move\n");
			return false;
		}

		// Catch specific errors that can occur during a move
		if (m_node->Status.RT.Value().cpm.Disabled) {
			printf("  ERROR: [%d] disabled during move\n",
				m_node->Info.Ex.Addr());
			return false;
		}
		if (m_node->Status.RT.Value().cpm.NotReady) {
			printf("  ERROR: [%d] went NotReady during move\n",
				m_node->Info.Ex.Addr());
			return false;
		}
	}

	return m_node->Status.RT.Value().cpm.MoveDone;
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
void Axis::AxisMain() {
	bool moveDone;

	// If we don't have nodes, exit
	if (m_node == NULL) {
		printf("No node, exiting\n");
		return;
	}

	try {
		// Get the node ready to go
		Enable();
		// Initialize the motion values
		InitMotionParams();
	}
	catch (mnErr &err) {
		fprintf(stderr, "Setup error: %s\n", err.ErrorMsg);
	}
	catch (...) {
		fprintf(stderr, "Setup failed generically.\n");
	}

	if (!m_quitting) {
		// If your node has appropriate hardware (hardstop/home switch) 
		// and is configured in ClearView, uncomment the line below to 
		// initiate homing
		//theNode.Motion.Homing.Initiate();

		// Set the hardware brake to allow motion
		m_node->Port.BrakeControl.BrakeSetting(0, BRAKE_ALLOW_MOTION);

		// Zero out the commanded position
		m_node->Motion.PosnCommanded.Refresh();
		double posn = m_node->Motion.PosnCommanded.Value();
		m_node->Motion.AddToPosition(-posn);
		m_node->Motion.PosnCommanded.Refresh();
		PrintStats(true);
	}

	m_state = STATE_IDLE;

	// Start the machine cycling
	while (!m_quitting) {
		try {
			// Save last state for debugging purposes
			m_lastState = m_state;

			// Check for alerts
			CheckForAlerts();

			switch (m_state) {
			case STATE_IDLE:
				// Update idle state
				// Quitting?
				if (m_quitting)
					continue;
				m_state = STATE_SEND_MOVE;
				break;
			case STATE_SEND_MOVE:
				// Initiate the motion
				if (m_moveCount % 2 == 0) {
					PosnMove(5000);
				}
				else {
					//Set new motion parameters before the next move
					ChangeMotionParams(1500, 7500, 4,
						m_node->Info.PositioningResolution.Value() / 4, 95);

					VelMove(50);
				}

				// Reset our motion parameters.  The parameters for the 
				// next move could be entered instead to get a head start
				InitMotionParams();

				m_state = STATE_WAIT_FOR_MOVE_DONE;
				break;
			case STATE_WAIT_FOR_MOVE_DONE:
				//Check if the move is done yet
				moveDone = WaitForMove(m_moveTimeoutMs);
				if (!moveDone) {
					printf("ERROR: [%2d] timed out waiting for move done\n",
						m_node->Info.Ex.Addr());
					return;
				}
				else {
					m_moveCount++;
					m_state = STATE_IDLE;
				}
				if (m_moveCount == 4) {
					// Stop once we've properly demonstrated movement
					Quit();
				}
				PrintStats();
				if (m_quitting)
					continue;
				break;
			default:
				fprintf(stderr, "Illegal state");
				return;
			}
		} // Try block
		catch (mnErr &err) {
			fprintf(stderr, "Link error: %s\n", err.ErrorMsg);
			ResetToIdle();
			continue;
		}
		catch (...) {
			fprintf(stderr, "Node failed generically.\n");
			ResetToIdle();
		}
	}

	m_state = STATE_EXITED;
	ResetToIdle();
	printf("[%s] Quitting node...\n", m_node->Info.UserID.Value());
	return;
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::CheckForAlerts
//
//	DESCRIPTION:
//		Checks the status register if there are alerts present.
//		If there are, print some information about them and quit.
//
void Axis::CheckForAlerts()
{
	// Refresh our relevant registers
	m_node->Status.Alerts.Refresh();
	m_node->Status.RT.Refresh();

	//Check for alerts
	if (m_node->Status.RT.Value().cpm.AlertPresent) {
		char alertList[256];
		m_node->Status.Alerts.Value().StateStr(alertList, 256);
		printf("Node %d has alerts! Alert: %s",
			m_node->Info.Ex.Addr(), alertList);
		m_quitting = true;
	}
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::SetBrakeControl
//
//	DESCRIPTION:
//		Setup the hardware brake control on the node's port. Set's the indicated
//		brake's mode to the passed-in value
//
void Axis::SetBrakeControl(size_t brakeNum, BrakeControls brakeMode)
{
	m_node->Port.BrakeControl.BrakeSetting(brakeNum, brakeMode);
}
//																			   *
//******************************************************************************

//******************************************************************************
//	NAME																	   *
//		Axis::GetPosition
//
//	DESCRIPTION:
//		Return the node's commanded position scaled properly to wraps of the
//		number space.
//
int64_t Axis::GetPosition(bool includeWraps)
{
	// Create a variable to return and refresh the position
	long long scaledPosn;
	m_node->Motion.PosnCommanded.Refresh();

	// If there have been no number wraps, just return the position
	scaledPosn = int64_t(m_node->Motion.PosnCommanded.Value());

	if (includeWraps) {
		scaledPosn += int64_t(m_positionWrapCount) << 32;
	}

	return scaledPosn;
}
//																			   *
//******************************************************************************

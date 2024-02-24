
/******************************************************************************
 * Includes
 ******************************************************************************/

#include <vector>

#include "physical_params.hpp"

/******************************************************************************
 * Typedefs
 ******************************************************************************/

typedef enum state_t {
    state_defense,
    state_shot_defense,
    state_uncontrolled,
    state_controlled_three_bar,
    state_controlled_five_bar,
    state_snake,
    state_test,
    state_unknown,
    num_state_t
} state_t;

// Controlled 3 bar
typedef enum c3b_t {
    c3b_init,
    c3b_raise,
    c3b_move_lateral,
    c3b_lower,
    c3b_push,
    c3b_shoot_wait,
    c3b_shoot_straight,
    c3b_shoot_straight_2,
    c3b_shoot_middle,
    c3b_shoot_middle_2,
    c3b_shoot_end,
    c3b_shoot_end_2,
    c3b_shoot_end_3,
    c3b_shoot_end_slow,
    c3b_shoot_end_slow_2,
    c3b_shoot_end_slow_3,
    c3b_adjust_move,
    c3b_adjust_down,
    c3b_idle,
} c3b_t;

// Controlled 5 bar
typedef enum c5b_t {
    c5b_init,
    c5b_tic_tac,
    c5b_threaten,
    c5b_idle,
} c5b_t;

struct motor_cmd {
    // NAN for unchanged
    double pos;
    double vel;
    double accel;
};


/******************************************************************************
 * Public Functions
 ******************************************************************************/

// Not currently being used, probably ultimately better practice but keeping
// everything we need in scope is a pain
state_t algo_action(
        state_t cur_state,
        vector<double> ball_pos,
        vector<double> ball_vel,
        vector<motor_cmd> (&mtr_cmds)[num_axis_t]
);

/**
 * Gets index of player currently closest to target_cm
 */
int closest_plr(int rod, double target_cm, double cur_pos);

/**
 * Gets x offset of a player on a specific rod
 */
double plr_offset(int plr, int rod);

/**
 * Gets rod closest to ball_cm
 * ret[0]: side_t
 * ret[1]: rod_t
 */
pair<side_t, rod_t> closest_rod(double ball_cm);

/**
 * Determines if ball shot from a position will be blocked
 * rod: rod ball is being shot from
 * ball_cm: ball x coordinate
 * rod_pos: positions of 0th plr on each human rod
 * tol: extra tolerance for error
 */
bool is_blocked(int rod, double ball_cm, double rod_pos[num_axis_t], double tol=0);



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
    state_controlled_move,
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
    c5b_threaten_1,
    c5b_threaten_2,
    c5b_threaten_3,
    c5b_threaten_4,
    c5b_idle,
} c5b_t;

typedef enum cmove_t {
    cmove_init,
    cmove_side_1, // Go to side of ball
    cmove_side_2,
    cmove_side_3,
    cmove_side_4,
    cmove_tap_1, // Tap ball sideways and catch it
    cmove_tap_2,
    cmove_tap_3,
    cmove_tap_4,
    cmove_adjust_1, // Adjust ball by sliding it
    cmove_adjust_2,
    cmove_unstuck_1, // Get unstuck by gripping the ball
    cmove_unstuck_2,
    cmove_bounce_1, // Bounce ball away from edge
    cmove_bounce_2,
    cmove_bounce_3,
    cmove_idle,
} cmove_t;

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
 * start_rod: rod ball is being shot from
 * end_rod: rod ball is being shot from
 * ball_cm: ball x coordinate
 * rod_pos: positions of 0th plr on each human rod
 * tol: extra tolerance for error
 * end_rod: rod passing to, -1 for shots
 */
bool is_blocked(int start_rod, double ball_cm, double rod_pos[num_axis_t][num_rod_t], double tol=0, int end_rod=-1);

/**
 * Calculates x position of ball when ball has reached location y
 * Assumes that y is in the direction of ball_vel[1]
 */
double kin_ball_dist(vector<double> ball_pos, vector<double> ball_vel, double y);


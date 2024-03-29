
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


// Controlled 5 bar
typedef enum c5b_t {
    c5b_init,
    c5b_threaten_1,
    c5b_threaten_2,
    c5b_threaten_3,
    c5b_threaten_4,
    c5b_threaten_5,
    c5b_fast_1,
    c5b_fast_2,
    c5b_fast_wall_3,
    c5b_fast_wall_4,
    c5b_fast_lane_3,
    c5b_fast_lane_4,
    c5b_fast_5,
    c5b_fast_6,
    c5b_idle,
} c5b_t;

typedef enum cmove_t {
    cmove_init,
    cmove_decide_1,
    cmove_decide_2,
    cmove_side_1, // Go to side of ball
    cmove_side_2,
    cmove_side_3,
    cmove_side_4,
    cmove_top_1, // Go on top of ball
    cmove_top_2,
    cmove_top_3,
    cmove_top_4,
    cmove_angle_1,
    cmove_angle_2,
    cmove_tap_1, // Tap ball sideways and catch it
    cmove_tap_2,
    cmove_tap_3,
    cmove_tap_4,
    cmove_adjust_1, // Adjust ball by sliding it
    cmove_adjust_2,
    cmove_unstuck_1, // Get unstuck by gripping the ball
    cmove_unstuck_2,
    cmove_unstuck_3,
    cmove_bounce_1, // Bounce ball away from edge
    cmove_bounce_2,
    cmove_bounce_3,
    cmove_bounce_4,
    cmove_give_up_1, // Give up trying to control ball, just shoot randomly
    cmove_give_up_2,
    cmove_give_up_3,
    cmove_pin_1, // Set up a pin for a snake shot
    cmove_pin_2,
    cmove_pin_3,
    cmove_pin_4,
    cmove_pin_5,
    cmove_pin_6,
    cmove_goalie_1,
    cmove_goalie_2,
    cmove_goalie_3,
    cmove_idle,
} cmove_t;

typedef enum csnake_t {
    csnake_init,
    csnake_plan,
    csnake_shoot,
} csnake_t;

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

/**
 * Whether the given player can reach target_cm. Generally most useful for
 * five bar, since for any other bar everywhere is reachable by a player
 */
bool can_plr_reach(int plr, int rod, double target_cm, double tol=0);


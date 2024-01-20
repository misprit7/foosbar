
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
    state_shot_offense,
    state_goalie_possess,
    state_two_bar_possess,
    state_five_bar_possess,
    state_three_bar_possess,
    state_unknown,
    num_state_t
} state_t;

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

int closest_plr(int rod, double target_cm, double cur_pos);




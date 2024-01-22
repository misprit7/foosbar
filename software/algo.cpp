
#include "algo.hpp"
#include "physical_params.hpp"
#include <algorithm>

using namespace std;

/******************************************************************************
 * Defines
 ******************************************************************************/

// Pretty ugly hack to avoid rewriting the common parameters a bunch
// I could wrap them in a struct or something but seems kinda pointless
#define params \
        state_t cur_state,\
        vector<double> ball_pos,\
        vector<double> ball_vel,\
        vector<motor_cmd> (&mtr_cmds)[num_axis_t]

/******************************************************************************
 * Private functions
 ******************************************************************************/

state_t algo_defense(params){
        
    return state_defense;
}

/******************************************************************************
 * Public functions
 ******************************************************************************/

state_t (*algo_fns[num_state_t])(params) = {
    [state_defense]=algo_defense,
};

state_t algo_action(params){

    return algo_fns[cur_state](cur_state, ball_pos, ball_vel, mtr_cmds);
}

/*
 * Gets index of player currently closest to target_cm
 */
int closest_plr(int rod, double target_cm, double cur_pos){
    int plr1 = clamp((int)floor(num_plrs[rod] * target_cm / play_height), 0, num_plrs[rod]-1);
    double plr1_pos = bumper_width + plr_width/2 + plr1*plr_gap[rod] + cur_pos;

    int plr2 = plr1_pos > target_cm ? plr1-1 : plr1+1;

    if(plr2 >= num_plrs[rod] || plr2 < 0) return plr1;
    
    double plr2_rest = bumper_width + plr_width/2 + plr2*plr_gap[rod];
    double plr2_pos = plr2_rest + cur_pos;
    if(abs(plr2_pos-target_cm) < abs(plr1_pos-target_cm) 
            && target_cm > plr2_rest && target_cm < plr2_rest+lin_range_cm[rod])
        return plr2;
    else
        return plr1;
}

/*
 * Gets rod closest to ball_cm
 * ret[0]: side_t
 * ret[1]: rod_t
 */
pair<side_t, rod_t> closest_rod(double ball_cm){
    if(ball_cm >= 3*rod_gap) return {human, goalie};
    if(ball_cm >= 2*rod_gap) return {human, two_bar};
    if(ball_cm >= rod_gap) return {bot, three_bar};
    if(ball_cm >= 0) return {human, five_bar};
    if(ball_cm >= -rod_gap) return {bot, five_bar};
    if(ball_cm >= -2*rod_gap) return {human, three_bar};
    if(ball_cm >= -3*rod_gap) return {bot, two_bar};
    return {bot, goalie};
}




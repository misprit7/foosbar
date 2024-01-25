#pragma once

#include <string>
#include <cmath>

using namespace std;

/******************************************************************************
 * Rod defintions
 ******************************************************************************/

typedef enum rod_t {
    three_bar,
    five_bar,
    two_bar,
    goalie,
    num_rod_t
} rod_t;

const string rod_names[] = {
    "three-bar",
    "five-bar",
    "two-bar",
    "goalie",
};

typedef enum axis_t {
    lin,
    rot,
    num_axis_t
} axis_t;

typedef enum side_t {
    bot,
    human,
    num_side_t
} side_t;

/******************************************************************************
 * Table Dimensions
 ******************************************************************************/

/*
 * All dimensions are in cm
 *
         table_height
     ◄──────────────────────────────────────►
     ┌──────────────────────────────────────┐▲
     │                                      ││
     │                                      ││
     │ ┌──────────────────────────────────┐ ││
     │ │                plr_width        ▲│ ││
     │ │  bumper_width  ◄►               ││ ││
     │ │  ◄►                             ││ ││
     │ │    ┌┐          ┌┐          ┌┐   ││ ││
─────┼─┼────┼┼──────────┼┼──────────┼┼───┼┼─┼┼────
     │ │    └┘          └┘          └┘   ││ ││
     │ │                 ◄──────────►    ││ ││
     │ │                   plr_gap       ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││table_width
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                       play_width││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │                                 ││ ││
     │ │      play_height                ││ ││
     │ │◄───────────────────────────────►▼│ ││
     │ └──────────────────────────────────┘ ││
     │                                      ││
     │                                      ││
     └──────────────────────────────────────┘▼
 */

const double play_height = 68;
const double play_width = 116.6;

const double table_height = 75.6;
const double table_width = 139.5;

const double plr_width = 3.073;
const double foot_width = 2.23;
const double plr_height = 6.75;

const double bumper_width = 2.496;

const double rod_gap = 101.7/7;

// From the perspective of robot, obviously minus for human
const double rod_pos[num_rod_t] = {
    1.5*rod_gap, -0.5*rod_gap, -2.5*rod_gap, -3.5*rod_gap
};

// Slightly more accurate to measure multiple then divide
const double plr_gap[num_rod_t] = {
    (45.1 - 2*bumper_width - plr_width) / 2,
    (52.6 - 2*bumper_width - plr_width) / 4,
    (32.8 - 2*bumper_width - plr_width),
    (45.1 - 2*bumper_width - plr_width) / 2,
};

const int num_plrs[num_rod_t] = {
    3,
    5,
    2,
    3,
};

const double ball_rad = 3.475/2;

/******************************************************************************
 * Vision Parameters
 ******************************************************************************/

const double cal_offset[3] = {-4, 0.5, 0};
const int vision_fps = 200;

/******************************************************************************
 * Motor Parameters
 ******************************************************************************/
const int lin_range_cnts[][2] = {
    /* {-20200, 20}, */
    {-20180, 20},
    /* {-10150, 20}, */
    {-10130, 20},
    {-30370, 20},
    {21000, 20},
};

const int lin_mid_cnts[] = {
    (lin_range_cnts[three_bar][0] + lin_range_cnts[three_bar][1])/2,
    (lin_range_cnts[five_bar][0] + lin_range_cnts[five_bar][1])/2,
    (lin_range_cnts[two_bar][0] + lin_range_cnts[two_bar][1])/2,
    (lin_range_cnts[goalie][0] + lin_range_cnts[goalie][1])/2,
};

const double lin_cm_to_cnts[] = {
    (lin_range_cnts[three_bar][1] - lin_range_cnts[three_bar][0])
            / (play_height - (num_plrs[three_bar]-1)*plr_gap[three_bar] - plr_width - 2*bumper_width),
    (lin_range_cnts[five_bar][1] - lin_range_cnts[five_bar][0])
            / (play_height - (num_plrs[five_bar]-1)*plr_gap[five_bar] - plr_width - 2*bumper_width),
    (lin_range_cnts[two_bar][1] - lin_range_cnts[two_bar][0])
            / (play_height - (num_plrs[two_bar]-1)*plr_gap[two_bar] - plr_width - 2*bumper_width),
    (lin_range_cnts[goalie][1] - lin_range_cnts[goalie][0])
            / (play_height - (num_plrs[goalie]-1)*plr_gap[goalie] - plr_width - 2*bumper_width),
};

// cm always starts at 0
const double lin_range_cm[] = {
    (lin_range_cnts[three_bar][1] - lin_range_cnts[three_bar][0])
            / lin_cm_to_cnts[three_bar],
    (lin_range_cnts[five_bar][1] - lin_range_cnts[five_bar][0])
            / lin_cm_to_cnts[five_bar],
    (lin_range_cnts[two_bar][1] - lin_range_cnts[two_bar][0])
            / lin_cm_to_cnts[two_bar],
    (lin_range_cnts[goalie][1] - lin_range_cnts[goalie][0])
            / lin_cm_to_cnts[goalie],
};

// 0-360 degree
const int rot_range_cnts[][2] = {
    {0,800},
    {0,800},
    {0,800},
    {0,800},
};

const double deg_to_rad = (2*M_PI) / 360;

const double rot_deg_to_cnts[] = {
    (rot_range_cnts[three_bar][1] - rot_range_cnts[three_bar][0]) / 360.0,
    (rot_range_cnts[five_bar][1] - rot_range_cnts[five_bar][0]) / 360.0,
    (rot_range_cnts[two_bar][1] - rot_range_cnts[two_bar][0]) / 360.0,
    (rot_range_cnts[goalie][1] - rot_range_cnts[goalie][0]) / 360.0,
};

const double rot_rad_to_cnts[] = {
    rot_deg_to_cnts[three_bar] / deg_to_rad,
    rot_deg_to_cnts[five_bar] / deg_to_rad,
    rot_deg_to_cnts[two_bar] / deg_to_rad,
    rot_deg_to_cnts[goalie] / deg_to_rad,
};



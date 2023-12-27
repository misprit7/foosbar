#pragma once

#include <string>

using namespace std;

/******************************************************************************
 * Rod defintions
 ******************************************************************************/

typedef enum rod_t {
    three_bar,
    five_bar,
    two_bar,
    goalie,
    num_rod_t,
} rod_t;

const string rod_names[] = {
    "three-bar",
    "five-bar",
    "two-bar",
    "goalie",
};


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

const double bumper_width = 2.496;

// Slightly more accurate to measure multiple then divide
const double plr_gap[] = {
    (45.1 - 2*bumper_width - plr_width) / 2,
    (52.6 - 2*bumper_width - plr_width) / 4,
    (32.8 - 2*bumper_width - plr_width),
    (45.1 - 2*bumper_width - plr_width) / 2,
};

const int num_plrs[] = {
    3,
    5,
    2,
    3,
};

/******************************************************************************
 * Motor Parameters
 ******************************************************************************/
const int lin_range_cnts[][2] = {
    {-20200, 0},
    {-10200, 0},
    {-30370, 0},
    {0, 21000},
};

const int lin_mid_cnts[] = {
    (lin_range_cnts[three_bar][0] + lin_range_cnts[three_bar][1])/2,
    (lin_range_cnts[five_bar][0] + lin_range_cnts[five_bar][1])/2,
    (lin_range_cnts[two_bar][0] + lin_range_cnts[two_bar][1])/2,
    (lin_range_cnts[goalie][0] + lin_range_cnts[goalie][1])/2,
};

const double lin_cm_to_cnts[] = {
    (lin_range_cnts[three_bar][1] - lin_range_cnts[three_bar][0])
            / (play_height - 2*plr_gap[three_bar] - plr_width - 2*bumper_width),
    (lin_range_cnts[five_bar][1] - lin_range_cnts[five_bar][0])
            / (play_height - 2*plr_gap[five_bar] - plr_width - 2*bumper_width),
    (lin_range_cnts[two_bar][1] - lin_range_cnts[two_bar][0])
            / (play_height - 2*plr_gap[two_bar] - plr_width - 2*bumper_width),
    (lin_range_cnts[goalie][1] - lin_range_cnts[goalie][0])
            / (play_height - 2*plr_gap[goalie] - plr_width - 2*bumper_width),
};

// 0-360 degree
const int rot_range_cnts[][2] = {
    {0,800},
    {0,800},
    {0,800},
    {0,800},
};

const double rot_deg_to_cnts[] = {
    (rot_range_cnts[three_bar][1] - rot_range_cnts[three_bar][0]) / 360.0,
    (rot_range_cnts[five_bar][1] - rot_range_cnts[five_bar][0]) / 360.0,
    (rot_range_cnts[two_bar][1] - rot_range_cnts[two_bar][0]) / 360.0,
    (rot_range_cnts[goalie][1] - rot_range_cnts[goalie][0]) / 360.0,
};



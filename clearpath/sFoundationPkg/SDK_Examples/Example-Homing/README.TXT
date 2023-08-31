Example-Motion
======================

This is an example for a network of ClearPath SC nodes. This example will open all connected SC-Hub boards, and home each motor which has had homing defined through the ClearView setup software

Requirements:

-This program may execute motion on some or all nodes, nodes should be configured through the ClearView software.



Example-Homing.cpp - This is the main function for the program. It holds a reference to the system manager of the network and Port and Node objects. 
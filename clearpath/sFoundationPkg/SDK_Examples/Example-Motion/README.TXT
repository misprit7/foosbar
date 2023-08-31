Example-Motion
======================

This is an example for a network of ClearPath SC nodes. This example will discover all the nodes on each SC-hub port, and execute sequential moves on each Node.  The program will wait for each move to be finished before executing the next move on the next Node so only one node will be moving at a time. Node's which have been setup to home through the ClearView software will be homed before motion begins.

Requirements:

-This program executes motion, so all nodes should be unloaded with factory default settings, or should be autotuned on mechanics.

-This program executes repeated motion in one direction, and is not reccommend to be used on axis with limited range of motion.



Example-Motion.cpp - This is the main function for the program. It holds a reference to the system manager of the network and Port objects. The main program executes a series of moves, then waits for the user to press a key in order to end the program.

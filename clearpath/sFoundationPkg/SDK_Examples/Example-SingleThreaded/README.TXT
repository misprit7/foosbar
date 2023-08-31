Example-SingleThreaded(Polling) 
======================

This is an example for a network of ClearPath SC nodes. This program will discover all the nodes on each port and squentially perform a series of moves on each Node meant to exercise the basic features of a ClearPath SC.

Requirements:

-This program executes motion, so all nodes should be unloaded with factory default settings, or should be autotuned on mechanics.

-This program executes repeated motion in one direction, and is not recommended to be used on axis with limited range of motion.

 

Example-SingleThreaded.cpp - This is the main function for the program. It holds a reference to the system manager of the network and Axes objects. The main program then waits for the user to press a key in order to end the program.

Axis class - This class talks to the nodes directly; there is one Axis object per node on the network. Each Axis runs in its own operations before calling the next Axis on the network. The Axis class basically starts up a node, issues a series of move and other operations, then closes down.


Example-MultiThreaded(Attentions)
======================

This is a multi-Threaded example for a network of ClearPath SC nodes. This program will discover all the nodes on each port and synchronize moves among those nodes. The synchronization is fairly simple - the moves are all started at the same time (using a trigger) and the new moves are not sent until all the moves have completed. Each node has a different length of move, so they all finish at different times. The Axis class handles the interaction with a node on the network. The Supervisor class interacts with the Axis objects, telling them when to send their moves to the node. After each node has sent its move, the Supervisor starts all the moves with a Trigger command. The Axis then waits for MoveDone, and reports back to the Supervisor. The Supervisor waits for all the moves to finish, and starts the cycle again.

Requirements:

-This program uses attentions, and triggered moves, so each Node on the port must be an advanced Clearpath Node.

-This program executes motion, so all nodes should be unloaded with factory default settings, or should be autotuned on mechanics.

-This program executes repeated motion in one direction, and is not reccommend to be used on axis with limited range of motion.



Example-MultiThreaded.cpp - This is the main function for the program. It holds a reference to the system manager of the network and also creates the Supervisor object. The main program then waits for the user to press a key in order to end the program.

Supervisor class - This class holds the references to all the nodes in the network, and synchronizes the moves among those nodes. This class runs its own thread to handle the synchronization without having to wait for the user input to end the program. The Supervisor thread is a state machine that waits for the Axes to be idle, kicks the Axes to tell them to send their move to their node, waits for all the moves to be sent, sends a Trigger to start the moves, waits for all the Axes to report MoveDone, then starts all over again.

Axis class - This class talks to the nodes directly; there is one Axis object per node on the network. Each Axis runs in its own thread, and is synchronized with the other nodes through the Supervisor class. The Axis class basically waits for the Supervisor to tell it to send its move, then waits for MoveDone, reporting back to the Supervisor when it is received. It also reports to the Supervisor if an error was encountered with the node.


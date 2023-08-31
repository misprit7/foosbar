Example-GPIO
======================

This is an example for a network of ClearPath SC nodes connected to an SC-HUB. This program will assert the General purpose Brake output on the SC-HUB board, and discover all the nodes on the port. Finnally the program will record the current state of inputs A, and B on each node before shutting down.

Requirements:

-This program asserts the Brake output of the SC_HUB board, so an SC-HUB board is required.  Do not attempt to execute this program if using a custom communication hub.

-Be aware that the Brake output 0 of the SC-HUB board will be temporarily asserted when this program is executed.


Example-GPIO.cpp - This is the main function for the program. It holds a reference to the system manager of the network and Port objects. The main program then waits for the user to press a key in order to end the program.

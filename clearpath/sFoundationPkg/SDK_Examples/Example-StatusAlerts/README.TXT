Example-StatusAlerts
======================

This is an example for a network of ClearPath SC nodes. This example will initiate communication with up to three ports.  The port number is automaticaly detected when using the USB connection on the SC4-Hub board. Every Node on each port is checked for alerts, and any found alerts are cleared if possible.


Example-StatusAlerts.cpp - This is the main function for the program. It holds a reference to the system manager of the network and Port objects. The main program executes, then waits for the user to press a key in order to end the program.

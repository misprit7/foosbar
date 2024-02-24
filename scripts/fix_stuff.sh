#!/bin/sh

# Fix ethernet link up state
sudo ip link set enp34s0 up

# Fixes for misc. stuff
# None of these are the best way of doing this and there's definitely a proper permanent way, but whatever

# Fix usb kernel drivers
cd ~/dev/clearpath/ExarKernelDriver
sudo make
sudo ./install_drvr


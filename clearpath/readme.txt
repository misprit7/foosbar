--------------------------------------------
ClearPath-SC Linux Driver Installation Notes
--------------------------------------------
Revision: $Date: 3/02/2017$

-------------------------
WHAT'S IN THE PACKAGE
-------------------------
The three tar files included: 
	- USB kernel driver for the SC-HUB 
	- sFoundation class library driver with an Eclipse Neon project.
	- Examples

-------------------------
REQUIRED SETUPS
-------------------------
For all distributions, the running user must be a member of the "dialout" group to
access the serial port. The command:
	sudo usermod -aG dialout {user acct}
should add the desired user accounts to this group. Reboot after this command.

For Fedora, you may need to disable the SELinux extension or setup applications
and users to allow access to the serial port device. 

The simple disable step involves editing the /etc/selinux/config file and finding 
the line:
	SELINUX=xxxx
Change this from "enforcing" to "disabled", write the file out and reboot. To 
verify you have disabled this mode run the command:
	sestatus 
It should say it is disabled. You may have to sudo yourself to root to perform this.

The class library built using Eclipse creates a shared object file in the "Release"
directory. You must setup a symbolic link to this file named libsFoundation20.so.1
and setup LD_LIBRARY_PATH and/or use the ldconfig command to tell your
system where the link to your shared library lives. Alternatively, you can
link your client code with the -rpath flag to point it to the library directory.

A nice tutorial exists at: 
http://www.tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html

The Eclipse project includes debug and release versions for both local
compilation and cross-compilation for the BeagleBone Black (build configurations
DebugBeagle and ReleaseBeagle). To use the cross-compilation configurations, you
will need to install the GNU ARM Eclipse plugins (see Development Notes below).
For systems without Eclipse, the library can be built manually using g++.

You must copy MNuserDriver20.xml to the same directory as libsFoundation.so


-------------------------
LIMITATIONS
-------------------------
Standard PC hardware Linux distributions limit the serial port rate to 115200 baud.
This was the limit for classic PC hardware. With PCI serial ports, other embedded
processors or USB serial bridges this limit could be removed with vendor kernel 
drivers.

Any multiple of 9600 baud can be specified for operations with ClearPath-SC. Slower
rates could have an impact on the command rates and latency. Link saturation may
also occur at slower rates using the Attention interrupt feature.

The ClearPath-SC family supports link rates up to 230400 baud. This limit
is specified during system initialization. See the documentation on the 
SysManager::ComHubPort function for details. The examples all assume 115200 rate
(MN_BAUD_12X). MN_BAUD_24X would specify 230400 rate.

Native serial ports typically have the best performance, especially if setup for
low latency. The linux command to change this is:
	setserial /dev/ttyX low_latency 
This may improve the performance at the expense of extra system overhead.

-------------------------
DEVELOPMENT NOTES
-------------------------
This package was developed using Eclipse Neon.2 for Linux 64-bit. The coding depends 
on C++ 11 features. If you are using other code generation tools, ensure that 
you set the dialect properly.

This package was tested on Ubuntu 16.04 LTS and Fedora 25 for x86-AMD64.

Cross compilation (ARM BeagleBone) was tested with the GNU Arm Eclipse Plugins at
    http://gnuarmeclipse.github.io/plugins/install/

Teknic support is limited to the original distribution as-is. If the library
is modified from this distribution, we will have a limited ability to help
you.

-------------------------
USING THE SC-HUB WITH USB
-------------------------
If USB operations with the SC-HUB are desired, make sure you have the kernel build
symbols included with your distribution. On systems with apt (e.g. Ubuntu, Debian),
you can install the requisite headers with the following command:
    sudo apt-get install linux-headers-$(uname -r)

If this exists, go to the KernelDriver folder and type: 
	make
If your system is properly setup, the KO module will be build. 

Type:
	sudo ./install_drvr 
to install this in the running kernel. You may need to unplug and
re-plug the SC-HUB to get Linux to recognize this device. If you type 
	ls /dev/ttyXRUSB*
you should see the if the device driver loaded and is ready.

Alternately you can build a kernel with this already installed. The scope of these
steps is beyond this readme.

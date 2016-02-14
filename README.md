SidPi v1
==========

A Raspberry Pi driver for the MOS 6581 SID chip used in the Commodore 64.  There are three components to the driver, a kernel driver that uses the same protocol as the hardsid driver.  A network listener that implements the netsid protocol used by ACID64, and a proxy is required to forward requests from a PC running ACID64 to a Raspberry Pi where the listener is running.

The GPIO connections and schematic can be found in the schematic folder which includes a simple implementation of the hardware required.  Any implementation of this design is at your own risk !  Do not blame me for any blown up Pi's or SID's or anything for that matter.

Build instructions to follow as I'm trying to automate it all, however if you wish to know before that then feel free to contact me at papawattu@gmail.com.

1) module -Raspberry Pi Kernel Module

You'll need to have the RPi kernel source headers which match your kernel version and then create a softlink /lib/modules/<kernelversion>/build to the root of the kernel source.  

Perform a make in the kernel source dir to make the module.

Then just do an insmod sidpi.ko and a dmesg to see if the driver has loaded.  You will also need to mknod /dev/sid0 c 248 0 to create the device.  At that point you should be able to use sidplay2 with the --hardsid parameter to play some rockin Sid tunes.

14/2/2016
The Module is still very much under development is broken at the moment.

2) sidpiserver - Network listener for ACID 64 - listens by default on port 6581

To build the server in the sidpiserver src directory

gcc -pthread -o sidpiserver sidpiserver.c sidpithread.c rpi.c fifo.c

To setup a proxy to forward the ACID64 traffic you can use (on Windows)..

netsh interface portproxy add v4tov4 listenport=6581 listenaddress=127.0.0.1 connectport=6581 connectaddress=<IP of your PI>

You can order the PCB from here https://oshpark.com/shared_projects/vRw7mWuD

More to come soon!

UPDATE 14/2/15

Changes to support Pi2 and some timing fixes.

UPDATE 27/4/15

The driver does not work with RP2 - Looking into this now


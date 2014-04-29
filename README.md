SidPi v0.1
==========

A Raspberry Pi driver for the MOS 6581 SID chip used in the Commodore 64.  There are three components to the driver, a kernel driver that uses the same protocol as the hardsid driver.  A network listener that implements the netsid protocol used by ACID64, and a simple proxy server that forwards requests from a PC running ACID64 to a Raspberry Pi where the listener is running.

The GPIO connections and schematic can be found in the schematic folder which includes a simple implementation of the hardware required.  Any implementation of this design is at your own risk !  Do not blame me for any blown up Pi's or SID's or anything for that matter.

Build instructions to follow as I'm trying to automate it all, however if you wish to know before that then feel free to contact me at jamie@wattu.com.

1) module -Raspberry Pi Kernel Module

You'll need to have the RPi kernel source and create a softlink /lib/modules/<kernelversion>/build to the root of the kernel source.  Then just do an insmod sidpi.ko and a dmesg to see if the driver has loaded.  You will also need to mknod /dev/sid0 c 248 0 to create the device.  At that point you should be able to use sidplay2 with the --hardsid parameter to play some rockin Sid tunes.

2) sidpiserver - Network listener for ACID 64 - listens by default on port 6581

To build the server just gcc -o sidpiserver sidpiserver.c rpi.c.

3) proxy -Simple proxy server that sends ACID64 commands to a Pi running the sidpiserver.

The proxy is there to forward the ACID64 traffic to the RPi.  Its pretty noddy at the moment and you'll need to change the IP address in the code to the IP of your RPi.  

Compile and run with your favourite Java IDE. 

More to come soon!

SidPi v0.1
==========

A Raspberry Pi driver for the MOS 6581 SID chip used in the Commodore 64.  There are three components to the driver, a kernel driver that uses the same protocol as the hardsid driver.  A network listener that implements the netsid protocol used by ACID64, and a simple proxy server that forwards requests from a PC running ACID64 to a Raspberry Pi where the listener is running.

The GPIO connections and schematic can be found in the schematic folder which includes a simple implementation of the hardware required.  Any implementation of this design is at your own risk !  Do not blame me for any blown up Pi's or SID's or anything for that matter.

Build instructions to follow as I'm trying to automate it all, however if you wish to know before that then feel free to contact me at jamie@wattu.com.

module
======

Raspberry Pi Kernel Module

sidpiserver
===========

Network listener for ACID 64 - listens by default on port 6581

proxy
=====

Simple proxy server that sends ACID64 commands to a Pi running the sidpiserver.

More to come soon!

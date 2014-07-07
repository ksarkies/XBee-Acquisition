XBee Data Acquisition
---------------------

This project develops a remote data collection board, primarily aimed at, but
not limited to, a water flow meter, that communicates collected data over an
XBee network to a master processing point. The remote unit includes a bootloader
for remote upload of new firmware.

The challenge with this project is to reduce power consumption to a minimum
to extend the life of the batteries, which may be difficult to access for
recharge.

In addition to the board design and firmware, a number of tools are provided
to manage the network and collect data using Linux based hardware. These
include an acquisition control process that is intended to run on a headless
Linux box. This manages the network and connects to a user interface either
on the same machine or separately. It therefore uses a TCP/IP sockets interface.

Subdirectories:

gEDA-XBee                   Schematic and PCB for remote unit
XBee-acqcontrol             Control of XBee network for Linux hardware
XBee-Arduino                Firmware for Arduino remote unit testbench
XBee-bootloader             Bootloader for remote unit
XBee-firmware               Firmware for remote unit
XBee-GUI                    Linux PC based GUI for network control
XBee-utility                Some tools for direct interaction with XBee

K. Sarkies
7 July 2014


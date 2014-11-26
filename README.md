XBee Data Acquisition
---------------------

This project develops a remote data collection board, primarily aimed at a
specific water flow meter. The board communicates collected data over
an XBee network to a master processing point. The remote unit includes a
bootloader for remote upload of new firmware.

The challenge with this project is to reduce power consumption to a minimum
to extend the life of the batteries, which may be difficult to access for
recharge.

In addition to the board design and firmware, a number of tools are provided
to manage the network and collect data using Linux based hardware. These
include an acquisition control process that is intended to run on a headless
Linux box. This manages the network and connects to a user interface either
on the same machine or separately. It therefore uses a TCP/IP sockets interface.

Subdirectories:

&bull; gEDA-XBee                   Schematic and PCB for remote unit

&bull; Kicad-Main                  Schematic and PCB for watermeter unit

&bull; Kicad-Mounts                Schematic and PCB for watermeter mounts

&bull; XBee-acqcontrol             Control of XBee network for Linux hardware

&bull; XBee-bootloader-M168        Bootloader for remote unit ATMega168

&bull; XBee-bootloader-T4313       Bootloader for remote unit ATTiny4313

&bull; XBee-firmware               Firmware for remote unit ATTiny4313

&bull; XBee-GUI                    Linux PC based GUI for network control

&bull; XBee-node-example-M168      Example to send dummy message ATMega168

&bull; XBee-utility                Some tools for direct interaction with XBee

K. Sarkies
20 September 2014


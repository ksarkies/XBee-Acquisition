XBee Data Acquisition
---------------------

This project develops a remote data collection board, primarily aimed at a
specific water flow meter. The board communicates collected counts of optical
transitions from a rotating slotted wheel, over an XBee network to a master
processing point. The remote unit includes a bootloader for remote upload of
new firmware.

The challenge with this project is to reduce power consumption to a minimum
to extend the life of the batteries, which may be difficult to access for
recharge in practice.

In addition to the board design and firmware, a number of tools are provided
to manage the network and collect data using Linux based hardware. These include
an acquisition coordination process that is intended to run on a small headless
Linux box such as a Raspberry Pi or BeagleBone. This manages the network and
connects to a user interface program running either on the same machine or
at another location. It uses a TCP/IP sockets interface that will allow remote
connections over the Internet.

A number of test and debug firmwares are available as well as schematics for
prototyping boards used as test nodes.

Subdirectories:

* **Documentation**:           A set of documents relevant to design and implementation.

* **gEDA-XBee-Test**:          Schematics for prototype boards for testing purposes.

* **Kicad-Main**:              Schematic and PCB for watermeter unit main board.

* **Kicad-Mounts**:            Schematic and PCB for XBee and LED mount boards.

* **watermeter-test**:         Firmware for the remote unit ATTiny481, in C, for use
 with flow bench testing over a serial interface. XBee is not used.

* **XBee-acqcontrol**:         Control software for the coordinator; in C++.

* **XBee-bootloader-M168**:    Bootloader for a remote unit with ATMega168.

* **XBee-experimental-T4313**: Bootloader for a remote unit with ATTiny4313.

* **XBee-firmware**:           Firmware for the remote unit ATTiny481, in C.

* **XBee-firmware-NARTOS**:    Firmware for remote unit ATTiny481 using the NARTOS
scheduler.

* **XBee-GUI**:                Linux PC based GUI for network control; C++ and QT.

* **XBee-node-example-M168**:  Example to send dummy message via XBee for testing.

* **XBee-utility**:            Some tools for direct interaction with XBee over a
serial interface.

K. Sarkies
29 January 2016


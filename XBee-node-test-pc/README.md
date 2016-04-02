XBee Data Acquisition Remote Test Code
--------------------------------------

This is a POSIX system based test code for a XBee remote unit. Written in C but
embedded in a QT/C++ wrapper. The purpose is to provide a testbed for remote
system firmware.

It is identical to the [xbee-node-test](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-node-test) code but is intended to run on a POSIX
system using a serial interface to an XBee.

The program can be run without a GUI from the command line with the -n option.
A debug mode is available for extended printout.

xbee-node-test -d -P /dev/ttyUSB0 -b 38400 -n

In command line mode, the program can only be stopped by ctl-C or process kill.

K. Sarkies
2 April 2016


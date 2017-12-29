XBee Data Acquisition Remote Test Code Qt version
-------------------------------------------------

This test code is written in C and embedded in a QT/C++ wrapper.

The program can be run without a GUI from the command line with the -n option.
A debug mode is available for extended printout.

$ ./xbee-node-test -d -P /dev/ttyUSB0 -b 38400 -n

Where the number at the end is the baud rate of the XBee serial interface.
Change this as needed.

In command line mode, the program can only be stopped by ctl-C or process kill.

Change the XBee-node-test.pro file to compile the different main program modules.

K. Sarkies
6 February 2017


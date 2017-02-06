XBee Data Acquisition Remote Test Code C version
-------------------------------------------------

This test code is written in C.

The program is run from the command line. A debug mode is available for
extended printout.

$ xbee-node-test -d -P /dev/ttyUSB0 -b 38400

The program can only be stopped by ctl-C or process kill.

Change the CMakeLists.txt file to compile the different main program modules.
The makefile is then created from within the directory using:

$ cmake .

K. Sarkies
6 February 2017


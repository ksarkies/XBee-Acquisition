XBee Data Acquisition Remote Test Code
--------------------------------------

This is a POSIX system based test code for a XBee remote unit. Written in C but
embedded in a QT/C++ wrapper. The purpose is to provide a testbed for remote
system firmware that is capable of communicating with an XBee and working as a
remote unit.

It is identical to the [xbee-node-test](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-node-test) code but is intended to run on a POSIX
system using a serial interface to an XBee.

The program can be run without a GUI from the command line with the -n option.
A debug mode is available for extended printout.

$ xbee-node-test -d -P /dev/ttyUSB0 -b 38400 -n

In command line mode, the program can only be stopped by ctl-C or process kill.

***ADAPTATION TO OTHER TEST CODE***
-----------------------------------

The following files may be used as a wrapper for other code tests with
appropriate adaption of the .pro and mainprog.cpp files:

* xbee-node-test-main.cpp
* xbee-node-test.cpp
* xbee-node-test.h
* xbee-node-test.pro
* xbee-node-test.ui
* serial.cpp

serial.cpp is a substitute set of communication functions calling on POSIX/QT
serial I/O.

mainprog.cpp must be provided with the code to be tested in the following way:

Place the code to be tested into the function called mainprog(), which is called
from the xbee-node-test.cpp wrapper. There should be no processor specific
statements. Any such should be replaced by a call to a local function that
contains code to emulate the statement. If this is not possible then the code is
not suitable for testing in this way.

Add all global variables as static. Add also any include files needed. These
should not contain references to any processor specific libraries or functions.

ISRs need to be changed to a function call and means to activate them devised.
For the timer in this example, an added call in the main loop to a function to
tick off millisecond counts is sufficient.

xbee.cpp is a file associated with the code under test, which has been copied
from the library directory with its extension changed from c to cpp to satisfy
the g++ compiler (otherwise it is not correctly linked).

K. Sarkies
3 April 2016


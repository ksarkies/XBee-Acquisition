XBee Data Acquisition Remote Test Code
--------------------------------------

This is a POSIX system based test code for a XBee remote unit. Written in C but
embedded in a QT/C++ wrapper which emulates the environment for the test code.
The purpose is to provide a testbed for remote system firmware that is capable
of communicating with an XBee and working as a remote unit. The emulator is not
an AVR emulator as such. Rather it provides a means to run the Xbee firmware
core on a PC for debugging.

NOTE: gcc required to provide function override for timerISR.

The program can be run without a GUI from the command line with the -n option.
A debug mode is available for extended printout.

$ xbee-node-test -d -P /dev/ttyUSB0 -b 38400 -n

In command line mode, the program can only be stopped by ctl-C or process kill.

The following main program modules allow different tests to be run. Change the
XBee-node-test.pro file to compile the different tests.

***mainprog-test***
-------------------

This is taken from the [xbee-node-test](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-node-test) code.

***mainprog-firmware***
-----------------------

This is taken from the [xbee-firmware](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-firmware) code.

***ADAPTATION TO OTHER TEST CODE***
-----------------------------------

The following files may be used as the environment emulator for other code tests
with appropriate adaption of the .pro and mainprog.cpp files:

* xbee-node-test-main.cpp
* xbee-node-test.cpp
* xbee-node-test.h
* xbee-node-test.pro
* xbee-node-test.ui
* serial.cpp

serial.cpp is a substitute set of communication functions calling on POSIX/QT
serial I/O. This should contain functions that match the calls in the test code.

mainprog.cpp must be provided with the code to be tested in the following way:

Split the code to be tested into an initialization part and an operational
part that normally falls within an infinite loop. Place the initialization part
into the function called mainprogInit(), and the operational part into
mainprog(). Both these are called from the xbee-node-test.cpp wrapper, with the
operational part having its loop emulated in the wrapper code.

There should be no compilable processor specific statements. These can be
replaced by a call to a local function that should contain code to emulate the
statement. If this is not possible then the code is not suitable for testing in
this way.

Add all global variables as static. Add also any include files needed. These
should not contain references to any processor specific libraries or functions.

ISRs need to be changed to a function call and means to activate them devised.
A timer is emulated in the emulator code. Timer initialization is done by
calling an initialization function in the emulator code. This function
sets the number of one millisecond tick counts at which the timer fires and
calls the ISR. If a timer ISR is not defined, a null ISR is substituted. The
timer ISR can be defined in the test code by replacing the actual ISR call with
the function: timerISR().

xbee.cpp is a file associated with the code under test, which has been copied
from the library directory with its extension changed from c to cpp to satisfy
the g++ compiler (otherwise it is not correctly linked).

K. Sarkies
3 April 2016


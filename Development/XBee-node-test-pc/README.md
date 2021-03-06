XBee Data Acquisition Remote Test Code
--------------------------------------

This is a POSIX system based test code for an XBee remote unit. Written in C but
embedded in a wrapper that emulates the environment for the test code.
It can be run with any XBee adapter interface board that contains a USB-serial
converter, or with an XBee carrier board having a serial interface along with an
external USB-serial converter.

Both a QT5 based emulator and an emulator based on libc are provided, the latter
being command-line only, of course. It is intended for small headless machines
running Linux such as the Raspberry Pi or BeagleBone.

The purpose is to provide a PC Linux based testbed for emulating remote system
firmware that is capable of communicating directly with an XBee over a serial
port, and working as a remote unit. The emulator is not an AVR emulator. Rather
it provides a means to run the Xbee firmware core on a PC for debugging.

NOTE: gcc is required to allow function override for timerISR.

The program runs from the command line and is located in the emulator-c/ or
emulator-qt5/ directory. A debug mode is available for extended printout.

$ ./xbee-node-test -d -P /dev/ttyUSB0 -b 38400

Where the number at the end is the baud rate of the XBee serial interface.
Change this as needed.

In command line mode, the program can only be stopped by ctl-C or process kill.

The following main program modules allow different tests to be run. Follow the
instructions in the emulator directory README documentation to compile the
different tests.

***mainprog-test***
-------------------

This is taken from the [xbee-node-test](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-node-test) code.

***mainprog-firmware***
-----------------------

This is taken from the [xbee-firmware](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-firmware) code.

***ADAPTATION TO OTHER TEST CODE***
-----------------------------------

The following files may be used as the environment emulator for other code tests
with appropriate adaption of the makefiles and mainprog-*.cpp files. For QT5:

* xbee-node-test-main.cpp
* xbee-node-test.cpp
* xbee-node-test.h
* xbee-node-test.ui
* serial-libs.cpp
* xbee-libs.cpp
* xbee-node-test.pro - qmake definitions for generating makefile

For C:

* xbee-node-test.c
* xbee-node-test.h
* xbee-node-test.pro
* serial-libs.c
* CMakeLists.txt  - cmake definitions for generating makefile

**serial-libs.cpp** and **serial-libs.c** are substitute sets of communication
functions calling on POSIX/QT serial I/O. These should contain functions that
match the calls in the test code.

**xbee-libs.cpp** is a file associated with the code under test, which has been
copied from the library directory with its extension changed from c to cpp to
satisfy the g++ compiler (otherwise it is not correctly linked).

**xbee-lib.c** and **mainprog-*.c** are include wrappers used to force C
compilation on cpp source files.

**mainprog-*.cpp** are common to both emulators. They must be provided with the
code to be tested in the following way:

Split the code to be tested into an initialization part and an operational
part that normally falls within an infinite loop. Place the initialization part
into the function called mainprogInit(), and the operational part into
mainprog(). Both these are called from the xbee-node-test wrapper, with the
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

K. Sarkies
3 February 2017


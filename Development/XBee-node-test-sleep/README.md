XBee Data Acquisition Remote Firmware with Sleep Modes
------------------------------------------------------

This is a test firmware for a remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega48 series. The hardware is defined in
[gEDA-XBee](https://github.com/ksarkies/XBee-Acquisition/tree/master/Development/gEDA-XBee-Test) but is also useable with the watermeter board which uses the ATTiny841.

A message containing counts from an external pulse source is sent to the
coordinator at timed intervals in the timer ISR. The program will flash an
activity LED on the test board.

Before beginning to monitor the XBee for incoming messages the firmware will
query it for association indication. This is to ensure that the XBee is not put
to sleep before association has occurred. Rapid flashing of the activity LED at
0.5 second indicates that the communication with the XBee is taking place but
the response is indicating either an error or that association has not occurred.
A slower flashing of 1 second period indicates that communications not being
received and is timing out. If this is not resolved the firmware can remain in
the loop looking for association to be signalled. The XBee will then never be
put to sleep.

Once associated the XBee is put to sleep (if it is an END device) and the AVR is
also put to sleep. The code is essentially the same as that in
[XBee-node-test](https://github.com/ksarkies/XBee-Acquisition/tree/master/Development/XBee-node-test) with the exception that the watchdog timer is used in interrupt
mode to time the wakeup and transmission of data. When a timed message is to be
sent, the XBee will be woken and put back to sleep after the message is sent.

**NOTE:** CTS must be set in the XBee and USE_HARDWARE_FLOW also enabled.

Differences in registers, ports and ISRs between devices are incorporated into
the defines.h header file in the lib directory, which calls on separate
defines-xxx.h files.

Open source versions of avr-libc earlier than release 2.0.0 did not support the
ATTiny841 series. To get around this, either the latest version of avr-libc must
be compiled and installed. or the latest Atmel toolchain needs to be placed into
a directory somewhere where it can be referenced in the Makefile. Change the
DIRAVR environment variable in Makefile to suit the location chosen.

The makefile target is ATMega48 by default. To use another target eg ATTiny841.

$ make MCU=attiny841

K. Sarkies
14 January 2017


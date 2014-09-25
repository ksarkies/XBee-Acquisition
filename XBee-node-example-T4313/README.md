XBee Data Acquisition Remote Firmware Example
---------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATTiny4313. The hardware is defined in
gEDA-XBee/xbee-node-attiny4313.sch

A dummy message is sent to the coordinator at timed intervals and PORTB0 is
toggled in the timer ISR.

The code is written to permit use of other AVR microcontroller types that do
not have a separate bootloader section.

The MCU variable is passed to the source in various places to allow:

make MCU=xxx

where xxx is either attiny4313 or atmega48.

The makefile may need to be changed for other AVR types to account for
differences. A separate defines-xxx.h file may need to be incorporated.

Currently optimization level 1 only works with this code. Other optimization
levels create faulty code.

K. Sarkies
25 September 2014


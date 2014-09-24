XBee Data Acquisition Remote Firmware Example
---------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega168. The hardware is defined in
gEDA-XBee/xbee-node-atmega48.sch

A dummy message is sent to the coordinator at timed intervals and PORTB0 is
toggled in the timer ISR.

The code is written to permit use of other microcontroller types and currently
also supports ATTiny4313.

The MCU variable is passed to the source in various places to allow:

make MCU=xxx

where xxx is either attiny4313 or atmega168.

The makefile needs to be changed for other AVR types to adjust for differences.
A separate defines-xxx.h file needs to be incorporated.

Currently optimization level 1 only works with this code. Other optimization
levels create faulty code.

K. Sarkies
24 September 2014


XBee Data Acquisition Remote Firmware Example
---------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega168. The hardware is defined in
gEDA-XBee/xbee-node-atmega48.sch

A dummy message is sent to the coordinator at timed intervals.

The code is written to allow other microcontroller types and currently supports
ATTiny4313.

The MCU variable is passed to the source in various places to allow:

make MCU=xxx

where xxx is either attiny4313 or atmega168.

The defines.h and makefile need to be changed for other AVR types to adjust for
differences.

K. Sarkies
21 September 2014


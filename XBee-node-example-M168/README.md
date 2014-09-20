XBee Data Acquisition Remote Firmware Example
---------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega48/88/168/328 and other multiple UART
microcontrollers. The hardware is defined in gEDA-XBee/xbee-node-atmega48.sch

A dummy message is sent to the coordinator at timed intervals.

defines.h and makefile need to be changed to reflect differences in the
different microcontroller types.

K. Sarkies
20 September 2014


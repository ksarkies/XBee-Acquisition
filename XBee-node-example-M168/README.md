XBee Data Acquisition Remote Firmware Test Example
--------------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega168. The hardware is defined in
[gEDA-XBee](https://github.com/ksarkies/XBee-Acquisition/tree/master/gEDA-XBee).

A dummy message is sent to the coordinator at timed intervals and PORT PC4 is
toggled in the timer ISR. The program will flash an LED on the board.

The MCU variable is passed to the source in various places to allow compiling
for that MCU:

make MCU=xxx

where xxx is atmega48 or any of that series.

The makefile may need to be changed for other AVR types to account for
differences. A separate defines-xxx.h file may need to be incorporated.

K. Sarkies
9 March 2016


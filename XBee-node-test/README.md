XBee Data Acquisition Remote Test Firmware
------------------------------------------

This is a test firmware for a remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega48 series. The hardware is defined in
[gEDA-XBee](https://github.com/ksarkies/XBee-Acquisition/tree/master/gEDA-XBee) but is also useable with the watermeter board.

A message containing counts from an external pulse source is sent to the
coordinator at timed intervals in the timer ISR. The program will flash an LED
on the test board.

Differences in registers, ports and ISRs between devices are incorporated into
the defines.h header file in the lib directory, which calls on separate
defines-xxx.h files.

The watermeter board uses the ATTiny841 which is not supported by the open
source version of gcc-avr. The Atmel version of gcc-avr needs to be placed into
the an auxiliary directory and referenced in the Makefile. Change the DIRAVR
environment variable in Makefile to suit the location chosen.

The makefile target is ATMega48 by default. To use another target eg ATTiny841

$ make MCU=attiny841

K. Sarkies
9 March 2016


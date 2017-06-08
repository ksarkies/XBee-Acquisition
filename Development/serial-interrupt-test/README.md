Simple Test of Serial Interrupt Library
---------------------------------------

This is a test firmware for the serial library used in the XBee project.
Written in C under avr-gcc.

The firmware simply echoes characters sent to the serial port. Baud rate is set
to 38400 by default.

The makefile target is ATMega48 by default. To use another target eg ATTiny841.

$ make MCU=attiny841

K. Sarkies
8 June 2017


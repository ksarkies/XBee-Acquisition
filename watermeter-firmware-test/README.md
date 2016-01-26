Watermeter Firmware Test Version
--------------------------------

This is a test firmware for the AVR in the watermeter. Written in C under
avr-gcc.

This test version transmits the count of the photointerrupter at regular
intervals (1 sec) via the serial port in csv format for capture and analysis
off-line.

The Atmel version of gcc-avr needs to be placed into the auxiliary directory
and referenced in the Makefile as the open source version doesn't support
the ATTiny841.

The defines used are placed in the libs directory under each processor. The
makefile target is the ATMega168 by default. To use another target eg ATTiny841

$ make MCU=attiny841

The environment variable MCU_TYPE is passed to the source.

K. Sarkies
28 February 2015


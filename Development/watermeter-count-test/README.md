Watermeter Count Test
---------------------

This is a test firmware for the AVR in the watermeter. Written in C under
avr-gcc.

This test transmits the count of the photointerrupter at regular
intervals (1 sec) via the serial port at 9600 baud in csv format for capture
and analysis off-line. This link is normally connected to an XBee in the
watermeter assembly. The test can ONLY be used where the XBee is not present
and the serial link is brought out to a PC serial port.

Open source versions of gcc-avr earlier than release 2.0.0 did not support the
ATTiny841 series. To get around this, either a recent version of gcc-avr or the
Atmel version needs to be placed into a directory somewhere where it can be
referenced in the Makefile. Change the DIRAVR environment variable in Makefile
to suit the location chosen.

The defines used are placed in the libs directory under each processor. The
makefile target is the ATTiny841 by default. To use another target eg ATMega48

$ make MCU=atmega48

The environment variable MCU_TYPE is passed to the source.

K. Sarkies
7 December 2016


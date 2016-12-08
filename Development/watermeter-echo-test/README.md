Watermeter Echo Test
--------------------

This is a simple test firmware for the AVR in the watermeter. Written in C under
avr-gcc.

This test simply echoes characters back on the serial port at 9600 baud. Use
to verify that the serial link to the AVR is working. This link is normally
connected to an XBee in the watermeter assembly. The test can ONLY be used
where the XBee is not present and the serial link is brought out to a PC serial
port.

Open source versions of avr-libc earlier than release 2.0.0 did not support the
ATTiny841 series. To get around this, either the latest version of avr-libc must
be compiled and installed. or the latest Atmel toolchain needs to be placed into
a directory somewhere where it can be referenced in the Makefile. Change the
DIRAVR environment variable in Makefile to suit the location chosen.

The defines used are placed in the libs directory under each processor.
The makefile target is attiny841 by default. To use another target eg ATMega48.


$ make MCU=atmega48

The environment variable MCU_TYPE is passed to the source.

K. Sarkies
7 December 2016


XBee Data Acquisition Remote Bootloader
---------------------------------------

This provides a means to upload firmware updates through the XBee network to
a remote unit. Written in C under avr-gcc.

This bootloader works only with AVR microcontrollers that have a bootloader
block in high memory that can be entered on reset.

If the bootloader pin tests high the bootloader will load firmware to FLASH and
jump to the application start address 0x0000. If the bootload enable is low it
will simply jump to the application start address. The size of the bootload
block must be programmed into the fuse bits and the microcontroller configured
to jump to the bootloader start address. The start of the bootload block can be
found in the datasheets and must be set in the makefile along with the
microcontroller type. A bootload block size of 2048 bytes is needed, so the
fuse bits should be set to this value.

- ATMega168 with bootloader start address 0x3800. The serial port is set to
  38400 baud and requires hardware flow control set in the program (defines.h)
  and in the XBee.

The MCU variable is passed to the source in various places to allow:

make MCU=xxx BASEADDR=yyy

where xxx is atmega168 or another valid AVR type, and BASEADDR is the start
address of the bootloader.

K. Sarkies
25 September 2014


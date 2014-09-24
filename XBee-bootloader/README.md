XBee Data Acquisition Remote Bootloader
---------------------------------------

This provides a means to upload firmware updates through the XBee network to
a remote unit. Written in C under avr-gcc.

This bootloader is configured to work with AVR microcontrollers of two types,
those that have a bootloader block in high memory that can be entered on reset,
and those that do not.

In the former case, if the bootloader pin tests high the bootloader will load
firmware to FLASH and jump to the application start address 0x0000. If the
bootload enable is low it will simply jump to the application start address.
The size of the bootload block must be programmed into the fuse bits and the
microcontroller configured to jump to the bootloader start address. The start
of the bootload block can be found in the datasheets and must be set in
defines.h and in the makefile along with the microcontroller type. A bootload
block size of 2048 bytes is needed, so the fuse bits should be set to this
value.

In the latter case the bootloader is placed in high FLASH memory and requires
the application firmware to execute a jump to the bootloader if the bootloader
pin is active. When the firmware has been programmed, the bootloader jumps to
application start address, however the bootloader pin must be deactivated
before this jump occurs to ensure the application proceeds with execution.

This code supports two AVR types (other types are possible with appropriate
changes):

- ATMega168 with bootloader start address 0x3800. The serial port is set to
  38400 baud and requires hardware flow control set in the program (defines.h)
  and in the XBee.

- ATTiny4313 with bootloader start address 0x0800. The serial port is set to
  9600 baud and requires hardware flow control to be turned off in the program
  (defines.h) and in the XBee.

NOTE: The SPE (self programming enable) fuse bit must be set.

The MCU variable is passed to the source in various places to allow:

make MCU=xxx

where xxx is either attiny4313 or atmega168.

K. Sarkies
24 September 2014


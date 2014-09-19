XBee Data Acquisition Remote Bootloader
---------------------------------------

This provides a means to upload firmware updates through the XBee network to
a remote unit. Written in C under avr-gcc.

The bootloader must be programmed to the correct part of memory, which is
the uppermost portion. The starting address depends on the microcontroller type
and the size of the bootload block programmed into the fuse bits. The start of
the bootload block can be found in the datasheets and must be set in the
makefile along with the microcontroller type. A bootload block size of 2048
bytes is needed so the fuse bits should be set to this value. If a different
value is used it must be changed in defines.h.

K. Sarkies
20 September 2014


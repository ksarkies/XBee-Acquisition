XBee Data Acquisition Remote Bootloader
---------------------------------------

This provides a means to upload firmware updates through the XBee network to
a remote unit. Written in C under avr-gcc.

This bootloader is oriented to microcontrollers that do not have a bootloader
block in high memory that can be entered on reset. The bootloader is placed in
low memory and the firmware for this microcontroller must be placed above the
bootloader at a specified location. If a bootload enable I/O port tests high
it will load firmware and start execution at that location.

A bootload block size of 2048 bytes is needed. If a different value is used it
must be changed in defines.h.

K. Sarkies
20 September 2014


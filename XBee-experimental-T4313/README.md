XBee Data Acquisition Remote Firmware Experimental Bootloader Development
-------------------------------------------------------------------------

This is an test firmware for the remote unit. Written in C under avr-gcc.

This particular code example is identical to the node-example. It is used to
develop the bootloader interface for the ATTiny AVRs without a separate
bootloader section.

A dummy message is sent to the coordinator at timed intervals and PORTB0 is
toggled in the timer ISR.

The XBee is intended to not use hardware flow control, although RTS may be
possible. This is due to a lack of ports on the AVR package intended for use
(ATTiny841).

The AVR is set to 1MHz for low power, and a baud rate of 9600 to aimed at
giving time for processing messages.

No bootloader section is provided with many of the ATTiny AVRs, so it must
reside with the program that it will eventually overwrite. It is placed in a
section at the top of memory and is linked with the application program. A call
to the bootloader is made at the start following a test of the bootloader pin.

The node is transmitting the messages but the bootloader code is not working.

--------------------

The MCU variable is passed to the source in various places to allow:

make MCU=xxx

where xxx is either attiny841, attiny4313 or atmega48.

The makefile may need to be changed for other AVR types to account for
differences. A separate defines-xxx.h file may need to be incorporated.

K. Sarkies
12 November 2014

--------------------
                            BOOTLOADER NOTES

So far it has not been possible to trace reliably the action of the bootloader
past the point where incoming characters are received. There is also evidence
that the gcc compiler produces invalid code. Currently optimization level 1
only works with this code. Other optimization levels create faulty code.

The programming is unreliable and relies on the application code to effect a
jump to the bootloader. Thus if the programming fails and the application
is corrupted, the bootloader may never be re-entered.

An existing open source bootloader for these AVRS is TinySafeBoot:
http://jtxp.org/tech/tinysafeboot_en.htm

For future work, a possible solution may be to keep the first page of memory
unchanged and ensure that application ISRs are placed in fixed locations in
memory, possibly with just a call to the ISR code to allow flexibility. A small
main program at the start could call either the bootloader or the application
code at fixed locations according to the setting of the bootloader pin, so that
only the core application code can be changed by programming. Compilation must
ensure that all parts of the system remain in the correct memory relationship.

--------------------


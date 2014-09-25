XBee Data Acquisition Remote Firmware Example
---------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATTiny4313. The hardware is defined in
gEDA-XBee/xbee-node-attiny4313.sch

A dummy message is sent to the coordinator at timed intervals and PORTB0 is
toggled in the timer ISR.

The code is written to permit use of other AVR microcontroller types that do
not have a separate bootloader section.

The XBee is intended to not use hardware flow control, although RTS may be
possible. This is due to a lack of ports on the AVR package intended for use
(ATTiny841).

The AVR is set to 1MHz for low power, and a baud rate of 9600 to aimed at
giving time for processing messages.

No bootloader section is provided, so it must reside with the program that it
will eventually overwrite. It is placed in a section at the top of memory and
is linked with the application program. A call to the bootloader is made at
the start following a test of the bootloader pin.

The node is transmitting the messages but the bootloader code is not working.

--------------------
                            NOTES

So far it has not been possible to trace reliably the action of the bootloader
past the point where incoming characters are received. There is also evidence
that the gcc compiler produces invalid code. Currently optimization level 1
only works with this code. Other optimization levels create faulty code.

The programming is unreliable and relies on the application code to effect a
jump to the bootloader. Thus if the programming fails and the application
is corrupted, the bootloader may never be re-entered.

For future work, a possible solution is to keep the first page of memory
unchanged and ensure that application ISRs are placed in fixed locations in
memory, possibly with just a call to the ISR code to allow flexibility. A small
main program at the start could call either the bootloader or the application
code at fixed locations according to the setting of the bootloader pin, so that
only the core application code can be changed by programming. Compilation must
ensure that all parts of the system remain in the correct memory relationship.

--------------------

The MCU variable is passed to the source in various places to allow:

make MCU=xxx

where xxx is either attiny4313 or atmega48.

The makefile may need to be changed for other AVR types to account for
differences. A separate defines-xxx.h file may need to be incorporated.

K. Sarkies
25 September 2014


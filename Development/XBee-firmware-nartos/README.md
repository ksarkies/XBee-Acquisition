XBee Data Acquisition Remote Firmware using NARTOS Scheduler
------------------------------------------------------------

This is the firmware for the remote unit. Written in C under avr-gcc.
The NARTOS scheduler is used to manage the complexity of the logic.

The unit must use the lowest possible power drain, so the XBee is turned off
when not communicating, and the AVR is placed in power down sleep, to be woken
when a counter event occurs or at regular intervals for communication with the
base station.

Tasks are:

1. Manage sleep timing using the WDT.
2. Debounce and count pulses.
3. Transmit count and battery voltage.
4. Request any pending configuration instructions from the master.

The code uses an error checking protocol to ensure that data is transmitted
reliably. When woken by the WDT, it transmits the watermeter count and waits
for an acknowledgement with a checksum of its transmitted data. If no response
comes, or a response in error, or the base station reports an error, it will
repeat the transmission up to three times before giving up. Only when no errors
have been detected will it reset the watermeter count. The base station is
responsible for its own involvement in this protocol.

Options are provided for the base station to send commands to change
parameters in the remote unit. This must be sent BEFORE the acknowledgement
otherwise the remote unit will return to sleep once it has received a good
response.

The code is written to allow several AVR microcontroller types and currently
supports ATMega168, ATTiny4313 and ATTiny841, the latter being the one selected
for the watermeter.

Open source versions of avr-libc earlier than release 2.0.0 did not support the
ATTiny841 series. To get around this, either the latest version of avr-libc must
be compiled and installed. or the latest Atmel toolchain needs to be placed into
a directory somewhere where it can be referenced in the Makefile. Change the
DIRAVR environment variable in Makefile to suit the location chosen.

The XBee does not use hardware flow control, although RTS may be
possible. This is due to a lack of ports on the AVR package intended for use
(ATTiny841).

The AVR is set to 1MHz for low power, and a baud rate of 9600, aimed at
giving sufficient time for processing messages.

The MCU variable defaults to the attiny841 but can be passed to the makefile:

make MCU=xxx

where xxx is either attiny841, attiny4313 or atmega168.

The makefile may need to be changed for other AVR types to adjust for
differences, and a new defines file placed in the lib directory. The makefile
creates an integer variable MCU_TYPE, derived from the MCU variable, (see the
makefile) and passes it to the source files. This variable must then be used in
the defines.h header to incorporate the appropriate MCU specific defines file.

This code has been abandoned and is outdated and will need to be modified if the
scheduler approach is to be used. It needs modification before it will compile.

K. Sarkies
7 December 2016


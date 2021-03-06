XBee Data Acquisition Remote Firmware
-------------------------------------

This is the firmware for the remote unit. Written in C under avr-gcc.

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

Provision is made for the base station to send commands to change
parameters in the remote unit. This must be sent BEFORE the acknowledgement
otherwise the remote unit will return to sleep once it has received a good
response.

The code is written to allow several AVR microcontroller types and currently
supports ATMega168, ATTiny4313 and ATTiny841, the latter being the one selected
for the watermeter.

Open source versions of avr-libc earlier than release 2.0.0 did not support the
ATTiny841 series. To get around this, either compile and install the latest
version of avr-libc, or place the latest Atmel toolchain into a directory
somewhere where it can be referenced in the Makefile. Change the DIRAVR
environment variable in Makefile to suit the location chosen.

The XBee does not use hardware flow control, although RTS may be
possible. This is due to a lack of ports on the AVR package intended for use
(ATTiny841).

**NOTE (1):** CTS should be set in the XBee and USE_HARDWARE_FLOW also enabled when
using the ATMega48 series based test boards.

**NOTE (2):** The following ports on the XBee must be set as follows:
* For the watermeter board, set pullups as 457 and pins as:
pin 4 (DIO12) output high,
pin 7 (DIO11) output low,
pins 6 (DIO10), 11 (DIO4), 16 (DIO6), 17 (DIO3), 18 (DIO2) disabled with pullup,
pin 9 Sleep_RQ input without pullup,
pin 15 Associate output,
pin 19 (AD1) analogue input without pullup,
pin 20 Commissioning output.
* For the Node Test Board, set pullups as 453 and pins as:
pin 4 (DIO12) output high,
pin 7 (DIO11) output low,
pins 6 (DIO10), 11 (DIO4), 17 (DIO3) disabled with pullup,
pin 9 Sleep_RQ input without pullup,
pin 12 CTS output,
pin 15 Associate output,
pin 16 RTS input with pullup, 
pins 18, 19 analogue input without pullup,
pin 20 Commissioning output.

The AVR is set to 1MHz for low power, and a baud rate of 9600, aimed at
giving sufficient time for processing messages.

The MCU variable defaults to the attiny841 but other versions can be passed to
the makefile:

make MCU=xxx

where xxx is either attiny841, attiny4313 or atmega168.

The makefile may need to be changed for other AVR types to adjust for
differences, and a new defines file placed in the lib directory. The makefile
creates an integer variable MCU_TYPE, derived from the MCU variable, (see the
makefile) and passes it to the source files. This variable must then be used in
the defines.h header to incorporate the appropriate MCU specific defines file.

K. Sarkies
28 November 2014


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

The code is written to allow several AVR microcontroller types and currently
supports ATMega168, ATTiny4313 and ATTiny841, the latter selected for the
watermeter.

The MCU variable defaults to the atmega168 but can be passed to the makefile:

make MCU=xxx

where xxx is either attiny841, attiny4313 or atmega168.

The makefile may need to be changed for other AVR types to adjust for
differences, and a new defines file placed in the lib directory. The makefile
creates an integer variable MCU_TYPE, derived from the MCU variable, (see the
makefile) and passes it to the source files. This variable must then be used in
the defines.h header to incorporate the appropriate MCU specific defines file.

K. Sarkies
19 November 2014


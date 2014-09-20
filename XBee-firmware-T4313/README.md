XBee Data Acquisition Remote Firmware
-------------------------------------

This is the firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATTiny4313 and is defined in
gEDA-XBee/xbee-node-attiny4313.sch

Tasks are:

1. Manage sleep timing using the WDT.
2. Count pulses: ISR and debounce procedure.
3. Transmit count, time, duration and battery voltage.
4. Request any pending configuration instructions from the master.
5. Sleep.

defines.h and makefile need to be changed to reflect differences in the
different microcontroller types.

K. Sarkies
20 September 2014


XBee Data Acquisition Remote Firmware Example
---------------------------------------------

This is an example firmware for the remote unit. Written in C under avr-gcc.
The node hardware is based on the ATMega48/88/168 as given in
gEDA-XBee/xbee-node-atmega48.sch

Tasks are:

1. Count pulses: ISR and debounce procedure.
2. Detect end of count sequence.
3. Transmit count, time, duration and battery voltage.
4. Request any pending configuration instructions from the master.
5. Sleep.

A dummy message is sent to the coordinator at timed intervals.

K. Sarkies
7 July 2014


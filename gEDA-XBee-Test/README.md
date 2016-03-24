XBee Test Board PCB
-------------------

These are small test board PCBs.

### xbee-node-atmega48

This includes an XBee with ATMega48 and with an analogue battery monitor and
digital counter. It takes an input voltage 5V or more and produces 3.3V power
for the electronics.

The microcontroller has the following I/O pin allocations:

* PB2 (in) Bootloader Enable
* PB3 (out) Sleep Request
* PB4 (in) XBee Asleep (inv)
* PB5 (out) XBee Reset (inv)
* PC0 (in) Board Analogue Input
* PC1 (in) Battery Monitor Analogue Input
* PC4 (out) Microcontroller Activity LED
* PC5 (out) Battery Monitor Control
* PD5 (in) Board Digital Input

Three LEDs are provided:
* Green     PC4 Microcontroller Activity from ATMega48.
* Yellow    Association status from XBee.
* Red       Power.

The [firmware](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-node-example-M168) for ATMega48 series is provided for test transmissions.

### xbee-node-attiny4313

This board is similar to the above but analogue input is taken directly to the
XBee AD1 input pin as the ATTiny4313 does not have analogue capability.

### xbee-node-serial

This board is intended for use with End devices. It provides an external serial
output at RS-232 levels and a 6V or more power input via on-board regulators.

Four LEDs are provided:
* Green     On/Sleep.
* Orange    Sleep_RQ status.
* Red       Power.
* Yellow    Association.

Three switches are also provided:

* Sleep_RQ toggle (located on the prototype board against the LEDs). When the
XBee is asleep, toggle high then low to wake it to a new round of wake time.
* Commissioning momentary-on button (located on the prototype board in front of
the XBee).
* Reset momentary-on button (located on the prototype board near the XBee pin 1).

K. Sarkies
20 March 2016


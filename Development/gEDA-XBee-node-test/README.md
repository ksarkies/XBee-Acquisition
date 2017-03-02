XBee Test Board PCB
-------------------

These are small test board PCBs. They were built as prototypes. The serial
board is still in use but the others have been obsoleted, replaced by the
PCB defined under Kicad-XBee-node-test. 

### xbee-node-serial

This board is intended for use with End devices and connects serially to a PC
for purposes of extensive debug. It provides an external serial output at RS-232
levels and a 6V or more power input via on-board regulators.

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

### xbee-node-atmega48 (obsolete)

This includes an XBee with ATMega48 and with an analogue battery monitor and
digital counter. It takes an input voltage 5V or more and produces 3.3V power
for the electronics.

The microcontroller has the following I/O pin allocations:

* PB2 (in) Bootloader Enable taken from XBee DIO11 pin..
* PB3 (out) XBee Sleep Request.
* PB4 (in) XBee On/Sleep.
* PB5 (out) XBee Reset (inv) taken to the XBee reset pin.
* PC0 (in) Board Analogue Input also taken to XBee AD2 pin.
* PC1 (in) Battery Monitor Analogue Input also taken to XBee AD1 pin.
* PC4 (out) Microcontroller Activity LED.
* PC5 (out) Battery Monitor Control.
* PD2 (in) taken from XBee CTS pin.
* PD5 (in) Board Digital Input.
* Microcontroller Reset taken from XBee DIO12 pin.

Three LEDs are provided:
* Green     PC4 Microcontroller Activity (test port) from ATMega48.
* Yellow    Association status from XBee.
* Red       Power.

The [firmware](https://github.com/ksarkies/XBee-Acquisition/tree/master/Development/XBee-node-test) for ATMega48 series is provided for test transmissions.

### xbee-node-attiny4313 (obsolete)

This board is similar to the above but analogue input is taken directly to the
XBee AD1 input pin as the ATTiny4313 does not have analogue capability. The
Green LED is PB0 Microcontroller Activity (test port).

* PB4 (in) Bootloader Enable taken from XBee DIO11 pin..
* PB3 (out) XBee Sleep Request.
* PB1 (in) XBee On/Sleep.
* PB2 (out) XBee Reset (inv) taken to the XBee reset pin.
* PB0 (out) Microcontroller Activity LED.
* PD5 (out) Battery Monitor Control.
* PA1 (in) taken from XBee CTS pin.
* PD2 (in) Board Digital Input.
* Microcontroller Reset taken from XBee DIO12 pin.

The board requires 3.3V power input.

K. Sarkies
20 March 2016


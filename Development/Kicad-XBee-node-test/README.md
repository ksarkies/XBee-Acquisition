XBee Node Test Board PCB
------------------------

This is a small test board PCB.

This includes an XBee with ATMega48 and with an analogue battery monitor,
digital counter input and a second analogue input. It takes an input 5-14V power
and produces 3.3V power for the electronics.

The microcontroller has the following I/O pin allocations:

* PB2 (in) Bootloader Enable taken from XBee DIO11 pin..
* PB3 (out) Programming SPI interface MOSI.
* PB4 (in) Programming SPI interface MISO.
* PB5 (in) Programming SPI interface SCK.
* PC0 (in) Board Analogue Input also taken to XBee AD2 pin.
* PC1 (in) Battery Monitor Analogue Input also taken to XBee AD1 pin.
* PC3 (out) Debug Signal LED.
* PC4 (out) Microcontroller Activity LED.
* PC5 (out) Battery Monitor Control.
* PD2 (in) taken from XBee CTS pin.
* PD3 (out) taken to XBee RTS pin.
* PD4 (in) Board Digital Input.
* PD5 (out) XBee Sleep Request.
* PD6 (in) XBee On/Sleep.
* PD7 (out) XBee Reset (inv) taken to the XBee reset pin.
* Microcontroller Reset taken from XBee DIO12 pin.

Four LEDs are provided:
* Red       Power.
* Green     PC4 Microcontroller Activity (test port) from ATMega48.
* Blue      Debug Signal.
* Yellow    Association status from XBee.

The [node test firmware](https://github.com/ksarkies/XBee-Acquisition/tree/master/Development/XBee-node-test) is provided for test transmissions.

The [firmware](https://github.com/ksarkies/XBee-Acquisition/tree/master/XBee-firmware) is provided for full protocol transmissions.

Firmware must be compiled for MCU=atmega48 or whichever processor is used.

The firmware README provides instructions for setting up the XBee configuration.

K. Sarkies
2 March 2017


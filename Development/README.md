Development Modules
-------------------

All code and PCB modules used for testing at various points in the development
process is provided here. Some of these will become parts of a testbed if
production proceeds.

1. **gEDA-XBee-Test**. Circuit board schematics for test versions of XBee remote nodes.

2. **socket-test**. Something to do with TCP/IP sockets for the external interface
   to the base station.

3. **watermeter-count-test**. Transmits the count at 1 sec intervals via the
   serial port in csv format. XBee not used.

4. **watermeter-echo-test**. Basic test echoes characters back on the serial port.

5. **XBee-bootloader-M168**. Attempt at bootloader for ATMega168 series.

6. **XBee-experimental-T4313**. Attempt at bootloader for ATTiny4313 plus early
   version of XBee code as for the XBee-node-test.

7. **XBee-firmware-nartos**. Early attempt to use a tiny scheduler for managing the
   communication protocol.

8. **XBee-node-test**. A test firmware for the ATTiny841/ATMega168 to send counts via the XBee
   at timed intervals.

9. **XBee-node-test-pc**. This is POSIX software to allow testing of firmware core
   behaviour (without any microcontroller specific hardware operations) aimed
   at verification of communications protocol in the final watermeter firmware.

10. **XBee-node-test-sleep**. A test firmware for the ATTiny841/ATMega168 to send counts via the XBee
   at timed intervals using sleep modes in the AVR.

K. Sarkies
19 April 2015


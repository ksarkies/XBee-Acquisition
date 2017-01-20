XBee Data Acquisition TestBoard
-------------------------------

This board is for use with the watermeter assembly. It provides power, an
interface to an AVR programmer, and a serial interface to an external PC.
LEDs are provided for association indication, on/sleep and commissioning
signals from the XBee. A reset switch for the AVR is provided.

There are pins for a CRO to view the count signal from the assembly.

Jumpers are provided as follows:

* Disconnect external serial line. This **must** be left open when the XBee is
present in the watermeter assembly otherwise both serial sources are
mutually interfering and the XBee could be damaged.

* Disconnect power to watermeter assembly. This **must** be left open when the
watermeter assembly has an on-board battery. With Lithium batteries there is a
strong chance of explosion or fire if an external voltage source is connected.

K. Sarkies
19 January 2017


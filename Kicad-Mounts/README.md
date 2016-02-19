XBee Data Acquisition XBee/LED Mountings
----------------------------------------

Refer to [Kicad-setup](https://github.com/ksarkies/XBee-Acquisition/blob/master/Documentation/Kicad-Setup.md) in the Documentation directory for usage information.

TDB: At this time the PCB has missing footprints.

This board provides a mounting PCB for the LED and a mounting PCB for the XBee
with matching connector for right-angle mount to the main PCB.

XBee footprint is taken from Ian McInerney's library:
https://github.com/imciner2/KiCad-Libraries RF_OEM_PARTS.lib and footprints.mod

The LED mount provides for two LEDs VSMY2850 and SFH4056 reverse mounted (under
the board and facing through a cutout) and a top mounted LED SFH4258.

Other symbols and footprints are provided by KiCad developers.

27 November 2014: Flip XBee over, adjust XBee footprint, reduce size the of the
                  two boards, interchange Rx and Tx.

A separate regulated supply was added to provide 3.3V power for the XBee as the
XBee can pull a relatively heavy current pulse on transmission.

K. Sarkies
19 February 2016


XBee Data Acquisition PCB
-------------------------

Refer to [Kicad-setup](https://github.com/ksarkies/XBee-Acquisition/blob/master/Documentation/Kicad-Setup.md) in the Documentation directory for usage information.

PCB for a remote unit, including XBee with battery monitor and photodiode
counter. A major requirement for this circuit is ultra-low power to allow the
selected batteries to last for over a year.

The PCB is intended to fit inside a cylindrical watermeter with the batteries.
Due to space constraints the XBee is mounted on a separate board that sits at
right angles to the PCB. The XBee connects to an external aerial through an RF
cable. For measurement of battery voltage, a switched divider circuit is
provided for the case that the batteries provide a higher voltage than the
circuit power supply (which is sourced via a 3.3V voltage regulator). The LED is
mounted on a separate board that will straddle an interrupter wheel for counting
revolutions of the meter.

The board has a 10-pin debug connector for testing purposes:

1. Battery power or board power (if a battery is present take extreme care not
to short or connect any other power source).
2. Count signal from board comparator.
3. Reset for microcontroller.
4. MOSI microcontroller programming pin.
5. SCK microcontroller programming pin.
6. MISO microcontroller programming pin.
7. Tx from microcontroller to XBee Rx (monitor this only unless XBee is removed).
8. Rx to microcontroller from XBee Tx (monitor this only unless XBee is removed).
9. On/Sleep (high if XBee is awake, low if asleep).
10. Ground.

The TPS77033 symbol from KB1LQC.lib at Rochester Institute of technology
(http://edge.rit.edu/edge/P13271/public/KiCad/Libraries).

A photodiode symbol was taken from
(http://vkoeppel.free.fr/files/diy/kicad_diy_libraries.zip)

Other symbols and footprints except one are provided by KiCad developers.

The provided Photodiode.pretty footprint library should contain all necessary
footprints. Place any legacy footprint mod files into the modules directory. In
CvPcb go to "Library Tables", then select the tab "Project Specific Libraries".
Append a new entry, add the library path (including the name of the .mod file),
choosing "Legacy" for the plugin type. The nickname will then appear in the main
listing of CvPcb and the library footprints should appear. If a mistake is made,
no footprints will show, but no error message will be given until CvPcb is
restarted.

A custom footprint S1133 is provided for the photodiode. This actually has a
truncated circular form 8mm diameter with 6mm across the truncation, and through
hole pins. In CvPcb append a new KiCad entry and add the library path to the
.pretty directory (not the file, which is a single footprint). Also included
are some 2mm pin spacing header footprints.

* 27 November 2014: Add missing ground points. Correct error in battery voltage
  circuit.
* 24 February 2016: Changed position of connectors to avoid housing structures.

K. Sarkies
19 February 2016

Note; pin 1 of the XBee connector links to CTS on the XBee mount, but is not
connected on this board.



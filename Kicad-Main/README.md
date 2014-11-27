XBee Data Acquisition PCB
-------------------------

PCB for a remote unit, including XBee with battery monitor and photodiode
counter. A major requirement for this circuit is ultra-low power to allow the
selected batteries to last for over a year.

The PCB is intended to fit inside a cylindrical watermeter with the batteries.
Due to space constraints the XBee is mounted on a separate board that sits at
right angles to the PCB. The XBee connects to an external aerial through an RF
cable. For measurement of battery voltage, a switched divider circuit is
provided for the case that the batteries provide a higher voltage than the
circuit power supply (which is sourced via a voltage regulator). The LED is
mounted on a separate board that will straddle an interrupter wheel for counting
revolutions of the meter.

An XBee footprint was taken from Ian McInerney's library:
https://github.com/imciner2/KiCad-Libraries RF_OEM_PARTS.lib and footprints.mod
The legacy footprint has been modified to the newer kicad_mod format and solder
mask has been added to the pads.

The TPS77033 symbol from Rochester Institute of technology:
http://edge.rit.edu/edge/P13271/public/KiCad/Libraries KB1LQC.lib

A photodiode symbol was taken from:
http://vkoeppel.free.fr/files/diy/kicad_diy_libraries.zip

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

27 November 2014: Add missing ground points. Correct error in battery voltage
                  circuit.

K. Sarkies
27 November 2014


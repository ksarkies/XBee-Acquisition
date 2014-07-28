XBee Data Acquisition PCB
-------------------------

PCB for remote unit, including XBee with battery monitor and photodiode
counter.

The PCB is intended to fit inside a cylindrical watermeter along with batteries,
and so is designed to allow a number of different configurations, including
having the XBee mounted on a separate board. Because of the presence of
batteries the XBee aerial pattern may be severely distorted. A possibility of
having two batteries in parallel through Schottky diodes is provided. A
divider circuit for the battery voltage is also provided for the case that
the batteries provide a higher voltage than the circuit power supply (via
a voltage regulator). The photodiode circuit is mounted on a separate board
that will straddle an interrupter wheel for counting revolutions of the meter.

XBee and 5x2 connector symbol and footprint is from Ian McInerney:
https://github.com/imciner2/KiCad-Libraries RF_OEM_PARTS.lib and footprints.mod

TPS77033 symbol from Rochester Institute of technology:
http://edge.rit.edu/edge/P13271/public/KiCad/Libraries KB1LQC.lib and KB1LQC.mod

Both are legacy footprints. Place the files into modules directory. In CvPcb
go to "Library Tables", then select the tab "Project Specific Libraries". Append
a new entry, add the library path (including the name of the .mod file) and
choose "Legacy" for the plugin type. The nickname will then appear in the main
listing of CvPcb and the library footprints should appear. If a mistake is made,
no footprints will show, but no error message will be given until CvPcb is
restarted.

A custom footprint S1133 is provided for the photodiode. This actually has a
truncated circular form 8mm diameter with 6mm across the truncation, and through
hole pins. Due to height constraints it must be mounted through a matching hole
in the PCB and the pins bent and soldered to pads. In CvPcb append a new KiCad
entry and add the library path to the .pretty directory (not the file, which is
a single footprint).

K. Sarkies
24 July 2014


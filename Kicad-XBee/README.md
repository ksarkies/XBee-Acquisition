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

XBee symbol and footprint is from Ian McInerney:
https://github.com/imciner2/KiCad-Libraries RF_OEM_PARTS.lib and footprints.mod

TPS77033 symbol and footprint from Rochester Institute of technology:
http://edge.rit.edu/edge/P13271/public/KiCad/Libraries KB1LQC.lib and KB1LQC.mod

Both are legacy footprints. Place the files into a suitable location. In CvPcb
go to "Library Tables", then select the tab "Project Specific Libraries". Append
a new entry, add the library path and choose "Legacy" for the plugin type. The
nickname will then appear in the main listing of CvPcb and the library
footprints should appear. If a mistake is made, no footprints will show, but
no error message will be given until CvPcb is restarted.

K. Sarkies
24 July 2014


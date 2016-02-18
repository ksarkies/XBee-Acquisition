KiCad Installation and Setup
----------------------------

Refer to http://kicad-pcb.org/help/documentation/ for all instructions.

KiCad can be a bit (very) obtuse when it comes to setting up. Beware that
libraries on GitHub change from time to time.

INSTALL KICAD
-------------

Check with 'whereis kicad' to see if any bits are still around from previous
installs. Delete all these.

Install latest version of KiCad. Recommended that the latest stable version be
installed but the unstable versions may be OK. From the current documentation
(check that these are still valid) this installs the stable release 4:

$ sudo add-apt-repository --yes ppa:js-reynaud/kicad-4
$ sudo apt-get update
$ sudo apt-get install kicad

This installed 4.0.2-4+6225~38~ubuntu14.04.1

Starting KiCad directly sets up some default environment variables, but these
can be invalid depending on the history of how KiCad was installed in the past.
In Preferences/Configure Paths the default variables can be set.

The following script kicad-start will setup the environment variables needed to
access the existing libraries for cvpcb. Use this if the libraries aren't
correctly set by default. In cvpcb Preferences/Configure Paths the default
variables can be set. However these don't seem to be used, so the script is
likely to be essential.

	KIGITHUB=https://github.com/KiCad
	export KIGITHUB
	KISYSMOD=/usr/share/kicad/modules/
	export KISYSMOD
	kicad

SETUP CVPCB
-----------

This utility provides linking of schematic symbols with footprints. It must be
setup correctly to be able to access libraries.

The documentation states that a fp-lib-table file should be present in the home
directory, containing references to all footprint libraries. However this
doesn't now seem to be necessary, nor is it accessed. The location of the
library definitions is unknown.

Invoke from EESCHEMA in Tools/Assign Component Footprint.

If an error window appears, take note of all the mentioned libraries that were
"not found". Close the window and in cvpcb open Preferences/Footprint Libraries.
The window should have all the libraries that are defined. Delete those that
were not found (if they were on GitHub) or correct the library entry otherwise.
Other GitHub libraries can be added directly or via the wizard.

Any other entries may be deleted as desired if they are not needed. Note this
seems to be a setting for all projects.

PORTABILITY
-----------

The schematic file .sch, the board file .kicad_pcb, the project file .pro and
the netlist file .net, the schematic parts file .lib and the footprints file
.kicad_mod are all that is needed.



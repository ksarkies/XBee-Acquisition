KiCad Installation and Setup
----------------------------

GRUMBLE 17/2/2016. Rather regret having chosen KiCad EDA for this job. Although
it has an excellent user interface and is very comprehensive, the configuration
of KiCad can change over time and older files don't load correctly. There appear
to be configuration files hidden away that are not removed or replaced between
reinstalls, making the correction of problems difficult. The developers seem
too ready to make changes without warning that adversely affect users. Many
forum posts echo similar sentiments.

POLICY
------

The best way to use KiCad it seems is not to rely on the provided footprint
or symbol libraries, but to keep all footprints and symbols in local libraries,
stored with the job, that will remain static over time and between different
installations.

Refer to http://kicad-pcb.org/help/documentation/ for all instructions on setup.

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

This installed 4.0.2-4+6225~38~ubuntu14.04.1. Ubuntu has at this time a stable
version from 2013 that predates a major change to the way libraries are managed.

This is normally all that is needed for new installations. Download the KiCad
project files and open the .pro file in KiCad.

Starting KiCad directly sets up some default environment variables, but these
can be invalid depending on the history of how KiCad was installed in the past.
In Preferences/Configure Paths the default variables can be set.

The following script kicad-start will setup the environment variables needed to
access the existing libraries for CvPcb. Use this if the libraries aren't
correctly set by default (this may happen if past history has had configuration
problems). In CvPcb Preferences/Configure Paths the default variables can also
be set. Sometimes these don't seem to be used, so the script may be essential
in some cases.

	KIGITHUB=https://github.com/KiCad
	export KIGITHUB
	KISYSMOD=/usr/share/kicad/modules/
	export KISYSMOD
	kicad

SETUP CvPcb
-----------

This utility provides linking of schematic symbols with footprints. It must be
setup correctly to be able to access footprint libraries.

The documentation states that a fp-lib-table file should be present in the home
directory, containing references to all footprint libraries. However this
doesn't seem to be necessary during normal use. It may be accessed when CvPcb is
first started and is used to define the libraries. If not present some default
file will be created. The location of the working library definitions is
unknown.

Invoke from EESCHEMA in Tools/Assign Component Footprint.

If old GitHub libraries had been defined and do not match the existing GitHub
library names, then an error window appears. Take note of all the mentioned
libraries that were "not found". Close the window and in CvPcb open
Preferences/Footprint Libraries. The window should show all the libraries that
are defined. Delete those that were not found (if they were on GitHub) or
correct the library entry otherwise. Other GitHub libraries can be added
directly or via the wizard.

Any other entries may be deleted as desired if they are not needed.

On a new installation the wizard can be used to access the GitHub or locally
stored libraries. The GitHub libraries may be downloaded to a local directory
for use with the current project or for all projects. This avoids the need to
access Internet. Select Preferences/Footprint Libraries and select "Append with
Wizard". Remove any previous GitHub entries.

Footprints can be copied directly from the GitHub libraries to the local
library. If a PCB is provided, then the footprints in that file can be saved to
a local library. Right-click on each footprint, select the footprint menu and
select "Edit with Footprint Editor". It can then be edited and/or saved to the
desired local library.

PORTABILITY
-----------

The schematic file .sch, the board file .kicad_pcb, the project file .pro,
the netlist file .net, the schematic parts files .lib and the footprints
files .kicad_mod are all that is needed.


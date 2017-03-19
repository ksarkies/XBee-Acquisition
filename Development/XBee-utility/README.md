XBee Data Acquisition XBee AP Mode Utility
------------------------------------------

This is a GUI application written in C++ and using QT5. This allows user
interaction at a low level with an XBee. Assembles and sends API commands to an
XBee on a serial port (or USB-serial port via an adapter).

This was written when Digi's XCTU was behaving badly. The latest version of
XCTU is much more reliable. This code is left as legacy.

Download [libxbee](https://github.com/attie/libxbee3/archive/master.zip) zip file. Unpack to a suitable directory and
compile according to the included instructions. The latest version at this time
is 3.0.11. Install the libraries.

Under UBUNTU the packages QT5 and qt4-developer must be installed.

Then qmake and make the program. The executable is xbee-ap.

Refer to [Jiggerjuice](http://www.jiggerjuice.info/electronics/projects/XBee-network/xbee-data-acquisition.html) for more details on installation.

K. Sarkies
19 March 2017

CHANGES
-------
* 9 March 2016. Change to installed libxbee3 rather than explicit reference
to libraries. Change to QT5. The .pro file is now independent of the location of
all libraries.

* 19 March 2017. Add command line options.



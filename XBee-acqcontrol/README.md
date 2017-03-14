XBee Data Acquisition Control
-----------------------------

A background control process written in C. Its purpose is to manage the XBee
acquisition network and interface to an externally or locally running user
interface. This allows a small low-power on-site Linux machine to monitor the
system and communicate over LAN or WAN to a GUI running on a separate PC.

[Download libxbee](https://github.com/attie/libxbee3) from GitHub. This can be done with 'git clone' or downloading
the provided zip file.

The latest stable version at this time is 3.0.11. Compile libxbee according to
the included instructions and install. Then compile xbee-acqcontrol.

The program when run will announce the results filename and loop waiting for
communication from the network. The default filename will be time-stamped, but
this can be overriden by a command line option. Debug information is written to
/var/log/syslog. Optional command line options are:

-D _directory_ to store results file (default is /data/XBee/).

-P _port_ (default is /dev/ttyUSB0).

-b _baudrate_ (default 38400 baud).

-d debug mode causing printout of various actions. Same as -e 1

-e enhanced debug mode: level 0=none, 1=basic, 2=enhanced.

More information is available on [Jiggerjuice](http://www.jiggerjuice.info/electronics/projects/XBee-network/xbee-data-acquisition.html)

K. Sarkies
2 March 2016


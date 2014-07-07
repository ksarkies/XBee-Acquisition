AVR Serial Programmer GUI
=========================

Copyright 2007 Ken W. Sarkies

Target:    Linux
Author:    Ken Sarkies
Date:      6/4/2010
Licence:   GPL 2
gcc:       4.4.1
QT         4.5.3

The Serial Debug Tool is a general purpose tool for transmitting ASCII or binary
data to a target device over a serial link with a specified baudrate. Any
response is displayed in hexadecimal and ASCII form.

The Tool begins by opening the PC serial port ttyUSB0 for use with a Prolific
compatible USB-serial converter. This can be changed as necessary (for example
to ttyS0 if you are using a serial port) by editing the serialdebugtoolmain.cpp
file. The programmer communicates with the device using no parity, 1 stop bit
and 8 bit data.

Select the baudrate from the drop-down box. Then enter the string into the line
edit at the top of the window. Any printable character, including spaces, will
be transmitted as plain 8-bit ASCII except for a backslash. This is used as an
escape character. The two characters following will be interpreted as a hexadecimal
representation of an 8-bit binary number. If these two characters do not form a
valid hexadecimal number, the entire sequence of backslash and two characters will
be transmitted as ASCII. A backslash alone can be transmitted by escaping it with
another backslash.

A response from the target is displayed in two forms: a hexadecimal form and a
printable ASCII form. If the binary number does not form a printable ASCII
character, it is substituted by a dot. A maximum of 16 8-bit numbers can be
displayed.

K. Sarkies 6/4/2010


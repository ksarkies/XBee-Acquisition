PROJECT =       XBee Node test
TEMPLATE =      app
TARGET          += 
DEPENDPATH      += .
QT              += widgets
QT              += serialport

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_DIR          = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release

# Input
FORMS           += xbee-node-test.ui
HEADERS         += xbee-node-test.h ../../../libs/xbee.h ../../../libs/serial.h
SOURCES         += xbee-node-test-main.cpp xbee-node-test.cpp
SOURCES         += serial-libs.cpp ../xbee-libs.cpp
SOURCES         += ../mainprog-firmware.cpp


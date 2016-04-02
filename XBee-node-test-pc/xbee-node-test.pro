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
HEADERS         += xbee-node-test.h
SOURCES         += xbee-node-test-main.cpp xbee-node-test.cpp


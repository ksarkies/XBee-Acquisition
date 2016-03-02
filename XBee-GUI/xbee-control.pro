PROJECT =       XBee Control and Display GUI Tool
TEMPLATE =      app
TARGET          += 
DEPENDPATH      += .
QT              += widgets

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_DIR          = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release
QT              += network

# Input
FORMS           += xbee-control.ui xbee-dialog.ui
HEADERS         += xbee-control.h xbee-dialog.h
SOURCES         += xbee-control-main.cpp xbee-control.cpp xbee-dialog.cpp


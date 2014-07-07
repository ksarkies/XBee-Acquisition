PROJECT =       XBee AP Mode Tool
TEMPLATE =      app
TARGET          += 
DEPENDPATH      += .
include(../../../qextserialport/src/qextserialport.pri)

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_HEADERS_DIR  = ui
UI_SOURCES_DIR  = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release

LIBS            += -lqextserialport
LIBS            += -lxbeep

# Input
FORMS           += xbee-ap.ui
HEADERS         += xbee-ap.h serialport.h
HEADERS         += ../../../libxbee-v3/xbeep.h
SOURCES         += xbee-ap-main.cpp xbee-ap.cpp serialport.cpp


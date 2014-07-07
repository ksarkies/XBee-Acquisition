PROJECT =       XBee AP Mode Tool
TEMPLATE =      app
TARGET          += 
INCLUDEPATH     += ../auxiliary/libxbee-3.0.9/

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_HEADERS_DIR  = ui
UI_SOURCES_DIR  = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release

LIBS            += -L../auxiliary/libxbee-3.0.9/lib/ -lxbeep -lxbee

# Input
FORMS           += xbee-ap.ui
HEADERS         += xbee-ap.h
SOURCES         += xbee-ap-main.cpp xbee-ap.cpp


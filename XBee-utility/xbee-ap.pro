PROJECT =       XBee AP Mode Tool
TEMPLATE =      app
TARGET          += 
QT              += widgets

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_DIR          = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release

LIBS            += -lxbeep -lxbee

# Input
FORMS           += xbee-ap.ui
HEADERS         += xbee-ap.h
SOURCES         += xbee-ap-main.cpp xbee-ap.cpp


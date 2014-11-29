PROJECT = xbee-acqcontrol

LIBXBEE = ../auxiliary/libxbee-3.0.11

CFLAGS  = -pipe -O2 -Wall -W -pedantic
INCLUDE = -I. -I$(LIBXBEE)/
LDFLAGS = -L$(LIBXBEE)/lib

OBJECTS = $(PROJECT).o

all: $(PROJECT)

.cpp.o:
		g++ -c $(CFLAGS) $(INCLUDE) $<

$(PROJECT): $(OBJECTS)
		g++ -Wl,-O1 -o $(PROJECT) $(OBJECTS) $(LDFLAGS) -lxbee -lrt -lpthread

clean:
	rm *.o $(PROJECT)


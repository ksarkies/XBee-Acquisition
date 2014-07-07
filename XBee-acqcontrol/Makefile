PROJECT = xbee-acqcontrol

CFLAGS  = -pipe -O2 -Wall -W -pedantic
INCLUDE = -I . -I ../auxiliary/libxbee-v3/

OBJECTS = $(PROJECT).o

all: $(PROJECT)

.cpp.o:
		g++ -c $(CFLAGS) $(INCLUDE) $<

$(PROJECT): $(OBJECTS)
		g++ -Wl,-O1 -o $(PROJECT) $(OBJECTS) -L. -lxbee -lrt -lpthread

clean:
	rm *.o $(PROJECT)


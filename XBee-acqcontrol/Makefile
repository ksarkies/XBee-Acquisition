PROJECT = xbee-acqcontrol

CFLAGS  = -pipe -O2 -Wall -W -pedantic
INCLUDE = -I.
LDFLAGS = 

OBJECTS = $(PROJECT).o

all: $(PROJECT)

.cpp.o:
		gcc -c $(CFLAGS) $(INCLUDE) $<

$(PROJECT): $(OBJECTS)
		gcc -Wl,-O1 -o $(PROJECT) $(OBJECTS) $(LDFLAGS) -lxbee -lrt -lpthread

clean:
	rm *.o $(PROJECT)


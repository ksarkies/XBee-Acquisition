/*****************************************************************************
*           Serial Programmer for AVR
*       Ken Sarkies ksarkies@trinity.asn.au
*
* File              : XBee-node-test.h
* Compiler          : AVR-GCC/avr-libc(>= 1.2.5)
* Revision          : $Revision: 0.1 $
* Updated by        : $ author K. Sarkies 18/07/2014 $
*
* Target platform   : ATTiny2313
*
****************************************************************************/

/* baud rate register value calculation */
#ifndef F_CPU
#define F_CPU       1000000
#endif
#define BAUD_RATE   19200

// FLASH Pagesize in words
#define FPAGESIZE   32
// EEPROM Pagesize in words
#define EPAGESIZE   4



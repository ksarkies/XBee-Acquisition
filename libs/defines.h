/* This file is adapted to incorporate different sets of define headers
into a common header for the source files */

#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#if (MCU_TYPE==328)
#include "defines-M168.h"
#elif (MCU_TYPE==168)
#include "defines-M168.h"
#elif (MCU_TYPE==88)
#include "defines-M168.h"
#elif (MCU_TYPE==48)
#include "defines-M168.h"
#elif (MCU_TYPE==4313)
#include "defines-T4313.h"
#elif (MCU_TYPE==841)
#include "defines-T841.h"
#else
#error "Processor not defined"
#endif

/* Convenience macros */
#define _BV(bit) (1 << (bit))
#define inb(sfr) _SFR_BYTE(sfr)
#define inw(sfr) _SFR_WORD(sfr)
#define outb(sfr, val) (_SFR_BYTE(sfr) = (val))
#define outw(sfr, val) (_SFR_WORD(sfr) = (val))
#define inbit(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define toggle(sfr, bit) (_SFR_BYTE(sfr) ^= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define high(x) ((uint8_t) (x >> 8) & 0xFF)
#define low(x) ((uint8_t) (x & 0xFF))



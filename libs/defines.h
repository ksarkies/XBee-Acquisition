/* This file is adapted to incorporate different sets of define headers
into a common header for the source files */

#if (MCU_TYPE==168)
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



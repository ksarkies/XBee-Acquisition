/* Header file for AVR UART simple usage

*/

/****************************************************************************
 *   Copyright (C) 2007 by Ken Sarkies (www.jiggerjuice.info)               *
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License");          *
 * you may not use this file except in compliance with the License.         *
 * You may obtain a copy of the License at                                  *
 *                                                                          *
 *     http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                          *
 * Unless required by applicable law or agreed to in writing, software      *
 * distributed under the License is distributed on an "AS IS" BASIS,        *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 * See the License for the specific language governing permissions and      *
 * limitations under the License.                                           *
 ***************************************************************************/

#ifndef SERIAL_H
#define SERIAL_H

/* UART Error Definitions. */
#define NO_DATA                 0x01
#define BUFFER_OVERFLOW         0x02
#define OVERRUN_ERROR           0x04
#define FRAME_ERROR             0x08
#define STATE_MACHINE           0x10
#define CHECKSUM                0x11

/*----------------------------------------------------------------------*/
/* Prototypes */

void uartInit(void);
void sendch(uint8_t c);
uint8_t getchb(void);
uint16_t getch(void);

#endif


/*	Circular Buffer Management

Copyright (C) K. Sarkies <ksarkies@internode.on.net>

19 September 2012

Intended for libopencm3 but can be adapted to other libraries provided
data types defined.
*/

/****************************************************************************
 *   Copyright (C) 2012 by Ken Sarkies ksarkies@internode.on.net            *
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

#ifndef BUFFER_H
#define BUFFER_H

#include <inttypes.h>
#include <stdbool.h>

void buffer_init(uint8_t buffer[], uint8_t size);
uint16_t buffer_get(uint8_t buffer[]);
uint16_t buffer_put(uint8_t buffer[], uint8_t data);
bool buffer_output_free(uint8_t buffer[]);
bool buffer_input_available(uint8_t buffer[]);

#endif 


/*
Title:    XBee Acquisition Control process
*/

/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies ksarkies@internode.on.net            *
 *                                                                          *
 *   This file is part of XBee-Acquisition                                  *
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

#ifndef XBEE_ACQCONTROL_H
#define XBEE_ACQCONTROL_H

#define DEBUG   1

// Limit definitions
#define MAXNODES                25
#define SIZE                   256
#define DATA_BUFFER_SIZE        64
#define FLUSH_LIMIT              8
#define FILE_LIMIT            1024

// Length of data field in a data message
#define DATA_LENGTH             10

#define DATA_PATH           "/data/XBee/"

#define SERIAL_PORT         "/dev/ttyUSB0"
#define BAUDRATE            38400

#define PORT "58532"        // port for the external command I/F
#define MAX_CLIENTS 2       // Number of external connections allowed

#include "xbee.h"
#include <stdint.h>

#define FALSE   0
#define TRUE    1

/* Serial Port Parameters */

enum DataBitsType
{
    DATA_5 = 5,
    DATA_6 = 6,
    DATA_7 = 7,
    DATA_8 = 8
};

enum ParityType
{
    PAR_NONE,
    PAR_ODD,
    PAR_EVEN,
    PAR_SPACE
};

enum StopBitsType
{
    STOP_1,
    STOP_2
};

enum FlowType
{
    FLOW_OFF,
    FLOW_HARDWARE,
    FLOW_XONXOFF
};

/* Structure for a node table entry.
The node table holds all useful information about the nodes in the XBee
network. */

typedef struct {
    uint16_t adr;           // 16 bit address
    uint32_t SH;            // Upper serial number
    uint32_t SL;            // Lower serial number
    char nodeIdent[20];     // Node identifier string
    uint16_t parentAdr;
    uint8_t deviceType;     // 0=coordinator, 1=router, 2=end device
    uint8_t status;         // reserved
    uint16_t profileID;
    uint16_t manufacturerID;
    uint8_t valid;          // Indicates if the record has received a valid node ident.
    char dataResponse;      // First character of a response to a data message
    char remoteResponse;    // First character of a response to a remote AT message
    struct xbee_con *dataCon;// libxbee connection for data reception;
    struct xbee_con *atCon; // libxbee connection for AT commands reception;
    struct xbee_con *ioCon; // libxbee connection for I/O received frames
} nodeEntry;

extern int numberNodes;
extern nodeEntry nodeInfo[25];

/* libxbee errors
XBEE_ENONE                 =  0,
XBEE_EUNKNOWN              = -1,

XBEE_ENOMEM                = -2,

XBEE_ESELECT               = -3,
XBEE_ESELECTINTERRUPTED    = -4,

XBEE_EEOF                  = -5,
XBEE_EIO                   = -6,

XBEE_ESEMAPHORE            = -7,
XBEE_EMUTEX                = -8,
XBEE_ETHREAD               = -9,
XBEE_ELINKEDLIST           = -10,

XBEE_ESETUP                = -11,
XBEE_EMISSINGPARAM         = -12,
XBEE_EINVAL                = -13,
XBEE_ERANGE                = -14,
XBEE_ELENGTH               = -15,

XBEE_EFAILED               = -18,
XBEE_ETIMEOUT              = -17,
XBEE_EWOULDBLOCK           = -16,
XBEE_EINUSE                = -19,
XBEE_EEXISTS               = -20,
XBEE_ENOTEXISTS            = -21,
XBEE_ENOFREEFRAMEID        = -22,

XBEE_ESTALE                = -23,
XBEE_ENOTIMPLEMENTED       = -24,

XBEE_ETX                   = -25,

XBEE_EREMOTE               = -26,

XBEE_ESLEEPING             = -27,
XBEE_ECATCHALL             = -28,

XBEE_ESHUTDOWN             = -29,
*/

//-----------------------------------------------------------------------------
/* Prototypes */

bool command_handler(const int listener, unsigned char *buf);
void *get_in_addr(const struct sockaddr *sa);
int init_socket(int *listener);
int check_connections(fd_set *master, int *fd_max, const int listener);
int setupXbeeInstance();
void openRemoteConnection(int row);
void openRemoteConnections();
void closeRemoteConnection(int row);
void closeRemoteConnections();
int openGlobalConnections();
int closeGlobalConnections();
int nodeProbe();
int dataFileCheck();
int configFillNodeTable();
int readConfigFileHex();

#endif

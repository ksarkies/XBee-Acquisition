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
#define FLUSH_LIMIT             32
#define FILE_LIMIT            1024

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

//-----------------------------------------------------------------------------
/* Prototypes */

int setupXbeeInstance();
void openRemoteConnection(int row);
void openRemoteConnections();
void closeRemoteConnection(int row);
void closeRemoteConnections();
int openGlobalConnections();
int closeGlobalConnections();
int nodeProbe();
int command_handler(const int listener, unsigned char *buf);
void *get_in_addr(const struct sockaddr *sa);
int init_socket(int *listener);
int check_connections(fd_set *master, int *fd_max, const int listener);
xbee_err sendTxRequest(const int row, unsigned char * string);
int dataFileCheck();
int configFillNodeTable();
int readConfigFileHex();

#endif

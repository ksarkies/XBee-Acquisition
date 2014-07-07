/**
@mainpage XBee Acquisition Control process
@version 1.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 10 January 2013

This program is intended to run as a background process in a Unix environment
to interface to a local XBee coordinator and a network of XBee router/end
devices. Its purpose is to collect data from the networked devices and pass
it on to external storage with timestamping. An interface to an external GUI
program is provided using internet sockets for the purpose of status display,
configuration and firmware update.

The program starts by querying all nodes on the network and building a table
of information for each one, including libxbee connections to allow reception
of data streams from the device.

The program also sets up an Internet TCP port 58532 for connection externally
by a control program. Commands passed on that interface are ASCII as follows:

N   return number n of XBee nodes in table.
Ix  (where x is a node index <n), return a copy of the node information.
Sxs (where x is a node index <n), send a string s to the node on the data channel.
Rxs (where x is a node index <n), send a remote AT command s to the node.
r0  request a response from the most recent remote AT command.
L0s send a local AT command s to the attached coordinator XBee.
l0  request a response from the local AT command.

@note
May be portable to Windows and OS X, if anyone is so crazy.

@note
Compiler: gcc 4.6.3
*/
/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies ksarkies@internode.on.net            *
 *                                                                          *
 *   This file is part of xbee-control                                      *
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

#include "xbee.h"
#include "xbee-acqcontrol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>

/* Global Structures */
struct xbee *xbee;
struct xbee_con *localATCon;     // Connection for local AT commands
struct xbee_con *identifyCon;    // Connection for identify packets
int numberNodes;
nodeEntry nodeInfo[MAXNODES];   /* Allows up to 25 nodes */
uint8_t dataResponseRcvd;       /* Signal that a response to an AVR command has been received */
char dataResponseData[SIZE];    /* The data record received with a response */
uint8_t remoteATResponseRcvd;   /* Signal that a response to a remote AT command has been received */
uint8_t remoteATLength;
char remoteATResponseData[SIZE];/* The data record received with a remote AT response */
uint8_t localATResponseRcvd;    /* Signal that a response to a local AT command has been received */
uint8_t localATLength;
char localATResponseData[SIZE]; /* The data record received with a local AT response */
FILE *fp;                       /* File for results */
FILE *fpd;                      /* File for configuration XBee list */
int flushCount;                 /* Number of records at which buffers are flushed to the disk. */
int fileCount;                  /* Number of records at which the file is closed and a new one opened. */
char dirname[40];

/* Callback Prototypes */
void nodeIDCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data);
void dataCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data);
void remoteATCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data);
void localATCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data);
void ioCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data);
int min(int x, int y) {if (x>y) return y; else return x;}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/** @brief XBee Acquisition Control Main Program

*/

int main(int argc,char ** argv)
{

/*---------------------------------------------------------------------------------*/
/* Create a directory name for storage of data, from the arguments */
    if (argc == 2) strcpy(dirname,argv[1]);
    else strcpy(dirname,DATA_PATH);
    if (dirname[strlen(dirname)-1] != '/') dirname[strlen(dirname)] = '/';

/*---------------------------------------------------------------------------------*/
/* A bit of logging stuff. Update the rsyslog.conf files to link local7 to a log file */

    openlog("xbee_acqcontrol", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);
    syslog(LOG_INFO, "Starting XBee Instance\n");

/*---------------------------------------------------------------------------------*/
/* Initialise configuration file and fill node table.*/

    fpd = NULL;
    if (! configFillNodeTable()) return 1;

/*---------------------------------------------------------------------------------*/
/* Initialise results storage */

    fp = NULL;
    if (! dataFileCheck()) return 1;

/*---------------------------------------------------------------------------------*/
/* Initialise the xbee instance and probe for any new nodes on the network */

    setupXbeeInstance();
    openRemoteConnections();
    nodeProbe();
    openGlobalConnections();

/*---------------------------------------------------------------------------------*/
/* Setup the Internet command interface Internet socket */

    int error;

    fd_set master;              /* master file descriptor list */
    int fdmax;                  /* maximum file descriptor number */
    int listener;               /* listening socket descriptor */

    FD_ZERO(&master);
	if ((error = init_socket(&listener)) > 0)
    {
        syslog(LOG_INFO, "Could not setup interface %d\n", error);
        return false;
    }
/* add the listener to the master set (first entry - later ones are the clients) */
    FD_SET(listener, &master);

/* keep track of the biggest file descriptor */
    fdmax = listener;
    int fdnumber = fdmax;
    dataResponseRcvd = false;

/*---------------------------------------------------------------------------------*/
/* main loop */

    for(;;)
	{
/* Check Internet connections. When data arrives command_handler is called */
        check_connections(&master, &fdmax, listener);
        if (fdnumber > fdmax)
        {
            syslog(LOG_INFO, "New connection %d\n",fdmax);
            fdnumber = fdmax;
        }
    }

/*---------------------------------------------------------------------------------*/
/* Close up and quit (don't quite know how we're going to get here!) */

    closeGlobalConnections();
    closeRemoteConnections();
	xbee_shutdown(xbee);
    closelog();
    return 0;
}

/*-----------------------------------------------------------------------------*/
/** @brief Handle command interface for the Internet client

This receives a line of command from the client, processes it and returns any
relevant data to the external client.

The message received from the client must have the length of the message, the
command and the row number to refer to the node to be addressed.

The information passed back consists of the length of the response message,
the original command, a status byte and any data. The status byte will indicate
any error conditions, generally from the response of the libxbee Tx call.

The incoming data buffer is assumed to have all the message data, that is,
we do not wait for anything lagging behind.

L send a local AT command to the attached coordinator XBee.
l check for a response to a previously sent local AT command.
R send a remote AT command to the selected node. The AT command follows in the data field.
r check for a response to a previously sent remote AT command.
S send an ASCII string to a remote node on the established data connection.
s check for a response to a previously sent node AVR command.
N return the number of rows in the node table.
I return the information held about the node in the row.
X restart the XBee instance (not recommended).
Q reconstruct the three libxbee connections.
D delete a row from the table and compress it.
E Create a new entry with a given 64 bit address.
V Change validity of a node to invalid (zero) or valid (nonzero)

Responses to local and remote AT commands are stored in separate global variables.
These must be read before any further commands are sent to any node, including
the local coordinator XBee.

Three commands are used to check for a response to a previously send command.
These are returned with the first character an echo of the previous command
character, followed by any parameters. if the command echo is a null, then
there was no response received at that time.

@parameter  int listener: file handler for the socket created.
@parameter  unsigned char * buf: A buffer [256] with the received data.
@returns:   true if no error
*/
int command_handler(const int listener, unsigned char *buf)
{
    xbee_err ret;
    char reply[256];
    int replyLength = 0;
    int i,j,strLength;
    unsigned char commandLength = buf[0];
    unsigned char command = buf[1];
    uint32_t temp;
    uint32_t SH;
    uint32_t SL;
    int node;
    int row = buf[2];
    if (row > numberNodes) return false;
#ifdef DEBUG
    if ((command != 'r') && (command != 'l') && (command != 's'))
    {
        printf("Command from GUI: length %d", commandLength);
        if ((command == 'L') || (command == 'R'))
        {
            printf(" command %c %c%c" , command, buf[3], buf[4]);
            for (uint i=3; i<5; i++) printf("%d", buf[i]);
            for (uint i=5; i<commandLength; i++) printf("%d", buf[i]);
            printf("\n");
        }
        else
            printf(" command %02X row %d\n", command, row);
    }
#endif
    unsigned char str[100];
    strLength = 0;
    switch (command)
    {
/* Reset the entire XBee process, probe for nodes again and setup the global
connections. This may be needed after an Xbee network or software reset. */
        case 'X':
            closeGlobalConnections();
            closeRemoteConnections();
        	xbee_shutdown(xbee);
            sleep(10);                  /* Wait for coordinator to initialise */
            setupXbeeInstance();
            openRemoteConnections();
            nodeProbe();
            openGlobalConnections();
            break;
/* Send a remote AT command to a node on its established  connection.
Use the libxbee connTx command as there may be zeros which would be
misinterpreted as end of string. */
        case 'R':
            for (j=0; j<commandLength-3; j++) str[j] = buf[j+3];
#ifdef DEBUG
            printf("Remote AT Command sent: %c%c ", str[0], str[1]);
            for (j=2; j<commandLength-3; j++) printf("%02X ",str[j]);
            printf("\n");
#endif
            replyLength = 3;
            ret = xbee_connTx(nodeInfo[row].atCon, NULL, str, commandLength-3);
            reply[3] = ret;
            break;

/* Check for a response to a previously sent remote AT command.
An 'R' is sent before the parameters.
If no response was received, a short message is sent back without data.*/
        case 'r':
            if (remoteATResponseRcvd)
            {
                replyLength = 3;
                reply[2] = 'R';
                nodeInfo[row].remoteResponse = remoteATResponseData[0];
                remoteATResponseRcvd = false;
                for (;replyLength < remoteATLength+3; replyLength++)
                    reply[replyLength] = remoteATResponseData[replyLength-3];
            }
            else
            {
                replyLength = 3;
                reply[2] = 0;
            }
            break;

/* Send a local AT command to a node on its established connection.
Use the libxbee connTx command as there may be zeros which would be
misinterpreted as end of string. */
        case 'L':
            for (j=0; j<commandLength-3; j++) str[j] = buf[j+3];
#ifdef DEBUG
            printf("Local AT Command sent: ");
            for (j=0; j<commandLength-3; j++) printf("%02X ",str[j]);
            printf("\n");
#endif
            replyLength = 3;
            ret = xbee_connTx(localATCon, NULL, str, commandLength-3);
            reply[3] = ret;
            break;

/* Check for a response to a previously sent local AT command.
An 'A' is sent before the parameters.
If no response was received, a short message is sent back without data.*/
        case 'l':
            if (localATResponseRcvd)
            {
                replyLength = 3;
                reply[2] = 'A';
                localATResponseRcvd = false;
                for (;replyLength < localATLength+3; replyLength++)
                    reply[replyLength] = localATResponseData[replyLength-3];
            }
            else
            {
                replyLength = 3;
                reply[2] = 0;
            }
            break;

/* Send an ASCII string to a remote node on its established data connection.
This will be the basic command interface to the AVR. */
        case 'S':
            while (buf[strLength+3] > 0)
            {
                str[strLength] = buf[strLength+3];
                strLength++;
            }
#ifdef DEBUG
            printf("Data String sent: ");
            for (j=0; j<strLength; j++) printf("%c",str[j]);
            printf("\n");
#endif
            replyLength = 3;
            reply[2] = xbee_connTx(nodeInfo[row].dataCon, NULL, str, strLength);
            break;

/* Check for a response to a previously sent data command to the node AVR.
If no response was received, a short message is sent back without data.*/
        case 's':
            if (dataResponseRcvd)
            {
                replyLength = 3;
                reply[2] = 'S';
                dataResponseRcvd = false;
                nodeInfo[row].dataResponse = dataResponseData[0];
                while (dataResponseData[replyLength-3] > 0)
                {
                    reply[replyLength] = dataResponseData[replyLength-3];
                    replyLength++;
                }
            }
            else
            {
                replyLength = 3;
                reply[2] = 0;
            }
            break;

/* Return the number of nodes currently in the table */
        case 'N':
            replyLength = 3;
            reply[2] = numberNodes;
            break;

/* Return the information about a node at the given row */
        case 'I':
            replyLength = 3;
            reply[2] = row;
            reply[replyLength++] = (char) (nodeInfo[row].adr >> 8);
            reply[replyLength++] = (char) (nodeInfo[row].adr);
            reply[replyLength++] = (char) (nodeInfo[row].SH >> 24);
            reply[replyLength++] = (char) (nodeInfo[row].SH >> 16);
            reply[replyLength++] = (char) (nodeInfo[row].SH >> 8);
            reply[replyLength++] = (char) (nodeInfo[row].SH);
            reply[replyLength++] = (char) (nodeInfo[row].SL >> 24);
            reply[replyLength++] = (char) (nodeInfo[row].SL >> 16);
            reply[replyLength++] = (char) (nodeInfo[row].SL >> 8);
            reply[replyLength++] = (char) (nodeInfo[row].SL);
            reply[replyLength++] = (char) (nodeInfo[row].deviceType);
            reply[replyLength++] = (char) (nodeInfo[row].valid);
            j = 0;
            while (nodeInfo[row].nodeIdent[j] > 0)
                reply[replyLength++] = nodeInfo[row].nodeIdent[j++];
            reply[replyLength++] = 0;
            break;

/* Reset the connections for the selected node. */
        case 'Q':
            replyLength = 3;
            reply[2] = 'Q';
            closeRemoteConnection(row);
#ifdef DEBUG
            printf("Restarting Connection for node %d\n",row);
#endif
            openRemoteConnection(row);
            break;

/* Change the validity of the node */
        case 'V':
            replyLength = 3;
            reply[2] = '\0';
            nodeInfo[row].valid = buf[3];
            break;

/* Setup a new entry with a serial number ready for use. */
        case 'E':
            replyLength = 3;
            reply[2] = 'Y';
            i = 3;
            temp = buf[i++];
            temp = buf[i++] + (temp << 8);
            temp = buf[i++] + (temp << 8);
            temp = buf[i++] + (temp << 8);
            SH = temp;
            temp = buf[i++];
            temp = buf[i++] + (temp << 8);
            temp = buf[i++] + (temp << 8);
            temp = buf[i++] + (temp << 8);
            SL = temp;
/* Check if the serial number already exists. If not, then it is a new node. */
            node = 0;
            for (;node < numberNodes; node++)
                if ((nodeInfo[node].SH == SH) && (nodeInfo[node].SL == SL)) break;
            if ((node > MAXNODES) || (node < numberNodes))
            {
                reply[2] = 'N';     /* Indicate cannot create new node */
                break;
            }
            nodeInfo[numberNodes].SH = SH;
            nodeInfo[numberNodes].SL = SL;
            openRemoteConnection(numberNodes);
            numberNodes++;
            break;

/* Delete the selected entry and compress the table. */
        case 'D':
            replyLength = 3;
            reply[2] = 'D';
            closeRemoteConnection(row);             /* Close off its connections if any */
            numberNodes--;
            for (int i=row; i<numberNodes; i++)
                nodeInfo[i] = nodeInfo[i+1];
            nodeInfo[numberNodes].atCon = NULL;     /* Nullify defunct row pointers */
            nodeInfo[numberNodes].ioCon = NULL;
            nodeInfo[numberNodes].dataCon = NULL;
            if (ftruncate(fileno(fpd),0) != 0) break;       /* Wipe contents of file */
            for (int node = 0; node < numberNodes; node++)
            {
/* Write all node data back to the configuration file */
                fprintf(fpd,"%04X ",nodeInfo[node].adr);
                fprintf(fpd,"%08X ",nodeInfo[node].SH);
                fprintf(fpd,"%08X ",nodeInfo[node].SL);
                fprintf(fpd,"%s ",nodeInfo[node].nodeIdent);
                fprintf(fpd,"%04X ",nodeInfo[node].parentAdr);
                fprintf(fpd,"%02X ",nodeInfo[node].deviceType);
                fprintf(fpd,"%02X ",nodeInfo[node].status);
                fprintf(fpd,"%04X ",nodeInfo[node].profileID);
                fprintf(fpd,"%04X ",nodeInfo[node].manufacturerID);
                fprintf(fpd,"\n");
            }
            fflush(fpd);
            break;
    }
    reply[0] = replyLength;
    reply[1] = command;
#ifdef DEBUG
    if ((command != 'r') && (reply[2] != 0))
    {
        printf("Reply sent to GUI %02X %c ", reply[0], reply[1]);
        for (int i=2; i<replyLength; i++) printf("%02X ", reply[i]);
        printf("\n");
    }
#endif
/* Can't do much if not all sent or error. Client just has to lump it  */
    return (send(listener, reply, replyLength, 0) != replyLength);
}

/*-----------------------------------------------------------------------------*/
/** @brief Create the XBee instance

The xbee instance is created along with a local AT command interface temporarily
for checking that the XBee is working.

Globals: xbee. The libxbee instance created and used throughout the program.

@returns    libxbee error
*/
int setupXbeeInstance()
{
    xbee_err ret;
    unsigned char txRet;

/* Setup the xbee instance with fixed device and baudrate */
/* Use Unix stat to check if the file was created and find a suitable one.
Not guaranteed to be the XBee one so manually remove all other USB serial devices. */
    struct stat st;
    if(stat("/dev/ttyUSB0",&st) == 0)       /* Check if USB0 exists */
        ret = xbee_setup(&xbee, "xbee2", "/dev/ttyUSB0", 38400);
    else
        ret = xbee_setup(&xbee, "xbee2", "/dev/ttyUSB1", 38400);
    if (ret != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Unable to open XBee port: %d (%s)", ret, xbee_errorToStr(ret));
        return ret;
    }
/* Setup a connection for local AT commands */
	ret = xbee_conNew(xbee, &localATCon, "Local AT", NULL);
    if (ret != XBEE_ENONE)
    {
		syslog(LOG_INFO, "Local AT connection failed: %d (%s)", ret, xbee_errorToStr(ret));
		return ret;
	}
/* Attempt to send an AP command to revert to mode 1 (in case set to mode 2)
For some reason the XBee needs a command like this before sending out the ND
otherwise it doesn't pick up the responses. */
    ret = xbee_conTx(localATCon, &txRet, "AP\x01");
    if (ret != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Unable to change mode of XBee: %d (%s)", ret, xbee_errorToStr(ret));
    }
/* Close off the local AT connection to destroy unwanted responses that
would get mixed up with the ND. */
    xbee_conEnd(localATCon);
    return ret;
}

/*-----------------------------------------------------------------------------*/
/** @brief Open remote node connections

The table entry is set as valid and a set of connections are built if they are not
already present.

Globals: xbee, nodeInfo

@parameter  int node. The table node entry to be set.
*/

void openRemoteConnection(int node)
{
/* Setup an address to build connections for incoming data, remote AT and I/O frames */
    struct xbee_conAddress address;
    memset(&address, 0, sizeof(address));
    address.addr64_enabled = 1;
    address.addr64[0] = (nodeInfo[node].SH >> 24) & 0xFF;
    address.addr64[1] = (nodeInfo[node].SH >> 16) & 0xFF;
    address.addr64[2] = (nodeInfo[node].SH >> 8) & 0xFF;
    address.addr64[3] = nodeInfo[node].SH & 0xFF;
    address.addr64[4] = (nodeInfo[node].SL >> 24) & 0xFF;
    address.addr64[5] = (nodeInfo[node].SL >> 16) & 0xFF;
    address.addr64[6] = (nodeInfo[node].SL >> 8) & 0xFF;
    address.addr64[7] = nodeInfo[node].SL & 0xFF;
    if (nodeInfo[node].dataCon == NULL)
    {
        if ((xbee_conNew(xbee, &nodeInfo[node].dataCon, "Data", &address) != XBEE_ENONE) ||
            (xbee_conCallbackSet(nodeInfo[node].dataCon, dataCallback, NULL) != XBEE_ENONE))
        {
            nodeInfo[node].dataCon = NULL;
        }
    }
    if (nodeInfo[node].atCon == NULL)
    {
        if ((xbee_conNew(xbee, &nodeInfo[node].atCon, "Remote AT", &address) != XBEE_ENONE) ||
            (xbee_conCallbackSet(nodeInfo[node].atCon, remoteATCallback, NULL) != XBEE_ENONE))
        {
            nodeInfo[node].atCon = NULL;
        }
    }
    if (nodeInfo[node].ioCon == NULL)
    {
        if ((xbee_conNew(xbee, &nodeInfo[node].ioCon, "I/O", &address) != XBEE_ENONE) ||
            (xbee_conCallbackSet(nodeInfo[node].ioCon, ioCallback, NULL) != XBEE_ENONE))
        {
            nodeInfo[node].ioCon = NULL;
        }
    }
}

/*-----------------------------------------------------------------------------*/
/** @brief Open all remote node connections in the table

Globals: numberNodes
*/

void openRemoteConnections()
{
/* Invalidate all entries in the table */
    for (int node=0; node<numberNodes; node++)
    {
        openRemoteConnection(node);
    }
    return;
}

/*-----------------------------------------------------------------------------*/
/** @brief Close down a single remote node connections

Globals: nodeInfo

@parameter  int node. The table node entry to be invalidated.
*/

void closeRemoteConnection(int node)
{
    if (nodeInfo[node].dataCon != NULL) xbee_conEnd(nodeInfo[node].dataCon);
    nodeInfo[node].dataCon = NULL;
    if (nodeInfo[node].ioCon != NULL) xbee_conEnd(nodeInfo[node].ioCon);
    nodeInfo[node].ioCon = NULL;
    if (nodeInfo[node].atCon != NULL) xbee_conEnd(nodeInfo[node].atCon);
    nodeInfo[node].atCon = NULL;
    nodeInfo[node].valid = FALSE;
}

/*-----------------------------------------------------------------------------*/
/** @brief Close down all remote node connections

Globals: numberNodes
*/

void closeRemoteConnections()
{
/* Invalidate all entries in the table */
    for (int node=0; node<numberNodes; node++)
    {
        closeRemoteConnection(node);
    }
    return;
}

/*-----------------------------------------------------------------------------*/
/** @brief Probe for all nodes on the network

Send out a node discover signal and build the table of nodes. A local AT connection
is used.

Globals: xbee

@returns    libxbee error value
*/

int nodeProbe()
{
    xbee_err ret;
    unsigned char txRet;
    int sleeping = FALSE;
    struct xbee_con *probeATCon;
/* Check if the local AT connection is open and put it to sleep */
    if (xbee_conValidate(localATCon) == XBEE_ENONE)
    {
        if ((ret = xbee_conSleepSet(localATCon, CON_SLEEP)) != XBEE_ENONE)
        {
	        syslog(LOG_INFO, "Cannot sleep local connection: %d (%s)", ret, xbee_errorToStr(ret));
	        return ret;
        }
        sleeping = TRUE;
    }

/* Open a local AT connection without callback. */
    if ((ret = xbee_conNew(xbee, &probeATCon, "Local AT", NULL)) != XBEE_ENONE)
    {
	    syslog(LOG_INFO, "Local AT connection failed: %d (%s)", ret, xbee_errorToStr(ret));
    }
    else
    {
        ret = xbee_conTx(probeATCon, &txRet, "ND");
        syslog(LOG_INFO, "ND probe sent: %d (%s)", txRet, xbee_errorToStr(ret));
        if ((ret != XBEE_ENONE) && (ret != XBEE_ETX))
	        syslog(LOG_INFO, "ND probe Tx failed: %d (%s)", ret, xbee_errorToStr(ret));
        else
        {
/* Timeout always seems to occur so we won't act on this */
            if (txRet == (unsigned char)XBEE_ETIMEOUT)
        	    syslog(LOG_INFO, "ND probe Tx timeout: %d (%s)", txRet, xbee_errorToStr(XBEE_ETIMEOUT));
/* For this particular command callbacks have not been used and the Rx is polled for all
responding nodes */
            int rxWait = 20;            /* Default response delay max is 6sec, so wait up to 10sec */
            struct xbee_pkt *pkt;
            int pktRemaining;
            {
                for (int i = 0; i < rxWait; i++)
                {
                    if ((ret = xbee_conRx(probeATCon, &pkt, &pktRemaining)) != XBEE_ENOTEXISTS)
                    {
                        nodeIDCallback(xbee, probeATCon, &pkt, NULL);
#ifdef DEBUG
                        printf("Node found %d return %d\n",i,ret);
#endif
                    }
                    usleep(500000);
                }
            }
            xbee_pktFree(pkt);
#ifdef DEBUG
            printf("ND Ready\n");
            int i;
            for (i=0; i<numberNodes; i++) printf("\nNode ID %s Address %d Valid %d\n",
                                    nodeInfo[i].nodeIdent,nodeInfo[i].adr,nodeInfo[i].valid);
#endif
        }
/* Close off local AT connection */
	    ret = xbee_conEnd(probeATCon);
        if (ret != XBEE_ENONE)
        {
		    syslog(LOG_INFO, "Local AT connection exit failed: %d (%s)", ret, xbee_errorToStr(ret));
	    }
    }
    if (! sleeping) xbee_conSleepSet(localATCon, CON_AWAKE);
    return ret;
}
/*-----------------------------------------------------------------------------*/
/** @brief Open all long-running connections

Connections for local AT and identify are left open.

@returns    libxbee error value
*/

int openGlobalConnections()
{
    xbee_err ret;

    if ((ret = xbee_conNew(xbee, &localATCon, "Local AT", NULL)) != XBEE_ENONE)
    {
	    syslog(LOG_INFO, "Local AT connection failed: %d (%s)", ret, xbee_errorToStr(ret));
	    return ret;
    }
/* Set the callback for the local AT response. */
	if ((ret = xbee_conCallbackSet(localATCon, localATCallback, NULL)) != XBEE_ENONE)
    {
		syslog(LOG_INFO, "Local AT Callback Set failed: %d (%s)", ret, xbee_errorToStr(ret));
        xbee_conEnd(localATCon);
		return ret;
	}
/* Setup a long-running connection for Identify packets for nodes starting up later. */
	if ((ret = xbee_conNew(xbee, &identifyCon, "Identify", NULL)) != XBEE_ENONE)
    {
		syslog(LOG_INFO, "Identify Connection failed: %d (%s)", ret, xbee_errorToStr(ret));
		return ret;
	}
/* Set the callback for the identify response. */
	if ((ret = xbee_conCallbackSet(identifyCon, nodeIDCallback, NULL)) != XBEE_ENONE)
    {
		syslog(LOG_INFO, "Identify Callback Set failed: %d (%s)", ret, xbee_errorToStr(ret));
        xbee_conEnd(identifyCon);
	}
    return ret;
}

/*-----------------------------------------------------------------------------*/
/** @brief Close all long-running connections

Connections for local AT and identify are closed.

@returns    libxbee error value
*/

int closeGlobalConnections()
{
    xbee_err ret;

	if ((ret = xbee_conEnd(localATCon)) != XBEE_ENONE)
    {
		syslog(LOG_INFO, "Could not close local AT connection %d\n", ret);
		return ret;
	}
	if ((ret = xbee_conEnd(identifyCon)) != XBEE_ENONE)
    {
		syslog(LOG_INFO, "Could not close identify connection %d\n", ret);
	}
    return ret;
}

/*-----------------------------------------------------------------------------*/
/** @brief Get the Internet socket address IPv4 or IPv6

@parameter  struct sockaddr *sa: Socket address structure
@returns    pointer to the IP address part of the socket address
*/
void *get_in_addr(const struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*-----------------------------------------------------------------------------*/
/** @brief Initialise the Internet command interface socket

First get the address information for the local machine and port as given.
Try out the returned addresses and select the first that binds OK.

@returns    int *listener: file handler for the socket created 
@returns    0: OK
		    1: failed getting address information
		    2: failed to bind to address
		    3: failed to initiate listening
*/
int init_socket(int *listener)
{
int listen_fd;
struct addrinfo hints, *ai, *p;
int yes=1;        /* for setsockopt() SO_REUSEADDR, below */

/* Make up the address hints (options) */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

/* Get the address info for the selected port */
    if (getaddrinfo(NULL, PORT, &hints, &ai) != 0) return 1;
    
/* Try out all the returned addresses and pick the first available one */
    for(p = ai; p != NULL; p = p->ai_next)
	{
/* Try to open a socket for the address */
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_fd < 0) continue;
        
/* reuse the address */
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

/* If socket opened, try to bind the socket to the address */
        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) < 0)
		{
            close(listen_fd);
            continue;
        }
/* If we got the socket bound, then we can drop out */
        break;
    }

/* if we got to the end of the list, it means we didn't get bound */
    if (p == NULL) return 2;

    freeaddrinfo(ai); /* all done with this address */

/* listen - we should now be listening on the interface that was setup */
    if (listen(listen_fd, MAX_CLIENTS) == -1) return 3;

    *listener = listen_fd;

    return 0;
}

/*-----------------------------------------------------------------------------*/
/** @brief Check for Internet connections

This checks for any incoming connections and calls a command handler with the
data received.

@parameter  fd_set *master: list of file descriptors 
@parameter  int *fd_max: number of file descriptors in list
@parameter  int listener: file handler for the socket created 
@returns    updated *master and *fd_max
@returns    0: OK
		    1: failed select
		    2: failed accept
*/

int check_connections(fd_set *master, int *fd_max, const int listener)
{
fd_set read_fds;
int fdmax = *fd_max;
struct sockaddr_storage remoteaddr; /* client address */
socklen_t addrlen;
int newfd;                          /* newly accepted socket descriptor */
unsigned char buf[256];             /* buffer for client data */
int fd,nbytes;

    read_fds = *master;             /* copy the master FD list for the select() */
/* Select monitors a list of file descriptors for any that have become ready */
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) return 1; /* nothing ready */

/* Found one so run through the existing connections looking for data to read */
    for(fd = 0; fd <= fdmax; fd++)
	{
        if (FD_ISSET(fd, &read_fds))
		{
/* Check the local listening socket (means a new connection has arrived). */
            if (fd == listener)
			{
                addrlen = sizeof remoteaddr;
/* accept() doesn't block because we know there is a new connection pending */
                if ((newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen)) == -1) return 2;
                else
				{
                    FD_SET(newfd, master);  /* add to master list */
                    if (newfd > fdmax)
                    {
                        fdmax = newfd;      /* keep track of the maximum fd */
                        *fd_max = newfd;
                    }
                }
            }
			else
			{
/* If not the listener, then handle data from a client */
                if ((nbytes = recv(fd, buf, sizeof buf, 0)) <= 0)
				{
/* got error or connection was closed by client. Remove from the list. */
/* (in case of error just let client die as we are running as a background process) */
                    close(fd);
                    FD_CLR(fd, master); /* remove from master set */
                }
				else
				{
/* we got some data from a client so call a command handler. */
                    command_handler(fd, buf);
                }
            } /* finished handling new or data from client */
        } /* finished connection ready */
    }
    return 0;
}

/*-----------------------------------------------------------------------------*/
/** @brief Callback for the ND and Identify command, to load up the node information list

This is the callback required by libxbee. It operates on the global datastructure
that holds all information for each node returned by the ND command.

This checks for existence of the node information and adds a new node if not
present. Data, I/O and AT command connections are added.

Although there may be incomplete or erroneous entries, we may leave them and check
later by other means to attempt to correct the problem.

If a device powers off and back on again it will be dealt with adequately by this
callback. Connections are only added if the entry is not valid or is a new entry.
*/

void nodeIDCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    printf("Identify Packet: length %d, data ", (*pkt)->dataLen);
    for (int i=0; i<10; i++) printf("%02X ", (*pkt)->data[i]);
    for (int i=10; i<(*pkt)->dataLen-9; i++) printf("%c", (*pkt)->data[i]);
    for (int i=(*pkt)->dataLen-9; i<(*pkt)->dataLen; i++) printf(" %02X", (*pkt)->data[i]);
    printf("\n");
#endif
    int i=0;
    uint32_t temp;
    temp = (*pkt)->data[i++];
    uint16_t adr = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++];
    temp = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++] + (temp << 8);
    uint32_t SH = temp;
    temp = (*pkt)->data[i++];
    temp = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++] + (temp << 8);
    uint32_t SL = temp;
/* Check if the serial number already exists. If not, then it is a new node. */
    int node = 0;
    for (;node < numberNodes; node++)
        if ((nodeInfo[node].SH == SH) && (nodeInfo[node].SL == SL)) break;
/* Fill in or refresh the info fields if there is room in the table. */
    if (node > MAXNODES) return;
    nodeInfo[node].adr = adr;
    nodeInfo[node].SH = SH;
    nodeInfo[node].SL = SL;
    do
    {
        nodeInfo[node].nodeIdent[i-10] = (*pkt)->data[i];
        i++;
    }
    while (nodeInfo[node].nodeIdent[i-11] != 0);
    temp = (*pkt)->data[i++];
    nodeInfo[node].parentAdr = (*pkt)->data[i++] + (temp << 8);
    nodeInfo[node].deviceType = (*pkt)->data[i++];
    nodeInfo[node].status = (*pkt)->data[i++];
    temp = (*pkt)->data[i++];
    nodeInfo[node].profileID = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++];
    nodeInfo[node].manufacturerID = (*pkt)->data[i++] + (temp << 8);
    if (node == numberNodes)
    {
        numberNodes++;

/* Write new node data to the configuration file */
        fprintf(fpd,"%04X ",nodeInfo[node].adr);
        fprintf(fpd,"%08X ",nodeInfo[node].SH);
        fprintf(fpd,"%08X ",nodeInfo[node].SL);
        fprintf(fpd,"%s ",nodeInfo[node].nodeIdent);
        fprintf(fpd,"%04X ",nodeInfo[node].parentAdr);
        fprintf(fpd,"%02X ",nodeInfo[node].deviceType);
        fprintf(fpd,"%02X ",nodeInfo[node].status);
        fprintf(fpd,"%04X ",nodeInfo[node].profileID);
        fprintf(fpd,"%04X ",nodeInfo[node].manufacturerID);
        fprintf(fpd,"\n");
        fflush(fpd);

/* Disable all connections on new entry to allow them to be created below. */
        nodeInfo[node].valid = FALSE;
        nodeInfo[node].dataCon = NULL;
        nodeInfo[node].ioCon = NULL;
        nodeInfo[node].atCon = NULL;
    }
/* Add (for new nodes) or rebuild (for failed attempts earlier) connections. */
    openRemoteConnection(node);
    nodeInfo[node].valid = TRUE;
}

/*-----------------------------------------------------------------------------*/
/** @brief Determine the node information table row from a received packet address.

If the returned value equals the number of rows, then the node is not in the table.
*/

int findRow(unsigned char *addr)
{
/* Find the node in the table from its serial number */
    int row=0;
    for (; row<numberNodes; row++)
    {
	    if ((addr[0] == ((nodeInfo[row].SH >> 24) & 0xFF)) &&
	        (addr[1] == ((nodeInfo[row].SH >> 16) & 0xFF)) &&
	        (addr[2] == ((nodeInfo[row].SH >> 8) & 0xFF)) &&
	        (addr[3] == (nodeInfo[row].SH & 0xFF)) &&
	        (addr[4] == ((nodeInfo[row].SL >> 24) & 0xFF)) &&
	        (addr[5] == ((nodeInfo[row].SL >> 16) & 0xFF)) &&
	        (addr[6] == ((nodeInfo[row].SL >> 8) & 0xFF)) &&
	        (addr[7] == (nodeInfo[row].SL & 0xFF))) break;
    }
    return row;
}

/*-----------------------------------------------------------------------------*/
/** @brief Callback for the data packets sent from the nodes.

This is the callback required by libxbee. It interprets incoming packets on the
data connection, and for process data passes it on to an external file.

Other packets are responses to commands to the node AVR sent over the same connection.
The response is stored in a global array. This should be read before any other node
AVR command is sent to any node.
*/

void dataCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data)
{
    char buffer[DATA_BUFFER_SIZE];
    int row = findRow((*pkt)->address.addr64);
    char outstr[20];
    time_t now;
    now = time(NULL);
    struct tm *tmp;
    tmp = localtime(&now);
    strftime(outstr, sizeof(outstr),"%FT%H:%M:%S",tmp);
#ifdef DEBUG
    if (row == numberNodes) printf("Node Unknown ");
    else printf("Node %s ", nodeInfo[row].nodeIdent);
    printf("Data Packet: Time %s length %d, data ",outstr,(*pkt)->dataLen);
    for (int i=0; i<(*pkt)->dataLen; i++) printf("%c", (*pkt)->data[i]);
    printf("\n");
#endif
    int writeLength = (*pkt)->dataLen;
    if (writeLength > DATA_BUFFER_SIZE) writeLength = DATA_BUFFER_SIZE;
    if ((*pkt)->data[0] == 'D')
    {
        if (row == numberNodes) fprintf(fp,"Node Unknown ");
        else fprintf(fp,"Node %s", nodeInfo[row].nodeIdent);
        fprintf(fp," %s ", outstr);
        fprintf(fp, "Data ");
        for (int i=0; i<writeLength; i++) buffer[i] = (*pkt)->data[i+1];
        fwrite(buffer,1,writeLength,fp);
        fprintf(fp, "\n");
        dataFileCheck();
    }
    else
    {
        dataResponseRcvd = true;
        for (int i=0; i< min(SIZE,(*pkt)->dataLen); i++)
            dataResponseData[i] = (*pkt)->data[i];
    }
/* If we are hearing from this then it is a valid node */
    if (row < numberNodes) nodeInfo[row].valid = TRUE;

}

/*-----------------------------------------------------------------------------*/
/** @brief Callback for remote AT responses sent from the nodes.

This is the callback required by libxbee. It interprets incoming packets on the
remote AT connection.

The response is stored in a global array. This should be read before any other remote
AT command is sent to any node.
*/

void remoteATCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    printf("Remote AT Response: length %d, data ",(*pkt)->dataLen);
    for (int i=0; i<(*pkt)->dataLen; i++) printf("%02X", (*pkt)->data[i]);
    printf("\n");
#endif
    remoteATResponseRcvd = true;
    remoteATLength = (*pkt)->dataLen;
    for (int i=0; i < min(SIZE,remoteATLength); i++)
        remoteATResponseData[i] = (*pkt)->data[i];
}

/*-----------------------------------------------------------------------------*/
/** @brief Callback for local AT responses sent from the attached XBee.

This is the callback required by libxbee. It interprets incoming packets on the
remote AT connection.

The response is stored in a global array. This should be read before any other remote
AT command is sent to any node.
*/

void localATCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    printf("Local AT Response: length %d, data ",(*pkt)->dataLen);
    for (int i=0; i<(*pkt)->dataLen; i++) printf("%02X", (*pkt)->data[i]);
    printf("\n");
#endif
    localATResponseRcvd = true;
    localATLength = (*pkt)->dataLen;
    for (int i=0; i < min(SIZE,localATLength); i++)
        localATResponseData[i] = (*pkt)->data[i];
}

/*-----------------------------------------------------------------------------*/
/** @brief Callback for remote I/O received frames sent from the nodes.

This is the callback required by libxbee. It interprets incoming packets on the
I/O connection for XBees that have been setup to send these.

The I/O frames are values measured on selected ports of the XBee itself.

Received data is simply stored in an external file.
*/

void ioCallback(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data)
{
    char outstr[20];
    time_t now;
    now = time(NULL);
    struct tm *tmp;
    tmp = localtime(&now);
    strftime(outstr, sizeof(outstr),"%FT%H:%M:%S",tmp);
    int row = findRow((*pkt)->address.addr64);
    if (row == numberNodes) fprintf(fp,"Node Unknown ");
    else fprintf(fp,"Node %s ", nodeInfo[row].nodeIdent);
    fprintf(fp," %s ", outstr);
    fprintf(fp, "I/O ");
/* Compute the digital data fields from the response. Check the mask and
write the next word field directly as the set of digitial ports. */
    int index = 4;
    if ((((*pkt)->data[1] << 8) + (*pkt)->data[2]) > 0)
    {
        fprintf(fp, "Digital Mask %04X Ports %04X ",(((*pkt)->data[1] << 8) + (*pkt)->data[2]),
                                                    (((*pkt)->data[4] << 8) + (*pkt)->data[5]));
        index += 2;
    }
/* Compute the analogue data fields from the response */
    int aBitMask = 0;
    for (int i=0; i<4; i++)
    {
        if (((*pkt)->data[3] & (1<<(aBitMask++))) > 0)
        {
            fprintf(fp, "A%01d ",i);
            fprintf(fp, "%04d ", ((*pkt)->data[index] << 8) + (*pkt)->data[index+1]);
            index += 2;
        }
    }
/* Write to the disk now to ensure it is available */
    dataFileCheck();
#ifdef DEBUG
    if (row == numberNodes) printf("Node Unknown ");
    else printf("Node %s ", nodeInfo[row].nodeIdent);
    printf(" %s ", outstr);
    printf("I/O ");
/* Compute the analogue data fields from the response */
    int jindex = 4;
    int xBitMask = 0;
    for (int i=0; i<4; i++)
    {
        if (((*pkt)->data[3] & (1<<(xBitMask++))) > 0)
        {
            printf("A%01d ",i);
            printf("%04d ", ((*pkt)->data[jindex] << 8) + (*pkt)->data[jindex+1]);
            jindex += 2;
        }
    }
    printf("\n");
#endif
/* If we are hearing from this then it is a valid node */
    if (row < numberNodes) nodeInfo[row].valid = TRUE;
}

/*-----------------------------------------------------------------------------*/
/** @brief Check the data file usage.

If enough data has been written to the file, flush the buffers. When the file limit
has been reached, close and reopen another file.

The file descriptor fp is global, as are the record counters.
*/

int dataFileCheck()
{
    if (flushCount++ > FLUSH_LIMIT)
    {
        flushCount = 0;
        fflush(fp);
    }
    if (fileCount++ > FILE_LIMIT)
    {
        fileCount = 0;
        fclose(fp);
        fp = NULL;
    }
/* Initialise if the file descriptor hasn't been set or file was closed. */
    if (fp == NULL)
    {
        mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH);
        time_t now;
        now = time(NULL);
        char time_string[20];
        char str[48];
/* Current time set as day of year, hour, minute, second */
        strftime(time_string, sizeof(time_string), "%FT%H-%M-%S", localtime(&now));
/* Create the file and add the current time if possible */
        strcpy(str,dirname);
        strcat(str,"results-");
        if (time_string != NULL) strcat(str,time_string);
        strcat(str,".dat");
#ifdef DEBUG
        printf("New results file created: %s\n",str);
#endif
        fp = fopen(str, "a");
        if (fp == NULL)
        {
            syslog(LOG_INFO, "Cannot open new results file\n");
            return FALSE;
        }
        flushCount = 0;
        fileCount = 0;
    }

    return TRUE;
}
/*-----------------------------------------------------------------------------*/
/** @brief Read a hex field from the config file.

This assumes the exact file format. No checks are made for bad file contents.
Blanks are skipped, and the next field is read in entirety and converted to int.
The process stops when blank or EOL is found.

The file descriptor fpd is global, as are the record counters.
*/

int readConfigFileHex()
{
    char ch;
/* Skip blanks */
    while(1)
    {
        ch = fgetc(fpd);
        if (ch != ' ') break;
    }
//    printf("Character %c\n",ch);
    int value = 0;
    while(1)
    {
        value = (value << 4) + (ch - ((ch > '9') ? ('A'-10) : '0'));
        ch = fgetc(fpd);
        if ((ch == ' ') || (ch == EOF) || (ch == '\n'))
        {
            ungetc(ch,fpd);     /* Leave it there for further EOF tests */
            break;
        }
    }
    return value;
}

/*-----------------------------------------------------------------------------*/
/** @brief Read or Setup a Configuration File and fill node table if present.

The file descriptor fpd is global.
*/

int configFillNodeTable()
{
/* Initialise if the file descriptor hasn't been set or file was closed. */
    if (fpd == NULL)
    {
/* Check if directory present and make it */
        mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH);
/* Open the file for read and append, or create it if not present. */
        char filename[32];
        strcpy(filename,dirname);
        strcat(filename,"xbee-list.cfg");
        fpd = fopen(filename, "a+");
        if (fpd == NULL)
        {
            syslog(LOG_INFO, "Cannot open or create configuration file\n");
            return FALSE;
        }
    }
/* Start reading the file and fill the node table */
    numberNodes = 0;
    rewind(fpd);
    while(1)
    {
/* Skip leading white space to possible EOF (quit). */
        while(1)
        {
            char ch = fgetc(fpd);
            if (ch == EOF) return TRUE;
            if ((ch != ' ') && (ch != '\n'))
            {
                ungetc(ch,fpd);     /* Put character back for reading again */
                break;
            }
        }
        if (numberNodes > MAXNODES) break;
        nodeInfo[numberNodes].adr = readConfigFileHex();
        nodeInfo[numberNodes].SH = readConfigFileHex();
        nodeInfo[numberNodes].SL = readConfigFileHex();
/* Read Node identifier from first non blank up to next blank. */
        char ch;
        while((ch = fgetc(fpd)) == ' '); /* Skip leading blanks */
        int i = 0;
        while (ch != ' ')
        {
            nodeInfo[numberNodes].nodeIdent[i++] = ch;
            ch = fgetc(fpd);
        }
        nodeInfo[numberNodes].nodeIdent[i] = '\0'; /* terminate in null character */
        nodeInfo[numberNodes].parentAdr = readConfigFileHex();
        nodeInfo[numberNodes].deviceType = readConfigFileHex();
        nodeInfo[numberNodes].status = readConfigFileHex();
        nodeInfo[numberNodes].profileID = readConfigFileHex();
        nodeInfo[numberNodes].manufacturerID = readConfigFileHex();
        nodeInfo[numberNodes].valid = FALSE;
        nodeInfo[numberNodes].dataCon = NULL;
        nodeInfo[numberNodes].ioCon = NULL;
        nodeInfo[numberNodes].atCon = NULL;
        numberNodes++;
/* Skip all trailing rubbish to EOL (next entry) or EOF (quit). */
        while(1)
        {
            char ch = fgetc(fpd);
            if (ch == EOF) return TRUE;
            if (ch == '\n') break;
        }
    }
    return TRUE;
}


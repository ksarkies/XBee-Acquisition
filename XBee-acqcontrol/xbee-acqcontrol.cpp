/**
@mainpage XBee Acquisition Control Process
@version 1.2
@author Ken Sarkies (www.jiggerjuice.net)
@date 10 January 2013
@date 28 March 2017

This program is intended to run as a background process in a Unix environment
to interface to a local XBee coordinator and a network of XBee router/end
devices. Its purpose is to collect data from the networked devices and pass
it on to external storage with timestamping. An interface to an external GUI
program is provided using internet sockets for the purpose of status display,
configuration and firmware update.

The program starts by querying all nodes on the network and building a table
of information for each one, including libxbee connections to allow reception
of data streams from the device.

End devices may be sleeping when this occurs. To ensure they are recognised
start these up after acqcontrol has been started. The NodeID packet sent
will contain the correct information for the session.

The program also sets up an Internet TCP port 58532 for connection externally
by a control program. Refer to the command handler function for the commands
passed on that interface.

@note
The program uses libxbee3 by Attie Grande
http://attie.co.uk/libxbee3

@note
May be portable to Windows and OS X, if anyone is so crazy.

@note
Compiler: gcc 4.8.2
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

#include "xbee.h"
#include "xbee-acqcontrol.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>

#define TIMEOUT 5

/* Global Structures and Data */
struct xbee *xbee;
struct xbee_con *localATCon;    /* Connection for local AT commands */
struct xbee_con *identifyCon;   /* Connection for identify packets */
struct xbee_con *modemStatusCon; /* Connection for modem status packets */
struct xbee_con *txStatusCon;   /* Connection for transmit status packets */
int numberNodes;
nodeEntry nodeInfo[MAXNODES];   /* Allows up to 25 nodes */
char remoteData[SIZE][MAXNODES];/* Temporary Data store. */
unsigned long int dataField = 0;/* Temporary store for processed data word. */
bool dataResponseRcvd;          /* Signal response to an AVR command received */
char dataResponseData[SIZE];    /* The data record received with a response */
bool remoteATResponseRcvd;      /* Signal response to a remote AT command received */
uint8_t remoteATLength;
char remoteATResponseData[SIZE];/* Data record received with remote AT response */
bool localATResponseRcvd;       /* Signal response to a local AT command received */
uint8_t localATLength;
char localATResponseData[SIZE]; /* The data record received with a local AT response */
FILE *fp;                       /* File for results */
FILE *fpd;                      /* File for XBee remode node table */
FILE *log;                      /* File for libxbee logging */
int flushCount;                 /* Number of records to flush buffers to disk. */
int fileCount;                  /* Number of records close file and open new. */
char dirname[40];
char inPort[40];
uint baudrate;
char debug;
bool xbeeLogging;
int xbeeLogLevel;

/* Callback Prototypes for libxbee */
void nodeIDCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);
void dataCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);
void remoteATCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);
void ioCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);
void txStatusCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);
void localATCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);
void modemStatusCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data);

/* Local Prototypes */
int min(int x, int y) {if (x>y) return y; else return x;}
int findRowBy64BitAddress(unsigned char *addr);
int findRowBy16BitAddress(uint16_t addr);
void debugDumpNodeTable(void);
void debugDumpPacket(struct xbee_pkt **pkt);
void printNodeID(struct xbee_pkt **pkt);
void printRemoteATResponse(struct xbee_pkt **pkt);
void printLocalATResponse(struct xbee_pkt **pkt);
void printTxStatus(struct xbee_pkt **pkt);
void printModemStatus(struct xbee_pkt **pkt);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/** @brief XBee Acquisition Control Main Program

*/

int main(int argc,char ** argv)
{
    debug = 0;                  /* no debug printout by default */
    xbeeLogging = false;
/*--------------------------------------------------------------------------*/
/* Parse the command line arguments.
P - serial port to use, (default /dev/ttyUSB0)
b - baud rate, (default 38400 baud)
D - directory for results file (default /data/XBee/)
e - enhanced debug mode 0=none, 1=basic, 2=enhanced.
d - basic debug mode 1.
 */
    strcpy(inPort,SERIAL_PORT);
    baudrate = BAUDRATE;
    strcpy(dirname,DATA_PATH);

    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, "P:b:D:L:de:")) != -1)
    {
        switch (c)
        {
        case 'D':
            strcpy(dirname,optarg);
            break;
        case 'e':
            debug = atoi(optarg);
            break;
        case 'd':
            debug = 1;
            break;
        case 'P':
            strcpy(inPort,optarg);
        case 'L':
            xbeeLogging = true;
            xbeeLogLevel = atoi(optarg);
            break;
        case 'b':
            baudrate = atoi(optarg);
            switch (baudrate)
            {
            case 2400:  break;
            case 4800:  break;
            case 9600:  break;
            case 19200:  break;
            case 38400:  break;
            case 57600:  break;
            case 115200:  break;
            default:
                fprintf (stderr, "Invalid Baudrate %i.\n", baudrate);
                return false;
            }
            break;
        case '?':
            if ((optopt == 'P') || (optopt == 'b') || (optopt == 'D'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
        }
    }
    if (dirname[strlen(dirname)-1] != '/') dirname[strlen(dirname)] = '/';

/*--------------------------------------------------------------------------*/
/* A bit of logging stuff.*/

#ifdef DEBUG
    if (debug) printf("XBee Acquisition Control version %s\n",VERSION);
#endif

/* Initialise the results storage. Abort the program if this fails. */

    fp = NULL;
    if (! dataFileCheck())
    {
        closelog();
        return 1;
    }

    fprintf(fp,"XBee Acquisition Control version %s\n",VERSION);

/* Update the rsyslog.conf files to link local7 to a
log file */

    openlog("xbee_acqcontrol", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL7);

/* Setup XBee only logging. */

#ifdef DEBUG
    if (debug && xbeeLogging)
    {
        if ((log = fopen(LOG_FILE, "w")) == NULL)
            printf("Unable to open logging file %s\n", LOG_FILE);
        else
/* Set logging for receive and transmit operations */
        {
            xbee_logTargetSet(xbee,log);
            xbee_logLevelSet(xbee, xbeeLogLevel);
            xbee_logRxSet(xbee,true);
            xbee_logTxSet(xbee,true);
        }
#endif

/*--------------------------------------------------------------------------*/
/* Initialise the libxbee instance. If failed to contact XBee, abort. */

    syslog(LOG_INFO, "Starting XBee Instance\n");
#ifdef DEBUG
    if (debug)
        printf("Starting XBee Instance\n");
#endif
    if (setupXbeeInstance())
    {
        closelog();
        return 1;
    }

#ifdef DEBUG
    if (debug)
        printf("XBee Instance Started\n");
#endif
    }
/*--------------------------------------------------------------------------*/
/* Initialise the node file and fill the node table. */

    fpd = NULL;
    if (! fillNodeTable())
    {
        closelog();
        return 1;
    }
    debugDumpNodeTable();

/*--------------------------------------------------------------------------*/
/* Create the necessary connections locally and globally and poll for existing
and new remote XBee nodes */

    openRemoteConnections();
    nodeProbe();
    openGlobalConnections();
#ifdef DEBUG
    if (debug)
        printf("Connections probed and opened\n");
#endif

/*--------------------------------------------------------------------------*/
/* Setup the Internet command interface socket. */

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

/*--------------------------------------------------------------------------*/
/* Main loop. This handles only the Internet interface. The remote node
interface is handled in callback functions via libxbee. */

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

/*--------------------------------------------------------------------------*/
/* Close up and quit (don't quite know how we would end up here!) */

    closeGlobalConnections();
    closeRemoteConnections();
    xbee_shutdown(xbee);
    closelog();
    return 0;
}

/*--------------------------------------------------------------------------*/
/** @brief Handle command interface for the Internet client

This receives a line of command from the client, processes it and returns any
relevant data to the external client.

The message received from the client must have the length of the message, the
command and the row number to refer to the node to be addressed. The latter will
be omitted if the command does not refer to a node.

The information passed back consists of the length of the response message,
the original command, a status byte and any data. The status byte will indicate
any error conditions, generally from the response of the libxbee Tx call.

The incoming data buffer is assumed to have all the message data, that is,
we do not wait for anything lagging behind.

L send a local AT command to the attached coordinator XBee.
l check for a response to a previously sent local AT command.
R send a remote AT command to the selected node. The command follows in the
   data field.
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

Responses to local and remote AT commands are stored in separate global
variables. These must be read before any further commands are sent to any node,
including the local coordinator XBee.

Three commands are used to check for a response to a previously send command.
These are returned with the first character an echo of the previous command
character, followed by any parameters. if the command echo is a null, then
there was no response received at that time.

@parameter  int listener: file handler for the socket created.
@parameter  unsigned char * buf: A buffer [256] with the received data.
@returns:   true if no error
*/
bool command_handler(const int listener, unsigned char *buf)
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
    if ((command != 'r') && (command != 'l') && (command != 's') && debug)
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
/* Send a local AT command to a node on its established connection.
Use the libxbee connTx command as there may be zeros which would be
misinterpreted as end of string. */
        case 'L':
            for (j=0; j<commandLength-3; j++) str[j] = buf[j+3];
#ifdef DEBUG
            if (debug)
            {
                printf("Local AT Command sent: ");
                for (j=0; j<commandLength-3; j++) printf("%02X ",str[j]);
                printf("\n");
            }
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

/* Send a remote AT command to a node on its established  connection.
Use the libxbee connTx command as there may be zeros which would be
misinterpreted as end of string. */
        case 'R':
            for (j=0; j<commandLength-3; j++) str[j] = buf[j+3];
#ifdef DEBUG
            if (debug)
            {
                printf("Remote AT Command sent: %c%c ", str[0], str[1]);
                for (j=2; j<commandLength-3; j++) printf("%02X ",str[j]);
                printf("\n");
            }
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

/* Send an ASCII string to a remote node on its established data connection.
This will be the basic command interface between the Internet client and the
AVR. The string contains the Parameter Change command P followed by the string
to send to the AVR. */
        case 'S':
            str[strLength++] = 'P';
            while (buf[strLength+2] > 0)
            {
                str[strLength] = buf[strLength+3];
                strLength++;
            }
#ifdef DEBUG
            if (debug)
            {
                printf("Data String sent: ");
                for (j=0; j<strLength; j++) printf("%c",str[j]);
                printf("\n");
            }
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

/* Reset the connections for the selected node. */
        case 'Q':
            replyLength = 3;
            reply[2] = 'Q';
            closeRemoteConnection(row);
#ifdef DEBUG
            if (debug)
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
            deleteNodeTableRow(row);
            break;
    }
    reply[0] = replyLength;
    reply[1] = command;
#ifdef DEBUG
    if ((command != 'r') && (reply[2] != 0) && debug)
    {
        printf("Reply sent to GUI %02X %c ", reply[0], reply[1]);
        for (int i=2; i<replyLength; i++) printf("%02X ", reply[i]);
        printf("\n");
    }
#endif
/* Can't do much if not all sent or error. Client just has to lump it  */
    return (send(listener, reply, replyLength, 0) != replyLength);
}

/*--------------------------------------------------------------------------*/
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

/*--------------------------------------------------------------------------*/
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

/*--------------------------------------------------------------------------*/
/** @brief Check for Internet connections

This checks for any incoming Internet connections and calls a command handler
with the data received.

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
                if ((newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen))
                          == -1) return 2;
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
/* got error or connection was closed by client. Remove from the list (in case
of error just let client die as we are running as a background process) */
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

/*--------------------------------------------------------------------------*/
/** @brief Create the XBee instance

The libxbee instance is created for an attached XBee, along with a local AT
command interface temporarily for checking that the XBee is working. This is a
requirement of libxbee that allows for more than one XBee network to be managed.

Globals:
xbee instance

@returns    libxbee error
*/
int setupXbeeInstance()
{
    xbee_err ret;
    unsigned char txRet;

/* Setup the XBee instance for the XBee ZB with given device and baudrate
(from command line).
Use Unix stat to check if the serial port is available. This is not guaranteed
to be the one actually allocated to the XBee. Abort if the attempted setup
fails. */
    struct stat st;
    if(stat(inPort,&st) == 0)           /* Check if serial port exists */
    {
        ret = xbee_setup(&xbee, "xbeeZB", inPort, baudrate);
        if (ret != XBEE_ENONE)
        {
            syslog(LOG_INFO, "Unable to open XBee port: %d (%s)", ret,
                   xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Unable to open XBee port: %d (%s)\n", ret,
                    xbee_errorToStr(ret));
#endif
            return ret;
        }
    }
    else
    {
        syslog(LOG_INFO, "Serial Port %s not available.\n", inPort);
#ifdef DEBUG
        if (debug)
            printf("Serial Port %s not available.\n", inPort);
#endif
        return -1;
    }
    
/* Setup a connection for local AT commands. */
    ret = xbee_conNew(xbee, &localATCon, "Local AT", NULL);
    if (ret != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Local AT connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Local AT connection failed: %d (%s)\n", ret,
                    xbee_errorToStr(ret));
#endif
        return ret;
    }
/* Attempt to send an AP command to change the coordinator to mode 1 (in case
it was set to mode 2). This also serves as a test of the XBee interface.
For some reason the XBee needs a command like this before sending out the ND,
otherwise it doesn't pick up the responses.
Try several times if a timeout occurs. */
    unsigned char timeCounter = TIMEOUT;
    while ((ret == XBEE_ENONE) && timeCounter--)
        ret = xbee_conTx(localATCon, &txRet, "AP\x01");
    if (ret != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Unable to change mode of XBee: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Unable to change mode of XBee: %d (%s)\n", ret,
                    xbee_errorToStr(ret));
#endif
    }
/* Close off the local AT connection to destroy unwanted responses that
could get mixed up with the ND responses. */
    xbee_conEnd(localATCon);
    return ret;
}

/*--------------------------------------------------------------------------*/
/** @brief Open a Remote Node connection

The entry to the node table structure is set as valid and a set of xbee
connections is built if these are not already present.

Globals:
xbee instance
nodeInfo: node information array

@parameter  int node. The table node entry to be set.
*/

void openRemoteConnection(int node)
{
/* Setup an address to build connections for incoming data, remote AT, I/O
and transmit status frames */
//    struct xbee_conSettings settings;
    xbee_err ret;
    struct xbee_conAddress address;
    memset(&address, 0, sizeof(address));
    address.addr64_enabled = 1;
    address.addr64[0] = (nodeInfo[node].SH >> 24) & 0xFF;
    address.addr64[1] = (nodeInfo[node].SH >> 16) & 0xFF;
    address.addr64[2] = (nodeInfo[node].SH >> 8) & 0xFF;
    address.addr64[3] = (nodeInfo[node].SH & 0xFF);
    address.addr64[4] = (nodeInfo[node].SL >> 24) & 0xFF;
    address.addr64[5] = (nodeInfo[node].SL >> 16) & 0xFF;
    address.addr64[6] = (nodeInfo[node].SL >> 8) & 0xFF;
    address.addr64[7] = (nodeInfo[node].SL & 0xFF);
    if (nodeInfo[node].dataCon == NULL)
    {
        if (((ret = xbee_conNew(xbee, &nodeInfo[node].dataCon, "Data", &address))
                    != XBEE_ENONE) ||
            ((ret = xbee_conCallbackSet(nodeInfo[node].dataCon, dataCallback, NULL))
                    != XBEE_ENONE))
        {
            nodeInfo[node].dataCon = NULL;
            syslog(LOG_INFO, "Unable to create Data connection for node %d, %s\n",
                node,xbee_errorToStr(ret));
#ifdef DEBUG
            if (debug)
                printf("Unable to create Data connection for node %d, %s\n",
                    node,xbee_errorToStr(ret));
#endif
        }
//        xbee_conSettings(nodeInfo[node].dataCon, NULL, &settings);
//        printf("Data Connection Made %d\n", settings.disableAck);
//        xbee_conSettings(nodeInfo[node].dataCon, &settings, NULL);
    }
    if (nodeInfo[node].atCon == NULL)
    {
        if (((ret = xbee_conNew(xbee, &nodeInfo[node].atCon, "Remote AT", &address))
                    != XBEE_ENONE) ||
            ((ret = xbee_conCallbackSet(nodeInfo[node].atCon, remoteATCallback, NULL))
                    != XBEE_ENONE))
        {
            syslog(LOG_INFO, "Unable to create Remote AT connection for node %d, %s\n",
                node,xbee_errorToStr(ret));
#ifdef DEBUG
            if (debug)
                printf("Unable to create Remote AT connection for node %d, %s\n",
                    node,xbee_errorToStr(ret));
#endif
            nodeInfo[node].atCon = NULL;
        }
    }
    if (nodeInfo[node].ioCon == NULL)
    {
        if (((ret = xbee_conNew(xbee, &nodeInfo[node].ioCon, "I/O", &address))
                    != XBEE_ENONE) ||
            ((ret = xbee_conCallbackSet(nodeInfo[node].ioCon, ioCallback, NULL))
                    != XBEE_ENONE))
        {
            syslog(LOG_INFO, "Unable to create I/O connection for node %d, %s\n",
                node,xbee_errorToStr(ret));
#ifdef DEBUG
            if (debug)
                printf("Unable to create I/O connection for node %d, %s\n",
                    node,xbee_errorToStr(ret));
#endif
            nodeInfo[node].ioCon = NULL;
        }
    }
}

/*--------------------------------------------------------------------------*/
/** @brief Open all remote node connections in the node information table

Globals:
xbee instance
nodeInfo: node information table
numberNodes:
*/

void openRemoteConnections()
{
    for (int node=0; node<numberNodes; node++)
    {
        openRemoteConnection(node);
    }
    return;
}

/*--------------------------------------------------------------------------*/
/** @brief Close down a single remote node connection

If the connection address is present in the table, set to NULL and terminate the
associated libxbee connection. This is done silently.

Globals:
xbee instance
nodeInfo: node information array

@parameter  int node. The table node entry to be invalidated.
*/

void closeRemoteConnection(int node)
{
    if (nodeInfo[node].dataCon != NULL) xbee_conEnd(nodeInfo[node].dataCon);
    nodeInfo[node].dataCon = NULL;
    if (nodeInfo[node].atCon != NULL) xbee_conEnd(nodeInfo[node].atCon);
    nodeInfo[node].atCon = NULL;
    if (nodeInfo[node].ioCon != NULL) xbee_conEnd(nodeInfo[node].ioCon);
    nodeInfo[node].ioCon = NULL;
    nodeInfo[node].valid = false;
}

/*--------------------------------------------------------------------------*/
/** @brief Close down all remote node connections

Globals:
xbee instance
nodeInfo: node information array
numberNodes
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

/*--------------------------------------------------------------------------*/
/** @brief Open all long-running connections for the coordinator XBee.

Connections for local AT response, modem status and identify are opened.

@returns    libxbee error value
*/

int openGlobalConnections()
{
    xbee_err ret;

/* Set the local AT response connection */
    xbee_conEnd(localATCon);       /* terminate if it happens to be open */
    if ((ret = xbee_conNew(xbee, &localATCon, "Local AT", NULL)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Local AT connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Local AT connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
        return ret;
    }
#ifdef DEBUG
    if (debug) printf("Local AT Connection OK\n");
#endif
/* Set the callback for the local AT response. */
    if ((ret = xbee_conCallbackSet(localATCon, localATCallback, NULL))
            != XBEE_ENONE)
    {
        xbee_conEnd(localATCon);
        syslog(LOG_INFO, "Local AT Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Local AT Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
        return ret;
    }
#ifdef DEBUG
    if (debug) printf("Local AT Callback OK\n");
#endif

/* Set the Modem Status Connection */
    xbee_conEnd(modemStatusCon);   /* terminate if it happens to be open */
    if ((ret = xbee_conNew(xbee, &modemStatusCon, "Modem Status", NULL))
                != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Modem Status connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Modem Status Connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
        return ret;
    }
#ifdef DEBUG
    if (debug) printf("Modem Status Connection OK\n");
#endif
/* Set the callback for the Modem Status response. */
    if ((ret = xbee_conCallbackSet(modemStatusCon, modemStatusCallback, NULL))
                != XBEE_ENONE)
    {
        xbee_conEnd(modemStatusCon);
        syslog(LOG_INFO, "Modem Status Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Modem Status Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
        return ret;
    }
#ifdef DEBUG
    if (debug) printf("Modem Status Callback OK\n");
#endif

/* Set the Identify packets connection. */
    xbee_conEnd(identifyCon);      /* terminate if it happens to be open */
    if ((ret = xbee_conNew(xbee, &identifyCon, "Identify", NULL)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Identify Connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Identify Connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
        return ret;
    }
#ifdef DEBUG
    if (debug) printf("Identify Connection OK\n");
#endif
/* Set the callback for the identify response. */
    if ((ret = xbee_conCallbackSet(identifyCon, nodeIDCallback, NULL))
            != XBEE_ENONE)
    {
        xbee_conEnd(identifyCon);
        syslog(LOG_INFO, "Identify Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Identify Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
    }
#ifdef DEBUG
    if (debug) printf("Identify Callback OK\n");
#endif

/* Set the Transmit Status Connection. */
    xbee_conEnd(txStatusCon);      /* terminate if it happens to be open */
    if ((ret = xbee_conNew(xbee, &txStatusCon, "Transmit Status", NULL)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Transmit Status Connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Transmit Status Connection failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
        return ret;
    }
#ifdef DEBUG
    if (debug) printf("Transmit Status Connection OK\n");
#endif
/* Set the callback for the Transmit Status response. */
    if ((ret = xbee_conCallbackSet(txStatusCon, txStatusCallback, NULL))
            != XBEE_ENONE)
    {
        xbee_conEnd(txStatusCon);
        syslog(LOG_INFO, "Transmit Status Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Transmit Status Callback Set failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
    }
#ifdef DEBUG
    if (debug) printf("Transmit Status Callback OK\n");
#endif

    return ret;
}

/*--------------------------------------------------------------------------*/
/** @brief Close all long-running connections

Connections for local AT and identify are closed.

@returns    libxbee error value
*/

int closeGlobalConnections()
{
    xbee_err ret;

    if ((ret = xbee_conEnd(localATCon)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Could not close local AT connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Could not close local AT connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#endif
        return ret;
    }
    if ((ret = xbee_conEnd(modemStatusCon)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Could not close modem status connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Could not close modem status connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#endif
    }
    if ((ret = xbee_conEnd(identifyCon)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Could not close identify connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Could not close identify connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#endif
    }
    if ((ret = xbee_conEnd(txStatusCon)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Could not close transmit status connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Could not close transmit status connection %d (%s)", ret,
                   xbee_errorToStr(ret));
#endif
    }
    return ret;
}

/*--------------------------------------------------------------------------*/
/** @brief Probe for all nodes on the network

Send out a node discover signal and build the table of nodes. A local AT
connection is used.

Globals:
xbee instance
nodeInfo node information array

@returns    libxbee error value
*/

int nodeProbe()
{
    xbee_err ret;
    unsigned char txRet;
    int sleeping = false;
    struct xbee_con *probeATCon;
/* Check if the local AT connection is open and put it to sleep */
    if (xbee_conValidate(localATCon) == XBEE_ENONE)
    {
        if ((ret = xbee_conSleepSet(localATCon, CON_SLEEP)) != XBEE_ENONE)
        {
            syslog(LOG_INFO, "Cannot sleep local connection: %d (%s)", ret,
                   xbee_errorToStr(ret));
#ifdef DEBUG
            if (debug)
                printf("Cannot sleep local connection: %d (%s)", ret,
                   xbee_errorToStr(ret));
#endif
            return ret;
        }
        sleeping = true;
    }

/* Open a local AT connection without callback. */
    if ((ret = xbee_conNew(xbee, &probeATCon, "Local AT", NULL)) != XBEE_ENONE)
    {
        syslog(LOG_INFO, "Local AT connection for node probe failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("Local AT connection for node probe failed: %d (%s)", ret,
               xbee_errorToStr(ret));
#endif
    }
    else
    {
        ret = xbee_conTx(probeATCon, &txRet, "ND");
        syslog(LOG_INFO, "ND node probe sent: Tx return code: %d error: (%s)",
               txRet, xbee_errorToStr(ret));
#ifdef DEBUG
        if (debug)
            printf("ND node probe sent: Tx return code: %d error: (%s)\n",
               txRet, xbee_errorToStr(ret));
#endif
        if ((ret != XBEE_ENONE) && (ret != XBEE_ETIMEOUT))
        {
            syslog(LOG_INFO, "ND node probe Tx failed: %s",
                   xbee_errorToStr(ret));
#ifdef DEBUG
            if (debug)
                printf("ND node probe Tx failed: %s\n",
                   xbee_errorToStr(ret));
        }
#endif
        else
        {
/* Timeout always seems to occur so we won't act on this */
            if (ret == XBEE_ETIMEOUT)
                syslog(LOG_INFO, "ND probe Tx timeout: Tx return code: %d (%s)", txRet,
                       xbee_errorToStr(ret));
/* For this particular command callbacks have not been used and the Rx is
polled for all responding nodes. This is because we need to process it as a
Node Identification message but it is an AT Command Response. */
            int rxWait = 20;            /* Wait up to 10sec for response. */
            struct xbee_pkt *pkt;
            int pktRemaining;
            for (int i = 0; i < rxWait; i++)
            {
                if ((ret = xbee_conRx(probeATCon, &pkt, &pktRemaining))
                    != XBEE_ENOTEXISTS)
                {
                    nodeIDCallback(xbee, probeATCon, &pkt, NULL);
#ifdef DEBUG
                    if (debug)
                        printf("Node found %d return %d (%s)\n",i,ret,
                                xbee_errorToStr(ret));
#endif
                }
                usleep(500000);
            }
            xbee_pktFree(pkt);
#ifdef DEBUG
            if (debug)
            {
                int i;
                for (i=0; i<numberNodes; i++)
                {
                    printf("Node ID %s Address %4X",
                           nodeInfo[i].nodeIdent,nodeInfo[i].adr);
                    if (!nodeInfo[i].valid) printf(" not");
                    printf(" valid\n");
                }
            }
#endif
        }
/* Close off local AT connection */
        ret = xbee_conEnd(probeATCon);
        if (ret != XBEE_ENONE)
        {
            syslog(LOG_INFO, "Local AT connection exit for node probe failed: %d (%s)",
                    ret,xbee_errorToStr(ret));
#ifdef DEBUG
            if (debug)
                printf("Local AT connection exit for node probe failed: %d (%s)\n",
                    ret,xbee_errorToStr(ret));
#endif
        }
    }
    if (! sleeping) xbee_conSleepSet(localATCon, CON_AWAKE);
#ifdef DEBUG
    if (debug)
        printf("ND node probe complete\n");
#endif
    return ret;
}

/*--------------------------------------------------------------------------*/
/* libxbee CALLBACKS */
/*--------------------------------------------------------------------------*/
/** @brief Callback for the ND and Identify command, to load up the node list.

This is a callback required by libxbee. It operates on the global
datastructure that holds all information for each node returned by the ND
command. It is invoked when an identify packet is received.

This checks for existence of the node information and adds a new node if not
present. Data, I/O and AT command and Transmit Status connections are added.

Although there may be incomplete or erroneous entries, we may leave them and
check later by other means to attempt to correct the problem.

If a device already has an entry, its remote connections are closed and
reopened.

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void nodeIDCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    if (debug) printNodeID(pkt);
#endif
/* Check if the serial number already exists. If not, then it is a new node. */
    int node = findRowBy64BitAddress((*pkt)->data+2);
    if (node > MAXNODES)
    {
#ifdef DEBUG
        if (debug) printf("Node table Full.\n");
#endif
        return;
    }
/* Fill in or refresh the info fields if there is room in the table. */
    nodeInfo[node].adr = ((*pkt)->data[0] << 8) + (*pkt)->data[1];
    nodeInfo[node].SH = ((*pkt)->data[2] << 24) + ((*pkt)->data[3] << 16) + 
                        ((*pkt)->data[4] << 8) + (*pkt)->data[5];
    nodeInfo[node].SL = ((*pkt)->data[6] << 24) + ((*pkt)->data[7] << 16) + 
                        ((*pkt)->data[8] << 8) + (*pkt)->data[9];
    int i = 10;
    do
    {
        nodeInfo[node].nodeIdent[i-10] = (*pkt)->data[i];
        i++;
    }
    while (nodeInfo[node].nodeIdent[i-11] != 0);
    uint32_t temp = (*pkt)->data[i++];
    nodeInfo[node].parentAdr = (*pkt)->data[i++] + (temp << 8);
    nodeInfo[node].deviceType = (*pkt)->data[i++];
    nodeInfo[node].status = (*pkt)->data[i++];
    temp = (*pkt)->data[i++];
    nodeInfo[node].profileID = (*pkt)->data[i++] + (temp << 8);
    temp = (*pkt)->data[i++];
    nodeInfo[node].manufacturerID = (*pkt)->data[i++] + (temp << 8);
/* Add in a new node at the end of the table */
    if (node == numberNodes)
    {
        numberNodes++;

/* Write new node data to the node file */
        if (fpd != NULL)
        {
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
        }

/* Clear all connection addresses on the new entry to allow them to be created
below. */
        nodeInfo[node].valid = false;
        nodeInfo[node].dataCon = NULL;
        nodeInfo[node].ioCon = NULL;
        nodeInfo[node].atCon = NULL;
    }
/* If an existing entry, close off its remote connections */
    else closeRemoteConnection(node);
        
/* Add or rebuild connections. */
    openRemoteConnection(node);
    nodeInfo[node].valid = true;
}

/*--------------------------------------------------------------------------*/
/** @brief Callback for the data packets sent from the nodes.

This is a callback required by libxbee. It interprets incoming packets on the
remote node data connection. Application data is passed on to an external
file.

Other packets are responses to commands to the node AVR sent over the same
connection. The response is stored in a global array. This should be read
before any other node AVR command is sent to any node.

- 'C' xx xx aa aa … (11 bytes) Original message with application data.
- 'E' xx xx aa aa … (11 bytes) Repeat message in response to reception errors.
- 'S' xx xx aa aa … (11 bytes) Repeat message in response to delivery failure.
- 'N' xx xx aa aa … (11 bytes) Repeat message in response to a NAK.
- 'T' xx xx aa aa … (11 bytes) Repeat message in response to a timeout.
- 'X' (1 byte) Remote will abandon the communication attempt.
- 'A' (1 byte) Remote accepts the communication.
- 'D' aa aa … (unlimited bytes) Debug message.

Refer to the documentation for a description and analysis of the protocol. The
protocol maintains a state across calls to this function so that in the last
stage the probability of lost data is minimized.

TODO The data received is oriented to 32 bit counts. Needs generalization to
other data types.

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void dataCallback(struct xbee *xbee, struct xbee_con *con,
                  struct xbee_pkt **pkt, void **data)
{
    int row = findRowBy64BitAddress((*pkt)->address.addr64);
    char timeString[20];
    time_t now;
    now = time(NULL);
    struct tm *tmp;
    tmp = localtime(&now);
    strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",tmp);
    int writeLength = (*pkt)->dataLen;
    if (writeLength > DATA_BUFFER_SIZE) writeLength = DATA_BUFFER_SIZE;
/* If the node is not recognised, abort processing */
    if (row == numberNodes)
#ifdef DEBUG
    {
        if (debug)
        {
            printf("Node Unknown, unable to process packet.\n");
            printf("Packet Dump:");
            if (fp != NULL)
            {
                fprintf(fp,"Node Unknown, unable to process packet.\n");
                fprintf(fp,"Packet Dump:");
            }
            debugDumpNodeTable();
        }
        debugDumpPacket(pkt);
#endif
        return;
    }
#ifdef DEBUG
    if (debug)
    {
        printf("Node %s Data Packet:", nodeInfo[row].nodeIdent);
        if (fp != NULL)
        {
            fprintf(fp,"Node %s Data Packet:",
                                       nodeInfo[row].nodeIdent);
        }
        debugDumpPacket(pkt);
    }
#endif
/* Add 16 bit address to node table in case it was not already saved. */
    nodeInfo[row].adr = (((uint16_t)(*pkt)->address.addr16[0] << 8)+(*pkt)->address.addr16[1]);
/* Determine if the packet received is a data packet and check for errors.
The command is that sent by the application layer protocol in the remote. */
    char command = (*pkt)->data[0];
    DataError error = none;
    unsigned long int count = 0;
    bool storeData = false;
/* These messages result from initial data transmission and error conditions in
the remote. If the message length is 11, the defined length of a data packet,
then the data will be extracted and set aside for storage.
Error is signalled if length or checksum are wrong. */
    if ((command == 'C') || (command == 'T') || (command == 'N')
                         || (command == 'E') || (command == 'S'))
    {
        nodeInfo[row].protocolState = 1;        /* Start of protocol cycle. */
        if (writeLength != 11) error = badLength;
        if (error == none)
        {
/* Convert hex ASCII checksum and count to an integer from the latter part of
the string */
            for (int i=0; i<DATA_LENGTH-2; i++)
            {
                unsigned int digit=0;
                char hex = (*pkt)->data[i+3];
                if ((hex >= '0') && (hex <= '9')) digit = hex - '0';
                else if ((hex >= 'A') && (hex <= 'F')) digit = hex + 10 - 'A';
                else error = badHex;
                count = (count << 4) + digit;
            }
/* Convert checksum in first part of string */
            unsigned char checksum = 0;
            for (int i=0; i<2; i++)
            {
                unsigned int digit=0;
                char hex = (*pkt)->data[i+1];
                if ((hex >= '0') && (hex <= '9')) digit = hex - '0';
                else if ((hex >= 'A') && (hex <= 'F')) digit = hex + 10 - 'A';
                else error = badHex;
                checksum = (checksum << 4) + digit;
            }
/* Compute checksum of data and add to transmitted checksum */
            checksum += count + (count >> 8) + 
                       (count >> 16) + (count >> 24);
/* Checksum should add up to zero, so send an ACK, otherwise send a NAK */
            if (checksum != 0) error = badChecksum;
        }
        xbee_err txError;
        char ackResponse[3];
        ackResponse[1] = command;
        if (error != none)
        {
#ifdef DEBUG
            if (debug)
            {
                printf("Error detected - sent NAK %s error %d\n",
                        nodeInfo[row].nodeIdent, error);
                if (fp != NULL)
                    fprintf(fp,"Error detected - sent NAK %s error %d\n",
                            nodeInfo[row].nodeIdent, error);
            }
#endif
/* Negative Acknowledge */
            ackResponse[0] = 'N';
            txError = xbee_conTx(con, NULL, ackResponse);
        }
        else
        {
#ifdef DEBUG
            if (debug)
            {
                printf("Sent ACK %s\n",nodeInfo[row].nodeIdent);
                if (fp != NULL) fprintf(fp,"Sent ACK %s\n",nodeInfo[row].nodeIdent);
            }
#endif
/* Store data field aside for later recording. */
            for (int i=0; i<DATA_LENGTH; i++) remoteData[i][row] = (*pkt)->data[i+1];
            dataField = count;
/* Check if there are any pending data transmissions to the remote and send now */
// txError = xbee_conTx(con, NULL, "P");    /* Insert application packet for test */
// usleep(500000);                 /* Insert delay for test */
/* Acknowledge */
            ackResponse[0] = 'A';
            txError = xbee_conTx(con, NULL, ackResponse);
/* Advance the protocol state to indicate acceptance of any response as ACK. */
            nodeInfo[row].protocolState = 2;
        }
#ifdef DEBUG
        if (debug && (txError != XBEE_ENONE))
        {
            printf("Tx Fail %s: %s %s\n",
                    nodeInfo[row].nodeIdent, timeString, xbee_errorToStr(txError));
            if (fp != NULL) fprintf(fp,"Tx Fail %s: %s %s\n",
                        nodeInfo[row].nodeIdent, timeString, xbee_errorToStr(txError));
        }
#endif
    }
/* If the protocol state has reached the final stage, any response apart from
the Parameter Change or data commands is accepted as ACK since this is the
only response possible. The only way now that a cycle can give a wrong
result is if the response was not detected as a data packet. In that case
the remote will clear its data but the base will not record it. */
    else if (nodeInfo[row].protocolState == 2)
    {
        nodeInfo[row].protocolState = 1;         /* Reset protocol state */
#ifdef DEBUG
//        printf("Remote Accepted\n");
#endif
        storeData = true;
    }
/* Abandon the communication and discard the current count value as the remote
has detected ongoing errors and will now not reset its count. */
    else if (command == 'X')
    {
        nodeInfo[row].protocolState = 1;        /* Reset protocol state */
#ifdef DEBUG
        if (debug)
        {
            printf("Remote Abandoned %s\n",nodeInfo[row].nodeIdent);
            if (fp != NULL) fprintf(fp,"Remote Abandoned %s\n",nodeInfo[row].nodeIdent);
        }
#endif
        storeData = false;
    }

/* This is the response to a Parameter Change command which passes an arbitrary
string to the remote node. */
    else if (command == 'P')
    {
        dataResponseRcvd = true;
        for (int i=1; i< min(SIZE,writeLength); i++)
            dataResponseData[i] = (*pkt)->data[i+1];
    }

/* This is a transmission from a simple test firmware that doesn't follow the
protocol but only sends a single transmission. */
    else if (command == 'D')
    {
        storeData = true;
/* Store data field aside for later recording. */
        for (int i=0; i<DATA_LENGTH; i++) remoteData[i][row] = (*pkt)->data[i+1];
    }

/* Print out received data to the file once it is verified. */
#ifdef DEBUG
    if (debug)
    {
        printf("Command Received %c %s %s Count %lu Voltage %f V Parameter %lu\n",
            command, nodeInfo[row].nodeIdent, timeString, (dataField & 0xFFFF),
            (float)((dataField >> 16) & 0x3FF)*0.004799415, (dataField >> 26));
    }
#endif
    if (storeData && (fp != NULL))
    {
        fprintf(fp,"Command Received %c %s %s Count %lu Voltage %f V Parameter %lu\n",
            command, nodeInfo[row].nodeIdent, timeString, (dataField & 0xFFFF),
            (float)((dataField >> 16) & 0x3FF)*0.004799415, (dataField >> 26));
        dataFileCheck();
    }
/* If we are hearing from this then it is a valid node */
    if (row < numberNodes) nodeInfo[row].valid = true;
}

/*--------------------------------------------------------------------------*/
/** @brief Callback for remote AT responses sent from the nodes.

This is a callback required by libxbee. It interprets incoming packets on the
remote AT connection.

The response is stored in a global array. This should be read before any other
remote AT command is sent to any node.

Globals:
bool remoteATResponseRcvd
uint8_t remoteATLength
char remoteATResponseData[SIZE]

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void remoteATCallback(struct xbee *xbee, struct xbee_con *con,
                      struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    if (debug) printRemoteATResponse(pkt);
#endif
    remoteATResponseRcvd = true;
    remoteATLength = (*pkt)->dataLen;
    for (int i=0; i < min(SIZE,remoteATLength); i++)
        remoteATResponseData[i] = (*pkt)->data[i];
}

/*--------------------------------------------------------------------------*/
/** @brief Callback for local AT responses sent from the attached XBee.

This is a callback required by libxbee. It interprets incoming packets on the
remote AT connection.

The response is stored in a global array. This should be read before any other
remote AT command is sent to any node.

Globals:
bool localATResponseRcvd
uint8_t localATLength
char localATResponseData[SIZE]

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void localATCallback(struct xbee *xbee, struct xbee_con *con,
                     struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    if (debug) printLocalATResponse(pkt);
#endif
    localATResponseRcvd = true;
    localATLength = (*pkt)->dataLen;
    for (int i=0; i < min(SIZE,localATLength); i++)
        localATResponseData[i] = (*pkt)->data[i];
}

/*--------------------------------------------------------------------------*/
/** @brief Callback for remote I/O received frames sent from the nodes.

This is a callback required by libxbee. It interprets incoming packets on the
I/O connection for XBees that have been setup to send these.

The I/O frames are values measured on selected ports of the XBee itself. The
XBee must be configured to read these ports and transmit at given intervals.

Received data is simply stored in an external file.
Node information array is global.

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void ioCallback(struct xbee *xbee, struct xbee_con *con,
                struct xbee_pkt **pkt, void **data)
{
    char timeString[20];
    time_t now = time(NULL);
    struct tm *tmp = localtime(&now);
    strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",tmp);
    int row = findRowBy64BitAddress((*pkt)->address.addr64);
    if (fp != NULL)
    {
        if (row == numberNodes) fprintf(fp,"Node Unknown ");
        else fprintf(fp,"Node %s ", nodeInfo[row].nodeIdent);
        fprintf(fp," %s ", timeString);
        fprintf(fp, "I/O ");
    }
#ifdef DEBUG
    if (debug)
    {
        if (row == numberNodes) printf("Node Unknown ");
        else printf("Node %s ", nodeInfo[row].nodeIdent);
        printf(" %s ", timeString);
        printf( "I/O ");
    }
#endif
/* Compute the digital data fields from the response. Check the mask and
write the next word field directly as the set of digital ports. */
    if ((((*pkt)->data[1] << 8) + (*pkt)->data[2]) > 0)
    {
        if (fp != NULL)
        {
            fprintf(fp, "Digital Mask %04X Ports %04X ",
                    (((*pkt)->data[1] << 8) + (*pkt)->data[2]),
                    (((*pkt)->data[4] << 8) + (*pkt)->data[5]));
        }
#ifdef DEBUG
        if (debug)
        {
            printf("Digital Mask %04X Ports %04X ",
                    (((*pkt)->data[1] << 8) + (*pkt)->data[2]),
                    (((*pkt)->data[4] << 8) + (*pkt)->data[5]));
        }
#endif
    }
/* Compute the analogue data fields from the response */
    int index = 6;          /* Index into packet data for analogue fields */
    int aBitMask = 0;
    for (int i=0; i<4; i++)
    {
        if (((*pkt)->data[3] & (1<<(aBitMask++))) > 0)
        {
            if (fp != NULL)
            {
                fprintf(fp, "A%01d ",i);
                fprintf(fp, "%04d ",
                    ((*pkt)->data[index] << 8) + (*pkt)->data[index+1]);
            }
#ifdef DEBUG
            if (debug)
            {
                printf("A%01d ",i);
                printf("%04d ",
                        ((*pkt)->data[index] << 8) + (*pkt)->data[index+1]);
#endif
            }
            index += 2;
        }
    }
    if (fp != NULL) fprintf(fp, "\n");
/* Write to the disk now to ensure it is available */
    dataFileCheck();
#ifdef DEBUG
    if (debug) printf("\n");
#endif
/* We are hearing from this node so it is a valid node */
    if (row < numberNodes) nodeInfo[row].valid = true;
}

/*--------------------------------------------------------------------------*/
/** @brief Callback for the Transmit Status Packet.

This is a callback required by libxbee. It operates on the global
datastructure that holds all information for each node returned by the ND
command. It is invoked when an tx status packet is received.

Simply print out the message if the advanced debug is selected.

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void txStatusCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    if (debug > 1) printTxStatus(pkt);
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Callback for the Modem Status Packet.

This is a callback required by libxbee. It is invoked in response to a certain
number of conditions when a modem status packet is received. It consists of a
single byte.

Simply print out the message.

@param struct xbee *xbee. The XBee instance created in setupXbeeInstance().
@param struct xbee_con *con. Connection (not used).
@param struct xbee_pkt **pkt. Packet from the XBee that invoked this callback.
@param void **data. Data (not used).
*/

void modemStatusCallback(struct xbee *xbee, struct xbee_con *con,
                    struct xbee_pkt **pkt, void **data)
{
#ifdef DEBUG
    if (debug > 1) printModemStatus(pkt);
#endif
}

/*--------------------------------------------------------------------------*/
/* UTILITY FUNCTIONS */
/*--------------------------------------------------------------------------*/
/** @brief Check the data file usage.

If enough data has been written, flush the buffers to the file. When the file
limit has been reached, close and reopen another file.

If the file hasn't been opened, create it.

Globals:
The file descriptor fp and the record counters.

@returns bool: file successfully opened.
*/

int dataFileCheck()
{
    if ((fp != NULL) && (flushCount++ > FLUSH_LIMIT))
    {
        flushCount = 0;
        fflush(fp);
    }
    if ((fp != NULL) && (fileCount++ > FILE_LIMIT))
    {
        fileCount = 0;
        fclose(fp);
        fp = NULL;
    }
/* Create the file if the file descriptor hasn't been set or file was closed. */
    if (fp == NULL)
    {
/* Check if the directory exists, and create it if not */
        struct stat dirStat;
        if (stat(dirname, &dirStat) != 0)
        {
            mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH);
#ifdef DEBUG
            if (debug)
                printf("Storage directory creation: %s\n",strerror(errno));
#endif
        }
        time_t now;
        now = time(NULL);
        char time_string[20];
        char str[48];
/* Current time set as day of year, hour, minute, second */
        strftime(time_string, sizeof(time_string), "%FT%H-%M-%S", localtime(&now));
/* Build the file name and add the current time if possible to make a unique
name. */
        strcpy(str,dirname);
        strcat(str,"results-");
        if (time_string != NULL) strcat(str,time_string);
        strcat(str,".dat");
#ifdef DEBUG
        if (debug) printf("New results file created: %s\n",str);
#endif
        fp = fopen(str, "a");
        if (fp == NULL)
        {
            syslog(LOG_INFO, "Cannot open new results file\n");
            return false;
        }
        flushCount = 0;
        fileCount = 0;
    }

    return true;
}
/*--------------------------------------------------------------------------*/
/** @brief Read a hex field from the Node File.

This reads a single hexadecimal character from the node file,
from the global file descriptor and record counters. It assumes the exact file
format. No checks are made for bad file contents. Blanks are skipped, and the
next field is read in entirely and converted to int. The process stops when
blank or EOL is found.

Globals:
The file descriptor fpd.
The file record counters.

@returns int: interpreted hex integer.
*/

int readNodeFileHex()
{
    int ch;
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

/*--------------------------------------------------------------------------*/
/** @brief Read or Setup a Node File

The node file is created if not present and fill the node table if
present.

Globals:
fpd: The file descriptor.
dirname: The directory for the node file.
nodeInfo: node information table
numberNodes: the number of nodes in the table

@returns bool: Node file was successfully established.
*/

int fillNodeTable()
{
/* Initialise if the file descriptor hasn't been set or file was closed. */
    if (fpd == NULL)
    {
/* Check if directory present and make it */
        struct stat dirStat;
        if (stat(dirname, &dirStat) != 0)
        {
            mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH);
#ifdef DEBUG
            if (debug)
                printf("Storage directory creation: %s\n",strerror(errno));
#endif
        }
/* Open the file for read and append, or create it if not present. */
        char filename[32];
        strcpy(filename,dirname);
        strcat(filename,"xbee-list.cfg");
        fpd = fopen(filename, "a+");
        if (fpd == NULL)
        {
            syslog(LOG_INFO, "Cannot open or create node file");
            return false;
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
            int ch = fgetc(fpd);
            if (ch == EOF) return true;
            if ((ch != ' ') && (ch != '\n'))
            {
                ungetc(ch,fpd);     /* Put character back for reading again */
                break;
            }
        }
        if (numberNodes > MAXNODES) break;
        nodeInfo[numberNodes].adr = readNodeFileHex();
        nodeInfo[numberNodes].SH = readNodeFileHex();
        nodeInfo[numberNodes].SL = readNodeFileHex();
/* Read Node identifier from first non blank up to next blank. */
        int ch;
        while((ch = fgetc(fpd)) == ' '); /* Skip leading blanks */
        int i = 0;
        while (ch != ' ')
        {
            nodeInfo[numberNodes].nodeIdent[i++] = ch;
            ch = fgetc(fpd);
        }
        nodeInfo[numberNodes].nodeIdent[i] = '\0'; /* terminate in null character */
        nodeInfo[numberNodes].parentAdr = readNodeFileHex();
        nodeInfo[numberNodes].deviceType = readNodeFileHex();
        nodeInfo[numberNodes].status = readNodeFileHex();
        nodeInfo[numberNodes].profileID = readNodeFileHex();
        nodeInfo[numberNodes].manufacturerID = readNodeFileHex();
        nodeInfo[numberNodes].valid = false;
        nodeInfo[numberNodes].dataCon = NULL;
        nodeInfo[numberNodes].ioCon = NULL;
        nodeInfo[numberNodes].atCon = NULL;
        numberNodes++;
/* Skip all trailing rubbish to EOL (next entry) or EOF (quit). */
        while(1)
        {
            int ch = fgetc(fpd);
            if (ch == EOF) return true;
            if (ch == '\n') break;
        }
    }
    return true;
}

/*--------------------------------------------------------------------------*/
/** @brief Delete a node from the Node Table

The node file is refreshed.

Globals:
nodeInfo: node information table
numberNodes: the number of nodes in the table

@param[in] int row: the row to be deleted. If greater then the number of nodes,
                    nothing happens.
*/

void deleteNodeTableRow(int row)
{
    if (row > numberNodes) return;
    closeRemoteConnection(row);             /* Close off its connections if any */
    numberNodes--;
    for (int i=row; i<numberNodes; i++)
        nodeInfo[i] = nodeInfo[i+1];
    nodeInfo[numberNodes].dataCon = NULL;
    nodeInfo[numberNodes].ioCon = NULL;
    nodeInfo[numberNodes].atCon = NULL;     /* Nullify defunct row pointers */
    writeNodeFile();
}

/*--------------------------------------------------------------------------*/
/** @brief Write the Node Table to the Node File

Globals:
nodeInfo: node information table
numberNodes: the number of nodes in the table

@param[in] int row: the row to be deleted. If greater then the number of nodes,
                    nothing happens.
*/

void writeNodeFile(void)
{
/* Wipe contents of file and write back node table as is. */
    if (fpd != NULL)
    {
        if (ftruncate(fileno(fpd),0) != 0) return;
        for (int node = 0; node < numberNodes; node++)
        {
    /* Write all node data back to the node file */
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
    }
}

/*--------------------------------------------------------------------------*/
/** @brief Determine the node table row from a received packet 64 bit address.

If the returned value equals the number of rows, then the node is not in the
table.

Globals:
nodeInfo: node information table
numberNodes: the number of nodes in the table

@param unsigned char *addr. Address of the XBee as an 8 element array of bytes.
@returns int row. The row at which the address occurs. If equal to numberNodes,
                    then node is not found.
*/

int findRowBy64BitAddress(unsigned char *addr)
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
            (addr[7] ==  (nodeInfo[row].SL & 0xFF))) break;
    }
    return row;
}

/*--------------------------------------------------------------------------*/
/** @brief Determine the node table row from a received packet 16 bit address.

If the returned value equals the number of rows, then the node is not in the
table.

Note this address varies from session to session and may not match that in the
node file. To ensure a match, start up each node after acqcontrol has been
started.

Globals:
nodeInfo: node information table
numberNodes: the number of nodes in the table

@param uint16_t addr. 16 bit network address of the XBee.
@returns int row. The row at which the address occurs. If equal to numberNodes,
                    then node is not found.
*/

int findRowBy16BitAddress(uint16_t addr)
{
/* Find the node in the table from its serial number */
    int row=0;
    for (; row<numberNodes; row++)
    {
        if (addr == nodeInfo[row].adr) break;
    }
    return row;
}

/*--------------------------------------------------------------------------*/
/* DEBUG PRINT */
/*--------------------------------------------------------------------------*/
/** @brief Print out contents of node table.

This is a debug only function. Only the first four components are printed.
*/

void debugDumpNodeTable(void)
{
#ifdef DEBUG
    if (debug > 1)
    {
        printf("Node table\n");
        printf("Row 16-adr 32-adr     Node Ident\n");
        int row=0;
        for (; row<numberNodes; row++)
        {
            printf(" %2d  %04X %04X%04X %s\n", row, nodeInfo[row].adr, nodeInfo[row].SH,
                   nodeInfo[row].SL, nodeInfo[row].nodeIdent);
        }
    }
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Print out contents of a packet.

This is a debug only function. It displays only the time, addresses and data
portion.
*/

void debugDumpPacket(struct xbee_pkt **pkt)
{
#ifdef DEBUG
    if (debug > 1)
    {
        char timeString[20];
        time_t now;
        now = time(NULL);
        struct tm *tmp;
        tmp = localtime(&now);
        strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",tmp);
        printf(" %s ", timeString);
        printf("A16: %02X%02X ", (*pkt)->address.addr16[0], (*pkt)->address.addr16[1]);
        printf("A64: %02X%02X", (*pkt)->address.addr64[0], (*pkt)->address.addr64[1]);
        printf("%02X%02X", (*pkt)->address.addr64[2], (*pkt)->address.addr64[3]);
        printf("%02X%02X", (*pkt)->address.addr64[4], (*pkt)->address.addr64[5]);
        printf("%02X%02X ", (*pkt)->address.addr64[6], (*pkt)->address.addr64[7]);
        printf("len %d ", (*pkt)->dataLen);
        for (int i=0; i<(*pkt)->dataLen; i++) printf("%c", (*pkt)->data[i]);
        printf("\n");

/* Also print to file */
        if (fp != NULL)
        {
            fprintf(fp," %s ", timeString);
            fprintf(fp,"A16: %02X%02X ", (*pkt)->address.addr16[0], (*pkt)->address.addr16[1]);
            fprintf(fp,"A64: %02X%02X", (*pkt)->address.addr64[0], (*pkt)->address.addr64[1]);
            fprintf(fp,"%02X%02X", (*pkt)->address.addr64[2], (*pkt)->address.addr64[3]);
            fprintf(fp,"%02X%02X", (*pkt)->address.addr64[4], (*pkt)->address.addr64[5]);
            fprintf(fp,"%02X%02X ", (*pkt)->address.addr64[6], (*pkt)->address.addr64[7]);
            fprintf(fp,"len %d ", (*pkt)->dataLen);
            for (int i=0; i<(*pkt)->dataLen; i++) fprintf(fp,"%c", (*pkt)->data[i]);
            fprintf(fp,"\n");
        }
    }
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Print out contents of a Node ID packet.

This is a debug only function. It displays all relevant information in the
packet.
*/

void printNodeID(struct xbee_pkt **pkt)
{
#ifdef DEBUG
    if (debug > 1)
    {
        uint16_t adr = ((*pkt)->data[0] << 8) + (*pkt)->data[1];
        uint32_t SH =  ((*pkt)->data[2] << 24) + ((*pkt)->data[3] << 16) + 
                       ((*pkt)->data[4] << 8) + (*pkt)->data[5];
        uint32_t SL =  ((*pkt)->data[6] << 24) + ((*pkt)->data[7] << 16) + 
                       ((*pkt)->data[8] << 8) + (*pkt)->data[9];
        printf("Identify Packet: length %d", (*pkt)->dataLen);
        printf(", 16 bit address %04X", adr);
        printf(", 64 bit address %08X %08X", SH, SL);
        printf(", ID ");
        if (fp != NULL)
        {
            fprintf(fp,"Identify Packet: length %d", (*pkt)->dataLen);
            fprintf(fp,", 16 bit address %04X", adr);
            fprintf(fp,", 64 bit address %08X %08X", SH, SL);
            fprintf(fp,", ID ");
        }
        int i = 10;
        while ((*pkt)->data[i] > 0) printf("%c", (*pkt)->data[i++]);
        i++;
        unsigned int parent =  (*pkt)->data[i++];
        parent = (parent << 8) +  (*pkt)->data[i++];
        int type = (*pkt)->data[i++];
        printf(", parent address %04X",parent);
        if (type == 0) printf(" Coordinator");
        else if (type == 1) printf(" Router");
        else if (type == 2) printf(" End Device");
        else printf(" Unknown type");
        for (; i<(*pkt)->dataLen; i++) printf(" %02X", (*pkt)->data[i]);
        printf("\n");

        if (fp != NULL)
        {
            int i = 10;
            while ((*pkt)->data[i] > 0) fprintf(fp,"%c", (*pkt)->data[i++]);
            fprintf(fp,", parent address %04X",parent);
            if (type == 0) fprintf(fp," Coordinator");
            else if (type == 1) fprintf(fp," Router");
            else if (type == 2) fprintf(fp," End Device");
            else fprintf(fp," Unknown type");
            for (; i<(*pkt)->dataLen; i++) fprintf(fp," %02X", (*pkt)->data[i]);
            fprintf(fp,"\n");
        }
    }
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Print out contents of a remote AT Response packet.

This is a debug only function. It displays all relevant information in the
packet.
*/

void printRemoteATResponse(struct xbee_pkt **pkt)
{
#ifdef DEBUG
    if (debug > 1)
    {
        printf("Remote AT Response: length %d, data ",(*pkt)->dataLen);
        for (int i=0; i<(*pkt)->dataLen; i++) printf("%02X", (*pkt)->data[i]);
        printf("\n");
        if (fp != NULL)
        {
            fprintf(fp,"Remote AT Response: length %d, data ",(*pkt)->dataLen);
            for (int i=0; i<(*pkt)->dataLen; i++) fprintf(fp,"%02X", (*pkt)->data[i]);
            fprintf(fp,"\n");
        }
    }
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Print out contents of a local AT Response packet.

This is a debug only function. It displays all relevant information in the
packet.
*/

void printLocalATResponse(struct xbee_pkt **pkt)
{
#ifdef DEBUG
    if (debug > 1)
    {
        printf("Local AT Response: length %d, data ",(*pkt)->dataLen);
        for (int i=0; i<(*pkt)->dataLen; i++) printf("%02X", (*pkt)->data[i]);
        printf("\n");
        if (fp != NULL)
        {
            fprintf(fp,"Local AT Response: length %d, data ",(*pkt)->dataLen);
            for (int i=0; i<(*pkt)->dataLen; i++) fprintf(fp,"%02X", (*pkt)->data[i]);
            fprintf(fp,"\n");
        }
    }
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Print out contents of a Transmit Status packet.

This is a debug only function. It displays all relevant information in the
packet.
*/

void printTxStatus(struct xbee_pkt **pkt)
{
#ifdef DEBUG
    if (debug > 1)
    {
        int row = findRowBy16BitAddress(((uint16_t)(*pkt)->data[1] << 8) + (*pkt)->data[2]);
        char timeString[20];
        time_t now;
        now = time(NULL);
        struct tm *tmp;
        tmp = localtime(&now);
        strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",tmp);
        printf("Transmit Status: %s ",timeString);
        printf("Length: %d ",(*pkt)->dataLen);
        printf("FrameID %02X, ", (*pkt)->data[0]);
        printf("16 Bit Address %02X%02X, ", (*pkt)->data[1], (*pkt)->data[2]);
        if (row == numberNodes) printf("Unknown ");
        else printf("%s ", nodeInfo[row].nodeIdent);
        printf("Retry count %d, ", (*pkt)->data[3]);
        printf("Delivery status %02X, ", (*pkt)->data[4]);
        printf("Discovery status %02X", (*pkt)->data[5]);
        printf("\n");
    /* Also print to file */
        if (fp != NULL)
        {
            fprintf(fp,"Transmit Status: %s ",timeString);
            fprintf(fp,"Length: %d ",(*pkt)->dataLen);
            fprintf(fp,"FrameID %02X, ", (*pkt)->data[0]);
            fprintf(fp,"16 Bit Address %02X%02X, ", (*pkt)->data[1], (*pkt)->data[2]);
            if (row == numberNodes) fprintf(fp,"Unknown ");
            else fprintf(fp,"%s ", nodeInfo[row].nodeIdent);
            fprintf(fp,"Retry count %d, ", (*pkt)->data[3]);
            fprintf(fp,"Delivery status %02X, ", (*pkt)->data[4]);
            fprintf(fp,"Discovery status %02X", (*pkt)->data[5]);
            fprintf(fp,"\n");
        }
    }
#endif
}

/*--------------------------------------------------------------------------*/
/** @brief Print out contents of a Modem Status packet.

This is a debug only function. It displays all relevant information in the
packet.
*/

void printModemStatus(struct xbee_pkt **pkt)
{
#ifdef DEBUG
    if (debug > 1)
    {
        char timeString[20];
        time_t now;
        now = time(NULL);
        struct tm *tmp;
        tmp = localtime(&now);
        strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",tmp);
        printf("Modem Status: %s Status %02X\n",timeString,(*pkt)->data[0]);
    /* Also print to file */
        if (fp != NULL)
        {
            fprintf(fp,"Modem Status: %s Status %02X\n",timeString,(*pkt)->data[0]);
        }
    }
#endif
}


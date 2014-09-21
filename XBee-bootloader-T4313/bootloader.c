/**
@mainpage AVR/XBee Bootloader
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 16 January 2013
@brief Code for a bootloader for AVR with an XBee

This code provides the firmware update loader for an AVR using an XBee version
2 in API mode. Firmware is in iHex format, with the semicolon at the start of
each line taken as a command to write the line. Pages are erased as needed
before writing starts, with lockout bits being used to prevent them being erased
at later times. If the process needs to be restarted, the microcontroller must
be reset.

This bootloader is written for microcontrollers without a separate bootloader
section, therefore it must be placed at the beginning of memory. The application
starts at address 0x800 above the bootloader. If the bootloader pin is not
active the bootloader will jump directly to the application code, and also after
firmware has been uploaded.

When each line has been programmed an acknowledgement is returned as 'Y'
representing OK, or other responses (see below) representing errors. When an
error occurs the line is not programmed and must be retransmitted.

As iHex records are presented, pages are erased as required (which saves time
waiting for a full erase) and a lock bit is set to prevent re-erasure.
The remote program is responsible for ensuring that the firmware records
follow a sequence and do not attempt to write over previously written data
(this would be a normal requirement for any firmware bootloader). After the
last line has been written, the RWW section is enabled and a jump to the
application start occurs. The erase lock bits are reset only when the
bootloader is restarted.

Commands are:
; - program an iHex line.
X - erase all Flash in application area (excluding bootloader).
Q - quit the bootloader and jump to application code.
Mxxxx - dump memory in hex, 16 bytes from the hex address xxxx

One of the input pins on the microcontroller is tested and used as an
instruction to remain in the bootloader. This should be attached to an XBee
output pin. After firmware update is complete the program may be entered by
simply removing the signal on this pin. While the bootloader is active, an
output pin on the microcontroller is used to force the XBee to remain awake.
This must be attached to the Sleep Request pin of the XBee.

Responses are single ASCII character
'Y' command completed successfully
'N' Unspecified error
'I' Invalid command
'H' Invalid hexadecimal character
'L' Invalid iHex data length
'C' Invalid Checksum
'J' Jump to application occurred

If there are basic framing errors in the Xbee receive packet the entire packet
is ignored and no response will be sent.

@note
Software: AVR-GCC 4.5.3
@note
Target:   Any AVR with sufficient output ports and a timer
@note
Tested:   ATMega168 at 8MHz internal clock.
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

#include <avr/boot.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
/* definitions, structures, macros for the bootloader */
#include "bootloader.h"
/* macros for UART send/receive */
#include "serial.h"
#include <util/delay.h>

static inline void initxbee(void);
static inline uint8_t loaderPinSet(void);
static inline void setXbeeWake(void);
static inline uint8_t hexToNybble(uint8_t hex);
static inline uint8_t nybbleToHex(uint8_t x);
static inline uint8_t isHex(uint8_t hex);

/**********************************************************************
	Start of Main Program
*/

__attribute__ ((OS_main)) void main(void)   /* (This disables normal register saving) */
{
    uint8_t ch;

/* Initialization */
/* function pointer to app start. This is above the bootloader. */
    void (*jumpToApp)( void ) = 0x0800;

/* Reset the watchdog timer as required for the ATMega family */
    wdt_disable();
/* Initialise the UART */
    uartInit();
#ifdef MCUSR
    MCUSR = 0;			                /* Clear MCU Status Register Watchdog Flag */
#endif
    initxbee();           	            /* Initialize the XBee interface. */
    setXbeeWake();                      /* Keep awake (XBee sleep mode 1 only!) */
    uint8_t pageEraseLock[PAGE_FLAGS];  /* Lock bits for page erase */
    rxFrameType rxMessage;              /* Initialise message blocks for Rx and Tx */
    memset(pageEraseLock,0,PAGE_FLAGS); /* clear all flags */
    pState programState = pageStart;    /* State variable to track programming */
    uint16_t pageByteAddress;
    uint8_t writeInProgress = FALSE;    /* writes must finish before starting more */
    uint8_t response[64];
    uint8_t responseLength;

/*---------------------------------------------------------------------------*/
/* Main loop. */
    {
        for(;;)
        {
/* ---- Message receive ----- */
            uint8_t messageState = 0;   /* Initialise state variable for message start */
/* Wait for a data message which should carry the bootloader commands */
/* Only transmit and modem status messages would be relevant but must ignore
these */
            do
            {
/* Loop until all message characters received and Rx message built */
/* If an error occurs the state will be set back to zero and the message
discarded */
                do
                {
/* Test for the selected bootloader pin pulled low, then jump directly to the
application. If this is not used, the Q instruction will provide the jump.*/
#if AUTO_ENTER_APP == 1
                    if (loaderPinSet())
                    {
#ifdef RWWSRE
                        boot_rww_enable_safe();
#endif
                        jumpToApp();
                    }
#endif
                    ch = getch();           /* Check for a command character. */
                }
                while (parseMessage(ch, &messageState, &rxMessage) != ready);
            }
            while (rxMessage.frameType != DATA_RX);

            uint8_t command = rxMessage.message.rxRequest.data[0];
            responseLength = 1;             /* For 1 character responses */
/* ---- Erase Command ----- */
/* This erases each page up to the top of application memory */
            if(command=='X')
            {
/* Step through each page and erase it if not already erased, and mark as erased. */
/* NOTE: address refers to Flash words, while the commands use byte addresses. */
/* This can take up to 1 second to complete so send nothing else till done */
/* NOTE code block common with a later erase */
                for(uint16_t address = APP_START; address < APP_END; address += PAGESIZE)
                {
                    uint8_t page = (address / PAGESIZE);
                    uint8_t pageBit = (1 << (page & 0x07));
                    uint8_t addridx = (page / 8);
                    if ((pageEraseLock[addridx] & pageBit) == 0)
                    {
                        boot_spm_busy_wait();
                        boot_page_erase(address);
                        pageEraseLock[addridx] |= pageBit;
                    }
                }
                response[0] = 'Y';           /* Send OK back. */
            }

/* ---- Page Programming ----- */
/* iHex start of line. Parse the line into byte valued array. */
/* Only allow 16 byte lines max. 12 bytes precede data field, then 43 characters
max. */
            else if ((command==':') && (rxMessage.length <= 55))
            {
/* Convert hex to byte binary and place in a line buffer */
/* For iHex, line[0] has length, line[1], line[2] has address, line[3] has type */
                uint8_t line[20];               /* line of iHex in binary */
                uint8_t hexGood = TRUE;
                uint8_t checksum = 0;
                uint8_t dataCount = 0;
                uint8_t i;
                for (i=1; i<rxMessage.length-12; i+=2)
                {
                    hexGood = hexGood && isHex(rxMessage.message.rxRequest.data[i])
                                      && isHex(rxMessage.message.rxRequest.data[i+1]);
                    line[dataCount] = (hexToNybble(rxMessage.message.rxRequest.data[i]) << 4)
                                    +  hexToNybble(rxMessage.message.rxRequest.data[i+1]);
                    checksum += line[dataCount++];
                }
/* Some integrity checks before committing any Flash writes */
/* line[0] has data length which must tally with the data byte counter */
                if (! hexGood)
                    response[0] = 'H';
                else if(line[0] != dataCount-5)
                    response[0] = 'L';
                else if(checksum != 0)
                    response[0] = 'C';
                else
                {
                    response[0] = 'Y';
/* line[1] and line[2] have the byte address */
                    uint16_t byteAddress = (line[1] << 8) + line[2];
/* Fill the page buffer writing the line, with data going in 16 bit words */
/* TODO deal with odd byte addresses in iHex */
                    uint8_t indx;
                    for (indx=4; indx<dataCount-1; indx+=2)
                    {
/* If we move outside the application area, bomb out with a NAK */
                        if((byteAddress >= APP_END) || (byteAddress < APP_START))
                        {
                            response[0] = 'N';
                            break;
                        }
/* If page write in progress, need to wait for previous page write to complete. */
/* Will not need to wait for page erase */
                        if (writeInProgress)    
                        {
                            boot_spm_busy_wait();
                            writeInProgress = FALSE;
                        }
/* Check if we previously hit a page boundary, or are just starting. */
/* If we are starting a new page, keep its start address for later writing.
This address must be on a page boundary. if not, pad out the buffer with 0xFFFF
as these will not cause any changes in the Flash memory contents when written.
Then erase */
                        if (programState == pageStart)
                        {
                            pageByteAddress = (byteAddress & ~(PAGESIZE-1));
                            for (uint16_t i=pageByteAddress; i<byteAddress; i+=2)
                                boot_page_fill(i, 0xFFFF);      /* Padding */
/* Check if erase is needed and get on with it. Wait for all prior write/erase
to complete */
                            uint8_t page = (byteAddress / PAGESIZE);
                            uint8_t pageBit = (1 << (page & 0x07));
                            uint8_t addridx = (page / 8);
                            if ((pageEraseLock[addridx] & pageBit) == 0)
                            {
                                boot_spm_busy_wait();
                                boot_page_erase(byteAddress);
                                pageEraseLock[addridx] |= pageBit;
                            }
                        }
/* Fill page */
                        boot_page_fill(byteAddress, (line[indx+1] << 8) + line[indx]);
                        byteAddress += 2;
                        programState = pageFilling;
/* If we hit a page boundary, we need to write the page just filled.
Also do this if we finish with the last line (record type in line[3] is 1) */
                        if ((byteAddress % PAGESIZE) == 0)
                        {
                            boot_spm_busy_wait();
                            boot_page_write(pageByteAddress);
                            writeInProgress = TRUE;
                            programState = pageStart;
                        }
                    }
                    if (response[0] == 'Y')
                    {
/* The line has been written to the page buffer, and possibly a page write
completed. Check if we have a file end record type, and have not just started
writing a page. Then we need to write the last part page. */
                        if ((programState != pageStart) && (line[3] == 1))
                        {
                            boot_spm_busy_wait();
                            boot_page_write(pageByteAddress);
                            writeInProgress = TRUE;
                            programState = pageStart;
                        }
/* Check for end of file record and jump to application after enabling RWW section */
                        if (line[3] == 1)
                        {
                            response[0] = 'J';
                            sendDataMessage(response,1);
                            boot_spm_busy_wait();
#ifdef RWWSRE
                            boot_rww_enable();
#endif
                            jumpToApp();        /* Jump to Application Reset vector 0x0000 */
                        }
                    }
                }
            }

/* Exit bootloader */
            else if (command=='Q')
            {
#ifdef RWWSRE
                boot_rww_enable_safe();
#endif
                response[0] = 'J';
                jumpToApp();                /* Jump to Application Reset vector 0x0000 */
            }
/* Dump of Memory */
            else if (command == 'M')
            {
                uint16_t address = 0;
                for (uint8_t i = 1; i <= 4; i++)
                    address = (address << 4) + hexToNybble(rxMessage.message.rxRequest.data[i]);
#ifdef RWWSRE
                boot_rww_enable_safe();
#endif
                response[0] = 'M';
                for (uint8_t i = 1; i < 33; i+=2)
                {
                    uint8_t memoryByte = pgm_read_byte_near(address++);
                    response[i] = nybbleToHex((memoryByte >> 4));
                    response[i+1] = nybbleToHex((memoryByte & 0x0F));
                }
                responseLength = 33;
            }
            else response[0] = 'I';         /* invalid command or line */
            sendDataMessage(response,responseLength);
        }
    }
}

/*-----------------------------------------------------------------------------*/
/* Create and send an outgoing XBee message data frame to the coordinator

The address field is set to that of the coordinator (all zeros 64 bit address,
unknown 16 bit address). The data to be sent is an ASCII string, ending in 0.

@parameter uint8_t ch: character string to send
*/

void sendDataMessage(uint8_t* ch, uint8_t messageLength)
{
    uint8_t messageString[15];
    messageString[0] = 0x03;            /* The frame ID */
    uint8_t idx;
    for (idx=1; idx<9; idx++)
        messageString[idx] = 0x00;      /* coordinator 64 bit address */
    messageString[9] = 0xFF;            /* Unknown 16 bit address as req'd */
    messageString[10] = 0xFE;
    messageString[11] = 0x00;           /* Radius */
    messageString[12] = 0x00;           /* Options */
    uint16_t length = messageLength+14;
    sendch(0x7E);                       /* Start sending frame */
    sendch(high(length));
    sendch(low(length));
    sendch(DATA_TX);
    uint8_t checksum = DATA_TX;
    for (idx=0; idx<13; idx++)
    {
        sendch(messageString[idx]);
        checksum += messageString[idx];
    }
    for (uint8_t i=0; i<messageLength; i++)
    {
        sendch(ch[i]);
        checksum += ch[i];
    }
    sendch(0xFF-checksum);
}

/*-----------------------------------------------------------------------------*/
/* Parse an incoming XBee message frame

The message is an XBee data frame. This builds the message structure and tests for
error conditions. The state variable must be set to zero on first entry. It
resets to zero after a message has been received.

@parameter uint8_t ch: the received character
@parameter uint8_t *messageState: state variable for the message, changed and returned
@parameter rxFrameType *message: structure containing the received message
@returns ready if the message was received correctly, inprogress if still working,
         otherwise error state
*/

messageError parseMessage(const uint8_t inputChar, uint8_t *messageState, rxFrameType *message)
{
/* Two byte length */
    if (*messageState == 1) message->length = (inputChar << 8);
    else if (*messageState == 2) message->length += inputChar;
/* Frame type */
    else if (*messageState == 3)
    {
        message->frameType = inputChar;
        message->checksum = inputChar;
    }
/* Rest of message, may include addresses or just data depending on frame type */
    else if (*messageState >3)
    {
        if (message->length + 3 > *messageState)
        {
            message->message.array[*messageState-4] = inputChar;
            message->checksum += inputChar;
        }
/* Check some error conditions then exit gracefully if done */
        else
        {
            *messageState = 0;
/* Supershort message */
            if (*messageState > message->length + 3) return statemachine;
/* Checksum mismatch */
            if (((message->checksum + inputChar + 1) & 0xFF) > 0) return checksum;
            else return ready;
        }
    }
/* Only messageState == 0 left, don't advance until Sync character found */
    if (!((*messageState == 0) && (inputChar != 0x7E))) (*messageState)++;
    return inprogress;
}
/*-----------------------------------------------------------------------------*/
void initxbee(void)
{
#if AUTO_ENTER_APP == 1
    cbi(PROG_PORT_DIR,PROG_PIN);
#endif
}

/*-----------------------------------------------------------------------------*/
uint8_t loaderPinSet(void)
{
    return ((inb(PROG_PORT) & _BV(PROG_PIN)) > 0);
}
/*-----------------------------------------------------------------------------*/
void setXbeeWake(void)
{
#if XBEE_STAY_AWAKE == 1
    sbi(WAKE_PORT_DIR,WAKE_PIN);
    cbi(WAKE_PORT,WAKE_PIN);
#endif
}
/*-----------------------------------------------------------------------------*/
uint8_t hexToNybble(uint8_t hex)
{
    return (hex - ((hex > '9') ? ('A'-10) : '0'));
}
/*-----------------------------------------------------------------------------*/
uint8_t nybbleToHex(uint8_t x)
{
    return (x + ((x > 9) ? ('A'-10) : '0'));
}
/*-----------------------------------------------------------------------------*/
uint8_t isHex(uint8_t hex)
{
    return (((hex - '0') <= 9) || ((hex - 'A') <= 5));
}
/*-----------------------------------------------------------------------------*/


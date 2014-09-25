#include <avr/boot.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#if (MCU_TYPE==1)
#include "../libs/defines-M168.h"
#elif (MCU_TYPE==2)
#include "../libs/defines-T4313.h"
#endif
#include "bootloader.h"
#include <util/delay.h>

/**********************************************************************
	Start of Main Program
*/

/* (Disables call register saving) */
__attribute__ ((OS_main)) void bootloader(void)
{
    AppPtr_t jumpToApp = (AppPtr_t)0x0000;

/* Initialise the UART */
    uartInit();
    initxbee();           	            /* Initialize the XBee interface. */
    setXbeeWake();                      /* Keep awake (XBee sleep mode 1 only!) */
    uint8_t pageEraseLock[PAGE_FLAGS];  /* Lock bits for page erase */
    memset(pageEraseLock,0,PAGE_FLAGS); /* clear all flags */
    pState programState = pageStart;    /* State variable to track programming */
    uint16_t pageByteAddress;
    uint8_t writeInProgress = FALSE;    /* writes must finish before starting more */
    uint8_t response[64];
    uint8_t responseLength;

    uint8_t messageState = 0;
    uint8_t messageReady = FALSE;
    uint8_t messageError = 0;
/* Main loop */
    for(;;)
    {
        wdt_reset();

/* Check for incoming messages */
/* The Rx message variable is re-used and must be processed before the next */
        rxFrameType rxMessage;
/* Wait for data to appear */
        uint16_t inputChar = getch();
        messageError = high(inputChar);
        if (messageError != NO_DATA)
        {
/* Pull in the next character and look for message start */
/* Read in the length (16 bits) and frametype then the rest to a buffer */
            uint8_t inputValue = low(inputChar);
            switch(messageState)
            {
/* Sync character */
                case 0:
                    if (inputChar == 0x7E) messageState++;
                    break;
/* Two byte length */
                case 1:
                    rxMessage.length = (inputChar << 8);
                    messageState++;
                    break;
                case 2:
                    rxMessage.length += inputValue;
                    messageState++;
                    break;
/* Frame type */
                case 3:
                    rxMessage.frameType = inputValue;
                    rxMessage.checksum = inputValue;
                    messageState++;
                    break;
/* Rest of message, maybe include addresses or just data */
                default:
                    if (messageState > rxMessage.length + 3)
                        messageError = STATE_MACHINE;
                    else if (rxMessage.length + 3 > messageState)
                    {
                        rxMessage.message.array[messageState-4] = inputValue;
                        messageState++;
                        rxMessage.checksum += inputValue;
                    }
                    else
                    {
                        messageReady = TRUE;
                        messageState = 0;
                        if (((rxMessage.checksum + inputValue + 1) & 0xFF) > 0)
                            messageError = CHECKSUM;
                    }
            }
        }

        if (messageReady)
        {
sbi(TEST_PORT,TEST_PIN);
            messageReady = FALSE;
            if (messageError > 0) sendch(messageError);
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
                            sendDataMessageCoordinator(response,1);
                            boot_spm_busy_wait();
#ifdef RWWSRE
                            boot_rww_enable();
#endif
                            jumpToApp();    /* Jump to Application Reset vector 0x0000 */
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
                jumpToApp();           /* Jump to Application Reset vector 0x0000 */
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
            sendDataMessageCoordinator(response,responseLength);
        }
    }
}

/*-----------------------------------------------------------------------------*/
/* Create and send an outgoing XBee message data frame to the coordinator

The address field is set to that of the coordinator (all zeros 64 bit address,
unknown 16 bit address). The data to be sent is an ASCII string, ending in 0.

@parameter uint8_t ch: character string to send
*/

void sendDataMessageCoordinator(uint8_t* ch, uint8_t messageLength)
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

messageErrorType parseMessage(const uint8_t inputChar, uint8_t *messageState,
                              rxFrameType *message)
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
void setXbeeWake(void)
{
#if XBEE_STAY_AWAKE == 1
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);
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
/* Initialise the UART, setting baudrate, Rx/Tx enables, and flow controls

Baud rate is derived from the header call to setbaud.h.
UBRRL_VALUE and UBRRH_VALUE and USE_2X are returned, the latter requires the
U2X bit to be set in UCSRA to force a double baud rate clock.
*/

void uartInit(void)
{
    BAUD_RATE_LOW_REG = UBRRL_VALUE;
    BAUD_RATE_HIGH_REG = UBRRH_VALUE;
#if USE_2X
    sbi(UART_STATUS_REG,DOUBLE_RATE);
#else
    cbi(UART_STATUS_REG,DOUBLE_RATE);
#endif
    UART_FORMAT_REG = (3 << FRAME_SIZE);                // Set 8 bit frames
    UART_CONTROL_REG |= _BV(ENABLE_RECEIVER_BIT) |
                        _BV(ENABLE_TRANSMITTER_BIT);    // enable receive and transmit 
#ifdef USE_HARDWARE_FLOW
    cbi(UART_CTS_PORT_DIR,UART_CTS_PIN);                // Set flow control pins CTS input
    sbi(UART_RTS_PORT_DIR,UART_RTS_PIN);                // RTS output
    cbi(UART_RTS_PORT,UART_RTS_PIN);                    // RTS cleared to enable
#endif
}

/*-----------------------------------------------------------------------------*/
/* Send a character when the Tx is ready

The function waits until CTS is asserted low then waits until the UART indicates
that the character has been sent.
*/

void sendch(unsigned char c)
{
#ifdef USE_HARDWARE_FLOW
        while (inb(UART_CTS_PORT) & _BV(UART_CTS_PIN));     // wait for clear to send
#endif
        UART_DATA_REG = c;                                  // send
        while (!(UART_STATUS_REG & _BV(TRANSMIT_COMPLETE_BIT)));    // wait till gone
        UART_STATUS_REG |= _BV(TRANSMIT_COMPLETE_BIT);      // reset TXCflag
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (blocking)

The function asserts RTS low then waits for the receive complete bit is set.
RTS is then cleared high. The character is then retrieved.
*/

unsigned char getch(void)
{
#ifdef USE_HARDWARE_FLOW
    cbi(UART_RTS_PORT,UART_RTS_PIN);                        // Enable RTS
#endif
    while (!(UART_STATUS_REG & _BV(RECEIVE_COMPLETE_BIT)));
#ifdef USE_HARDWARE_FLOW
    sbi(UART_RTS_PORT,UART_RTS_PIN);                        // Disable RTS
#endif
    return UART_DATA_REG;
}
/*-----------------------------------------------------------------------------*/


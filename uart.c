// uart.c
// Functions to provide a shim between the C standard library functions
// and UART1 peripheral device on the PIC16F1/PIC18F microcontroller.
// Build with linker set to link in the C99 standard library.
// PJ,
// 2023-12-01 PIC18F16Q41 attached to a MAX3082 RS485 transceiver
// 2024-07-01 PIC18F46Q71 attached to a TTL-232-5V cable.

#include <xc.h>
#include "global_defs.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

void uart1_init(long baud)
{
    // Follow recipe given in PIC18F46Q71 data sheet
    // Sections 35.2.1.1 and 35.2.2.1
    // We are going to use hardware control for CTSn/RTSn.
    unsigned int brg_value;
    //
    // Configure PPS RX1=RC7, TX1=RC0, RTS1=RC1, CTS1=RC6 
    GIE = 0;
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 0;
    U1RXPPS = 0b010111; // RC7
    U1CTSPPS = 0b010110; // RC6
    RC0PPS = 0x15; // UART1 TX
    RC1PPS = 0x17; // UART1 RTS
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKED = 1;
    ANSELCbits.ANSELC0 = 0; // TX pin
    TRISCbits.TRISC0 = 0; // output
    ANSELCbits.ANSELC1 = 0; // RTS pin
    TRISCbits.TRISC1 = 0; // output
    ANSELCbits.ANSELC7 = 0; // Turn on digital input buffer for RX1
    TRISCbits.TRISC7 = 1; // RX1 is an input
    ANSELCbits.ANSELC6 = 0; // Turn on digital input buffer for CTS
    TRISCbits.TRISC6 = 1; // RX1 is an input
    //
    U1CON0bits.BRGS = 1;
    brg_value = (unsigned int) (FOSC/baud/4 - 1);
    // For 64MHz, 115200 baud, expect value of 137.
    //              9600 baud                 1665.
    U1BRG = brg_value;
    //
    U1CON0bits.MODE = 0b0000; // Use 8N1 asynchronous
    U1CON2bits.FLO = 0b10; // Hardware flow control (RTS/CTS)
    U1CON0bits.RXEN = 1;
    U1CON0bits.TXEN = 1;
    U1CON1bits.ON = 1;
    return;
}

void uart1_putch(char data)
{
    // Wait until shift-register empty, then send data.
    while (!U1ERRIRbits.TXMTIF) { CLRWDT(); }
    U1TXB = data;
    return;
}

void uart1_flush_rx(void)
{
    U1FIFObits.RXBE = 1;
}

char uart1_getch(void)
{
    char c;
    // Block until a character is available in buffer.
    while (U1FIFObits.RXBE) { CLRWDT(); }
    // Get the data that came in.
    c = U1RXB;
    return c;
}

void uart1_close(void)
{
    U1CON0bits.RXEN = 1;
    U1CON0bits.TXEN = 1;
    U1CON1bits.ON = 1;
    return;
}

// Functions to connect STDIO to UART1.

void putch(char data)
{
    uart1_putch(data);
    return;
}

int getch(void)
{
    return uart1_getch();
}

int getche(void)
{
    int data = getch();
    putch((char)data); // echo the character
    return data;
}

// Convenience functions for strings.

int getstr(char* buf, int nbuf)
// Read (without echo) a line of characters into the buffer,
// stopping when we see a return character.
// Returns the number of characters collected,
// excluding the terminating null char.
{
    int i = 0;
    char c;
    uint8_t done = 0;
    while (!done) {
        c = (char)getch();
        if (c != '\n' && c != '\r' && c != '\b' && i < (nbuf-1)) {
            // Append a normal character.
            buf[i] = c;
            i++;
        }
        if (c == '\r') {
            // Stop collecting on receiving a carriage-return character.
            done = 1;
            buf[i] = '\0';
        }
        if (c == '\b' && i > 0) {
            // Backspace.
            i--;
        }
    }
    return i;
}

void putstr(char* str)
{
    for (size_t i=0; i < strlen(str); i++) putch(str[i]); 
    return;
}

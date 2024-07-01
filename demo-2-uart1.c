// demo-2-uart1.c
// Demonstrate transmitting and receiving characters 
// with the PIC18F46Q71 EUSART1 peripheral with hardware flow control.
//
// PJ, 2024-07-01
//
// PIC18F46Q71 Configuration Bit Settings (generated in Memory View)
// CONFIG1
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ

// CONFIG2
#pragma config CLKOUTEN = OFF
#pragma config PR1WAY = OFF
#pragma config BBEN = OFF
#pragma config CSWEN = OFF
#pragma config FCMEN = OFF
#pragma config FCMENP = OFF
#pragma config FCMENS = OFF

// CONFIG3
#pragma config MCLRE = EXTMCLR
#pragma config PWRTS = PWRT_64
#pragma config MVECEN = OFF
#pragma config IVT1WAY = OFF
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS

// CONFIG4
#pragma config BORV = VBOR_1P9
#pragma config ZCD = OFF
#pragma config PPS1WAY = OFF
#pragma config STVREN = ON
#pragma config LVP = ON
#pragma config DEBUG = OFF
#pragma config XINST = OFF

// CONFIG5
#pragma config WDTCPS = WDTCPS_31
#pragma config WDTE = ON

// CONFIG6
#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC

// CONFIG7
// BBSIZE = No Setting

// CONFIG8
#pragma config SAFSZ = SAFSZ_NONE

// CONFIG9
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config WRTSAF = OFF
#pragma config WRTAPP = OFF

// CONFIG10
#pragma config CPD = OFF

// CONFIG11
#pragma config CP = OFF

#include <xc.h>
#include "global_defs.h"
#include <stdint.h>
#include <stdlib.h>

#include "uart.h"
#include <stdio.h>
#include <string.h>

#define GREENLED LATEbits.LATE0

#define NBUF 80 
char buf[NBUF];

int main(void)
{
    int m;
    int n;
    TRISEbits.TRISE0 = 0; // Pin as output for GREENLED.
    GREENLED = 0;
    uart1_init(115200);
    __delay_ms(10);
    n = printf("PIC18F46Q71 MCU\r\n");
    n = printf("Start typing text, pressing Enter at the end of each line.\r\n");
    // We will operate the MCU as a slave, waiting for commands
    // and only responding then.
    while (1) {
        GREENLED ^= 1;
        // Characters are echoed as they are typed.
        // Backspace deleting is allowed.
        // CR signals end of string.
        m = getstr(buf, NBUF);
        n = printf("\r\nEntered text was: ");
        if (m > 0) { puts(buf); }
        if (strncmp(buf, "quit", 4) == 0) break;
    }
    uart1_flush_rx();
    uart1_close();
    return 0; // Expect that the MCU will reset.
} // end main

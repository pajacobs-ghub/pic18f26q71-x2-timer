// pic18f46q71-x2timer.c
// X2-timer-ng build-3 with the PIC18F46Q71
//
// PJ, 2024-07-11 Start with the command interpreter.
//
#define VERSION_STR "v0.1 PIC18F46Q71 X2-timer-ng build-3 2024-07-11"
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

#define LED LATEbits.LATE0

// For incoming serial comms
#define NBUFA 80
char bufA[NBUFA];
// For outgoing serial comms
#define NBUFB 128
char bufB[NBUFB];

void interpret_command(char* cmdStr)
// We intend that valid commands are answered quickly
// so that the supervisory PC can infer the absence of a node
// by the lack of a prompt response.
// A command that does not do what is expected should return a message
// that includes the word "error".
{
    char* token_ptr;
    const char* sep_tok = ", ";
    int nchar;
    uint8_t i, j;
    // nchar = printf("DEBUG: cmdStr=%s", cmdStr);
    switch (cmdStr[0]) {
        case 'v':
            nchar = snprintf(bufB, NBUFB, "%s\n", VERSION_STR);
            puts(bufB);
            break;
        default:
            nchar = snprintf(bufB, NBUFB, "Error, unknown command: '%c'\n", cmdStr[0]);
            puts(bufB);
    }
} // end interpret_command()

int main(void)
{
    int m;
    int n;
    TRISEbits.TRISE0 = 0; // Pin as output for LED.
    LED = 0;
    uart1_init(115200);
    __delay_ms(10);
    // FVR_init();
    // ADC_init();
    __delay_ms(10);
    // Flash LED twice at start-up to indicate that the MCU is ready.
    for (int8_t i=0; i < 2; ++i) {
        LED = 1;
        __delay_ms(250);
        LED = 0;
        __delay_ms(250);
    }
    // Wait until we are reasonably sure that the MCU has restarted
    // and then flush the incoming serial buffer.
    __delay_ms(100);
    uart1_flush_rx();
    // We will operate the MCU as a slave, waiting for commands
    // and only responding then.
    while (1) {
        // Characters are not echoed as they are typed.
        // Backspace deleting is allowed.
        // CR signals end of incoming command string.
        m = getstr(bufA, NBUFA);
        if (m > 0) {
            interpret_command(bufA);
        }
    }
    // ADC_close();
    // FVR_close();
    uart1_flush_rx();
    uart1_close();
    return 0; // Expect that the MCU will reset.
} // end main

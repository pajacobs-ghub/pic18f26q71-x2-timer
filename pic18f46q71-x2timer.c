// pic18f46q71-x2timer.c
// X2-timer-ng build-3 with the PIC18F46Q71
//
// PJ, 2024-07-11 Start with the command interpreter.
//     2024-07-13 Virtual registers.
//
#define VERSION_STR "v0.2 PIC18F46Q71 X2-timer-ng build-3 2024-07-13"
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
#include "eeprom.h"
#include <stdio.h>
#include <string.h>

#define LED LATEbits.LATE0

// Parameters controlling the device are stored in virtual registers.
#define NUMREG 7
int16_t vregister[NUMREG]; // working copy in SRAM

void set_registers_to_original_values()
{
    vregister[0] = 1;   // mode 0=simple trigger from INa, 1=time-of-flight(TOF) trigger)
    vregister[1] = 5;   // trigger level INa as a 8-bit count, 0-255
    vregister[2] = 5;   // trigger level INb as a 10-bit count, 0-255
    vregister[3] = 0;   // Vref selection for DAC 0=1v024 1=2v048, 3=4v096
    vregister[4] = 0;   // delay 0 as a 16-bit count
    vregister[5] = 0;   // delay 1
    vregister[6] = 0;   // delay 2
}

// EEPROM is used to hold the parameters when the power is off.
// Note little-endian layout.
__EEPROM_DATA(1,0, 5,0, 5,0, 0,0);
__EEPROM_DATA(0,0, 0,0, 0,0, 0,0);

char save_registers_to_EEPROM()
{
    for (uint16_t i=0; i < NUMREG; ++i) {
        DATAEE_WriteByte(2*i, (char)(vregister[i] & 0x00FF));
        DATAEE_WriteByte((2*i)+1, (char)((vregister[i] >> 8) & 0x00FF));
    }
    return 0;
}

char restore_registers_from_EEPROM()
{
    for (uint16_t i=0; i < NUMREG; ++i) {
        vregister[i] = (DATAEE_ReadByte((2*i)+1) << 8) | DATAEE_ReadByte(2*i);
    }
    return 0;
}

// For incoming serial communication
#define NBUFA 80
char bufA[NBUFA];
// For outgoing serial communication
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
    int16_t v;
    // nchar = printf("DEBUG: cmdStr=%s", cmdStr);
    switch (cmdStr[0]) {
        case 'v':
            nchar = snprintf(bufB, NBUFB, "%s\n", VERSION_STR);
            puts(bufB);
            break;
        case 'n':
            nchar = snprintf(bufB, NBUFB, "%u ok", NUMREG);
            puts(bufB);
            break;
        case 'p':
            nchar = snprintf(bufB, NBUFB, "\nRegister values:\n");
            for (i=0; i < NUMREG; ++i) {
                nchar = snprintf(bufB, NBUFB, "reg[%d]=%d\n", i, vregister[i]);
                puts(bufB);
            }
            puts("ok\n");
            break;
        case 'r':
            // Report a register value.
            token_ptr = strtok(&cmdStr[1], sep_tok);
            if (token_ptr) {
                // Found some nonblank text, assume register number.
                i = (uint8_t) atoi(token_ptr);
                if (i < NUMREG) {
                    v = vregister[i];
                    nchar = snprintf(bufB, NBUFB, "%d ok\n", v);
                    puts(bufB);
                } else {
                    nchar = puts("fail\n");
                }
            } else {
                nchar = puts("fail\n");
            }
            break;
        case 's':
            // Set a register value.
            token_ptr = strtok(&cmdStr[1], sep_tok);
            if (token_ptr) {
                // Found some nonblank text; assume register number.
                // printf("text:\"%s\"", token_ptr);
                i = (uint8_t) atoi(token_ptr);
                if (i < NUMREG) {
                    token_ptr = strtok(NULL, sep_tok);
                    if (token_ptr) {
                        // Assume text is value for register.
                        v = (int16_t) atoi(token_ptr);
                        vregister[i] = v;
                        nchar = snprintf(bufB, NBUFB, "reg[%u] %d ok\n", i, v);
                        puts(bufB);
                        if (i == 1 || i == 2) { /* update_dacs(); FIXME */ }
                    } else {
                        nchar = puts("fail\n");
                    }
                } else {
                    nchar = puts("fail\n");
                }
            } else {
                nchar = puts("fail\n");
            }
            break;
        case 'R':
            if (restore_registers_from_EEPROM()) {
                nchar = puts("fail\n");
            } else {
                nchar = puts("ok\n");
            }
            break;
        case 'S':
            if (save_registers_to_EEPROM()) {
                nchar = puts("fail\n");
            } else {
                nchar = puts("ok\n");
            }
            break;
        case 'F':
            set_registers_to_original_values();
            nchar = printf("ok\n");
            break;
        case 'a':
            // arm_and_wait_for_event();
            break;
        case 'c':
            // Report an ADC value.
            token_ptr = strtok(&cmdStr[1], sep_tok);
            if (token_ptr) {
                // Found some nonblank text, assume channel number.
                i = (uint8_t) atoi(token_ptr);
                if (i <= 31) {
                    v = 0; // read_adc(i); // FIX-ME
                    nchar = snprintf(bufB, NBUFB, "%d ok\n", v);
                    puts(bufB);
                } else {
                    nchar = puts("fail\n");
                }
            } else {
                nchar = puts("fail\n");
            }
            break;
        case 'h':
        case '?':
            nchar = puts("\nPIC18F46Q71-I/P X2-trigger+timer commands and registers\n");
            nchar = puts("\n");
            nchar = puts("Commands:\n");
            nchar = puts(" h or ? print this help message\n");
            nchar = puts(" v      report version of firmware\n");
            nchar = puts(" n      report number of registers\n");
            nchar = puts(" p      report register values\n");
            nchar = puts(" r <i>  report value of register i\n");
            nchar = puts(" s <i> <j>  set register i to value j\n");
            nchar = puts(" R      restore register values from EEPROM\n");
            nchar = puts(" S      save register values to EEPROM\n");
            nchar = puts(" F      set register values to original values\n");
            nchar = puts(" a      arm device and wait for event\n");
            nchar = puts(" c <i>  convert analogue channel i\n");
            nchar = puts("        i=30 DAC1_output\n");
            nchar = puts("        i=28 DAC2_output\n");
            nchar = puts("        i=5  RC1/AN5/C1IN1- (IN a)\n");
            nchar = puts("        i=6  RC2/AN6/C2IN2- (IN b)\n");
            nchar = puts("\n");
            nchar = puts("Registers:\n");
            nchar = puts(" 0  mode: 0= simple trigger from INa signal\n");
            nchar = puts("          1= time-of-flight(TOF) trigger\n");
            nchar = puts(" 1  trigger level a as an 8-bit count, 0-255\n");
            nchar = puts(" 2  trigger level b as an 8-bit count, 0-255\n");
            nchar = puts(" 3  Vref selection for DACs 0=1v024, 1=2v048, 2=4v096\n");
            nchar = puts(" 4  delay 0 as 16-bit count (8 ticks per us)\n");
            nchar = puts(" 5  delay 1 as 16-bit count (8 ticks per us)\n");
            nchar = puts(" 6  delay 2 as 16-bit count (8 ticks per us)\n");
            nchar = puts("ok\n");
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
    restore_registers_from_EEPROM();
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

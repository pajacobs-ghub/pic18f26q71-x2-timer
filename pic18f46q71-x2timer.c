// pic18f46q71-x2timer.c
// X2-timer-ng build-3 with the PIC18F46Q71
//
// PJ, 2024-07-11 Start with the command interpreter.
//     2024-07-13 Virtual registers and DACs plus ADC.
//
#define VERSION_STR "v0.3 PIC18F46Q71 X2-timer-ng build-3 2024-07-13"
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
    vregister[0] = 0;   // mode 0=simple trigger from INa, 1=time-of-flight(TOF) trigger)
    vregister[1] = 5;   // trigger level INa as a 8-bit count, 0-255
    vregister[2] = 5;   // trigger level INb as a 10-bit count, 0-255
    vregister[3] = 3;   // Vref selection for DAC 0=off, 1=1v024, 2=2v048, 3=4v096
    vregister[4] = 0;   // delay 0 as a 16-bit count
    vregister[5] = 0;   // delay 1
    vregister[6] = 0;   // delay 2
}

// EEPROM is used to hold the parameters when the power is off.
// Note little-endian layout.
__EEPROM_DATA(0,0, 5,0, 5,0, 3,0);
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

void init_pins()
{
    TRISEbits.TRISE0 = 0; // Pin as output for LED.
    // We have comparator input pins C1IN0-/RA0 and C2IN3-/RB1.
    TRISAbits.TRISA0 = 1;
    ANSELAbits.ANSELA0 = 1;
    TRISBbits.TRISB1 = 1;
    ANSELBbits.ANSELB1 = 1;
}

void FVR_init()
{
    // We want to both the ADC and the DAC references set the same.
    uint8_t vref = vregister[3] & 0x03;
    FVRCONbits.ADFVR = vref;
    FVRCONbits.CDAFVR = vref;
    FVRCONbits.EN = 1;
    while (!FVRCONbits.RDY) { /* should be less than 25 microseconds */ }
    return;
}

void FVR_close()
{
    FVRCONbits.EN = 0;
    FVRCONbits.ADFVR = 0;
    FVRCONbits.CDAFVR = 0;
    return;
}

void update_DACs()
{
    DAC2CONbits.PSS = 0b10; // FVR Buffer 2
    DAC2CONbits.NSS = 0; // VSS
    DAC2CONbits.EN = 1;
    DAC2DATL = (uint8_t)vregister[1];
    //
    DAC3CONbits.PSS = 0b10; // FVR Buffer 2
    DAC3CONbits.NSS = 0; // VSS
    DAC3CONbits.EN = 1;
    DAC3DATL = (uint8_t)vregister[2];
    //
    return;
}

void ADC_init()
{
    ADCON0bits.IC = 0; // single-ended mode
    ADCON0bits.CS = 1; // use dedicated RC oscillator
    ADCON0bits.FM = 0b01; // right-justified result
    ADCON2bits.ADMD = 0b000; // basic (legacy) behaviour
    PIR1bits.ADIF = 0;
    ADREFbits.NREF = 0; // negative reference is Vss
    ADREFbits.PREF = 0b11; // positive reference is FVR
    ADACQ = 0x10; // 16TAD acquisition period
    ADPCH = 0x00; // select RA0/C1IN0-
    ADCON0bits.ON = 1; // power on the device
    return;
}

int16_t ADC_read(uint8_t i)
{
    // Returns the value from the ADC.
    ADPCH = i;
    ADCON0bits.GO = 1;
    NOP();
    while (ADCON0bits.GO) { /* wait, should be brief */ }
    PIR1bits.ADIF = 0;
    return (int16_t)ADRES;
}

void ADC_close()
{
    ADCON0bits.ON = 0;
    return;
}

void trigger_simple()
{
    // FIXME
    return;
}

void trigger_TOF()
{
    // FIXME
    return;
}

void arm_and_wait_for_event(void)
{
    int nchar;
    switch (vregister[0]) {
        case 0:
            putstr("armed simple trigger, using INa only. ");
            if (CMOUTbits.MC1OUT) {
                putstr("C1OUT already high. fail");
                break;
            }
            trigger_simple();
            putstr("triggered. ok\n");
            break;
        case 1:
            putstr("armed time-of-flight trigger, using INa followed by INb. ");
            if (CMOUTbits.MC1OUT) {
                putstr("C1OUT already high. fail");
                break;
            }
            trigger_TOF();
            putstr("triggered. ok\n");
            break;
        default:
            putstr("unknown mode. fail\n");
    }
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
            putstr(bufB);
            break;
        case 'n':
            nchar = snprintf(bufB, NBUFB, "%u ok\n", NUMREG);
            putstr(bufB);
            break;
        case 'p':
            nchar = snprintf(bufB, NBUFB, "Register values:\n");
            putstr(bufB);
            for (i=0; i < NUMREG; ++i) {
                nchar = snprintf(bufB, NBUFB, "reg[%d]=%d\n", i, vregister[i]);
                putstr(bufB);
            }
            putstr("ok\n");
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
                    putstr(bufB);
                } else {
                    putstr("fail\n");
                }
            } else {
                putstr("fail\n");
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
                        if (i == 3) { FVR_init(); }
                        if (i == 1 || i == 2) { update_DACs(); }
                    } else {
                        putstr("fail\n");
                    }
                } else {
                    putstr("fail\n");
                }
            } else {
                putstr("fail\n");
            }
            break;
        case 'R':
            if (restore_registers_from_EEPROM()) {
                putstr("fail\n");
            } else {
                putstr("ok\n");
            }
            break;
        case 'S':
            if (save_registers_to_EEPROM()) {
                putstr("fail\n");
            } else {
                putstr("ok\n");
            }
            break;
        case 'F':
            set_registers_to_original_values();
            putstr("ok\n");
            break;
        case 'a':
            arm_and_wait_for_event();
            break;
        case 'c':
            // Report an ADC value.
            token_ptr = strtok(&cmdStr[1], sep_tok);
            if (token_ptr) {
                // Found some nonblank text, assume channel number.
                i = (uint8_t) atoi(token_ptr);
                if (i == 0 || i == 9 || i == 57 || i == 58) {
                    v = ADC_read(i);
                    nchar = snprintf(bufB, NBUFB, "%d ok\n", v);
                    putstr(bufB);
                } else {
                    putstr("fail\n");
                }
            } else {
                putstr("fail\n");
            }
            break;
        case 'h':
        case '?':
            putstr("\nPIC18F46Q71-I/P X2-trigger+timer commands and registers\n");
            putstr("\n");
            putstr("Commands:\n");
            putstr(" h or ? print this help message\n");
            putstr(" v      report version of firmware\n");
            putstr(" n      report number of registers\n");
            putstr(" p      report register values\n");
            putstr(" r <i>  report value of register i\n");
            putstr(" s <i> <j>  set register i to value j\n");
            putstr(" R      restore register values from EEPROM\n");
            putstr(" S      save register values to EEPROM\n");
            putstr(" F      set register values to original values\n");
            putstr(" a      arm device and wait for event\n");
            // Get ADC Positive Input Channel Selections from Table 41-7 in the data sheet
            putstr(" c <i>  convert analogue channel i\n");
            putstr("        i=57 DAC2_output (INa)\n");
            putstr("        i=58 DAC3_output (INb)\n");
            putstr("        i=0  RA0/C1IN0- (INa)\n");
            putstr("        i=9  RB1/C2IN3- (INb)\n");
            putstr("\n");
            putstr("Registers:\n");
            putstr(" 0  mode: 0= simple trigger from INa signal\n");
            putstr("          1= time-of-flight(TOF) trigger\n");
            putstr(" 1  trigger level for INa as an 8-bit count, 0-255\n");
            putstr(" 2  trigger level for INb as an 8-bit count, 0-255\n");
            putstr(" 3  Vref selection for DACs 0=off, 1=1v024, 2=2v048, 3=4v096\n");
            putstr(" 4  delay 0 as 16-bit count (8 ticks per us)\n");
            putstr(" 5  delay 1 as 16-bit count (8 ticks per us)\n");
            putstr(" 6  delay 2 as 16-bit count (8 ticks per us)\n");
            putstr("ok\n");
            break;
        default:
            nchar = snprintf(bufB, NBUFB, "Error, unknown command: '%c'\n", cmdStr[0]);
            putstr(bufB);
    }
} // end interpret_command()

int main(void)
{
    int m;
    int n;
    init_pins();
    LED = 0;
    uart1_init(115200);
    restore_registers_from_EEPROM();
    __delay_ms(10);
    FVR_init();
    update_DACs();
    ADC_init();
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
    ADC_close();
    FVR_close();
    uart1_flush_rx();
    uart1_close();
    return 0; // Expect that the MCU will reset.
} // end main

/*
 * File:   example3.c
 * Author: Microchip Technology Inc
 *
 * Created on October 21, 2015, 4:09 PM
 * Updated on December 18, 2018, 9:22 AM
 */

// PIC24FJ128GA010 Configuration Bit Settings

// For more on Configuration Bits, consult your device data sheet

// CONFIG2
#pragma config POSCMOD = XT   // XT Oscillator mode selected
#pragma config OSCIOFNC = ON  // OSC2/CLKO/RC15 as port I/O (RC15)
#pragma config FCKSM = CSDCMD // Clock Switching and Monitor disabled
#pragma config FNOSC = PRI    // Primary Oscillator (XT, HS, EC)
#pragma config IESO = ON      // Int Ext Switch Over Mode enabled

// CONFIG1
#pragma config WDTPS = PS32768 // Watchdog Timer Postscaler (1:32,768)
#pragma config FWPSA = PR128   // WDT Prescaler (1:128)
#pragma config WINDIS = ON     // Watchdog Timer Window Mode disabled
#pragma config FWDTEN = OFF    // Watchdog Timer disabled
#pragma config ICS = PGx2      // Emulator/debugger uses EMUC2/EMUD2
#pragma config GWRP = OFF      // Writes to program memory allowed
#pragma config GCP = OFF       // Code protection is disabled
#pragma config JTAGEN = OFF    // JTAG port is disabled

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

// Interrupt function
//void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void){ //Traditional interrupt
void __interrupt(no_auto_psv) _T1Interrupt(void){ //CCI interrupt
    // static variable for permanent storage duration
    static unsigned char portValue = 0;
    // write to port latch
    LATA = portValue++;
    // clear this interrupt condition
    _T1IF = 0;
}

int main(void) {

    // Port A access
    AD1PCFG = 0xFFFF; // set to digital I/O (not analog)
    TRISA = 0x0000;   // set all port bits to be output

    // Timer1 setup
    T1CON = 0x8010; // timer 1 on, prescaler 1:8, internal clock
    _T1IE = 1; // enable interrupts for timer 1
    _T1IP = 0x001; // set interrupt priority (lowest)

    while(1);

    return -1;
}

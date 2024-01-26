/*
 * File:   example1.c
 * Author: Microchip Technology Inc
 *
 * Created on October 13, 2015, 5:07 PM
 * Updated on October 19, 2016, 11:15 AM
 * Updated on December 17, 2018, 4:30 PM
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

int main(void) {

    unsigned char portValue = 0x55;
    
    // Port A access
    AD1PCFG = 0xFFFF;   // set to digital I/O (not analog)
    TRISA = 0x0000;     // set all port bits to be output
    LATA = portValue;   // write to port latch
    
    return 0;
}

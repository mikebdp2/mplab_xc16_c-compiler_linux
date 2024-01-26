/**
  Display on LEDs Source File

  @Company:
    Microchip Technology Inc.

  @File Name:
    LEDs.c

  @Summary:
    Display data on LEDs from three PIC24F32KA304 device ports: A, B, C

  @Description:
    From "Introducing the PIC24F32KA304 Plug-In Module" (DS51945)
    Explorer 16 Pin # (100 Pin)  PIM Device Pin # (44 Pin)
    ---------------------------  -------------------------
    17 LED(D3)/RA0               35 SS2/CN34/RA9
    38 LED(D4)/RA1/TCK           12 OC3/CN35/RA10
    58 LED(D5)/SCL2/RA2          13 IC3/CTED8/CN36/RA11
    59 LED(D6)/SDA2/RA3           4 OC2/CN20/RC8
    60 LED(D7)/TDI/RA4            5 IC2/CTED7/CN19/RC9
    61 LED(D8)/TDO/RA5           10 AN12+/LVDIN/CTED2/CN14/RB12
    91 LED(D9)/RA6               23 AN4/C1INB/C2IND/SDA2/T5CK/T4CK/CTED13/CN6/RB2
    92 LED(D10)/SWITCH(S5)/RA7   24 AN5/C1INA/C2INC/SCL2/CN7/RB3
 * 
 */

/*
Copyright (c) 2013 - 2015 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 */

#include "mcc_generated_files/mcc.h"
#include "LEDs.h"

volatile LAT_LEDSBITS LAT_LEDSbits;

/*****************************************************************************
 * Function: DisplayValueOnLEDs
 * Precondition: None.
 * Overview: Display input value on Explorer 16 LEDs
 * Input: Value to display
 * Output: None.
 *****************************************************************************/
void DisplayValueOnLEDs(unsigned int value) {

    _LEDS = value;
        
    _LATA9  = _LED0;
    _LATA10 = _LED1;
    _LATA11 = _LED2;
    _LATC8  = _LED3;
    _LATC9  = _LED4;
    _LATB12 = _LED5;
    _LATB2  = _LED6;
    _LATB3  = _LED7;

}
/**
 End of File
 */
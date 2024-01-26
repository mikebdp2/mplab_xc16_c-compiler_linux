/*-------------------------------------------------------------------------
 * PICF32KA304 LEDs header
 *
 * (c) Copyright 1999-2015 Microchip Technology, All rights reserved
 *
 * This software is developed by Microchip Technology Inc. and its
 * subsidiaries ("Microchip").
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1.      Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 * 2.      Redistributions in binary form must reproduce the above
 *         copyright notice, this list of conditions and the following
 *         disclaimer in the documentation and/or other materials provided
 *         with the distribution.
 * 3.      Microchip's name may not be used to endorse or promote products
 *         derived from this software without specific prior written
 *         permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICROCHIP "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL MICROCHIP BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING BUT NOT LIMITED TO
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWSOEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*****************************************************************************
 * Union of structures to hold value for display on LEDs
 * LAT_LEDx - bit fields of value
 * w - entire value
 *****************************************************************************/
typedef union {
    struct {
      unsigned LAT_LED0:1;
      unsigned LAT_LED1:1;
      unsigned LAT_LED2:1;
      unsigned LAT_LED3:1;
      unsigned LAT_LED4:1;
      unsigned LAT_LED5:1;
      unsigned LAT_LED6:1;
      unsigned LAT_LED7:1;
    };
    struct {
      unsigned w:16;  
    };
} LAT_LEDSBITS;
extern volatile LAT_LEDSBITS LAT_LEDSbits;

/* LAT_LEDSBITS */
#define _LED0 LAT_LEDSbits.LAT_LED0
#define _LED1 LAT_LEDSbits.LAT_LED1
#define _LED2 LAT_LEDSbits.LAT_LED2
#define _LED3 LAT_LEDSbits.LAT_LED3
#define _LED4 LAT_LEDSbits.LAT_LED4
#define _LED5 LAT_LEDSbits.LAT_LED5
#define _LED6 LAT_LEDSbits.LAT_LED6
#define _LED7 LAT_LEDSbits.LAT_LED7
#define _LEDS LAT_LEDSbits.w

/*****************************************************************************
 * Function: DisplayValueOnLEDs
 * Precondition: None.
 * Overview: Display input value on Explorer 16 LEDs
 * Input: Value to display
 * Output: None.
 *****************************************************************************/
void DisplayValueOnLEDs(unsigned int value);
/**
 End of File
 */

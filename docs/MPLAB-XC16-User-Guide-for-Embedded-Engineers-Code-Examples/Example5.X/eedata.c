/**
  Data EEPROM Write and Read 

  @Company:
    Microchip Technology Inc.

  @File Name:
    eedata.c

  @Summary:
    Functions to write to and read from PIC24F32KA304 device data EEPROM 
    (EEData) memory.

  @Description:
    Code from "20/28/44/48-Pin, General Purpose, 16-Bit Flash Microcontrollers 
    with XLP Technology", Section 6.0 "Data EEPROM Memory".
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

#include <xc.h>
#include "eedata.h"

#define ERASE_EEWORD 0x4058
#define WRITE_EEWORD 0x4004

int __attribute__ ((space(eedata))) eeData = 0x0;
unsigned int offset = 0x0;

/*****************************************************************************
 * Function: EEData_WTL
 * Precondition: None.
 * Overview: Write one word of EEData
 * Input: Action to take: Erase or Write, Data to write
 * Output: None.
 *****************************************************************************/
void EEData_WTL(unsigned int action, unsigned int data) {
    
    // Set up NVMCON to write one word of data EEPROM
    NVMCON = action;
    
    // Set up a pointer to the EEPROM location to be written
    TBLPAG = __builtin_tblpage(&eeData);
    offset = __builtin_tbloffset(&eeData);
    __builtin_tblwtl(offset, data);
    
    // Issue Unlock Sequence & Start Write Cycle
    __builtin_write_NVM();
    
    // Wait for completion
    while(NVMCONbits.WR);
}

/*****************************************************************************
 * Function: EEData_Erase
 * Precondition: None.
 * Overview: Set up erase of one word of EEData
 * Input: None.
 * Output: None.
 *****************************************************************************/
void EEData_Erase(void) {
    
    EEData_WTL(ERASE_EEWORD, 0);
}

/*****************************************************************************
 * Function: EEData_Write
 * Precondition: None.
 * Overview: Set up write of one word of EEData
 * Input: Data to write
 * Output: None.
 *****************************************************************************/
void EEData_Write(unsigned int data) {
    
    EEData_WTL(WRITE_EEWORD, data);
}

/*****************************************************************************
 * Function: EEData_Read
 * Precondition: None.
 * Overview: Read one word of EEData
 * Input: None.
 * Output: Value read from EEData
 *****************************************************************************/
unsigned int EEData_Read(void) {
    
    // Set up a pointer to the EEPROM location to be read
    TBLPAG = __builtin_tblpage(&eeData);
    offset = __builtin_tbloffset(&eeData);
    
    // Read the EEPROM data    
    return __builtin_tblrdl(offset);
}

/**
 End of File
 */
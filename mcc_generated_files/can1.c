/**
  CAN1 Generated Driver File

  @Company
    Microchip Technology Inc.

  @File Name
    can1.c

  @Summary
    This is the generated driver implementation file for the CAN1 driver using PIC24 / dsPIC33 / PIC32MM MCUs

  @Description
    This source file provides APIs for CAN1.
    Generation Information :
        Product Revision   :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device             :  dsPIC33CH256MP505
    The generated drivers are tested against the following:
        Compiler           :  XC16 v1.61
        MPLAB              :  MPLAB X v5.45
*/

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/

#include <string.h>    
#include "can1.h"

// CAN Message object arbitration field information
#define CAN_MSG_OBJ_DLC_FIELD_SIZE          0xFU
#define CAN_MSG_OBJ_ID_TYPE_FIELD_POS       0x10U
#define CAN_MSG_OBJ_ID_TYPE_SHIFT_POS       0x4U
#define CAN_MSG_OBJ_FRAME_TYPE_FIELD_POS    0x20U
#define CAN_MSG_OBJ_FRAME_TYPE_SHIFT_POS    0x5U
#define CAN_MSG_OBJ_BRS_FIELD_POS           0x40U
#define CAN_MSG_OBJ_BRS_SHIFT_POS           0x6U
#define CAN_MSG_OBJ_FORMAT_TYPE_FIELD_POS   0x80U
#define CAN_MSG_OBJ_FORMAT_TYPE_SHIFT_POS   0x7U
#define CAN_STD_MSG_ID_MAX_SIZE             0x7FFU
#define CAN_MSG_OBJ_SID_SHIFT_POS           0x12U
#define CAN_EXT_MSG_ID_HIGH_MAX_SIZE        0x1FFFU
#define CAN_EXT_MSG_ID_LOW_MAX_SIZE         0x1FU
#define CAN_MSG_OBJ_EID_LOW_SHIFT_POS       0xBU
#define CAN_MSG_OBJ_EID_HIGH_SHIFT_POS      0x5U

/**
 Section: Private Variable Definitions
*/

/**
  CAN1 FIFO parameters information data structure.

  @Summary
    Defines the CAN1 FIFO parameters information data structure.

  @Description
    This Data structure is to implement a CAN FIFO parameters information.
*/
typedef struct 
{
    uint8_t payloadSize;
    uint8_t msgDeepSize;
    uint16_t *address;
} CAN1_FIFO_INFO;

/**
 Section: Private Function Definitions
*/

/**
  @Summary
    Configure the CAN1 Bit rate settings.

  @Description
    This routine configure the CAN1 Bit rate settings.

  @Preconditions
    None

  @Param
    None  

  @Returns
    None
    
  @Example
    None
*/
static void CAN1_BitRateConfiguration(void)
{
    // SJW 31; TSEG2 31; 
    C1NBTCFGL = 0x1F1F;
    // BRP 0; TSEG1 126; 
    C1NBTCFGH = 0x7E;
}

/**
 Section: Driver Interface Function Definitions
*/
void CAN1_Initialize(void)
{
    /* Enable the CAN1 module */
    C1CONLbits.CON = 1;
    
    // RTXAT disabled; ESIGM disabled; TXBWS No delay; STEF disabled; SERRLOM disabled; ABAT disabled; REQOP Configuration mode; TXQEN disabled; 
    C1CONH = 0x400;  

    /* Place CAN1 module in configuration mode */
    if(CAN_OP_MODE_REQUEST_SUCCESS == CAN1_OperationModeSet(CAN_CONFIGURATION_MODE))
    {
        
        // BRSDIS enabled; CON enabled; WAKFIL enabled; WFT T11 Filter; ISOCRCEN enabled; SIDL disabled; DNCNT 0; PXEDIS enabled; CLKSEL disabled; 
        C1CONL = 0x9760;
    
        // Disabled CAN1 Store in Transmit Event FIFO bit
        C1CONHbits.STEF = 0;
        // Disabled CAN1 Transmit Queue bit
        C1CONHbits.TXQEN = 0;
        
        /*configure CAN1 Bit rate settings*/
        CAN1_BitRateConfiguration();        
		
        /* Place CAN1 module in Normal Operation mode */
        CAN1_OperationModeSet(CAN_NORMAL_2_0_MODE);
    }
}

CAN_OP_MODE_STATUS CAN1_OperationModeSet(const CAN_OP_MODES requestMode) 
{
    CAN_OP_MODE_STATUS status = CAN_OP_MODE_REQUEST_SUCCESS;
    
    if((CAN_CONFIGURATION_MODE == CAN1_OperationModeGet()) || (requestMode == CAN_DISABLE_MODE)
            || (requestMode == CAN_CONFIGURATION_MODE))
    {
        C1CONHbits.REQOP = requestMode;

        while(C1CONHbits.OPMOD != requestMode) 
        {
            //This condition is avoiding the system error case endless loop
            if(C1INTLbits.SERRIF == 1)
            {
                status = CAN_OP_MODE_SYS_ERROR_OCCURED;
                break;
            }
        }
    }
    else
    {
        status = CAN_OP_MODE_REQUEST_FAIL;
    }
    
    return status;
}

CAN_OP_MODES CAN1_OperationModeGet(void) 
{
    return C1CONHbits.OPMOD;
}

bool CAN1_IsBusOff(void)
{
    return C1TRECHbits.TXBO;
}

void CAN1_Sleep(void)
{
    C1INTLbits.WAKIF = 0;
    C1INTHbits.WAKIE = 1;
    
    // CAN Info Interrupt Enable bit
    IEC1bits.C1IE = 1;  
    
    /* put the module in disable mode */
    CAN1_OperationModeSet(CAN_DISABLE_MODE);
}

void __attribute__((weak)) CAN1_DefaultInvalidMessageHandler(void) 
{

}

void __attribute__((weak)) CAN1_DefaultBusWakeUpActivityHandler(void) 
{

}

void __attribute__((weak)) CAN1_DefaultBusErrorHandler(void) 
{

}

void __attribute__((weak)) CAN1_DefaultModeChangeHandler(void) 
{

}

void __attribute__((weak)) CAN1_DefaultSystemErrorHandler(void) 
{

}

void __attribute__((__interrupt__, no_auto_psv)) _C1Interrupt(void)
{
    // Bus Wake-up Activity Interrupt 
    if (C1INTLbits.WAKIF == 1) 
    {
        CAN1_DefaultBusWakeUpActivityHandler();
        C1INTLbits.WAKIF = 0;
    }
    
    IFS1bits.C1IF = 0;
}

void CAN1_Tasks( void )
{
    if (C1INTLbits.IVMIF == 1)
    {
        CAN1_DefaultInvalidMessageHandler();
        C1INTLbits.IVMIF = 0;
    }

    if (C1INTLbits.CERRIF == 1) 
    {
        CAN1_DefaultBusErrorHandler();
        C1INTLbits.CERRIF = 0;
    }

    if (C1INTLbits.MODIF == 1) 
    {
        CAN1_DefaultModeChangeHandler();
        C1INTLbits.MODIF = 0;
    }
    
    if (C1INTLbits.SERRIF == 1) 
    {
        CAN1_DefaultSystemErrorHandler();
        C1INTLbits.SERRIF = 0;
    }
}

/**
 End of File
*/


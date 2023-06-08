/**
  CAN2 Generated Driver File

  @Company
    Microchip Technology Inc.

  @File Name
    can2.c

  @Summary
    This is the generated driver implementation file for the CAN2 driver using PIC24 / dsPIC33 / PIC32MM MCUs

  @Description
    This source file provides APIs for CAN2.
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
#include "can2.h"

#define CAN2_FIFO_ALLOCATE_RAM_SIZE    16U //CAN FIFO allocated ram size based on (number of FIFO x FIFO message Payload size x Message object DLC size)
#define CAN2_NUM_OF_RX_FIFO            1U    // No of RX FIFO's configured
#define CAN2_RX_FIFO_MSG_DATA          8U   // CAN RX FIFO Message object data field size

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
  CAN FD Receive FIFO Status Enumeration

  @Summary
    Defines the CAN FD receive FIFO status Enumeration.

  @Description
    This enumeration defines the CAN FD receive FIFO status.
*/
typedef enum 
{
    CAN_RX_MSG_NOT_AVAILABLE = 0x0,
    CAN_RX_MSG_AVAILABLE = 0x1,
    CAN_RX_MSG_OVERFLOW = 0x8
} CAN2_RX_FIFO_STATUS;

//CAN RX FIFO Channels
typedef enum 
{
    CAN2_FIFO_1 = 1,
} CAN2_RX_FIFO_CHANNELS;

/**
 Section: Private Variable Definitions
*/
//Start CAN Message Memory Base Address
static uint8_t __attribute__((aligned(4)))can2FifoMsg[CAN2_FIFO_ALLOCATE_RAM_SIZE];

//CAN Receive FIFO Message object data field 
static uint8_t rxMsgData[CAN2_RX_FIFO_MSG_DATA];

/**
  CAN2 Receive FIFO Message Object head count information maintenance data structure

  @Summary
    Defines the CAN2 receive FIFO message object head count information maintenance data structure.

  @Description
    This defines the object required for the maintenance of the RX FIFO message object.
*/
typedef struct 
{
    const CAN2_RX_FIFO_CHANNELS channel;
    uint8_t headCount;
} CAN2_RX_FIFO_MSG;

static volatile CAN2_RX_FIFO_MSG rxFIFOMsg[CAN2_NUM_OF_RX_FIFO] = 
{
    //Receive FIFO, FIFO head count
    {CAN2_FIFO_1, 0U},
}; 

/**
  CAN2 FIFO parameters information data structure.

  @Summary
    Defines the CAN2 FIFO parameters information data structure.

  @Description
    This Data structure is to implement a CAN FIFO parameters information.
*/
typedef struct 
{
    uint8_t payloadSize;
    uint8_t msgDeepSize;
    uint16_t *address;
} CAN2_FIFO_INFO;

/**
 Section: Private Function Definitions
*/

/**
  @Summary
    Get the DLC enum based decimal value.

  @Description
    This routine get the DLC enum based decimal value.

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    dlc - Data Length Code enum value

  @Returns
    Return the Data Length Code decimal value
    
  @Example
    None
*/
static uint8_t CAN2_DlcToDataBytesGet(const CAN_DLC dlc)
{
    static const uint8_t dlcByteSize[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    return dlcByteSize[dlc];
}

/**
  @Summary
    Get the FIFO user address, message deep size and payload size information.

  @Description
    This routine get the FIFO user address, message deep size and payload size
    information.

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    fifoNum - FIFO Channel selection 
    fifoInfo - pointer to the CAN FIFO info. 

  @Returns
    None
    
  @Example
    None
*/

static void CAN2_FIFO_InfoGet(const uint8_t fifoNum, CAN2_FIFO_INFO *fifoInfo)
{   
    switch (fifoNum) 
    {
        case CAN2_FIFO_1:
            fifoInfo->address = (uint16_t *) &C2FIFOUA1L;
            fifoInfo->payloadSize = 8U;
            fifoInfo->msgDeepSize = 1U;
            break;
     
        default:
            fifoInfo->address = NULL;
            fifoInfo->payloadSize = 0U;
            fifoInfo->msgDeepSize = 0U;
            break;
    }
}

/**
  @Summary
    Reset the CAN2 Receive message head count.

  @Description
    This routine reset the CAN2 Receive message head count.

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    None  

  @Returns
    None

  @Example
    None
*/
static void CAN2_RX_FIFO_ResetInfo()
{
    uint8_t count;
    
    for(count = 0; count < CAN2_NUM_OF_RX_FIFO; count++)
    {
        rxFIFOMsg[count].headCount = 0;
    }
}

/**
  @Summary
    CAN2 Receive FIFO status.

  @Description
    This returns the CAN2 Receive FIFO status.

  @Preconditions
    CAN2_Initializer function should have been called before calling this function.

  @Param
    fifoNum  - FIFO Channel selection

  @Returns
    CAN Receive FIFO status.
    CAN_RX_MSG_NOT_AVAILABLE - CAN Receive FIFO is empty
    CAN_RX_MSG_AVAILABLE     - CAN Receive FIFO at least one message available
    CAN_RX_MSG_OVERFLOW      - CAN Receive FIFO is Overflow

  @Example
    None
*/
static CAN2_RX_FIFO_STATUS CAN2_RX_FIFO_StatusGet(const CAN2_RX_FIFO_CHANNELS fifoNum)
{
    CAN2_RX_FIFO_STATUS rxFifoStatus;
    
    switch (fifoNum) 
    {
        case CAN2_FIFO_1:
            rxFifoStatus = (C2FIFOSTA1 & (CAN_RX_MSG_AVAILABLE | CAN_RX_MSG_OVERFLOW));
            break;

        default:
            rxFifoStatus = CAN_RX_MSG_NOT_AVAILABLE;
            break;
    }
    
    return rxFifoStatus;
}

/**
  @Summary
    Clear the CAN2 Receive FIFO overflow status bit.

  @Description
    This routine is used to clear the CAN2 Receive FIFO overflow status bit

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    fifoNum - FIFO Channel selection

  @Returns
    None
    
  @Example
    None
*/
static void CAN2_RX_FIFO_OverflowStatusFlagClear(const CAN2_RX_FIFO_CHANNELS fifoNum)
{   
    switch (fifoNum) 
    {
        case CAN2_FIFO_1:
            C2FIFOSTA1bits.RXOVIF = false;
            break;
            
        default:
            break;
    }
}

/**
  @Summary
    Update the Receive FIFO message increment tail position.

  @Description
    This routine Update the Receive FIFO message increment tail position.

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    fifoNum - FIFO Channel selection  

  @Returns
    None
    
  @Example
    None
*/
static void CAN2_RX_FIFO_IncrementMsgPtr(const uint8_t fifoNum) 
{
    switch (fifoNum) 
    {   
        case CAN2_FIFO_1:
            C2FIFOCON1Lbits.UINC = 1; // Update the CAN2_FIFO_1 message pointer.
            break;
        
        default:
            break;
    }    
}

/**
  @Summary
    Get the Receiver FIFO message index value.

  @Description
    This routine get the Receiver FIFO message index value.

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    fifoNum - FIFO Channel selection  

  @Returns
    Return the FIFO message index value
    
  @Example
    None
*/
static uint16_t CAN2_RX_FIFO_MessageIndexGet(const CAN2_RX_FIFO_CHANNELS fifoNum) 
{
    uint16_t fifoMsgIndex;
    
    switch (fifoNum) 
    {            
        case CAN2_FIFO_1:
            fifoMsgIndex = C2FIFOSTA1bits.FIFOCI;
            break;

        default:
            fifoMsgIndex = 0;
            break;
    }
    
    return fifoMsgIndex;
}

/**
  @Summary
    Read the message object from Receive FIFO and update to the user message object 
    pointer.

  @Description
    This routine read the message object from Receive FIFO and update to the user  
    message object pointer.

  @Preconditions
    CAN2_Initialize function should be called before calling this function.

  @Param
    rxFifoObj - a pointer to the Receive FIFO where the message would be stored 
    rxCanMsg  - pointer to the CAN receive message object. 

  @Returns
    None
    
  @Example
    None
*/
static void CAN2_MessageReadFromFifo(uint16_t *rxFifoObj, CAN_MSG_OBJ *rxCanMsg)
{   
    uint8_t dlcByteSize = 0;
        
    // SID <10:0> and EID <4:0>
    uint16_t rx0Data = *rxFifoObj;
    rxFifoObj += 1;

    // SID11 and EID <18:5>
    uint16_t rx1Data = *rxFifoObj;
    rxFifoObj += 1;

    // DLC <3:0>, IDE <1>, FDF <1> 
    rxCanMsg->field.dlc = (*rxFifoObj & CAN_MSG_OBJ_DLC_FIELD_SIZE);
    rxCanMsg->field.idType = ((*rxFifoObj & CAN_MSG_OBJ_ID_TYPE_FIELD_POS) >> CAN_MSG_OBJ_ID_TYPE_SHIFT_POS);
    rxCanMsg->field.frameType = ((*rxFifoObj & CAN_MSG_OBJ_FRAME_TYPE_FIELD_POS) >> CAN_MSG_OBJ_FRAME_TYPE_SHIFT_POS);
    rxCanMsg->field.brs = ((*rxFifoObj & CAN_MSG_OBJ_BRS_FIELD_POS) >> CAN_MSG_OBJ_BRS_SHIFT_POS);
    rxCanMsg->field.formatType = ((*rxFifoObj & CAN_MSG_OBJ_FORMAT_TYPE_FIELD_POS) >> CAN_MSG_OBJ_FORMAT_TYPE_SHIFT_POS);

    /* message is standard identifier */
    if(rxCanMsg->field.idType == CAN_FRAME_STD) 
    {
        // SID <10:0>
        rxCanMsg->msgId = (rx0Data & CAN_STD_MSG_ID_MAX_SIZE);   
    }
    else 
    {
        /* message is extended identifier */
        // EID <28:18>, EID <17:0>
        rxCanMsg->msgId = (((uint32_t)(rx0Data & CAN_STD_MSG_ID_MAX_SIZE) << CAN_MSG_OBJ_SID_SHIFT_POS) | 
                            ((uint32_t)(rx1Data & CAN_EXT_MSG_ID_HIGH_MAX_SIZE) << CAN_MSG_OBJ_EID_HIGH_SHIFT_POS) | 
                            ((uint32_t)(rx0Data >> CAN_MSG_OBJ_EID_LOW_SHIFT_POS) & CAN_EXT_MSG_ID_LOW_MAX_SIZE));
    }

    rxFifoObj += 2;

    dlcByteSize = CAN2_DlcToDataBytesGet(rxCanMsg->field.dlc);

    // Coping Receive FIFO data starting memory location
    memset(rxMsgData, 0, CAN2_RX_FIFO_MSG_DATA);
    memcpy((char *) rxMsgData, (char *) rxFifoObj, dlcByteSize);
    rxCanMsg->data = rxMsgData;
}

/**
  @Summary
    Configure the CAN2 Receive FIFO settings.

  @Description
    This routine configure the CAN2 Receive FIFO settings.

  @Preconditions
    None

  @Param
    None  

  @Returns
    None
    
  @Example
    None
*/

static void CAN2_RX_FIFO_Configuration(void)
{          
    // TFHRFHIE disabled; TFERFFIE disabled; RXTSEN disabled; TXREQ disabled; RXOVIE disabled; RTREN disabled; TXEN disabled; TXATIE disabled; UINC disabled; FRESET enabled; TFNRFNIE disabled; 
    C2FIFOCON1L = 0x400;
    // TXAT Disabled; PLSIZE 8; FSIZE 1; TXPRI 0; 
    C2FIFOCON1H = 0x00;
}

/**
  @Summary
    Configure the CAN2 Filter and mask settings.

  @Description
    This routine configure the CAN2 Filter and mask settings.

  @Preconditions
    None

  @Param
    None  

  @Returns
    None
    
  @Example
    None
*/
static void CAN2_RX_FIFO_FilterMaskConfiguration(void)
{
    /* Configure RX FIFO Filter control settings*/
    
    // message stored in FIFO1
    C2FLTCON0Lbits.F0BP = 0x01;
    // EID 0; SID 1136; 
    C2FLTOBJ0L = 0x470;
    // EID 0; EXIDE disabled; SID11 disabled; 
    C2FLTOBJ0H = 0x00;
    // MSID 2044; MEID 0; 
    C2MASK0L = 0x7FC;
    // MEID 0; MSID11 disabled; MIDE enabled; 
    C2MASK0H = 0x4000;
    // Enable the filter 0
    C2FLTCON0Lbits.FLTEN0 = 0x01;
}

/**
  @Summary
    Configure the CAN2 Bit rate settings.

  @Description
    This routine configure the CAN2 Bit rate settings.

  @Preconditions
    None

  @Param
    None  

  @Returns
    None
    
  @Example
    None
*/
static void CAN2_BitRateConfiguration(void)
{
    // SJW 31; TSEG2 31; 
    C2NBTCFGL = 0x1F1F;
    // BRP 0; TSEG1 126; 
    C2NBTCFGH = 0x7E;
}

/**
 Section: Driver Interface Function Definitions
*/
void CAN2_Initialize(void)
{
    /* Enable the CAN2 module */
    C2CONLbits.CON = 1;
    
    // RTXAT disabled; ESIGM disabled; TXBWS No delay; STEF disabled; SERRLOM disabled; ABAT disabled; REQOP Configuration mode; TXQEN disabled; 
    C2CONH = 0x400;  

    /* Place CAN2 module in configuration mode */
    if(CAN_OP_MODE_REQUEST_SUCCESS == CAN2_OperationModeSet(CAN_CONFIGURATION_MODE))
    {
        /* Initialize the C2FIFOBAL with the start address of the CAN2 FIFO message object area. */
        C2FIFOBAL = (uint16_t) &can2FifoMsg;
        
        // BRSDIS enabled; CON enabled; WAKFIL enabled; WFT T11 Filter; ISOCRCEN enabled; SIDL disabled; DNCNT 0; PXEDIS enabled; CLKSEL disabled; 
        C2CONL = 0x9760;
    
        // Disabled CAN2 Store in Transmit Event FIFO bit
        C2CONHbits.STEF = 0;
        // Disabled CAN2 Transmit Queue bit
        C2CONHbits.TXQEN = 0;
        
        /*configure CAN2 Bit rate settings*/
        CAN2_BitRateConfiguration();        
        
        /*configure CAN2 FIFO settings*/
        CAN2_RX_FIFO_Configuration();
        
        /* Configure Receive FIFO Filter and Mask settings*/
        CAN2_RX_FIFO_FilterMaskConfiguration();    
        
        // Reset the CAN2 Receive FIFO head count
        CAN2_RX_FIFO_ResetInfo();
		
        /* Place CAN2 module in Normal Operation mode */
        CAN2_OperationModeSet(CAN_NORMAL_2_0_MODE);
    }
}

CAN_OP_MODE_STATUS CAN2_OperationModeSet(const CAN_OP_MODES requestMode) 
{
    CAN_OP_MODE_STATUS status = CAN_OP_MODE_REQUEST_SUCCESS;
    
    if((CAN_CONFIGURATION_MODE == CAN2_OperationModeGet()) || (requestMode == CAN_DISABLE_MODE)
            || (requestMode == CAN_CONFIGURATION_MODE))
    {
        C2CONHbits.REQOP = requestMode;

        while(C2CONHbits.OPMOD != requestMode) 
        {
            //This condition is avoiding the system error case endless loop
            if(C2INTLbits.SERRIF == 1)
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

CAN_OP_MODES CAN2_OperationModeGet(void) 
{
    return C2CONHbits.OPMOD;
}

bool CAN2_Receive(CAN_MSG_OBJ *rxCanMsg)
{
    uint8_t fifoChannel, count;
    CAN2_RX_FIFO_STATUS rxMsgStatus;
    CAN2_FIFO_INFO fifoInfo;
    bool status = false;
    
    //Iterate all receive FIFO's and read the message object
    for(count = 0; count < CAN2_NUM_OF_RX_FIFO; count++)
    {
        fifoChannel = rxFIFOMsg[count].channel;
        CAN2_FIFO_InfoGet(fifoChannel, &fifoInfo);
        rxMsgStatus = CAN2_RX_FIFO_StatusGet(fifoChannel);
        
        //If message object is available
        if(CAN_RX_MSG_AVAILABLE == (rxMsgStatus & CAN_RX_MSG_AVAILABLE)) 
        {           
            if(*(fifoInfo.address) != NULL)
            {
                CAN2_MessageReadFromFifo((uint16_t *) *fifoInfo.address, rxCanMsg);
                CAN2_RX_FIFO_IncrementMsgPtr(fifoChannel);
                
                // Update the RX FIFO Head count for CAN2_ReceivedMessageCountGet function
                rxFIFOMsg[count].headCount += 1; //Update the read one message
                if(rxFIFOMsg[count].headCount >= fifoInfo.msgDeepSize) 
                {
                    rxFIFOMsg[count].headCount = 0; //Reset the read message count
                }
                                
                //User have to clear manually RX Overflow status
                if(CAN_RX_MSG_OVERFLOW == (rxMsgStatus & CAN_RX_MSG_OVERFLOW))
                {
                    CAN2_RX_FIFO_OverflowStatusFlagClear(fifoChannel);
                }
                
                status = true;
            }
            
            break;
        }
    }   
    
    return status;
}

uint8_t CAN2_ReceivedMessageCountGet(void)
{
    uint8_t fifoChannel = 0, count = 0, numOfMsg = 0, totalMsgObj = 0;
    CAN2_RX_FIFO_STATUS rxMsgStatus;
    CAN2_FIFO_INFO fifoInfo;
    uint16_t rxfifoMsgTail;
    
    //Iterate all receive FIFO's and get the message object count
    for(count = 0; count < CAN2_NUM_OF_RX_FIFO; count++)
    {
        fifoChannel = rxFIFOMsg[count].channel;
        CAN2_FIFO_InfoGet(fifoChannel, &fifoInfo);
        rxMsgStatus = CAN2_RX_FIFO_StatusGet(fifoChannel);
        
        //If message object is available
        if(CAN_RX_MSG_AVAILABLE == (rxMsgStatus & CAN_RX_MSG_AVAILABLE)) 
        {
            //If receive FIFO overflow has occurred, FIFO is full 
            if(CAN_RX_MSG_OVERFLOW == (rxMsgStatus & CAN_RX_MSG_OVERFLOW))
            {
                numOfMsg = fifoInfo.msgDeepSize;
            } 
            else
            {
                rxfifoMsgTail = CAN2_RX_FIFO_MessageIndexGet(fifoChannel);
                
                if(rxfifoMsgTail < rxFIFOMsg[count].headCount)
                {
                    numOfMsg = ((rxfifoMsgTail + fifoInfo.msgDeepSize) - rxFIFOMsg[count].headCount);
                }
                else if(rxfifoMsgTail > rxFIFOMsg[count].headCount)
                {
                    numOfMsg = rxfifoMsgTail - rxFIFOMsg[count].headCount;
                }
                else
                {
                    numOfMsg = fifoInfo.msgDeepSize;
                }
            }
            
            totalMsgObj += numOfMsg;
        }
    }
    
    return totalMsgObj;
}

bool CAN2_IsBusOff(void)
{
    return C2TRECHbits.TXBO;
}

bool CAN2_IsRxErrorPassive(void)
{
    return C2TRECHbits.RXBP;
}

bool CAN2_IsRxErrorWarning(void)
{
    return C2TRECHbits.RXWARN;
}

bool CAN2_IsRxErrorActive(void)
{
    bool errorState = false;
    if((0 < C2TRECLbits.RERRCNT) && (C2TRECLbits.RERRCNT < 128)) 
    {
        errorState = true;
    }
    
    return errorState;
}

void CAN2_Sleep(void)
{
    C2INTLbits.WAKIF = 0;
    C2INTHbits.WAKIE = 1;
    
    // CAN Info Interrupt Enable bit
    IEC2bits.C2IE = 1;  
    
    /* put the module in disable mode */
    CAN2_OperationModeSet(CAN_DISABLE_MODE);
}

void __attribute__((weak)) CAN2_DefaultInvalidMessageHandler(void) 
{

}

void __attribute__((weak)) CAN2_DefaultBusWakeUpActivityHandler(void) 
{

}

void __attribute__((weak)) CAN2_DefaultBusErrorHandler(void) 
{

}

void __attribute__((weak)) CAN2_DefaultModeChangeHandler(void) 
{

}

void __attribute__((weak)) CAN2_DefaultSystemErrorHandler(void) 
{

}

void __attribute__((weak)) CAN2_DefaultRxBufferOverFlowHandler(void) 
{

}

void __attribute__((__interrupt__, no_auto_psv)) _C2Interrupt(void)
{
    // Bus Wake-up Activity Interrupt 
    if (C2INTLbits.WAKIF == 1) 
    {
        CAN2_DefaultBusWakeUpActivityHandler();
        C2INTLbits.WAKIF = 0;
    }
    
    IFS2bits.C2IF = 0;
}

void CAN2_Tasks( void )
{
    if (C2INTLbits.IVMIF == 1)
    {
        CAN2_DefaultInvalidMessageHandler();
        C2INTLbits.IVMIF = 0;
    }

    if (C2INTLbits.CERRIF == 1) 
    {
        CAN2_DefaultBusErrorHandler();
        C2INTLbits.CERRIF = 0;
    }

    if (C2INTLbits.MODIF == 1) 
    {
        CAN2_DefaultModeChangeHandler();
        C2INTLbits.MODIF = 0;
    }
    
    if (C2INTLbits.SERRIF == 1) 
    {
        CAN2_DefaultSystemErrorHandler();
        C2INTLbits.SERRIF = 0;
    }
    
    if(C2FIFOSTA1bits.RXOVIF == 1)
    {
        CAN2_DefaultRxBufferOverFlowHandler();
        C2FIFOSTA1bits.RXOVIF = 0;
    }
}

/**
 End of File
*/


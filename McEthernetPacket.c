/**********************************************************************************************************************
 * \file McEthernet.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of 
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and 
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all 
 * derivative works of the Software, unless such copies or derivative works are solely in the form of 
 * machine-executable object code generated by a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *********************************************************************************************************************/


/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "McEthernetPacket.h"
#include "Spi.h"
#include "pinsReadWrite.h"
#include "Bsp.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define DMA_ETHRECEIVEPRIORITY   4
#define MC_ETHERNET_RESET        &MODULE_P33,10
#define MC_ETHERNET_SPIBAUDRATE  10000000
#define MC_ETHERNET_WRITECOMMAND 2
#define MC_ETHERNET_READCOMMAND  0
#define MC_ETHERNET_WRITEBUFFER  0x7A
#define MC_ETHERNET_READBUFFER   0x3A
#define MC_ETHERNET_SETBIT       0x4
#define MC_ETHERNET_CLEARBIT     0x5
#define MC_ETHERNET_TYPESIZE     2
#define BANK0                    0
#define BANK1                    1
#define BANK2                    2
#define BANK3                    3
#define ECOCON                   0x15
#define ECON1                    0x1F
#define ECON2                    0x1E
#define MACON1                   0x00
#define MACON3                   0x02
#define MACON4                   0x03
#define ERXFCON                  0x18
#define MAADR1                   0x04
#define MAADR2                   0x05
#define MAADR3                   0x02
#define MAADR4                   0x03
#define MAADR5                   0x00
#define MAADR6                   0x01
#define MAMXFLL                  0x0A
#define MAMXFLH                  0x0B
#define ETXSTL                   0x04
#define ETXSTH                   0x05
#define ETXNDL                   0x06
#define ETXNDH                   0x07
#define ERDPTL                   0x00
#define ERDPTH                   0x01
#define EWRPTL                   0x02
#define EWRPTH                   0x03
#define ERXSTL                   0x08
#define ERXSTH                   0x09
#define ESTAT                    0x1D
#define ERXNDL                   0x0A
#define ERXNDH                   0x0B
#define EIR                      0x1C
#define EIE                      0x1B
#define MABBIPG                  0x04
#define MAIPGL                   0x06
#define MAIPGH                   0x07
#define EPKTCNT                  0x19
#define ERXWRPTL                 0x0E
#define ERXWRPTH                 0x0F
#define EPMM0                    0x08
#define EPMCSL                   0x10
#define EPMCSH                   0x11
#define ERXRDPTL                 0x0C
#define ERXRDPTH                 0x0D
#define EPMOL                    0x14
#define EPMOH                    0x15
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* SPI Configuration Registers */
SpiMasterPins_t McEthPins = {
    &IfxQspi1_SCLK_P10_2_OUT,
    &IfxQspi1_MTSR_P10_3_OUT,
    &IfxQspi1_MRSTA_P10_1_IN
};

SpiMaster_t McEthSpiMaster;

SpiMasterCfg_t McEthSpiConfig = {
    &McEthSpiMaster,
    46,
    47,
    48
};

SpiChannel_t McEthChannel;

SpiChannelConfig McEthChannelCfg = {
    &IfxQspi1_SLSO5_P11_2_OUT,
    MC_ETHERNET_SPIBAUDRATE,
    0,
    SpiIf_ShiftClock_shiftTransmitDataOnTrailingEdge,
    SpiIf_DataHeading_msbFirst
};

/* Definition of the Ethernet packet specific parts */
static uint8 McEth_TxDstMacAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8 McEth_TxSrcMacAddress[6] = {0, 0, 0, 0, 0, 0};
static uint8 McEth_RxDstMacAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8 McEth_RxSrcMacAddress[6] = {0, 0, 0, 0, 0, 0};
static uint8 McEth_TxTypeLength[2] = {0x00, 0x00};
static uint8 McEth_RxTypeLength[2] = {0x00, 0x00};
static uint8 McEth_TransmitStatusVector[7];
static uint8 McEth_ReceiveStatusVector[4] = {0, 0, 0, 0};
uint16 McEth_WriteAddress = 0;
static uint16 McEth_ReadAddress = 0;
uint8 McEth_ReceiveBytesNum = 0;
volatile uint16 McEth_WriteIndex = 0;
volatile uint16 McEth_ReadIndex = 0;

static MCETH_PACKET McEth_MasterPacketTransmit =
{
    McEth_TxDstMacAddress,
    McEth_TxSrcMacAddress,
    McEth_TxTypeLength,
    MC_ETHERNET_MACSIZE,
    MC_ETHERNET_TYPESIZE,
    TRANSMIT_MESSAGE
};

static MCETH_PACKET McEth_MasterPacketReceive =
{
    McEth_RxSrcMacAddress,
    McEth_RxDstMacAddress,
    McEth_RxTypeLength,
    MC_ETHERNET_MACSIZE,
    MC_ETHERNET_TYPESIZE,
    RECEIVE_MESSAGE
};

IfxDma_Dma_Channel McEthiIp_DmaReceivePachetHandler;

static uint8 McEth_RxBuffer[2];
uint8 McEth_Buf[MC_ETHERNET_MAXFRAME];
/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
static void McEth_WriteRegister(uint8 Register, uint8 Command);
static uint8 McEth_ReadRegister(uint8 Register);
static void McEth_WriteBuffer(uint8* Command, uint16 CommandSize);
static void McEth_ReadBuffer(uint8* Result ,uint16 ResultSize);
static void McEth_ReadPayload(uint16 ResultSize);
static void McEth_SetBit(uint8 Register, uint8 Mask);
static void McEth_ClearBit(uint8 Register, uint8 Mask);
static void McEth_ReadTransmitStatusVector(void);
static void McEth_SetBank(uint8 Bank);
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
IFX_INTERRUPT(SpimasterTxMcEth, 0, 46);                  /* SPI Master ISR for transmit data         */
IFX_INTERRUPT(SpimasterRxMcEth, 0, 47);                  /* SPI Master ISR for receive data          */
IFX_INTERRUPT(SpimasterErMcEth, 0, 48);                 /* SPI Master ISR for error                 */


/** \brief Isr function for the interrupt after the successfull reception
 * \param McEth_DmaReceiveIsr interrupt receive function
 * \return None
 */
IFX_INTERRUPT(McEth_DmaReceiveIsr, 0, DMA_ETHRECEIVEPRIORITY);
void McEth_DmaReceiveIsr(void)
{
    if (McEth_ReceiveBytesNum > 128)
    {
      McEth_ReadIndex = (McEth_ReadIndex + 128) % MC_ETHERNET_MAXFRAME;
    }
    else
    {
      McEth_ReadIndex = (McEth_ReadIndex + McEth_ReceiveBytesNum) % MC_ETHERNET_MAXFRAME;
    }
     /* It goes inside no need to do anything yet */
}


 void SpimasterTxMcEth(void)
 {
     IfxCpu_enableInterrupts();
     IfxQspi_SpiMaster_isrTransmit(McEthSpiConfig.SpiMasterPtr);
 }

 void SpimasterRxMcEth(void )
 {
     IfxCpu_enableInterrupts();
     IfxQspi_SpiMaster_isrReceive(McEthSpiConfig.SpiMasterPtr);
 }

 void SpimasterErMcEth(void)
 {
     IfxCpu_enableInterrupts();
     IfxQspi_SpiMaster_isrError(McEthSpiConfig.SpiMasterPtr);
 }



/** \brief Update internal data when incremental mode is using T2.\n
 * This function is the SPI, RESET_PIN and Microchip ethernet module configuration
 * \param McEth_Init configuration setup function
 * \return None
 */
void McEth_Init(uint8* MacAddress)
{
  McEthiIp_DmaReceivePachetHandler.channelId = IfxDma_ChannelId_1;
  /* Initialize the Source and Destination to be the same  then it will change in the next transaction */
  Dma_Init(&McEthiIp_DmaReceivePachetHandler, 128, (uint32)(&McEth_Buf[0]), (uint32)(&McEth_Buf[0]), DMA_ETHRECEIVEPRIORITY);

  uint8 tmp;
  Ifx_TickTime delay100ms = IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER,100);
  Ifx_TickTime delay1ms = IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER,1);
  /* Reset the module */
  setPinOutput(MC_ETHERNET_RESET);
  wait(delay1ms);
  setPinOutputTrue(MC_ETHERNET_RESET);

  Spi_Init(&McEthPins, &McEthSpiConfig);
  Spi_ChannelInit(&McEthSpiConfig, &McEthChannel,&McEthChannelCfg);
  wait(delay100ms);

  /* Bank 0 register */
  McEth_SetBank(BANK0);

  /* Receive Buffer space */
  McEth_WriteRegister(ERXNDL, (uint8)(0xFFF & 0x00FF));
  McEth_WriteRegister(ERXNDH, (uint8)(0xFFF >> 8));

  McEth_WriteRegister(ERXSTL, (uint8)(0x000 & 0x00FF));
  McEth_WriteRegister(ERXSTH, (uint8)(0x000 >> 8));

  McEth_WriteRegister(ERXRDPTL, (uint8)(0xFFE & 0x00FF));
  McEth_WriteRegister(ERXRDPTH, (uint8)(0xFFE >> 8));

  /* Initialize read pointer */
  McEth_WriteRegister(ERDPTL, (uint8)(0x002 & 0x00FF));
  McEth_WriteRegister(ERDPTH, (uint8)(0x002 >> 8));

  McEth_ReadAddress = 0x000;

  /* Transmit Buffer space */
  McEth_WriteRegister(ETXNDL, (uint8)(0x1000 & 0x00FF));
  McEth_WriteRegister(ETXNDH, (uint8)(0x1000 >> 8));

  McEth_WriteRegister(ETXSTL, (uint8)(0x1000 & 0x00FF));
  McEth_WriteRegister(ETXSTH, (uint8)(0x1000 >> 8));

  McEth_WriteAddress = 0x1000;

  /* Bank 1 register */
  McEth_SetBank(BANK1);

  /* Filter receive register */
  McEth_WriteRegister(ERXFCON, 0xE0);

  /* PATTERN REGISTERS are set 0x0800  (this need to be checked one day still doesn't work) */
  McEth_WriteRegister(EPMOL, 0x00);
  McEth_WriteRegister(EPMOH, 0x00);
  McEth_WriteRegister(EPMCSL, 0x00);
  McEth_WriteRegister(EPMCSH, 0x08);
  McEth_WriteRegister(EPMM0, 0x00);
  McEth_WriteRegister((EPMM0 + 1), 0x30);
  for (uint8 i = 2; i < 8; i++)
  {
    McEth_WriteRegister((EPMM0 + i), 0x00);
  }

  /* Wait for OST */
  while ((McEth_ReadRegister(ESTAT) & 0x1) != 0x1) {}

  /* Bank 2 register */
  McEth_SetBank(BANK2);
  tmp = McEth_ReadRegister(ECON1);
 /* MACON 1 */
  McEth_WriteRegister(MACON1, 0xF);

  /* MACON 3 */
  McEth_WriteRegister(MACON3, 0xF6);

  /* MACON 4 */
  McEth_WriteRegister(MACON4, 0x40);

  /* Max Number of bytes per message */
  McEth_WriteRegister(MAMXFLL, (uint8)(MC_ETHERNET_MAXFRAME & 0x00FF));

  McEth_WriteRegister(MAMXFLH, (uint8)(MC_ETHERNET_MAXFRAME >> 8));

  /* Enable Back to Back inter packet */
  McEth_WriteRegister(MABBIPG, 0x12);

  /* Non back to back inter packet gap low byte */
  McEth_WriteRegister(MAIPGL, 0x12);
  McEth_WriteRegister(MAIPGH, 0xC0);

  McEth_SetSrcMacAddress(MacAddress);

  /* Auto increment of buffer pointer */
  McEth_WriteRegister(ECON2, 0x80);
}



/** \brief Update internal data when incremental mode is using T2.\n
 * This function set ups the mac address
 * \param McEth_SetSrcMacAddress configuration setup function
 * \return None
 */
void McEth_SetSrcMacAddress(uint8* MacAddress)
{
  uint8 tmp;
  for (uint8 i = 0; i < MC_ETHERNET_MACSIZE; i++)
  {
    McEth_MasterPacketTransmit.SrcMacAddress[i] = MacAddress[i];
  }
  /* Bank 3 register */
  McEth_SetBank(BANK3);
  tmp = McEth_ReadRegister(ECON1);
  /* Mac Address registers */
  McEth_WriteRegister(MAADR1, MacAddress[0]);
  McEth_WriteRegister(MAADR2, MacAddress[1]);
  McEth_WriteRegister(MAADR3, MacAddress[2]);
  McEth_WriteRegister(MAADR4, MacAddress[3]);
  McEth_WriteRegister(MAADR5, MacAddress[4]);
  McEth_WriteRegister(MAADR6, MacAddress[5]);
}



/** \brief Returns the Mac Address to a specific position of an array
 * \param McEth_GetSrcMacAddress
 * \return None
 */
void McEth_GetSrcMacAddress(uint8* ptr)
{
  for (uint8  i = 0; i < MC_ETHERNET_MACSIZE; i++)
  {
    ptr[i] =  McEth_MasterPacketTransmit.SrcMacAddress[i];
  }
}



/** \brief Sets the DstMacAddress
 * \param McEth_SetDstMacAddress
 * \return None
 */
void McEth_SetDstMacAddress(uint8* MacAddress)
{
  for (uint8 i = 0; i < MC_ETHERNET_MACSIZE; i++)
  {
    McEth_MasterPacketTransmit.DstMacAddress[i] = MacAddress[i];
  }
}



/** \brief Gets the DstMacAddress
 * \param McEth_GetDstMacAddress
 * \return None
 */
void McEth_GetDstMacAddress(uint8* ptr)
{
  for (uint8 i = 0; i < MC_ETHERNET_MACSIZE; i++)
  {
     ptr[i] = McEth_MasterPacketTransmit.DstMacAddress[i];
  }
}



/** \brief Creates a simple ethernet packet
 * This function creates the main Transmit Packet Structure
 * \param McEth_CreateTransmitPacket configuration setup function
 * \return void
 */
void McEth_CreateTransmitPacket(uint8* Type)
{
  uint16 EndAddress;
  static uint8 MessagePacket[MC_ETHERNET_MACSIZE];
  /* Bank 0  Register */
  McEth_SetBank(BANK0);
  McEth_WriteRegister(ECON1, 0x0);

  /* Set Write Pointer */
  McEth_WriteRegister(EWRPTL, (uint8)((0x1000+1) & 0x00FF));
  McEth_WriteRegister(EWRPTH, (uint8)((0x1000+1) >> 8));

  /* End of message Address transmission */
  EndAddress = 14 + McEth_WriteAddress;

  McEth_WriteRegister(ETXNDL, (uint8)(EndAddress & 0x00FF));
  McEth_WriteRegister(ETXNDH, (uint8)(EndAddress >> 8));


  McEth_GetDstMacAddress(MessagePacket);
  McEth_WriteBuffer(MessagePacket, MC_ETHERNET_MACSIZE);

  McEth_GetSrcMacAddress(MessagePacket);
  McEth_WriteBuffer(MessagePacket, MC_ETHERNET_MACSIZE);

  McEth_MasterPacketTransmit.TypeLength[0] = Type[0];
  McEth_MasterPacketTransmit.TypeLength[1] = Type[1];
  MessagePacket[0] = McEth_MasterPacketTransmit.TypeLength[0];
  MessagePacket[1] = McEth_MasterPacketTransmit.TypeLength[1];
  McEth_WriteBuffer(MessagePacket, MC_ETHERNET_TYPESIZE);


  /* Update WriteAddress to read the Transmit Status Vector */
  McEth_WriteAddress = EndAddress + 1;

}



/** \brief Pushes data to the transmit buffer
 * This function returns true if the message has been pushed successfully
 * \param McEth_PushTransmitMessage Transmit Packet Setup function
 * \return boolean
 */
boolean McEth_PushTransmitMessage(uint8* Message, uint16 MessageSize)
{

  /* End of message Address transmission */
  uint8 EndAddress = MessageSize + McEth_WriteAddress;

  if (EndAddress >= 0x1FFF)
  {
    return FALSE;
  }
  else
  {
    McEth_WriteAddress = EndAddress + 1;
  }


  McEth_WriteRegister(ETXNDL, (uint8)(EndAddress & 0x00FF));
  McEth_WriteRegister(ETXNDH, (uint8)(EndAddress >> 8));

  /* Append message */
  McEth_WriteBuffer(Message, MessageSize);

  return TRUE;
}



/** \brief Transmits the created message from the transmit buffer
 * This function returns true when transmition is succeed
 * \param McEth_TransmitMessage Transmit Message function
 * \return boolean
 */
boolean McEth_TransmitMessage(void)
{
  boolean Result = TRUE;
  /* Enable transmit receive */
  McEth_WriteRegister(ECON1, 0x8);
  /* Clear TXIF */
  McEth_ClearBit(EIR, 0x8);

  /* Set TXIE, INTIE */
  McEth_WriteRegister(EIE, 0x88);
  uint8 EIRVAL = McEth_ReadRegister(EIR);

  /* RETURN AFTER transmission succeed */
  while ((EIRVAL & 0x8) == 0x8)
  {
    if ((EIRVAL & 0x2) & 0x2)
    {
      Result = FALSE;
      break;
    }
    EIRVAL = McEth_ReadRegister(EIR);

  }


  McEth_ReadTransmitStatusVector();
  return Result;
}



/** \brief Update internal data when incremental mode is using T2.\n
 * This function reads the status vector during transmittion
 * \param McEth_ReadTransmitStatusVector read function
 * \return None
 */
static void McEth_ReadTransmitStatusVector(void)
{
  /* BANK 0 register */
  McEth_SetBank(BANK0);
  /* Set Read Pointer */
  McEth_WriteRegister(ERDPTL, (uint8)((McEth_WriteAddress) & 0x00FF));
  McEth_WriteRegister(ERDPTH, (uint8)((McEth_WriteAddress) >> 8));

  McEth_ReadBuffer(McEth_TransmitStatusVector, sizeof(McEth_TransmitStatusVector));

  /* Reset the Write Pointer */
  McEth_WriteRegister(EWRPTL, (uint8)((0x1000+1) & 0x00FF));
  McEth_WriteRegister(EWRPTH, (uint8)((0x1000+1) >> 8));

  McEth_WriteAddress = 0x1000;

}



/** \brief Takes the Received Message
 * This function reads if the message is received for time in ms
 * \param McEthernert_ReceiveMsg read function
 * \return boolean
 */
boolean McEth_ReceiveMsg(float32 time)
{
  uint64 stmTickTimeInit = IfxStm_get(&MODULE_STM0);
  uint64 stmTickTimeAcl;
  boolean success = FALSE;
  boolean readFlag = FALSE;
  uint16 ReceiveWritePointer;
  uint8 EIRreg = 0;


  McEth_SetBank(BANK0);
  /* Check FIFO */
  ReceiveWritePointer = (uint16)((McEth_ReadRegister(ERXWRPTH) << 8) | (McEth_ReadRegister(ERXWRPTL) & 0x00FF));

  if (ReceiveWritePointer > (0xFFF - 100))
  {
    McEth_WriteRegister(ECON1, 0x0);

    McEth_WriteRegister(ERXWRPTH, (uint8)(0x0 >> 8));
    McEth_WriteRegister(ERXWRPTL, (uint8)(0x0 & 0x00FF));

    /* Initialize read pointer */
    McEth_WriteRegister(ERDPTL, (uint8)(0x2 & 0x00FF));
    McEth_WriteRegister(ERDPTH, (uint8)(0x2 >> 8));

    McEth_WriteRegister(ERXRDPTL, (uint8)(0xFFE & 0x00FF));
    McEth_WriteRegister(ERXRDPTH, (uint8)(0xFFE >> 8));

    McEth_ReadAddress = 0x0;
  }
  /* Enable Rx Interrupt */
  McEth_WriteRegister(EIE, 0xC1);
  EIRreg = McEth_ReadRegister(EIE);
  /* Enable Reception */
  McEth_WriteRegister(ECON1, 0x4);
  McEth_SetBank(BANK0);

  while (readFlag == FALSE)
  {
    EIRreg = McEth_ReadRegister(EIR);
    /* Check if Error occured */
    if ((EIRreg & 0x1) == 0x1)
    {
      McEth_ClearBit(EIR, 0x1);
      McEth_SetBit(EIE, 0x81);
      McEth_ClearBit(ECON1, 0x4);
      success = FALSE;
      readFlag = TRUE;
    }
    stmTickTimeAcl = IfxStm_get(&MODULE_STM0);
    /* Check if Timeout occured */
    if ((stmTickTimeAcl - stmTickTimeInit) > IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, time))
    {
      McEth_ClearBit(ECON1, 0x4);
      success = FALSE;
      readFlag = TRUE;
    }
    /* Packet sent interrupt bit is set */
    if ((EIRreg & 0x40) == 0x40)
    {
      success = TRUE;
      McEth_ClearBit(ECON1, 0x4);
      EIRreg = McEth_ReadRegister(ECON1);
      readFlag = TRUE;
    }

  }
  return success;
}



/** \brief Update internal data when incremental mode is using T2.\n
 * This function reads if the message is received for ever
 * \param McEth_ReceiveMsgInf read function
 * \return boolean
 */
boolean McEth_ReceiveMsgInf(void)
{
  boolean success = FALSE;
  boolean readFlag = FALSE;
  uint16 ReceiveWritePointer = 0;
  uint8 EIRreg = 0;

  McEth_SetBank(BANK0);
  /* Check FIFO */
  ReceiveWritePointer = (uint16)((McEth_ReadRegister(ERXWRPTH) << 8) | (McEth_ReadRegister(ERXWRPTL) & 0x00FF));

  if (ReceiveWritePointer > (0xFFF - 100))
  {
    McEth_WriteRegister(ECON1, 0x0);

    McEth_WriteRegister(ERXWRPTH, (uint8)(0x0 >> 8));
    McEth_WriteRegister(ERXWRPTL, (uint8)(0x0 & 0x00FF));

    /* Initialize read pointer */
    McEth_WriteRegister(ERDPTL, (uint8)(0x2 & 0x00FF));
    McEth_WriteRegister(ERDPTH, (uint8)(0x2 >> 8));

    McEth_WriteRegister(ERXRDPTL, (uint8)(0xFFE & 0x00FF));
    McEth_WriteRegister(ERXRDPTH, (uint8)(0xFFE >> 8));

    McEth_ReadAddress = 0x0;
  }

  /* Enable Rx Interrupt */
  McEth_WriteRegister(EIE, 0xC1);
  EIRreg = McEth_ReadRegister(EIE);
  /* Enable Reception */
  McEth_WriteRegister(ECON1, 0x4);
  McEth_SetBank(BANK0);


  while (readFlag == FALSE)
  {

    EIRreg = McEth_ReadRegister(EIR);
    /* Check if Error occured */
    if ((EIRreg & 0x1) == 0x1)
    {
      McEth_ClearBit(EIR, 0x1);
      McEth_SetBit(EIE, 0x81);
      McEth_ClearBit(ECON1, 0x4);
      success = FALSE;
      readFlag = TRUE;
    }
    /* Packet sent interrupt bit is set */
    if ((EIRreg & 0x40) == 0x40)
    {
      success = TRUE;
      McEth_ClearBit(ECON1, 0x4);
      EIRreg = McEth_ReadRegister(ECON1);
      readFlag = TRUE;
    }

  }
  return success;
}



/** \brief Reads the main properties of the received Packet
 * This function reads the packet after successfull receive
 * \param McEth_ReadEthernetProperties read function
 * \return None
 */
void McEth_ReadEthernetProperties(void)
{

  /* Bank Register 0 */
  McEth_SetBank(BANK0);

  /* Read Status Vector */
  McEth_ReadBuffer(McEth_ReceiveStatusVector , sizeof(McEth_ReceiveStatusVector));

  McEth_ReceiveBytesNum = (uint16)((McEth_ReceiveStatusVector[1] << 8) | (McEth_ReceiveStatusVector[0] & 0x00FF));

  /* Read Destination Address */
  McEth_ReadBuffer(McEth_MasterPacketReceive.DstMacAddress, McEth_MasterPacketReceive.MacAddressSize);
  McEth_ReceiveBytesNum -= McEth_MasterPacketReceive.MacAddressSize;

  /* Read Source Address */
  McEth_ReadBuffer(McEth_MasterPacketReceive.SrcMacAddress, McEth_MasterPacketReceive.MacAddressSize);
  McEth_ReceiveBytesNum -= McEth_MasterPacketReceive.MacAddressSize;

  /* Read The TypeSize */
  McEth_ReadBuffer(McEth_MasterPacketReceive.TypeLength, McEth_MasterPacketReceive.TypeLengthSize);
  McEth_ReceiveBytesNum -= McEth_MasterPacketReceive.TypeLengthSize;
}


/** \brief Reads the main properties of the received Packet
 * This function reads the packet after successfull receive
 * \param McEth_ReadEthernetProperties read function
 * \return None
 */
uint16 McEth_ReturnReceiveType(void)
{
  uint16 TypeLength = (uint16)((McEth_MasterPacketReceive.TypeLength[0] << 8) | (McEth_MasterPacketReceive.TypeLength[1] & 0x00FF));
  return TypeLength;
}


/** \brief Reads a specific length of bytes and stores to the ReceivePtr
 * This function reads the packet after receive alarm
 * \param McEth_ReadReceivedBytes read function
 * \return None
 */
void McEth_ReadReceivedBytes(uint8* ReceivePtr, uint16 ReceivePtrSize)
{
  if (McEth_ReceiveBytesNum < ReceivePtrSize)
  {
    ReceivePtrSize = McEth_ReceiveBytesNum;
  }
  /* Read the Actual Packet */
  McEth_ReadBuffer(ReceivePtr, ReceivePtrSize);

  McEth_ReceiveBytesNum -= ReceivePtrSize;
}



/** \brief Reads a specific length of bytes and stores to the ReceivePtr
 * This function reads the packet after receive alarm
 * \param McEth_ReadReceivedPayload read function
 * \return None
 */
void McEth_ReadReceivedPayload(uint8* ReceivePtr, uint16 ReceivePtrSize)
{
  if (McEth_ReceiveBytesNum < ReceivePtrSize)
  {
    ReceivePtrSize = McEth_ReceiveBytesNum;
  }
  /* Read the Actual Packet */
  McEth_ReadPayload(ReceivePtrSize);

  IfxDma_Dma_setChannelTransferCount(&McEthiIp_DmaReceivePachetHandler, ReceivePtrSize);
  /* We choose the next index since we don't read at the same time we write with SPI during the first byte */
  Dma_SetSourceAddress (&McEthiIp_DmaReceivePachetHandler, Spi_ReturnSpiRxBufferAddr(1));
  Dma_SetDestinationAddress(&McEthiIp_DmaReceivePachetHandler, (uint32)ReceivePtr);
  Dma_Transfer(&McEthiIp_DmaReceivePachetHandler);

  McEth_ReceiveBytesNum -= ReceivePtrSize;

}



/** \brief Resets the Receive buffer and prepares for new read
 * This function resets the Receive buffer
 * \param McEth_ResetReceiveBuffer read function
 * \return None
 */
void McEth_ResetReceiveBuffer(void)
{
  /* Update ReadAddress */
  McEth_ReadAddress = 0;

  /* Bank Register 1 */
  McEth_SetBank(BANK1);

  /* Bank Register 1 */
  McEth_SetBank(BANK0);

  McEth_SetBit(ECON2, 0x40);
  while ((McEth_ReadRegister(EIR) & 0x40 == 0x40)) {}
  McEth_ClearBit(ECON2, 0x40);

  /* Receive Buffer space */
  McEth_WriteRegister(ERXNDL, (uint8)(0xFFF & 0x00FF));
  McEth_WriteRegister(ERXNDH, (uint8)(0xFFF >> 8));

  McEth_WriteRegister(ERXSTL, (uint8)(0x000 & 0x00FF));
  McEth_WriteRegister(ERXSTH, (uint8)(0x000 >> 8));
  /* Initialize read pointer */
  McEth_WriteRegister(ERDPTL, (uint8)((McEth_ReadAddress+2) & 0x00FF));
  McEth_WriteRegister(ERDPTH, (uint8)((McEth_ReadAddress+2) >> 8));


  McEth_WriteRegister(ERXRDPTL, (uint8)(0xFFE & 0x00FF));
  McEth_WriteRegister(ERXRDPTH, (uint8)(0xFFE >> 8));


  McEth_ReadAddress += 2;

}



/** \brief Update the transmit buffer in specific memory area
 * This McEth_WriteTransmitMemory Microchip Ethernet function
 * \param McEth_WriteTransmitMemory write function
 * \return boolean
 */
boolean McEth_WriteTransmitMemory(uint16 startMemory, uint8* newMessage, uint16 size)
{
  boolean result;
  uint16 currentMemory;
  if ((startMemory < 0x1000) & (startMemory > 0x1FFF) )
  {
    result = FALSE;
  }
  else
  {
    result = TRUE;
  }
  currentMemory  = (uint16)((McEth_ReadRegister(EWRPTH) << 8) | ( McEth_ReadRegister(EWRPTL) & 0x00FF));

  /* Set Write Pointer */
  McEth_WriteRegister(EWRPTL, (uint8)((startMemory) & 0x00FF));
  McEth_WriteRegister(EWRPTH, (uint8)((startMemory) >> 8));

  /* Overwrite the newMessage in the specific address */
  McEth_WriteBuffer(newMessage, size);

  /* Set Write Pointer Back to it's previous value */
  McEth_WriteRegister(EWRPTL, (uint8)((currentMemory) & 0x00FF));
  McEth_WriteRegister(EWRPTH, (uint8)((currentMemory) >> 8));


  return result;
}



/** \brief Reads the Write Pointer and return it
 * This McEth Write Microchip Ethernet function
 * \param McEth_ReadWritePointer write function
 * \return uint16
 */
uint16 McEth_ReadWritePointer(void)
{
  uint16 currentMemory = (uint16)((McEth_ReadRegister(EWRPTH) << 8) | (McEth_ReadRegister(EWRPTL) & 0x00FF));
  return currentMemory;
}

/** \brief Update internal data when incremental mode is using T2.\n
 * This McEth Write Microchip Ethernet function
 * \param McEth_WriteRegister write function
 * \return void
 */
static void McEth_WriteRegister(uint8 Register, uint8 Command)
{
  uint8 SpiBuffer[2] = {(MC_ETHERNET_WRITECOMMAND<<5) |Register, Command};
  Spi_WriteBytes(&McEthChannel, SpiBuffer, 2);
}



/** \brief Update internal data when incremental mode is using T2.\n
 * This McEth Reads Microchip Ethernet Register function
 * \param McEth_ReadRegister read function
 * \return uint8
 */
static uint8 McEth_ReadRegister(uint8 Register)
{
  uint8 SpiBuffer = (MC_ETHERNET_READCOMMAND<<5) |Register;
  Spi_ReadRegister(&McEthChannel, SpiBuffer, &McEth_RxBuffer[0], 1);
  return McEth_RxBuffer[0];
}



/** \brief This McEth Reads Microchip Ethernet Register function
 * \param McEth_WriteBuffer read function
 * \return void
 */
static void McEth_WriteBuffer(uint8* Command, uint16 CommandSize)
{
  if (CommandSize > 130)
  {
    return;
  }
  uint8 SpiBuffer[130];
  SpiBuffer[0] = MC_ETHERNET_WRITEBUFFER;
  for(uint16 i = 0; i < CommandSize; i++)
  {
    SpiBuffer[i+1] = Command[i];
  }
  Spi_WriteBytes(&McEthChannel, SpiBuffer, CommandSize+1);
}



/** \brief This McEth Reads Microchip Ethernet Register function and saves to spi buffer and then the spi buffer returns it to result
 * \param McEth_WriteBuffer read function
 * \return void
 */
static void McEth_ReadBuffer(uint8* Result ,uint16 ResultSize)
{
  if (ResultSize > 130)
  {
    return;
  }
  uint8 SpiBuffer = MC_ETHERNET_READBUFFER;
  Spi_ReadBytes(&McEthChannel, &SpiBuffer, 1, Result, ResultSize);
}



/** \brief This McEth Reads Microchip Ethernet Register function and saves to spi buffer
 * \param McEth_ReadPayload read function
 * \return void
 */
static void McEth_ReadPayload(uint16 ResultSize)
{
  if (ResultSize > 130)
  {
    return;
  }
  uint8 SpiBuffer = MC_ETHERNET_READBUFFER;
  Spi_ReadBuffer(&McEthChannel, &SpiBuffer, 1,ResultSize);
}



/** \brief This sets a specific bit in a register
 * \param McEth_SetBit read function
 * \return void
 */
static void McEth_SetBit(uint8 Register, uint8 Mask)
{
  uint8 SpiBuffer[2] = {(MC_ETHERNET_SETBIT<<5)|Register, Mask};
  Spi_WriteBytes(&McEthChannel, SpiBuffer, 2);
}



/** \brief This clears a specific bit in a register
 * \param McEth_ClearBit read function
 * \return void
 */
static void McEth_ClearBit(uint8 Register, uint8 Mask)
{
  uint8 SpiBuffer[2] = {(MC_ETHERNET_CLEARBIT<<5)|Register, Mask};
  Spi_WriteBytes(&McEthChannel, SpiBuffer, 2);
}



/** \brief This sets a specific bank of registers
 * \param McEth_SetBank read function
 * \return void
 */
static void McEth_SetBank(uint8 Bank)
{
  uint8 ECON1reg = 0;
  switch (Bank)
  {
    case 0:
      ECON1reg = McEth_ReadRegister(ECON1);
      ECON1reg = (ECON1reg & 0xFC) | 0x0;
      McEth_WriteRegister(ECON1, ECON1reg);
      break;

    case 1:
      ECON1reg = McEth_ReadRegister(ECON1);
      ECON1reg = (ECON1reg & 0xFC) | 0x1;
      McEth_WriteRegister(ECON1, ECON1reg);
      break;

    case 2:
      ECON1reg = McEth_ReadRegister(ECON1);
      ECON1reg = (ECON1reg & 0xFC) | 0x2;
      McEth_WriteRegister(ECON1, ECON1reg);
      break;

    case 3:
      ECON1reg = McEth_ReadRegister(ECON1);
      ECON1reg = (ECON1reg & 0xFC) | 0x3;
      McEth_WriteRegister(ECON1, ECON1reg);
      break;

    default:
      ECON1reg = McEth_ReadRegister(ECON1);
      ECON1reg = (ECON1reg & 0xFC) | 0x0;
      McEth_WriteRegister(ECON1, ECON1reg);
      break;
  }
}


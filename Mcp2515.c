/**********************************************************************************************************************
 * \file Mcp2515.c
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
#include "Mcp2515.h"
#include "Bsp.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define WRITE_COMMAND 2
#define READ_COMMAND 3
#define BIT_MODIFY 5

/* Can Transmit Registers */
#define CNF1     0x2A
#define CNF2     0x29
#define CNF3     0x28
#define TXB1CTRL 0x30
#define TXB2CTRL 0x40
#define TXB3CTRL 0x50
#define TXRTSCTRL 0x0D
#define TXB1SIDH 0x31
#define TXB1SIDL 0x32
#define TXB1EID8 0x33
#define TXB1EID0 0x34
#define TXB1DLC  0x35
#define TXB1D0   0x36
#define CANCTRL  0xF
#define CANSTAT  0xE
#define SPIBAUDRATE 10000000.0

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/* SPI Configuration Registers */
SpiMasterPins_t Mcp2515SpiPins = {
    &IfxQspi1_SCLK_P10_2_OUT,
    &IfxQspi1_MTSR_P10_3_OUT,
    &IfxQspi1_MRSTA_P10_1_IN
};

SpiMaster_t Mcp2515SpiMaster;

SpiMasterCfg_t Mcp2515SpiConfig = {
    &Mcp2515SpiMaster,
    43,
    44,
    45
};

SpiChannel_t Mcp2515Channel;

SpiChannelConfig McpChannelCfg = {
    &IfxQspi1_SLSO5_P11_2_OUT,
    SPIBAUDRATE,
    0,
    SpiIf_ShiftClock_shiftTransmitDataOnTrailingEdge,
    SpiIf_DataHeading_msbFirst
};

uint8 Mcp2515ReadBuffer[30];

/* There are three dedicated registers for read/write , we choose the first */
uint8 regNum = 0;

MCP2515_BAUDRATE Mcp2515Baudrate;
/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
static inline boolean Mcp2515_checkMsgSent(void);
static inline void Mcp2515_WriteRegister(uint8 Register, uint8 Command);
static inline void Mcp2515_BitModifyReg(uint8 Register, uint8 Mask, uint8 Value);
static inline void Mcp2515_ReadRegister(uint8 Register, uint8 RegsNum);
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
IFX_INTERRUPT(SpimasterTxMcp2515, 0, 43);                  /* SPI Master ISR for transmit data         */
IFX_INTERRUPT(SpimasterRxMcp2515, 0, 44);                  /* SPI Master ISR for receive data          */
IFX_INTERRUPT(SpimasterErMcp2515, 0, 45);                 /* SPI Master ISR for error                 */


 void SpimasterTxMcp2515(void)
 {
     IfxCpu_enableInterrupts();
     IfxQspi_SpiMaster_isrTransmit(Mcp2515SpiConfig.SpiMasterPtr);
 }

 void SpimasterRxMcp2515(void )
 {
     IfxCpu_enableInterrupts();
     IfxQspi_SpiMaster_isrReceive(Mcp2515SpiConfig.SpiMasterPtr);
 }

 void SpimasterErMcp2515(void)
 {
     IfxCpu_enableInterrupts();
     IfxQspi_SpiMaster_isrError(Mcp2515SpiConfig.SpiMasterPtr);
 }


void Mcp2515_Init(void)
{
  Spi_Init(&Mcp2515SpiPins, &Mcp2515SpiConfig);
  Spi_ChannelInit(&Mcp2515SpiConfig, &Mcp2515Channel,&McpChannelCfg);
  Ifx_TickTime delay = IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER,100);
  wait(delay);
  uint8 ResetVal = 0xC0;
  //setPinOutputFalse(&MODULE_P11,2);
  Spi_WriteBytes(&Mcp2515Channel, &ResetVal, 1);
  Mcp2515_WriteRegister(CANCTRL, 0x80);
  Mcp2515_ReadRegister(CANCTRL, 1);

  Mcp2515_ReadRegister(CANSTAT,1);

  /* Set Baudrate 500000 */
  Mcp2515Baudrate = KBPS_500;

  Mcp2515_ReadRegister(CNF1, 1);
  Mcp2515_ReadRegister(CNF2, 1);
  Mcp2515_ReadRegister(CNF3, 1);

  Mcp2515_WriteRegister(CNF3, 0x82);
  Mcp2515_WriteRegister(CNF2, 0x90);
  Mcp2515_WriteRegister(CNF1, 0x00);

  Mcp2515_ReadRegister(CNF1, 1);
  Mcp2515_ReadRegister(CNF2, 1);
  Mcp2515_ReadRegister(CNF3, 1);

  Mcp2515_BitModifyReg(CANCTRL, 0xE0, 0x00);
  Mcp2515_ReadRegister(CANCTRL, 1);

  uint8 TXBnCTRL = TXB1CTRL + regNum*0x10;
  uint8 TxBnCTRLcmd = 0x3;
  Mcp2515_BitModifyReg(TXBnCTRL, 0xB ,TxBnCTRLcmd);
  //setPinOutputTrue(&MODULE_P11,2);
}


void Mcp2515_SetBaudrate(MCP2515_BAUDRATE Baudrate)
{
  Mcp2515Baudrate = Baudrate;
  uint8 CNF1cmd = 0;
  uint8 CNF2cmd = 0;
  uint8 CNF3cmd = 0;

  switch (Mcp2515Baudrate)
  {
    case KBPS_125:
      CNF1cmd = 0x07;
      CNF2cmd = 0xB1;
      CNF3cmd = 0x85;
      break;

    case KBPS_250:
      CNF1cmd = 0x00;
      CNF2cmd = 0xB1;
      CNF3cmd = 0x85;
      break;

    case KBPS_500:
      CNF1cmd = 0x00;
      CNF2cmd = 0x90;
      CNF3cmd = 0x82;
      break;

    case KBPS_1000:
      CNF1cmd = 0x00;
      CNF2cmd = 0x80;
      CNF3cmd = 0x80;
      break;

    default:
      CNF1cmd = 0x00;
      CNF2cmd = 0x90;
      CNF3cmd = 0x82;
      break;
  }

  /* Set New Baudrate */
  Mcp2515_WriteRegister(CNF3, CNF3cmd);
  Mcp2515_WriteRegister(CNF2, CNF2cmd);
  Mcp2515_WriteRegister(CNF1, CNF1cmd);
}


boolean Mcp2515_SetRegNum(uint8 Num)
{
  if(Num < 3)
  {
    regNum = Num;
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


static inline void Mcp2515_WriteRegister(uint8 Register, uint8 Command)
{
  uint8 SpiBuffer[3] = {WRITE_COMMAND, Register, Command};
  Spi_WriteBytes(&Mcp2515Channel, SpiBuffer, 3);
}

static inline void Mcp2515_WriteRegisters(uint8 StartReg, uint8* Command, uint8 CommandSize)
{
  uint8 SpiBuffer[CommandSize+2];
  SpiBuffer[0] = WRITE_COMMAND;
  SpiBuffer[1] = StartReg;
  for(uint8 i = 0; i < CommandSize; i++)
  {
    SpiBuffer[i+2] = Command[i];
  }
  Spi_WriteBytes(&Mcp2515Channel, SpiBuffer, CommandSize+2);
}

static inline void Mcp2515_BitModifyReg(uint8 Register, uint8 Mask, uint8 Value)
{
  uint8 SpiBuffer[4] = {BIT_MODIFY, Register, Mask , Value};
  Spi_WriteBytes(&Mcp2515Channel, SpiBuffer, 4);
}


static inline void Mcp2515_ReadRegister(uint8 Register, uint8 RegsNum)
{
  uint8 SpiBuffer[2] = {READ_COMMAND, Register};
  Spi_ReadBytes(&Mcp2515Channel, SpiBuffer, 2, Mcp2515ReadBuffer, RegsNum);
}


void Mcp2515_SetTransmitMsgId(uint16 CanId, boolean extIdentifier, uint32 Identifier)
{
  //setPinOutputFalse(&MODULE_P11,2);
  uint8 TxBnSIDH = TXB1SIDH + regNum*0x10;
  uint8 TXBnSIDL = TXB1SIDL + regNum*0x10;

  uint8 TxBnSIDHcmd = 0;
  uint8 TxBnSIDLcmd = 0;


  if (extIdentifier == FALSE)
  {
    TxBnSIDHcmd = TxBnSIDHcmd|(uint8)(CanId>>(11-8));
    TxBnSIDLcmd = TxBnSIDLcmd|(uint8)(CanId<<5);

    Mcp2515_WriteRegister(TxBnSIDH, TxBnSIDHcmd);
    Mcp2515_WriteRegister(TXBnSIDL, TxBnSIDLcmd);
  }
  else
  {
    uint8 TxBnEID8 = TXB1EID8 + regNum*0x10;
    uint8 TxBnEID0 = TXB1EID0 + regNum*0x10;

    uint8 TxBnEID8cmd = (uint8)(Identifier >> 8);
    uint8 TxBnEID0cmd = (uint8)(Identifier);

    TxBnSIDHcmd = TxBnSIDHcmd|(uint8)(CanId>>(11-8));
    TxBnSIDLcmd = TxBnSIDLcmd|(uint8)(CanId<<5);
    TxBnSIDLcmd = TxBnSIDLcmd|8;
    TxBnSIDLcmd = TxBnSIDLcmd | (uint8)(Identifier >> 16);

    Mcp2515_WriteRegister(TxBnSIDH, TxBnSIDHcmd);
    Mcp2515_WriteRegister(TXBnSIDL, TxBnSIDLcmd);
    Mcp2515_WriteRegister(TxBnEID8, TxBnEID8cmd);
    Mcp2515_WriteRegister(TxBnEID0, TxBnEID0cmd);
  }
  //setPinOutputTrue(&MODULE_P11,2);
}


boolean Mcp2515_Transmit(uint8* CanMsg, uint8 CanMsgSize)
{
  //setPinOutputFalse(&MODULE_P11,2);
  if(!Mcp2515_checkMsgSent())
  {
    return FALSE;
  }
  else
  {
    /* Just continue */
  }

  /* Write the number of bytes that need to send */
  uint8 TxBnDLC = TXB1DLC + regNum*0x10;
  uint8 TxBnDLCcmd = CanMsgSize;
  Mcp2515_WriteRegister(TxBnDLC, TxBnDLCcmd);
  Mcp2515_ReadRegister(TxBnDLC, 1);

  /* Write in the Tx Buffers the message */
  uint8 TxBnDm = TXB1D0 + regNum*0x10;
  Mcp2515_WriteRegisters(TxBnDm, CanMsg, CanMsgSize);

  /* Transmit the message */
  uint8 TXBnCTRL = TXB1CTRL + regNum*0x10;
  uint8 TxBnCTRLcmd = 0x8;
  Mcp2515_BitModifyReg(TXBnCTRL, 0x8 ,TxBnCTRLcmd);
  Mcp2515_ReadRegister(TXBnCTRL, 1);

  //setPinOutputTrue(&MODULE_P11,2);
  return TRUE;
}


static inline boolean Mcp2515_checkMsgSent(void)
{
  uint8 Result = 0;
  uint8 TXBnCTRL = TXB1CTRL + regNum*0x10;

  Mcp2515_ReadRegister(TXBnCTRL, 1);
  Result = Mcp2515ReadBuffer[0];

  if((Result & 0x8) == 0x0)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

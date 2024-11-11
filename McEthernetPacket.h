/**********************************************************************************************************************
 * \file MicrochipEthernet.h
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

#ifndef INFINEONARDUINOLIKE_MCETHERNETPACKET_H_
#define INFINEONARDUINOLIKE_MCETHERNETPACKET_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Ifx_Types.h"
#include "Dma.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MC_ETHERNET_MAXFRAME     1500
#define MC_ETHERNET_MACSIZE      6
#define MC_ETHERNET_DATASIZE     256

typedef enum
{
  TRANSMIT_MESSAGE,
  RECEIVE_MESSAGE
}MCETH_TYPE;

typedef struct
{
    uint8*          SrcMacAddress;
    uint8*          DstMacAddress;
    uint8*          TypeLength;
    uint8           MacAddressSize;
    uint8           TypeLengthSize;
    MCETH_TYPE      Type;
}MCETH_PACKET;


/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
extern uint16 McEth_WriteAddress;
extern uint8 McEth_Buf[2048];
extern uint8 McEth_ReceiveBytesNum;
extern uint16 McEth_WriteIndex;
extern uint16 McEth_ReadIndex;
/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void McEth_SetSrcMacAddress(uint8* MacAddress);
void McEth_GetSrcMacAddress(uint8* ptr);
void McEth_SetDstMacAddress(uint8* MacAddress);
void McEth_GetDstMacAddress(uint8* ptr);
void McEth_Init(uint8* MacAddress);
void McEth_CreateTransmitPacket(uint8* Type);
boolean McEth_PushProtocolIndex(uint8* Message, uint16 MessageSize);
boolean McEth_PushTransmitPayload(uint8* Message, uint16 MessageSize);
boolean McEth_TransmitMessage(void);
boolean McEth_ReceiveMsg(float32 time);
boolean McEth_ReceiveMsgInf(void);
void McEth_ReadEthernetProperties(void);
uint16 McEth_ReturnReceiveType(void);
void McEth_ReadReceivedBytes(uint8* ReceivePtr, uint16 ReceivePtrSize);
void McEth_ReadReceivedPayload(uint8* ReceivePtr, uint16 ReceivePtrSize);
void McEth_ResetReceiveBuffer(void);
boolean McEth_WriteTransmitMemory(uint16 startMemory, uint8* newMessage, uint16 size);
uint16 McEth_ReadWritePointer(void);

#endif /* INFINEONARDUINOLIKE_McEthPACKET_H_ */

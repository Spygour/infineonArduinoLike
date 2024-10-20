/**********************************************************************************************************************
 * \file Serialinit.h
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

#ifndef SERIALINIT_H_
#define SERIALINIT_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "IfxAsclin_ASC.h"
#include "Ifx_Types.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/
 
/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void SerialReadnl(Ifx_SizeT length);
void SerialReadwithChar(Ifx_SizeT length);
void SerialRead(Ifx_SizeT length);
void SerialReadChar(void);
void SerialWritenl(uint8 *message,Ifx_SizeT length);
void SerialWriteWithChar(uint8 *message,Ifx_SizeT length,uint8 special_char);
void SerialWrite(uint8 *message,Ifx_SizeT length);
void UartInit(IfxAsclin_Rx_In* RX_PIN,IfxAsclin_Tx_Out* TX_PIN,float32 baudrate);
void SerialWriteChar(uint8 write_char);
void SerialWriteString(uint8* write_string,Ifx_SizeT length);
void SerialReadString(Ifx_SizeT length);

/*Read messages compare functions*/
boolean charCmp(uint8 chr);
boolean strcmp(uint8* message,uint8* read_string,Ifx_SizeT message_len);
boolean SerialCmpCmd(uint8* message,Ifx_SizeT message_len);
boolean SerialCmpchar(uint8* message,Ifx_SizeT message_len,uint8 compareChar);
boolean SerialCmpNl(uint8* message,Ifx_SizeT message_len);
boolean SerialCmpString(uint8* message,Ifx_SizeT message_len);

/*Read-Write buffer return function */
uint8 Get_readBuf(Ifx_SizeT index);
void SerialremoveReadFifo(void);
void RstReadBuf(void);
void RstWriteBuf(void);
#endif /* SERIALINIT_H_ */

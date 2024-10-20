/**********************************************************************************************************************
 * \file Spi.h
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

#ifndef INFINEONARDUINOLIKE_SPI_H_
#define INFINEONARDUINOLIKE_SPI_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "IfxQspi_SpiMaster.h"
#include "IfxQspi_SpiSlave.h"
#include "Ifx_Types.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
typedef struct
{
    const IfxQspi_Slso_Out* ChannelOutput;
    float32                Baudrate;
    uint32                 ClockPolarity;
    SpiIf_ShiftClock       ShiftClock;
    SpiIf_DataHeading      DataHeading;
}SpiChannelConfig;

typedef IfxQspi_SpiMaster SpiMaster_t;
typedef IfxQspi_SpiMaster_Channel SpiChannel_t;
typedef struct
{
    IfxQspi_Sclk_Out* SpiClk;
    IfxQspi_Mtsr_Out* SpiMosi;
    IfxQspi_Mrst_In*  SpiMiso;
}SpiMasterPins_t;

typedef struct
{
    SpiMaster_t*          SpiMasterPtr;
    uint8                 TxIsr;
    uint8                 RxIsr;
    uint8                 ErIsr;
    boolean               IsActive;
}SpiMasterCfg_t;

typedef struct
{
    IfxQspi_Sclk_In*  SpiClkIn;
    IfxQspi_Mtsr_In*  SpiMosi;
    IfxQspi_Mrst_Out* SpiMiso;
    IfxQspi_Slsi_In*  SpiChipSelect;
}SpiSlavePins_t;
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
void Spi_Init(SpiMasterPins_t* SpiMasterPins, SpiMasterCfg_t* MasterCfg);
void Spi_ChannelInit(SpiMasterCfg_t* SpiMasterCfg, SpiChannel_t* SpiChannel, SpiChannelConfig* ChannelConfig);
void Spi_WriteRegister(SpiChannel_t* SpiChannel, uint8 Reg);
void Spi_WriteRegisterVal(SpiChannel_t* SpiChannel, uint8 Reg, uint8 Val);
void Spi_ReadRegister(SpiChannel_t* SpiChannel, uint8 Reg, uint8* regVal, uint16 size);
void Spi_WriteBytes(SpiChannel_t* SpiChannel, uint8* Src, uint16 size);
void Spi_ReadBytes(SpiChannel_t* SpiChannel,uint8* Src, uint16 SrcSize, uint8* Dest, uint16 DestSize);

void Spi_SlaveInit(IfxQspi_SpiSlave* SpiSlave,SpiSlavePins_t* SpiSlavePins, SpiChannelConfig* ChannelConfig);
void Spi_SlaveExchange(IfxQspi_SpiSlave* SpiSlave, uint8* SpiSlaveTx, uint8* SpiSlaveRx, uint16 size);

#endif /* INFINEONARDUINOLIKE_SPI_H_ */

/**********************************************************************************************************************
 * \file ResistorCalculate.c
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
#include "adc_uart.h"

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define TX_LENGTH 6

#define VADC_GROUP IfxVadc_GroupId_4   /*Single channel set*/
#define CHANNEL_ID  4                  /*Channel ID for single vadc*/
#define CHANNEL_RESULT_REGISTER     5 /*register where you will save the result for a single channel(this is not needed */
#define N_CHANNELS 8                  /*Number of channels for the group vadc initialize*/

#define ISR_PRIORITY_ASCLIN_TX 10
#define ISR_PRIORITY_ASCLIN_RX 11
#define ISR_PRIORITY_ASCLIN_ER 12
#define RX_PIN &IfxAsclin0_RXA_P14_1_IN
#define TX_PIN &IfxAsclin0_TX_P14_0_OUT
#define ASC_TX_BUFFER_SIZE 64
#define ASC_RX_BUFFER_SIZE 64
#define ASC_PRESCALER 1
#define ASC_BAUDRATE 115200
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxAsclin_Asc g_asc;                            /* Handle for the ASC communication module registers                */
IfxVadc_Adc g_vadc;                             /* Handle for VADC registers instance                               */
IfxVadc_Adc_Group g_adcGroup;                   /* Handle for the VADC group registers                              */
IfxVadc_Adc_Channel g_adcChannel[N_CHANNELS];

uint8 SerialPrintBuffer[128] = {''};

uint8 g_uartTxBuffer[ASC_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];
uint8 g_uartRxBuffer[ASC_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8];


uint8 UsbRxBuffer[128] = {''};
ApplicationVadcBackgroundScan g_vadcBackgroundScan;

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void InitSerialInterface(void);
void UartWrite(uint8 *message,Ifx_SizeT length);
void send_vadc_single(uint32 adcVal);
void send_vadc_group(uint32 chnIx, uint32 adcVal);
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/*vadc for many channels initialize and set part */
void Adc_InitGroup(IfxVadc_ChannelId * g_vadcChannelIDs, IfxVadc_GroupId adcGroup,uint8 channels_size)
{
    /* Create and initialize the module configuration */
    IfxVadc_Adc_Config adcConf;                             /* Define a configuration structure for the VADC module */
    IfxVadc_Adc_initModuleConfig(&adcConf, &MODULE_VADC);   /* Fill it with default values                          */

    IfxVadc_Adc_initModule(&g_vadc, &adcConf);              /* Apply the configuration to the module                */

    /* Create and initialize the group configuration */
    IfxVadc_Adc_GroupConfig adcGroupConf;                   /* Define a configuration structure for the VADC group  */
    IfxVadc_Adc_initGroupConfig(&adcGroupConf, &g_vadc);    /* Fill it with default values                          */

    /* Configuration of the group */

    adcGroupConf.groupId = adcGroup;
              /* Select the group                                     */
    adcGroupConf.master = adcGroupConf.groupId;             /* Select the master group                              */

    adcGroupConf.arbiter.requestSlotScanEnabled = TRUE;     /* Enable scan source                                   */
    adcGroupConf.scanRequest.autoscanEnabled = TRUE;        /* Enable auto scan mode                                */

    /* Enable all gates in "always" mode (no edge detection) */
    adcGroupConf.scanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

    IfxVadc_Adc_initGroup(&g_adcGroup, &adcGroupConf);      /* Apply the configuration to the group                 */

    /* Create and initialize the channels configuration */
    uint32 chnIx;

    /* Create channel configuration */
    IfxVadc_Adc_ChannelConfig adcChannelConf[channels_size]; /* Define a configuration structure for the VADC channels */

    for(chnIx = 0; chnIx < channels_size; ++chnIx)
    {
        IfxVadc_Adc_initChannelConfig(&adcChannelConf[chnIx], &g_adcGroup);     /* Fill it with default values      */

        adcChannelConf[chnIx].channelId = g_vadcChannelIDs[chnIx];              /* Select the channel ID            */
        adcChannelConf[chnIx].resultRegister = (IfxVadc_ChannelResult)(chnIx);  /* Use dedicated result register    */

        /* Initialize the channel */
        IfxVadc_Adc_initChannel(&g_adcChannel[chnIx], &adcChannelConf[chnIx]);

        /* Add the channel to the scan sequence */
        uint32 enableChnBit = (1 << adcChannelConf[chnIx].channelId);   /* Set the the corresponding input channel  */
        uint32 mask = enableChnBit;                                     /* of the respective group to take part in  */
        IfxVadc_Adc_setScan(&g_adcGroup, enableChnBit, mask);           /* the background scan sequence.            */
    }

    /* Start the scan */
    IfxVadc_Adc_startScan(&g_adcGroup);
}
void run_vadc_group(uint32 channels)
{
    uint32 chnIx;

    /* Get the VADC conversions */
    for(chnIx = 0; chnIx < channels; ++chnIx)
    {
        Ifx_VADC_RES conversionResult;

        /* Wait for a new valid result */
        do
        {
            conversionResult = IfxVadc_Adc_getResult(&g_adcChannel[chnIx]);
        } while(!conversionResult.B.VF); /* B for Bitfield access, VF for Valid Flag */

        /* Print the conversion to the UART */
        send_vadc_group(chnIx, conversionResult.B.RESULT);
    }
}

void send_vadc_group(uint32 chnIx, uint32 adcVal)
{
    uint8 str[12] = {'C','h','.','X',':',' ','X','X','X','X','\n','\r'};  /* X to be replaced by correct values*/

    str[3] = (uint8) chnIx + 48;                                        /* Channel index                    */

    /* Turns the digital converted value into its ASCII characters, e.g. 1054 -> '1','0','5','4' */
    /* 12-bits range value: 0-4095 */
    str[6] = (uint8)(adcVal / 1000) + 48;                                     /* Thousands                        */
    str[7] = (uint8)((adcVal % 1000) / 100) + 48;                             /* Hundreds                         */
    str[8] = (uint8)((adcVal % 100) / 10) + 48;                               /* Tens                             */
    str[9] = (uint8)(adcVal % 10) + 48;                                       /* Units                            */

    /* Print via UART */
    UartWrite(str, 12);
}


void Adc_ReadGroup(uint32* ChannelsRes ,uint8 channels)
{
    uint32 chnIx;

    /* Get the VADC conversions */
    for(chnIx = 0; chnIx < channels; ++chnIx)
    {
        Ifx_VADC_RES conversionResult;

        /* Wait for a new valid result */
        do
        {
            conversionResult = IfxVadc_Adc_getResult(&g_adcChannel[chnIx]);
        } while(!conversionResult.B.VF); /* B for Bitfield access, VF for Valid Flag */
        ChannelsRes[chnIx] = conversionResult.B.RESULT;
    }
}


uint32 Adc_ReturnChannelVal(uint8 channel)
{
  Ifx_VADC_RES conversionResult;
  do
  {
      conversionResult = IfxVadc_Adc_getResult(&g_adcChannel[channel]);
  } while(!conversionResult.B.VF); /* B for Bitfield access, VF for Valid Flag */
  return conversionResult.B.RESULT;
}



/*vadc single channel initialize and set part */
void init_vadc_background(IfxVadc_GroupId vadc_group,IfxVadc_ChannelId channel_id,IfxVadc_ChannelResult result_register)
{
  IfxVadc_Adc_Config adcConf;
  IfxVadc_Adc_initModuleConfig(&adcConf,&MODULE_VADC);

  IfxVadc_Adc_initModule(&g_vadcBackgroundScan.vadc,&adcConf);
  IfxVadc_Adc_GroupConfig adcGroupConfig;

  IfxVadc_Adc_initGroupConfig(&adcGroupConfig,&g_vadcBackgroundScan.vadc);
  adcGroupConfig.groupId = vadc_group;
  adcGroupConfig.master = vadc_group;

  adcGroupConfig.arbiter.requestSlotBackgroundScanEnabled = TRUE;
  adcGroupConfig.backgroundScanRequest.autoBackgroundScanEnabled = TRUE;
  adcGroupConfig.backgroundScanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

  IfxVadc_Adc_initGroup(&g_vadcBackgroundScan.adcGroup,&adcGroupConfig);


  IfxVadc_Adc_initChannelConfig(&g_vadcBackgroundScan.adcChannelConfig,&g_vadcBackgroundScan.adcGroup);
  g_vadcBackgroundScan.adcChannelConfig.channelId = (IfxVadc_ChannelId)channel_id;
  g_vadcBackgroundScan.adcChannelConfig.resultRegister = (IfxVadc_ChannelResult)result_register;
  g_vadcBackgroundScan.adcChannelConfig.backgroundChannel = TRUE;
  IfxVadc_Adc_initChannel(&g_vadcBackgroundScan.adcChannel,&g_vadcBackgroundScan.adcChannelConfig);

  IfxVadc_Adc_setBackgroundScan(&g_vadcBackgroundScan.vadc,
                                &g_vadcBackgroundScan.adcGroup,
                                (1 << (IfxVadc_ChannelId)channel_id),
                                (1 << (IfxVadc_ChannelId)channel_id));
  IfxVadc_Adc_startBackgroundScan(&g_vadcBackgroundScan.vadc);
}



void run_vadc_background(void)
{
  Ifx_VADC_RES conversionResult;
  do
  {
      conversionResult = IfxVadc_Adc_getResult(&g_vadcBackgroundScan.adcChannel);
  }while(!conversionResult.B.VF);

  send_vadc_single(conversionResult.B.RESULT);
}


void send_vadc_single(uint32 adcVal)
{
  uint8 str[TX_LENGTH] = {'X','X','X','X','\n','\r'};  /* X to be replaced by correct values*/
                                  /* Channel index                    */

  /* Turns the digital converted value into its ASCII characters, e.g. 1054 -> '1','0','5','4' */
  /* 12-bits range value: 0-4095 */
  str[0] = (uint8)(adcVal / 1000) + 48;                                     /* Thousands                        */
  str[1] = (uint8)((adcVal % 1000) / 100) + 48;                             /* Hundreds                         */
  str[2] = (uint8)((adcVal % 100) / 10) + 48;                               /* Tens                             */
  str[3] = (uint8)(adcVal % 10) + 48;                                       /* Units                            */

  /* Print via UART */
  UartWrite(str, TX_LENGTH);
}
/*uart ISR and read/write part */

void UartWriteWord(uint8 *message,Ifx_SizeT length)
{
    IfxAsclin_Asc_write(&g_asc,message,&length,TIME_INFINITE);
}

void UartWriteln(uint8 *message,Ifx_SizeT length)
{
  for (Ifx_SizeT i=0;i<length;i++)
  {
    SerialPrintBuffer[i] = message[i];
  }
  SerialPrintBuffer[length-1] = '\r';
  SerialPrintBuffer[length] = '\n';
  SerialPrintBuffer[length+1] = '\0';
  Ifx_SizeT real_size = length+2;
  IfxAsclin_Asc_write(&g_asc,&SerialPrintBuffer[0],&real_size,TIME_INFINITE);
}

void UartWrite(uint8 *message,Ifx_SizeT length)
{
  for (Ifx_SizeT i=0;i<length;i++)
  {
    SerialPrintBuffer[i] = message[i];
  }
  SerialPrintBuffer[length-1] = '\r';
  SerialPrintBuffer[length] = '\0';
  Ifx_SizeT real_size = length+1;
  IfxAsclin_Asc_write(&g_asc,&SerialPrintBuffer[0],&real_size,TIME_INFINITE);
}

void UartWriteWithChar(uint8 *message,Ifx_SizeT length,char special_char)
{
  for (Ifx_SizeT i=0;i<length;i++)
  {
    SerialPrintBuffer[i] = message[i];
  }
  SerialPrintBuffer[length-1] = '\r';
  SerialPrintBuffer[length] = special_char;
  SerialPrintBuffer[length+1] = '\0';
  Ifx_SizeT real_size = length+2;
  IfxAsclin_Asc_write(&g_asc,&SerialPrintBuffer[0],&real_size,TIME_INFINITE);
}


IFX_INTERRUPT(asc0TxISR, 0 , ISR_PRIORITY_ASCLIN_TX);

void asc0TxISR(void){
    IfxAsclin_Asc_isrTransmit(&g_asc);
}

IFX_INTERRUPT(asc0RxISR, 0 , ISR_PRIORITY_ASCLIN_RX);

void asc0RxISR(void){
    IfxAsclin_Asc_isrReceive(&g_asc);
}

IFX_INTERRUPT(asc0ErISR, 0 , ISR_PRIORITY_ASCLIN_ER);

void asc0ErISR(void){
    IfxAsclin_Asc_isrError(&g_asc);
}


void receive_data(Ifx_SizeT length)
{
    /* Receive data */
    IfxAsclin_Asc_read(&g_asc, &UsbRxBuffer[0], &length, TIME_INFINITE);
}

void UartRstRxBuffer(void)
{
  for (Ifx_SizeT i = 0; i < sizeof(UsbRxBuffer); i++)
  {
    UsbRxBuffer[i] = 0;
  }
}

uint8 Get_UsbRxBufferIndex(Ifx_SizeT index)
{
    return UsbRxBuffer[index];
}

/*Uart initialize with USB*/
void InitSerialInterface(void){
    IfxAsclin_Asc_Config ascConf;
    IfxAsclin_Asc_initModuleConfig(&ascConf, IfxAsclin0_TX_P14_0_OUT.module);

    ascConf.baudrate.prescaler = ASC_PRESCALER;
    ascConf.baudrate.baudrate = ASC_BAUDRATE;                                   /* Set the baud rate in bit/s       */
    ascConf.baudrate.oversampling = IfxAsclin_OversamplingFactor_16;
    ascConf.bitTiming.medianFilter = IfxAsclin_SamplesPerBit_three;             /* Set the number of samples per bit*/
    ascConf.bitTiming.samplePointPosition = IfxAsclin_SamplePointPosition_8;    /* Set the first sample position    */

    /* ISR priorities and interrupt target */
    ascConf.interrupt.txPriority = ISR_PRIORITY_ASCLIN_TX;/* Set the interrupt priority for TX events   */
    ascConf.interrupt.rxPriority = ISR_PRIORITY_ASCLIN_RX;
    ascConf.interrupt.erPriority = ISR_PRIORITY_ASCLIN_ER;
    ascConf.interrupt.typeOfService = IfxSrc_Tos_cpu0;
    /* Pin configuration */
    const IfxAsclin_Asc_Pins pins = {
            .cts        = NULL_PTR,                         /* CTS pin not used                                     */
            .ctsMode    = IfxPort_InputMode_pullUp,
            .rx         = RX_PIN,         /* Select the pin for RX connected to the USB port      */
            .rxMode     = IfxPort_InputMode_pullUp,         /* RX pin                                               */
            .rts        = NULL_PTR,                         /* RTS pin not used                                     */
            .rtsMode    = IfxPort_OutputMode_pushPull,
            .tx         = TX_PIN,         /* Select the pin for TX connected to the USB port      */
            .txMode     = IfxPort_OutputMode_pushPull,      /* TX pin                                               */
            .pinDriver  = IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    ascConf.pins = &pins;

    /* FIFO buffers configuration */
    ascConf.txBuffer = g_uartTxBuffer;                      /* Set the transmission buffer                          */
    ascConf.txBufferSize = ASC_TX_BUFFER_SIZE;
    ascConf.rxBuffer = g_uartRxBuffer;
    ascConf.rxBufferSize = ASC_RX_BUFFER_SIZE;/* Set the transmission buffer size                     */


    /* Init ASCLIN module */
    IfxAsclin_Asc_initModule(&g_asc, &ascConf);
}


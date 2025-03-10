/**********************************************************************************************************************
 * \file McEthernetIp.c
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
#include "McEthernetIp.h"
#include "McEthernetPacket.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MCETHIP_HEADER_LEN   20
#define MCETHIP_ARP_PACKETLEN 28
#define MCETHIP_ADDRESS_SIZE 4
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
uint16 McEthIp_TransmitTcpChecksum;
uint8 McEthIp_TransmitBuffer[MC_ETHERNET_DATASIZE];

static uint8 McEthIp_TxSrcIpAddress[MCETHIP_ADDRESS_SIZE] = {192, 168, 2, 4};
static uint8 McEthIp_TxDstIpAddress[MCETHIP_ADDRESS_SIZE] = {0, 0, 0, 0};

static MCETHIP_IP_ADDRESS McEthIp_IpAddress = {
    .IpSrcAddress = McEthIp_TxSrcIpAddress,
    .IpDstAddress = McEthIp_TxDstIpAddress
};

static MCETHIP_TCP_ADDRESS McEthIp_TcpAddress =
{
    .TcpSrc = 0x8795,
    .TcpDst = 0x01BB
};

static MCETHIP_IPHEADER McEthIp_IpHeader = {
    .SrcDstIpAddress = &McEthIp_IpAddress,
    .Version_HeaderLength = 0x45,
    .DifferecialServices = 0x00,
    .TotalLength = 0,
    .Identification = 0,
    .FragmentOffset = 0x4000,
    .TimeToLive = 0x40,
    .Protocol = 0x06,
    .CheckSum = 0
};

static MCETHIP_TCPHEADER McEthIp_TcpHeader = {
    .TcpSrcDst = &McEthIp_TcpAddress,
    .SeqNumber = 0x00000000,
    .Aknowledgment = 0x00000000,
    .DataOffsetAndFlags = 0x58,
    .WindowSize = 0xFFFF,
    .CheckSum = 0x0,
    .UrgentPointer = 0x0
};

MCETH_IP_TRANSMIT_STATE McEthIp_TransmitState = ETH_TRANSMIT_IDLE;
MCETH_IP_RECEIVE_STATE McEthIp_ReceiveState = ETH_RECEIVE_IDLE;

static uint16 McEthIp_PayloadRemain;

MCETHIP_ARP_TRANSMIT_STATE McEthIp_SendARPState = ETH_CREATE_ARP;
MCETHIP_ARP_RECEIVE_STATE McEthIp_GetARPState = ETH_GET_ARP;
/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
static void McEthIpUpdateIpCheckSum(uint8* IpHeader);
static void McEthIpUpdateTcpCheckSum(uint8* BytesArray, uint16 size);
static void McEthIpUpdateTcpCheckSumPayload(uint16 size);
static uint16 McEthIpCheckSumCalc(uint8* BytesArray, uint16 size);
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/** \brief Sets the Source IP Address
 * \param McEth_SetSrcIpAddress
 * \return void
 */
void McEthIp_SetSrcIpAddress(uint8* IpAddress)
{
  for (uint8 i = 0; i < 4; i++)
  {
    McEthIp_TxSrcIpAddress[i] = IpAddress[i];
  }
}



/** \brief Sets the Destination IP Address
 * \param McEth_SetDstIpAddress
 * \return void
 */
void McEthIp_SetDstIpAddress(uint8* IpAddress)
{
  for (uint8 i = 0; i < 4; i++)
  {
    McEthIp_TxDstIpAddress[i] = IpAddress[i];
  }
}



/** \brief Gets the Source IP Address
 * \param McEth_GetSrcIpAddress
 * \return void
 */
void McEthIp_GetSrcIpAddress(uint8* ptr)
{
  for (uint8 i = 0; i < 4; i++)
  {
    ptr[i] = McEthIp_TxSrcIpAddress[i];
  }
}



/** \brief Gets the Destination IP Address
 * \param McEth_GetDstIpAddress
 * \return void
 */
void McEthIp_GetDstIpAddress(uint8* ptr)
{
  for (uint8 i = 0; i < 4; i++)
  {
    ptr[i] = McEthIp_TxDstIpAddress[i];
  }
}



/** \brief Creates the Ip Header that needs to be send to the buffer
 * \param McEth_CreateIpHeader
 * \return void
 */
static void McEthIp_CreateIpHeader(uint16 PayloadSize)
{
  static uint8 McEthIp_IpHeaderTemp[20];
  uint16 MessageLength = 14 + MCETHIP_HEADER_LEN + MCETHIP_HEADER_LEN + PayloadSize;
  /* Type is Ipv4 for now */
  McEthIp_IpHeaderTemp[0] = McEthIp_IpHeader.Version_HeaderLength;
  /* Type of service is 0 */
  McEthIp_IpHeaderTemp[1] = McEthIp_IpHeader.DifferecialServices;
  /* Total Length */
  McEthIp_IpHeaderTemp[2] = (uint8)(MessageLength >> 8);
  McEthIp_IpHeaderTemp[3] = (uint8)(MessageLength & 0x00FF);
  McEthIp_IpHeader.TotalLength = MessageLength;
  /* Identification */
  McEthIp_IpHeaderTemp[4] = (uint8)(McEthIp_IpHeader.Identification >> 8);
  McEthIp_IpHeaderTemp[5] = (uint8)(McEthIp_IpHeader.Identification & 0x00FF);
  /* Fragment Deactivated */
  McEthIp_IpHeaderTemp[6] = (uint8)(McEthIp_IpHeader.FragmentOffset >> 8);
  McEthIp_IpHeaderTemp[7] = (uint8)(McEthIp_IpHeader.FragmentOffset & 0x00FF);
  /* TTL in case we speak with router */
  McEthIp_IpHeaderTemp[8] = McEthIp_IpHeader.TimeToLive;
  /* TCP protocol for now */
  McEthIp_IpHeaderTemp[9] = McEthIp_IpHeader.Protocol;
  /* Header Checksum 0 after the initialization will be updated */
  McEthIp_IpHeaderTemp[10] = 0x00;
  McEthIp_IpHeaderTemp[11] = 0x00;
  /* Store the SrcAddress */
  McEthIp_GetSrcIpAddress(&McEthIp_IpHeaderTemp[12]);
  /* Store the DstAddress */
  McEthIp_GetDstIpAddress(&McEthIp_IpHeaderTemp[16]);

  McEthIpUpdateIpCheckSum(McEthIp_IpHeaderTemp);
  McEthIp_IpHeader.CheckSum = (uint16)((McEthIp_IpHeaderTemp[10] << 8) | (McEthIp_IpHeaderTemp[11]));
  McEthIp_IpHeader.Identification = (McEthIp_IpHeader.Identification == 0xFFFF) ? 1 : McEthIp_IpHeader.Identification+1;
  if (McEth_PushProtocolIndex(McEthIp_IpHeaderTemp, MCETHIP_HEADER_LEN))
  {
    /* Ok */
  }
  else
  {
    /* Buffer is full */
  }
}



/** \brief Initializes the TCP header for transmit the TCP packet
 * \param McEth_CreateTcpHeader
 * \return void
 */
static void McEthIp_CreateTcpHeader(uint16 PayloadSize)
{
  /* This variable needs to be stored in order to update the checksum correctly */
  McEthIp_TransmitTcpChecksum = McEth_WriteAddress + 17;
  static uint8 McEthIp_TcpHeaderTemp[20];
  /* Source Port 2 bytes */
  McEthIp_TcpHeaderTemp[0] = (uint8)(McEthIp_TcpHeader.TcpSrcDst->TcpSrc >> 8);
  McEthIp_TcpHeaderTemp[1] = (uint8)(McEthIp_TcpHeader.TcpSrcDst->TcpSrc & 0x00FF);
  /* Destination Port 2 bytes */
  McEthIp_TcpHeaderTemp[2] = (uint8)(McEthIp_TcpHeader.TcpSrcDst->TcpDst >> 8);
  McEthIp_TcpHeaderTemp[3] = (uint8)(McEthIp_TcpHeader.TcpSrcDst->TcpDst & 0x00FF);
  /* Sequence Number  4 Byte */
  McEthIp_TcpHeaderTemp[4] = (uint8)(McEthIp_TcpHeader.SeqNumber >> 24);
  McEthIp_TcpHeaderTemp[5] = (uint8)(McEthIp_TcpHeader.SeqNumber >> 16);
  McEthIp_TcpHeaderTemp[6] = (uint8)(McEthIp_TcpHeader.SeqNumber >> 8);
  McEthIp_TcpHeaderTemp[7] = (uint8)(McEthIp_TcpHeader.SeqNumber & 0x00FF);
  /* Aknowledgment Number  4 Bytes */
  McEthIp_TcpHeaderTemp[8] = (uint8)(McEthIp_TcpHeader.Aknowledgment >> 24);
  McEthIp_TcpHeaderTemp[9] = (uint8)(McEthIp_TcpHeader.Aknowledgment >> 16);
  McEthIp_TcpHeaderTemp[10] = (uint8)(McEthIp_TcpHeader.Aknowledgment >> 8);
  McEthIp_TcpHeaderTemp[11] = (uint8)(McEthIp_TcpHeader.Aknowledgment & 0x00FF);
  /* Data Offset 4 Bits and flags by 4 bits*/
  McEthIp_TcpHeaderTemp[12] = (uint8)(McEthIp_TcpHeader.DataOffsetAndFlags >> 8);
  McEthIp_TcpHeaderTemp[13] = (uint8)(McEthIp_TcpHeader.DataOffsetAndFlags & 0x00FF);
  /* Window Size 2 Bytes */
  McEthIp_TcpHeaderTemp[14] = (uint8)(McEthIp_TcpHeader.WindowSize >> 8);
  McEthIp_TcpHeaderTemp[15] = (uint8)(McEthIp_TcpHeader.WindowSize & 0x00FF);
  /* CheckSum Is 0 for now 2 Bytes */
  McEthIp_TcpHeaderTemp[16] = 0;
  McEthIp_TcpHeaderTemp[17] = 0;
  /* UrgentPointer 2 Bytes */
  McEthIp_TcpHeaderTemp[18] = (uint8)(McEthIp_TcpHeader.UrgentPointer >> 8);
  McEthIp_TcpHeaderTemp[19] = (uint8)(McEthIp_TcpHeader.UrgentPointer & 0x00FF);

  McEthIpUpdateTcpCheckSum(McEthIp_TcpHeaderTemp, MCETHIP_HEADER_LEN);

  /* CheckSum 2 Bytes */
  McEthIp_TcpHeaderTemp[15] = (uint8)(McEthIp_TcpHeader.CheckSum >> 8);
  McEthIp_TcpHeaderTemp[16] = (uint8)(McEthIp_TcpHeader.CheckSum & 0x00FF);

  /* Push Tcp Header to buffer */
  if (McEth_PushProtocolIndex(McEthIp_TcpHeaderTemp, MCETHIP_HEADER_LEN))
  {
    /* Ok */
  }
  else
  {
    /* Buffer is full */
  }
  /* Prepare Aknowledgment for the receive Packet */
  McEthIp_TcpHeader.Aknowledgment = ((0xFFFFFFFF - McEthIp_TcpHeader.SeqNumber) > PayloadSize) ? (0xFFFFFFFF- McEthIp_TcpHeader.SeqNumber + PayloadSize) : (McEthIp_TcpHeader.SeqNumber + PayloadSize);
  /* Increase SeqNumber for the next packet */
  McEthIp_TcpHeader.SeqNumber = ((0xFFFFFFFF - McEthIp_TcpHeader.SeqNumber) > PayloadSize) ? (0xFFFFFFFF- McEthIp_TcpHeader.SeqNumber + PayloadSize) : (McEthIp_TcpHeader.SeqNumber + PayloadSize);
  /* Message will be pushed with checksum of tcp for now Write value of CheckSum will be stored for future use */
}



/** \brief Calculates the Payload's checksum and adds the payload to transmit buffer
 * \param McEth_SendPayload
 * \return void
 */
static void McEthIp_CreatePayload(void)
{
  McEthIp_PayloadRemain = McEth_Payload.WriteBufferSize;
  uint8 NewCheckSum[2];

  while (McEthIp_PayloadRemain >= MC_ETHERNET_DATASIZE)
  {
    McEthIpUpdateTcpCheckSumPayload(MC_ETHERNET_DATASIZE);
    McEth_PushTransmitPayload(&McEth_Payload.Buffer[McEth_Payload.WriteIndex], MC_ETHERNET_DATASIZE);
    McEthIp_PayloadRemain -= MC_ETHERNET_DATASIZE;
  }

  if (McEthIp_PayloadRemain > 0)
  {
    McEthIpUpdateTcpCheckSumPayload(McEthIp_PayloadRemain);
    McEth_PushTransmitPayload(&McEth_Payload.Buffer[McEth_Payload.WriteIndex], McEthIp_PayloadRemain);
  }
  else
  {
    /* Do nothing after this */
  }
  /* Update the checksum in the checksum area of the transmit memory */
  /* CheckSum 2 Bytes */
  McEthIp_TcpHeader.CheckSum = ~McEthIp_TcpHeader.CheckSum;
  NewCheckSum[0] = (uint8)(McEthIp_TcpHeader.CheckSum >> 8);
  NewCheckSum[1] = (uint8)(McEthIp_TcpHeader.CheckSum & 0x00FF);
  McEth_WriteTransmitMemory(McEthIp_TransmitTcpChecksum, NewCheckSum, sizeof(NewCheckSum));
}



/** \brief Creates a whole TCP packet along with ethernet, ip and tcp header
 * \param McEthIp_InitTcpTransmitPacket
 * \return void
 */
void McEthIp_TcpTransmitPacket(void)
{
  uint8 Ipv4Type[2] = {0x08, 0x00};
  switch (McEthIp_TransmitState)
  {
    case ETH_TRANSMIT_IDLE:
      McEthIp_TransmitState = SEND_ETHERNET_HEADER;
      break;

    case SEND_ETHERNET_HEADER:
      McEth_CreateTransmitPacket(Ipv4Type);
      McEthIp_TransmitState = SEND_IP_HEADER;
      break;

    case SEND_IP_HEADER:
      McEthIp_CreateIpHeader(McEth_Payload.WriteBufferSize);
      McEthIp_TransmitState = SEND_TCP_HEADER;
      break;
    case SEND_TCP_HEADER:
      McEthIp_CreateTcpHeader(McEth_Payload.WriteBufferSize);
      McEthIp_TransmitState = SEND_PAYLOAD;
      break;

    case SEND_PAYLOAD:
      McEthIp_CreatePayload();
      McEthIp_TransmitState = SEND_PACKET;
      break;

    case SEND_PACKET:
      /* Send the message */
      if (McEth_TransmitMessage())
      {
        McEth_Payload.ReadIndex = McEth_Payload.WriteIndex;
        McEthIp_TransmitState = ETH_SEND_SUCCESS;
      }
      else
      {
        McEthIp_TransmitState = ETH_SEND_FAIL;
        /* Wait till the message is sent */
      }
      break;

    case ETH_SEND_SUCCESS:
      break;

    case ETH_SEND_FAIL:
      break;

    default:
      break;
  }
}



/** \brief Evaluates the IP Header
 * \param McEthEvaluateIpHeader
 * \return boolean
 */
static boolean McEthIpEvaluateIpHeader(void)
{
  boolean success = TRUE;

  uint8 IpHeader_tmp[MCETHIP_HEADER_LEN];

  McEth_ReadReceivedBytes(IpHeader_tmp, MCETHIP_HEADER_LEN);

  /* Header length and type */
  if (((IpHeader_tmp[0] >> 4) != 0x4) || ((IpHeader_tmp[0] & 0x0F) < 0x05))
  {
    return FALSE;
  }

  /* Header time to live greater than 0 */
  if (IpHeader_tmp[8] == 0)
  {
    return FALSE;
  }

  /* Header protocol is TCP */
  if (IpHeader_tmp[9] != 0x06)
  {
    return FALSE;
  }

  /* Destination Address should be equal with the Module's IP address */
  for (uint8 i = 0; i < 4; i++)
  {
    if (IpHeader_tmp[16 + i] != McEthIp_IpHeader.SrcDstIpAddress->IpSrcAddress[i])
    {
      success = FALSE;
      break;
    }
  }

  if (success == FALSE)
  {
    return FALSE;
  }

  /* Store the CheckSum to evaluation */
  uint16 CheckSum = (uint16)((IpHeader_tmp[10] << 8) | (IpHeader_tmp[11] & 0x00FF));
  /* Set CheckSum to 0 in the array in order to recalculate it */
  IpHeader_tmp[10] = 0;
  IpHeader_tmp[11] = 0;

  /* Check the checksum */
  if (McEthIpCheckSumCalc(IpHeader_tmp, 20) != CheckSum)
  {
    return FALSE;
  }

  /* Check The remaining Length */
  uint16 message_Size = (uint16)((IpHeader_tmp[2] << 8) | (IpHeader_tmp[3] & 0x00FF));
  message_Size -= 20;
  if (McEth_ReceiveBytesNum != message_Size)
  {
    return FALSE;
  }

  /* Update the destination address for the next transmit message */
  for (uint8 i = 0; i< 4; i++)
  {
    McEthIp_IpHeader.SrcDstIpAddress->IpDstAddress[i] = IpHeader_tmp[12 + i];
  }

  uint8 RemainIpHeaderLen  = (IpHeader_tmp[0] & 0x0F) * 4 - MCETHIP_HEADER_LEN;

  if (RemainIpHeaderLen > 0)
  {
    /* Remove the remaining ip header bytes to move to the next step */
    McEth_ReadReceivedBytes(IpHeader_tmp, RemainIpHeaderLen);
  }

  return success;
}



/** \brief Evaluates the TCP Header
 * \param McEthEvaluateTcpHeader
 * \return boolean
 */
static boolean McEthIpEvaluateTcpHeader(void)
{
  boolean success = TRUE;

  uint8 TcpHeader_tmp[MCETHIP_HEADER_LEN];

  /* We suppose that options field does not exist */
  McEth_ReadReceivedBytes(TcpHeader_tmp, MCETHIP_HEADER_LEN);

  uint16 DstAddr = (uint16)((TcpHeader_tmp[2] << 8) | (TcpHeader_tmp[3] & 0x00FF));

  /* Check Dst Addr if its the correct one */
  if (DstAddr != McEthIp_TcpHeader.TcpSrcDst->TcpSrc)
  {
    return FALSE;
  }

  /* Check the aknowledgment */
  uint32 Aknowledgment = (uint32) ((TcpHeader_tmp[8] << 24) | (TcpHeader_tmp[9] << 16) | (TcpHeader_tmp[10] << 8) | (TcpHeader_tmp[11]));

  if (Aknowledgment != McEthIp_TcpHeader.Aknowledgment)
  {
    return FALSE;
  }

  /* Update the Dst Address */
  McEthIp_TcpHeader.TcpSrcDst->TcpDst = (uint16)((TcpHeader_tmp[0] << 8) | (TcpHeader_tmp[1] & 0x00FF));

  /* Get Sequence number */
  uint32 SeqNum = (uint32) ((TcpHeader_tmp[4] << 24) | (TcpHeader_tmp[5] << 16) | (TcpHeader_tmp[6] << 8) | (TcpHeader_tmp[7]));
  /* McEth_ReceiveBytesNum are updated in lower layer */
  McEthIp_TcpHeader.Aknowledgment = McEth_ReceiveBytesNum + SeqNum;


  /* Get the length of TcpHeader */
  uint8 TcpHeadLen = (McEthIp_TcpHeader.DataOffsetAndFlags >> 4) * 4;
  if (TcpHeadLen > MCETHIP_HEADER_LEN)
  {
    /* Get the remaining header bytes tbd include the options inside */
    McEth_ReadReceivedBytes(TcpHeader_tmp, TcpHeadLen - MCETHIP_HEADER_LEN);
  }
  else if (TcpHeadLen < 20)
  {
    /* Invalid number of bytes in tcp something is not correct here */
    return FALSE;
  }

  return success;
}



/** \brief Reads the Payload of the TCP packet
 * \param McEthReadPayload
 * \return boolean
 */
void McEthIpReadPayload(void)
{
  McEth_Payload.ReadBufferSize = (McEth_ReceiveBytesNum - 4);
  while(McEth_ReceiveBytesNum > MC_ETHERNET_DATASIZE)
  {
    McEth_ReadReceivedPayload(&McEth_Payload.Buffer[McEth_Payload.ReadIndex], MC_ETHERNET_DATASIZE);
  }

  if (McEth_ReceiveBytesNum > 0)
  {
    McEth_ReadReceivedPayload(&McEth_Payload.Buffer[McEth_Payload.ReadIndex], McEth_ReceiveBytesNum);
  }
}



/** \brief Receives and evaluates a TCP packet
 * \param McEthIp_InitTcpReceivePacket
 * \return void
 */
void McEthIp_TcpReceivePacket(void)
{
  switch (McEthIp_ReceiveState)
  {
    case ETH_RECEIVE_IDLE:
      if (McEth_ReceiveMsg(100))
      {
        McEthIp_ReceiveState = READ_ETHERNET_HEADER;
      }
      else
      {
        McEthIp_ReceiveState = ETH_RECEIVE_IDLE;
      }
      break;

    case READ_ETHERNET_HEADER:
      McEth_ReadEthernetProperties();
      /* Check that type length is IPV4 */
      if (McEth_ReturnReceiveType()==0x800)
      {
        McEthIp_ReceiveState = READ_IP_HEADER;
      }
      else
      {
        McEth_ResetReceiveBuffer();
        McEthIp_ReceiveState = ETH_READ_FAIL;
      }
      break;

    case READ_IP_HEADER:
      if (McEthIpEvaluateIpHeader())
      {
        McEthIp_ReceiveState = READ_TCP_HEADER;
      }
      else
      {
        McEth_ResetReceiveBuffer();
        McEthIp_ReceiveState = ETH_READ_FAIL;
      }
      break;

    case READ_TCP_HEADER:
      if (McEthIpEvaluateTcpHeader())
      {
        McEthIpReadPayload();
        McEth_Payload.WriteIndex = McEth_Payload.ReadIndex;
        McEthIp_ReceiveState = ETH_READ_SUCCESS;
      }
      else
      {
        McEth_ResetReceiveBuffer();
        McEthIp_ReceiveState = ETH_READ_FAIL;
      }
      break;

    case ETH_READ_SUCCESS:
      /* Success now read the Payload by higher layer */
      break;

    case ETH_READ_FAIL:
      McEthIp_ReceiveState = ETH_RECEIVE_IDLE;
      break;

    default:
      break;

  }

}



/** \brief Updates the CheckSum of the Ip Header
 * \param McEthUpdateCheckSum
 * \return void
 */
static void McEthIpUpdateIpCheckSum(uint8* McEthIp_IpHeader)
{
  uint32 CheckSum = 0;
  uint16 tmp;

  for (uint8 i = 0; i < 20; i = i+2)
  {
    tmp = (uint16)((McEthIp_IpHeader[i] << 8) | McEthIp_IpHeader[i+1]);
    CheckSum += tmp;

    if (CheckSum > 0xFFFF)
    {
      CheckSum = (CheckSum & 0xFFFF) + 1;
    }
  }
  McEthIp_IpHeader[10] = (uint8)(~CheckSum >> 8);
  McEthIp_IpHeader[11] = (uint8)(~CheckSum & 0x00FF);
}



/** \brief Calculate CheckSum of the Tcp Header
 * \param McEthUpdateTcpCheckSum
 * \return void
 */
static void McEthIpUpdateTcpCheckSum(uint8* BytesArray, uint16 size)
{
  uint32 CheckSum = (uint32)McEthIp_TcpHeader.CheckSum;
  for  (uint16 i = 0; i < size; i++)
  {
    CheckSum += BytesArray[i];

    if (CheckSum > 0xFFFF)
    {
      CheckSum = (CheckSum & 0xFFFF) + 1;
    }
  }
  McEthIp_TcpHeader.CheckSum = (uint16)CheckSum;
}


/** \brief Calculate CheckSum of the Tcp Header in the PayLoad
 * \param McEthUpdateTcpCheckSumPayload
 * \return void
 */
static void McEthIpUpdateTcpCheckSumPayload(uint16 size)
{
  uint32 CheckSum = (uint32)McEthIp_TcpHeader.CheckSum;

  if ((MC_ETHERNET_DMASIZE - McEth_Payload.WriteIndex) >= size)
  {
    /* Normal Case, size does not overflow */
    for  (uint16 i = 0; i < size; i++)
    {
      CheckSum += McEth_Payload.Buffer[McEth_Payload.WriteIndex + i];

      if (CheckSum > 0xFFFF)
      {
        CheckSum = (CheckSum & 0xFFFF) + 1;
      }
    }
  }
  else
  {
    /* In this case we know that the Write Bytes have been moved to
     * index zero of the cyclic buffer. In this case we need to
     * run the calculation till the max size and then till the rest of the bytes
     * Keep in mind that the cyclic buffer values for the transmit of packets
     * are stored in higher layer using again DMA, DMA stores bytes
     * like the PayLoad buffer is a cyclic buffer
     */
    for (uint16 i = McEth_Payload.WriteIndex; i < MC_ETHERNET_DMASIZE; i++)
    {
      CheckSum += McEth_Payload.Buffer[i];

      if (CheckSum > 0xFFFF)
      {
        CheckSum = (CheckSum & 0xFFFF) + 1;
      }
    }
    size = size - (MC_ETHERNET_DMASIZE - McEth_Payload.WriteIndex);

    for (uint16 i = 0; size; i++)
    {
      CheckSum += McEth_Payload.Buffer[i];

      if (CheckSum > 0xFFFF)
      {
        CheckSum = (CheckSum & 0xFFFF) + 1;
      }
    }
  }
}


/** \brief Calculate CheckSum of a message
 * \param McEthCheckSumCalc
 * \return void
 */
static uint16 McEthIpCheckSumCalc(uint8* BytesArray, uint16 size)
{
  uint16 CheckSum = 0;
  for  (uint16 i = 0; i < size; i++)
  {
    CheckSum += BytesArray[i];

    if (CheckSum > 0xFFFF)
    {
      CheckSum = (CheckSum & 0xFFFF) + 1;
    }
  }
  return CheckSum;
}



/** \brief Creates an ARP packet
 * \param McEthIp_CreateARP function
 * \return boolean
 */
static boolean McEthIp_CreateARP(uint8* DstIpAddress)
{
  uint8 ArpType[2] = {0x08, 0x06};
  uint8 ArpPacket[MCETHIP_ARP_PACKETLEN];
  uint8 MacAddrs[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  /* Prepare the Dst Ip Address */
  McEthIp_SetDstIpAddress(DstIpAddress);

  /* Create an Ethernet Header Packet */
  McEth_CreateTransmitPacket(ArpType);

  /* Hardware Type */
  ArpPacket[0] = 0x00;
  ArpPacket[1] = 0x01;

  /* Protocol Type IpV4 */
  ArpPacket[2] = 0x08;
  ArpPacket[3] = 0x00;

  /* Header length 6 bytes */
  ArpPacket[4] = 0x06;

  /* Protocol Address bytes 4 */
  ArpPacket[5] = 0x04;

  /* Operation */
  ArpPacket[6] = 0x00;
  ArpPacket[7] = 0x01;

  /* Src Mac Address */
  McEth_GetSrcMacAddress(&ArpPacket[8]);

  /* Src Ip Address */
  McEthIp_GetSrcIpAddress(&ArpPacket[14]);

  /* Dst Mac Address */
  McEth_SetDstMacAddress(MacAddrs);
  McEth_GetDstMacAddress(&ArpPacket[18]);

  /* Dst Ip Address */
  McEthIp_GetDstIpAddress(&ArpPacket[24]);

  return McEth_PushProtocolIndex(ArpPacket, sizeof(ArpPacket));
}



/** \brief Sends a ARP packet
 * \param McEthIp_SendARP function
 * \return boolean
 */
void McEthIp_SendARP(uint8* DstIpAddress)
{
  switch (McEthIp_SendARPState)
  {
    case ETH_CREATE_ARP:
      if (McEthIp_CreateARP(DstIpAddress))
      {
        McEthIp_SendARPState = ETH_SEND_ARP;
      }
      else
      {
        McEthIp_SendARPState = ETH_CREATE_ARP;
      }
      break;

    case ETH_SEND_ARP:
      if (McEth_TransmitMessage())
      {
        McEthIp_SendARPState = ETH_SUCCESS_SEND_ARP;
      }
      else
      {
        McEthIp_SendARPState = ETH_FAIL_SEND_ARP;
      }
      break;

    case ETH_FAIL_SEND_ARP:
      break;

    case ETH_SUCCESS_SEND_ARP:
      break;

    default:
      break;
  }
}



/** \brief Evaluates an ARP packet
 * \param McEthIp_EvalARP  function
 * \return boolean
 */
static boolean McEthIp_EvalARP(void)
{
  uint8 ArpPacket[MCETHIP_ARP_PACKETLEN];
  uint8 MacAddr[MC_ETHERNET_MACSIZE];
  boolean success = FALSE;

  McEth_GetSrcMacAddress(MacAddr);

  McEth_ReadReceivedBytes(ArpPacket, MCETHIP_ARP_PACKETLEN);
  if (McEth_ReceiveBytesNum < 28)
  {
    success = FALSE;
  }

  if (McEth_ReturnReceiveType()!=0x800)
  {
    success = FALSE;
  }

  if ((ArpPacket[0] != 0x00) || (ArpPacket[1] != 0x01))
  {
    success = FALSE;
  }

  if ((ArpPacket[6] != 0x00) || (ArpPacket[7] != 0x02))
  {
    success = FALSE;
  }

  for (uint8 i = 9; i< MCETHIP_HEADER_LEN; i++)
  {
    if (McEthIp_TxSrcIpAddress[i] != ArpPacket[24 + i])
    {
      success = FALSE;
      break;
    }
  }

  if (success == TRUE)
  {
    McEth_SetDstMacAddress(&ArpPacket[8]);
    return success;
  }
  else
  {
    return success;
  }
}



/** \brief Gets back a ARP packet
 * \param McEthIp_GetARP function
 * \return void
 */
void McEthIp_GetARP(void)
{
  switch(McEthIp_GetARPState)
  {
    case ETH_GET_ARP:
      if (McEth_ReceiveMsg(100))
      {
        McEth_ReadEthernetProperties();
        McEthIp_GetARPState = ETH_EVAL_ARP;
      }
      else
      {
        McEthIp_GetARPState = ETH_GET_ARP;
      }
      break;

    case ETH_EVAL_ARP:
      if (McEthIp_EvalARP())
      {
        McEth_ResetReceiveBuffer();
        McEthIp_GetARPState = ETH_SUCCESS_RECEIVE_ARP;
      }
      else
      {
        McEthIp_GetARPState = ETH_FAIL_RECEIVE_ARP;
      }
      break;

    case  ETH_SUCCESS_RECEIVE_ARP:
      break;

    case  ETH_FAIL_RECEIVE_ARP:
      break;

    default:
      break;
  }
}



/** \brief Initializes the Tcp header for a Http/Https Request
 * \param McEthIp_PrepareRequest function
 * \return void
 */
void McEthIp_PrepareRequest(MCETHIP_REQUEST RequestType)
{
  /* First set the Destination Mac Address to Broadcast */
  uint8 DstMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  McEth_SetDstMacAddress(DstMac);
  /* This value is mostly used by all the Ethernet Firmware */
  McEthIp_TcpHeader.WindowSize = 8192;
  /* As an initialize TCP the Flags need to be 2 */
  McEthIp_TcpHeader.DataOffsetAndFlags = (5 << 12) | 0x12;
  switch (RequestType)
  {
    case HTTP:
      /* Http is 80 */
      McEthIp_TcpHeader.TcpSrcDst->TcpDst = 0x50;
      break;

    case HTTPS:
      /* Https is 443 */
      McEthIp_TcpHeader.TcpSrcDst->TcpDst = 0x1BB;
      break;

    default:
      McEthIp_TcpHeader.TcpSrcDst->TcpDst = 0x50;
      break;
  }
}



/** \brief Updates Data Offset and flags after the first message
 * \param McEthIp_UpdateFlags function
 * \return void
 */
void McEthIp_UpdateFlags(void)
{
  /* Change only to ACK now */
  McEthIp_TcpHeader.DataOffsetAndFlags = (5 << 12) | 0x10;
}



/** \brief Updates the flags in case we want to terminate the connection
 * \param McEthIp_TerminateFlags function
 * \return void
 */
void McEthIp_TerminateFlags(void)
{
  /* Change only to ACK now */
  McEthIp_TcpHeader.DataOffsetAndFlags = (5 << 12) | 0x110;
}

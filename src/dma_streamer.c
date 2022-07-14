/*
 * dma_streamer.c
 *
 *  Created on: Sep 15, 2021
 *      Author: janoko
 */

#include "include/dma_streamer.h"
#include "include/dma_streamer/conf.h"


static uint16_t readBuffer(STRM_handlerTypeDef*, uint8_t *rBuf, uint16_t size, uint8_t readtype);


HAL_StatusTypeDef STRM_Init(STRM_handlerTypeDef *hdmas,
                            uint8_t *txBuffer, uint16_t txBufferSize,
                            uint8_t *rxBuffer, uint16_t rxBufferSize)
{
  hdmas->txBuffer = txBuffer;
  hdmas->txBufferSize = txBufferSize;
  hdmas->rxBuffer = rxBuffer;
  hdmas->rxBufferSize = rxBufferSize;
  hdmas->status = 0;
  hdmas->timeout = 2000;
  hdmas->pos = 0;

  if (hdmas->huart == NULL)
    return HAL_ERROR;

  return HAL_UART_Receive_DMA(hdmas->huart, hdmas->rxBuffer, hdmas->rxBufferSize);
}


void STRM_CheckOverlap(STRM_handlerTypeDef *hdmas)
{
  uint16_t dmaPos = hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx);

  if(hdmas->pos < dmaPos && (dmaPos - hdmas->pos) < (hdmas->rxBufferSize/2)){
    hdmas->pos = dmaPos;
    if(hdmas->pos == hdmas->rxBufferSize) hdmas->pos = 0;
  }
}


HAL_StatusTypeDef STRM_Write(STRM_handlerTypeDef *hdmas, uint8_t *rBuf, uint16_t length, uint8_t breakType)
{
  uint16_t i;
  STRM_Status status = STRM_TIMEOUT;
  uint32_t tickstart = STRM_GetTick();

  if (length > hdmas->txBufferSize) {
    status = STRM_ERROR;
    return status;
  }
  
  while (hdmas->huart->gState != HAL_UART_STATE_READY) {
    if((STRM_GetTick() - tickstart) >= hdmas->timeout) return status;
  }

  // write on buffer
  for (i = 0; i < length; i++)
  {
    hdmas->txBuffer[i] = *rBuf;
    rBuf++;
  }

  if (breakType == STRM_BREAK_CR || breakType == STRM_BREAK_CRLF) {
    hdmas->txBuffer[i] = '\r';
    length++;
    rBuf++;
    i++;
  }
  if (breakType == STRM_BREAK_LF || breakType == STRM_BREAK_CRLF) {
    hdmas->txBuffer[i] = '\n';
    length++;
    rBuf++;
    i++;
  }

  if (hdmas->config.txMode == STRM_CONF_TXMODE_DMA)
    return HAL_UART_Transmit_DMA(hdmas->huart, hdmas->txBuffer, length);
  else
    return HAL_UART_Transmit(hdmas->huart, hdmas->txBuffer, length, 1000);
}


uint8_t STRM_IsReadable(STRM_handlerTypeDef *hdmas)
{
  return (hdmas->pos != (hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx)));
}


uint16_t STRM_Read(STRM_handlerTypeDef *hdmas, uint8_t *rBuf, uint16_t size, uint32_t timeout)
{
  uint16_t len = 0;
  uint32_t tickstart = STRM_GetTick();

  if (timeout == 0) timeout = hdmas->timeout;

  while (len < size) {
    if((STRM_GetTick() - tickstart) >= timeout) break;
    len += readBuffer(hdmas, &(rBuf[len]), size-len, STRM_READALL);
    if(len == 0) STRM_Delay(1);
  }
  return len;
}


void STRM_Unread(STRM_handlerTypeDef *hdmas, uint16_t length)
{
  if (length > hdmas->pos)  {
    length -= hdmas->pos;
    hdmas->pos = hdmas->rxBufferSize-length;
  } else {
    hdmas->pos -= length;
  }
}


uint16_t STRM_Readline(STRM_handlerTypeDef *hdmas, uint8_t *rBuf, uint16_t size, uint32_t timeout)
{
  uint16_t len = 0;
  uint32_t tickstart = STRM_GetTick();
  uint8_t isSuccess = 0;

  if (timeout == 0) timeout = hdmas->timeout;
  if (hdmas->config.breakLine == 0) hdmas->config.breakLine = STRM_BREAK_LF;

  while (len < size) {
    if ((STRM_GetTick() - tickstart) >= timeout) break;
    len += readBuffer(hdmas, &(rBuf[len]), size-len, hdmas->config.breakLine);

    if((  hdmas->config.breakLine == STRM_BREAK_CRLF
          && len > 1 && rBuf[len-2] == '\r' && rBuf[len-1] == '\n')
      || (hdmas->config.breakLine == STRM_BREAK_CR
          && rBuf[len-1] == '\r')
      || (hdmas->config.breakLine == STRM_BREAK_LF
          && rBuf[len-1] == '\n'))
    {
      isSuccess = 1;
      break;
    }

    if(len == 0) STRM_Delay(1);
  }

  if (!isSuccess) {
    len = 0;
    STRM_Unread(hdmas, len);
  }
  return len;
}


HAL_StatusTypeDef STRM_ReadToBuffer(STRM_handlerTypeDef *hdmas, STRM_Buffer_t *rBuffer, uint16_t length, uint32_t timeout)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint16_t pos;
  uint16_t posLimited;
  uint32_t tickstart = STRM_GetTick();
  uint16_t remainingLength = length;
  uint16_t willReadLength;

  if(timeout == 0) timeout = hdmas->timeout;

  while(remainingLength){
    if ((STRM_GetTick() - tickstart) >= timeout) return HAL_TIMEOUT;

    // read dma available data
    pos = hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx);
    if (pos == hdmas->pos) {
      STRM_Delay(1);
      continue;
    }

    // copy dma buffer to rBuffer
    while (remainingLength && pos != hdmas->pos){
      if (pos > hdmas->pos) {
        posLimited = pos;
      } else {
        posLimited = hdmas->rxBufferSize;
      }

      willReadLength = (posLimited - hdmas->pos);
      if (willReadLength > remainingLength) {
        willReadLength = remainingLength;
      }
      STRM_Buffer_Write(rBuffer, &(hdmas->rxBuffer[hdmas->pos]), willReadLength);
      
      remainingLength -= willReadLength;
      hdmas->pos += willReadLength;
      if(hdmas->pos == hdmas->rxBufferSize) hdmas->pos = 0;
    }
  }

  return status;
}


void STRM_Reset(STRM_handlerTypeDef *hdmas)
{
  hdmas->pos = hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx);
}


static uint16_t readBuffer(STRM_handlerTypeDef *hdmas, uint8_t *rBuf, uint16_t size, uint8_t readtype)
{
  uint8_t prevByte, curByte;
  uint16_t len = 0;
  uint16_t pos = hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx);

  // read buffer until find "\r\n"
  while(pos != hdmas->pos && len < size){
    curByte = hdmas->rxBuffer[hdmas->pos];

    rBuf[len] = curByte;
    len++;

    // pos add as circular
    hdmas->pos++;
    if(hdmas->pos == hdmas->rxBufferSize) hdmas->pos = 0;

    if(readtype != STRM_READALL){
      if((  readtype == STRM_BREAK_CRLF
            && len > 1 && prevByte == '\r' && curByte == '\n')
        || (readtype == STRM_BREAK_CR
            && curByte == '\r')
        || (readtype == STRM_BREAK_LF
            && curByte == '\n'))
      {
        break;
      }
      // save current byte in prevByte
      prevByte = curByte;
    }
  }
  return len;
}

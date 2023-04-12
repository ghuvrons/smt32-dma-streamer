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

  return HAL_OK;
}


HAL_StatusTypeDef STRM_Start(STRM_handlerTypeDef *hdmas)
{
  if (hdmas->getTick == 0 || hdmas->delay == 0) return HAL_ERROR;
  return HAL_UART_Receive_DMA(hdmas->huart, hdmas->rxBuffer, hdmas->rxBufferSize);
}


void STRM_CheckOverlap(STRM_handlerTypeDef *hdmas)
{
  // TODO: need check

  uint16_t dmaPos = hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx);

  if (hdmas->pos < dmaPos && (dmaPos - hdmas->pos) < (hdmas->rxBufferSize/2)){
    hdmas->pos = dmaPos;
    if (hdmas->pos >= hdmas->rxBufferSize) hdmas->pos = 0;
  }
}


uint16_t STRM_Write(STRM_handlerTypeDef *hdmas, const uint8_t *wBuf, uint16_t length, uint8_t breakType)
{
  uint16_t i;
  STRM_Status status = STRM_TIMEOUT;
  uint32_t tickstart = hdmas->getTick();
  
  // wait ready
  while (hdmas->huart->gState != HAL_UART_STATE_READY) {
    if ((hdmas->getTick() - tickstart) >= hdmas->timeout) return status;
    hdmas->delay(1);
  }

  // write on buffer
  if (hdmas->config.txMode == STRM_CONF_TXMODE_DMA)
    status = HAL_UART_Transmit_DMA(hdmas->huart, wBuf, length);
  else
    status = HAL_UART_Transmit(hdmas->huart, wBuf, length, 1000);
  if (status != HAL_OK) return 0;

  // send break line
  i = 0;
  if ((breakType & STRM_BREAK_CR) != 0) {
    *(hdmas->txBuffer+i) = '\r';
    i++;
  }
  if ((breakType & STRM_BREAK_LF) != 0) {
    *(hdmas->txBuffer+i) = '\n';
    i++;
  }

  if (i > 0) {
    if (hdmas->config.txMode == STRM_CONF_TXMODE_DMA)
      status = HAL_UART_Transmit_DMA(hdmas->huart, hdmas->txBuffer, i);
    else
      status = HAL_UART_Transmit(hdmas->huart, hdmas->txBuffer, i, 1000);
    if (status != HAL_OK) return 0;
  }
  return length+i;
}


uint8_t STRM_IsReadable(STRM_handlerTypeDef *hdmas)
{
  return (hdmas->pos != (hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx)));
}


uint16_t STRM_Read(STRM_handlerTypeDef *hdmas, uint8_t *dst, uint16_t size, uint32_t timeout)
{
  uint16_t readlen = 0;
  uint16_t tmpReadLen = 0;
  uint32_t tickstart = hdmas->getTick();

  if (dst == NULL) return 0;
  if (timeout == 0) timeout = hdmas->timeout;

  while (size) {
    if ((hdmas->getTick() - tickstart) >= timeout) break;
    tmpReadLen = readBuffer(hdmas, dst, size, STRM_READALL);
    if (tmpReadLen == 0) hdmas->delay(1);
    size -= tmpReadLen;
    dst += tmpReadLen;
    readlen += tmpReadLen;
  }
  return readlen;
}


void STRM_Unread(STRM_handlerTypeDef *hdmas, uint16_t length)
{
  while (length > hdmas->rxBufferSize) {
    length -= hdmas->rxBufferSize;
  }
  if (length > hdmas->pos)  {
    length -= hdmas->pos;
    hdmas->pos = hdmas->rxBufferSize-length;
  } else {
    hdmas->pos -= length;
  }
}


uint16_t STRM_Readline(STRM_handlerTypeDef *hdmas, uint8_t *rBuf, uint16_t size, uint32_t timeout)
{
  uint16_t readlen = 0;
  uint16_t tmpReadLen = 0;
  uint32_t tickstart = hdmas->getTick();
  uint8_t isSuccess = 0;

  if (rBuf == NULL) return 0;
  if (timeout == 0) timeout = hdmas->timeout;
  if (hdmas->config.breakLine == 0) hdmas->config.breakLine = STRM_BREAK_LF;

  while (size) {
    if ((hdmas->getTick() - tickstart) >= timeout) break;
    if (hdmas->config.breakLine == STRM_BREAK_CRLF
        && readlen > 0
        && *(rBuf+readlen-1) == '\r')
    {
      tmpReadLen = readBuffer(hdmas, rBuf+readlen, 1, STRM_BREAK_NONE);
    } else {
      tmpReadLen = readBuffer(hdmas, rBuf+readlen, size, hdmas->config.breakLine);
    }

    readlen += tmpReadLen;
    size -= tmpReadLen;

    if (readlen > 0
        && ((hdmas->config.breakLine == STRM_BREAK_CRLF && readlen > 1
                                                              && *(rBuf+readlen-2) == '\r'
                                                              && *(rBuf+readlen-1) == '\n')
            || (hdmas->config.breakLine == STRM_BREAK_CR && *(rBuf+readlen-1) == '\r')
            || (hdmas->config.breakLine == STRM_BREAK_LF && *(rBuf+readlen-1) == '\n')))
    {
      isSuccess = 1;
      break;
    }

    if(tmpReadLen == 0) hdmas->delay(1);
  }

  if (!isSuccess) {
    readlen = 0;
    STRM_Unread(hdmas, readlen);
  }
  return readlen;
}


uint16_t STRM_ReadIntoBuffer(STRM_handlerTypeDef *hdmas, Buffer_t *rBuffer, uint16_t length, uint32_t timeout)
{
  uint16_t len = 0;
  uint16_t pos;
  uint16_t posLimited;
  uint32_t tickstart = hdmas->getTick();
  uint16_t remainingLength = length;
  uint16_t willReadLength;
  int32_t tmpWriteLen;

  if (timeout == 0) timeout = hdmas->timeout;

  while (remainingLength) {
    if ((hdmas->getTick() - tickstart) >= timeout) break;

    // read dma available data
    pos = hdmas->rxBufferSize - __HAL_DMA_GET_COUNTER(hdmas->huart->hdmarx);
    if (pos == hdmas->pos) {
      hdmas->delay(1);
      continue;
    }

    // copy dma buffer to rBuffer
    while (remainingLength && pos != hdmas->pos) {
      if (pos > hdmas->pos) {
        posLimited = pos;
      } else {
        posLimited = hdmas->rxBufferSize;
      }

      willReadLength = (posLimited - hdmas->pos);
      if (willReadLength > remainingLength) {
        willReadLength = remainingLength;
      }
      tmpWriteLen = Buffer_Write(rBuffer, hdmas->rxBuffer+hdmas->pos, willReadLength);

      if (tmpWriteLen > 0) {
        len += tmpWriteLen;
        remainingLength -= tmpWriteLen;
        hdmas->pos += tmpWriteLen;
        if (hdmas->pos >= hdmas->rxBufferSize) hdmas->pos = 0;
      } else if (tmpWriteLen < 0) break;
    }
  }

  return len;
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
  while (pos != hdmas->pos) {
    if (len >= size) break;
    curByte = *(hdmas->rxBuffer+hdmas->pos);

    *rBuf = curByte;
    rBuf++;
    len++;

    // pos add as circular
    hdmas->pos++;
    if (hdmas->pos >= hdmas->rxBufferSize) hdmas->pos = 0;

    if (readtype != STRM_READALL) {
      if ((readtype == STRM_BREAK_CRLF && len > 1 && prevByte == '\r' && curByte == '\n')
         || (readtype == STRM_BREAK_CR && curByte == '\r')
         || (readtype == STRM_BREAK_LF && curByte == '\n'))
      {
        break;
      }
      // save current byte in prevByte
      prevByte = curByte;
    }
  }
  return len;
}

/*
 * dma.h
 *
 *  Created on: Sep 15, 2021
 *      Author: janoko
 */

#ifndef DMA_STREAMER_INC_DMA_STREAMER_H_
#define DMA_STREAMER_INC_DMA_STREAMER_H_

#include <stm32f4xx_hal.h>
#include "dma_streamer/buffer.h"

#define STRM_BUF_IS_AVAILABLE 0x01
#define STRM_IS_READING       0x02

// readtypes
#define STRM_READALL       0x00
#define STRM_BREAK_CR      0x01
#define STRM_BREAK_LF      0x02
#define STRM_BREAK_CRLF    0x03
#define STRM_BREAK_NONE    0x00

// status
#define STRM_TIMEOUT    0x00
#define STRM_SUCCESS    0x01
#define STRM_ERROR      0x02

// config
#define STRM_CONF_TXMODE_STANDART   0
#define STRM_CONF_TXMODE_INTERRUPT  1
#define STRM_CONF_TXMODE_DMA        2


typedef uint8_t STRM_Status;

typedef struct {
  UART_HandleTypeDef  *huart;
  uint8_t             *txBuffer;
  uint16_t            txBufferSize;
  uint8_t             *rxBuffer;
  uint16_t            rxBufferSize;

  struct {
    uint8_t txMode;
    uint8_t breakLine;
  } config;
  /*
   * bit  0. is buffer available
   *      1. is readable is read
   */
  uint8_t status;
  uint16_t pos;
  uint16_t timeout;

} STRM_handlerTypeDef;

HAL_StatusTypeDef STRM_Init(STRM_handlerTypeDef*,
                            uint8_t *txBuffer, uint16_t txBufferSize,
                            uint8_t *rxBuffer, uint16_t rxBufferSize);
void STRM_Start(STRM_handlerTypeDef*);
void STRM_CheckOverlap(STRM_handlerTypeDef*);
HAL_StatusTypeDef STRM_Write(STRM_handlerTypeDef*, uint8_t *rBuf, uint16_t size, uint8_t breakType);
uint8_t STRM_IsReadable(STRM_handlerTypeDef*);
uint16_t STRM_Read(STRM_handlerTypeDef*, uint8_t *rBuf, uint16_t size, uint32_t timeout);
void STRM_Unread(STRM_handlerTypeDef*, uint16_t length);
uint16_t STRM_Readline(STRM_handlerTypeDef*, uint8_t *rBuf, uint16_t size, uint32_t timeout);
void STRM_ReadToBuffer(STRM_handlerTypeDef*, STRM_Buffer_t*, uint16_t length, uint32_t timeout);
void STRM_Reset(STRM_handlerTypeDef*);


#endif /* INC_DMA_STREAMER_H_ */

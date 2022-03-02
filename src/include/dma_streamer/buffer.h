/*
 * buffer.h
 *
 *  Created on: Dec 10, 2021
 *      Author: janoko
 */

#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

#include "stm32f4xx_hal.h"

typedef struct {
  uint8_t *buffer;
  uint16_t size;
  uint16_t w_idx;
  uint16_t r_idx;
  uint8_t isOverlap;
} STRM_Buffer_t;

uint16_t STRM_Buffer_Read(STRM_Buffer_t *buf, uint8_t *data, uint16_t length);
void STRM_Buffer_Write(STRM_Buffer_t *buf, uint8_t *data, uint16_t length);
uint8_t STRM_Buffer_IsAvailable(STRM_Buffer_t *buf);

#endif /* INC_BUFFER_H_ */

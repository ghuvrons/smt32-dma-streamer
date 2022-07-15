/*
 * buffer.c
 *
 *  Created on: Dec 10, 2021
 *      Author: janoko
 */

#include "include/dma_streamer/buffer.h"


uint16_t STRM_Buffer_Read(STRM_Buffer_t *buf, uint8_t *dstBuf, uint16_t length)
{
  uint16_t readLen = 0;
  if (buf->buffer == NULL || dstBuf == NULL) return 0;
  while (length--) {
    if (buf->r_idx == buf->w_idx && !buf->isOverlap) break;
    *dstBuf = *(buf->buffer+buf->r_idx);
    buf->isOverlap = 0;
    buf->r_idx++;
    if (buf->r_idx == buf->size) buf->r_idx = 0;
    dstBuf++;
    readLen++;
  }
  return readLen;
}

void STRM_Buffer_Write(STRM_Buffer_t *buf, uint8_t *srcBuf, uint16_t length)
{
  if (buf->buffer == NULL || srcBuf == NULL) return;

  while (length--) {
    *(buf->buffer+buf->w_idx) = *srcBuf;
    srcBuf++;
    buf->w_idx++;
    if (buf->w_idx == buf->size) buf->w_idx = 0;

    if (buf->isOverlap == 1) buf->r_idx = buf->w_idx;
    if (buf->w_idx == buf->r_idx) {
      buf->isOverlap = 1;
    }
  }
}

uint8_t STRM_Buffer_IsAvailable(STRM_Buffer_t *buf)
{
  return buf->buffer && !(buf->r_idx == buf->w_idx && !buf->isOverlap);
}

/*
 * conf.h
 *
 *  Created on: Dec 3, 2021
 *      Author: janoko
 */

#ifndef DMA_STREAMER_INC_DMA_STREAMER_CONF_H_
#define DMA_STREAMER_INC_DMA_STREAMER_CONF_H_

#ifndef STRM_GetTick
#define STRM_GetTick() HAL_GetTick()
#endif
#ifndef STRM_Delay
#define STRM_Delay(ms) HAL_Delay(ms)
#endif

#endif /* DMA_STREAMER_SRC_INCLUDE_DMA_STREAMER_CONF_H_ */

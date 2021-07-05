#ifndef _RTSP_H_
#define _RTSP_H_

#include <stdbool.h>
#include <stdint.h>

// Init RTSP
bool rtsp_init();

// Free RTSP
bool rtsp_free();

// Send data frame
bool rtsp_media_frame(int chn, signed char *data, size_t size, uint32_t timestamp, uint8_t type);

#endif

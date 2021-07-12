#ifndef _RTSP_H_
#define _RTSP_H_

#include <stdbool.h>
#include <stdint.h>

// Init RTSP
bool rtsp_init();

// Is enabled
bool rtsp_is_enabled(int channel);

// Free RTSP
bool rtsp_free();

// Send data frame
bool rtsp_media_frame(int channel, signed char *data, size_t size, uint32_t timestamp, uint8_t type);

#endif

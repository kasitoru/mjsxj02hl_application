#ifndef _RTSP_H_
#define _RTSP_H_

#include <stdbool.h>
#include <stdint.h>

bool rtsp_init();

bool rtsp_free();

bool rtsp_media_frame(signed char *data, size_t size, uint32_t timestamp, int type);

#endif

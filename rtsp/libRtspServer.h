#ifndef __LIBRTSPSERVER_H
#define __LIBRTSPSERVER_H

#define XOP_VIDEO_FRAME_I 0x01
#define XOP_VIDEO_FRAME_P 0x02
#define XOP_VIDEO_FRAME_B 0x03
#define XOP_AUDIO_FRAME   0x11

#ifdef __cplusplus
extern "C"{
#endif

// Create RTSP server
bool rtspserver_create(uint16_t port, bool multicast, uint8_t video_type, uint32_t framerate);

// Get primary session id
uint32_t rtspserver_primary_id();

// Get secondary session id
uint32_t rtspserver_secondary_id();

// Timestamp for H264
uint32_t rtspserver_timestamp_h264();

// Timestamp for H265
uint32_t rtspserver_timestamp_h265();

// Timestamp for G711A
uint32_t rtspserver_timestamp_g711a();

// Send media frame
bool rtspserver_frame(uint32_t session_id, signed char *data, uint8_t type, uint32_t size, uint32_t timestamp);

// Free RTSP server
bool rtspserver_free();

#ifdef __cplusplus
}
#endif

#endif


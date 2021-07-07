#ifndef __LIBRTSPSERVER_H
#define __LIBRTSPSERVER_H

#define RTSP_SERVER_TIMESTAMP_H264 1
#define RTSP_SERVER_TIMESTAMP_H265 2
#define RTSP_SERVER_TIMESTAMP_G711 3

#define XOP_VIDEO_FRAME_I 0x01
#define XOP_VIDEO_FRAME_P 0x02
#define XOP_VIDEO_FRAME_B 0x03
#define XOP_AUDIO_FRAME   0x11

#ifdef __cplusplus
extern "C"{
#endif

// Set log printf function
bool rtspserver_logprintf(int (*function)(const char*, ...));

// Create RTSP server
bool rtspserver_create(uint16_t port, char *username, char *password);

// Create new session
uint32_t rtspserver_session(char *name, bool multicast, uint8_t video_type, uint32_t framerate, bool audio);

// Get current timestamp
uint32_t rtspserver_timestamp(uint8_t source);

// Send media frame
bool rtspserver_frame(uint32_t session_id, signed char *data, uint8_t type, uint32_t size, uint32_t timestamp);

// Free RTSP server
bool rtspserver_free(uint32_t count, ...);

#ifdef __cplusplus
}
#endif

#endif


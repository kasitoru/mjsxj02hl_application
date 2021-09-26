#ifndef __LIBRTSPSERVER_H
#define __LIBRTSPSERVER_H

#define LIBRTSPSERVER_TYPE_NONE  0
#define LIBRTSPSERVER_TYPE_H264  1
#define LIBRTSPSERVER_TYPE_H265  2
#define LIBRTSPSERVER_TYPE_AAC   3
#define LIBRTSPSERVER_TYPE_G711A 4
#define LIBRTSPSERVER_TYPE_VP8   5

#define LIBRTSPSERVER_UNKNOWN_FRAME 0x00
#define LIBRTSPSERVER_VIDEO_FRAME_I 0x01
#define LIBRTSPSERVER_VIDEO_FRAME_P 0x02
#define LIBRTSPSERVER_VIDEO_FRAME_B 0x03
#define LIBRTSPSERVER_AUDIO_FRAME   0x11

#ifdef __cplusplus
extern "C"{
#endif

// Set log printf function
bool rtspserver_logprintf(int (*function)(const char*, ...));

// Set connected callback function
bool rtspserver_connected(void (*function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port));

// Set disconnected callback function
bool rtspserver_disconnected(void (*function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port));

// Create RTSP server
bool rtspserver_create(uint16_t port, char *username, char *password);

// Create new session
uint32_t rtspserver_session(char *name, bool multicast, uint8_t video_type, uint32_t framerate, uint8_t audio_type, uint32_t samplerate, uint32_t channels, bool has_adts);

// Get current timestamp
uint32_t rtspserver_timestamp(uint8_t source, uint32_t samplerate);

// Send media frame
bool rtspserver_frame(uint32_t session_id, signed char *data, uint8_t type, uint32_t size, uint32_t timestamp, bool split_video);

// Free RTSP server
bool rtspserver_free(uint32_t count, ...);

#ifdef __cplusplus
}
#endif

#endif


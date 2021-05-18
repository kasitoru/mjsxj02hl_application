#ifndef _LOCALSDK_H_
#define _LOCALSDK_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/********************
       GENERAL
********************/

#define LOCALSDK_OK               0
#define LOCALSDK_ERROR            1

#define LOCALSDK_CURRENT_VERSION  14

#define LOCALSDK_G711_BUFFER_SIZE 320
#define LOCALSDK_PCM_BUFFER_SIZE  640

#define LOCALSDK_PCM_TRACK_TYPE   1
#define LOCALSDK_G711_TRACK_TYPE  2

// Set printf function for debug messages
int localsdk_set_logprintf_func(int (*function)(const char*, ...));

// Set shell function
int localsdk_set_shellcall_func(int *param_1); // FIXME

// Initialize SDK
int localsdk_init();

// Destory SDK
int localsdk_destory();

// Get SDK version
int localsdk_get_version();

/********************
        VIDEO
********************/

#define LOCALSDK_VIDEO_FPS                  20
#define LOCALSDK_VIDEO_PAYLOAD_TYPE         1
#define LOCALSDK_VIDEO_RCMODE_TYPE          2

#define LOCALSDK_VIDEO_PRIMARY_CHANNEL      0
#define LOCALSDK_VIDEO_PRIMARY_BITRATE      110
#define LOCALSDK_VIDEO_PRIMARY_RESOLUTION   6
#define LOCALSDK_VIDEO_PRIMARY_WIDTH        1920
#define LOCALSDK_VIDEO_PRIMARY_HEIGHT       1080

#define LOCALSDK_VIDEO_SECONDARY_CHANNEL    1
#define LOCALSDK_VIDEO_SECONDARY_BITRATE    120
#define LOCALSDK_VIDEO_SECONDARY_RESOLUTION 3
#define LOCALSDK_VIDEO_SECONDARY_WIDTH      640
#define LOCALSDK_VIDEO_SECONDARY_HEIGHT     360

typedef struct {
    signed char *data;
    uint32_t size;
    uint32_t index;
    uint32_t timestamp;
    uint16_t unknown_4; // FIXME: what is it?
    uint16_t unknown_5; // FIXME: what is it?
    uint16_t unknown_6; // FIXME: what is it?
} LOCALSDK_H26X_FRAME_INFO;

typedef struct {
    uint32_t bitrate;
    uint32_t fps;
    uint32_t resolution;
    uint32_t flip;
    uint32_t mirror;
    uint32_t unknown_5; // FIXME: what is it?
    uint32_t unknown_6; // FIXME: what is it?
    uint32_t unknown_7; // FIXME: what is it?
    uint32_t payload;
    uint32_t rcmode;
    uint32_t gop;
    uint32_t screen_size;
    uint32_t unknown_12; // FIXME: what is it?
    uint32_t unknown_13; // FIXME: what is it?
    uint32_t unknown_14; // FIXME: what is it?
} LOCALSDK_VIDEO_OPTIONS;

typedef struct {
    uint32_t unknown_0; // FIXME: what is it?
    uint32_t unknown_1; // FIXME: what is it?
    uint32_t unknown_2; // FIXME: what is it?
    uint32_t unknown_3; // FIXME: what is it?
    uint32_t unknown_4; // FIXME: what is it?
    uint32_t unknown_5; // FIXME: what is it?
    uint32_t unknown_6; // FIXME: what is it?
    uint32_t unknown_7; // FIXME: what is it?
    uint32_t unknown_8; // FIXME: what is it?
} LOCALSDK_OSD_OPTIONS;

// Init video
int local_sdk_video_init(int fps);

// Create video
int local_sdk_video_create(int chn, LOCALSDK_VIDEO_OPTIONS *options);

// Set video parameters
int local_sdk_video_set_parameters(int chn, LOCALSDK_VIDEO_OPTIONS *options);

// Set video frame callback
int local_sdk_video_set_encode_frame_callback(int chn, int (*callback)(LOCALSDK_H26X_FRAME_INFO *frameInfo));
int local_sdk_video_set_yuv_frame_callback(int chn, int (*callback)(LOCALSDK_H26X_FRAME_INFO *frameInfo)); // FIXME: Need own structure?

// Set video algo module callback
int local_sdk_video_set_algo_module_register_callback(int (*callback)());
int local_sdk_video_set_algo_module_unregister_callback(int (*callback)());

// Start video
int local_sdk_video_start(int chn);

// Stop video
int local_sdk_video_stop(int chn, bool state);

// Run video
int local_sdk_video_run(int chn);

// Save video image to jpeg file
int local_sdk_video_get_jpeg(int chn, char *file);

// TODO:
int local_sdk_video_force_I_frame(int param_1);
int local_sdk_video_set_brightness(int param_1, int param_2, int param_3, int param_4);
int local_sdk_video_set_flip(int param_1, int param_2);
int local_sdk_video_set_fps(int param_1, int param_2, int param_3, int param_4);
int local_sdk_video_set_kbps(int param_1, int param_2);

/********************
        OSD
********************/

// Set osd parameters
int local_sdk_video_osd_set_parameters(int chn, LOCALSDK_OSD_OPTIONS *options);

// TODO:
int local_sdk_video_osd_update_logo(int param_1, unsigned int param_2);
int local_sdk_video_osd_update_rect_multi(int param_1, int param_2, int *param_3);
int local_sdk_video_osd_update_timestamp(int param_1, int param_2, struct tm * timeptr);

/********************
        AUDIO
********************/

#define LOCALSDK_AUDIO_CHANNEL     0
#define LOCALSDK_AUDIO_SAMPLE_RATE 8000
#define LOCALSDK_AUDIO_BIT_DEPTH   16
#define LOCALSDK_AUDIO_TRACK_TYPE  LOCALSDK_PCM_TRACK_TYPE

typedef struct {
    signed char *data;
    uint32_t size;
    uint32_t index;
    uint32_t timestamp;
} LOCALSDK_G711_FRAME_INFO;

typedef struct {
    uint32_t sample_rate;
    uint32_t bit_depth;
    uint32_t unknown_2; // FIXME: what is it?
    uint32_t track_type;
    uint32_t unknown_4; // FIXME: what is it?
    uint32_t unknown_5; // FIXME: what is it?
    uint32_t unknown_6; // FIXME: what is it?
    uint32_t unknown_7; // FIXME: what is it?
    uint32_t unknown_8; // FIXME: what is it?
    uint32_t unknown_9; // FIXME: what is it?
    uint32_t volume;
    uint32_t pcm_buffer_size;
    uint32_t g711_buffer_size;
} LOCALSDK_AUDIO_OPTIONS;

// Init audio
int local_sdk_audio_init();

// Create audio
int local_sdk_audio_create(int chn);

// Set audio parameters
int local_sdk_audio_set_parameters(int chn, LOCALSDK_AUDIO_OPTIONS *options);

// Enable AEC
int local_sdk_audio_set_aec_enable(int chn, bool state);

// Set volume
int local_sdk_audio_set_volume(int chn, int value);

// Set audio encode callback
int local_sdk_audio_set_encode_frame_callback(int chn, int (*callback)(LOCALSDK_G711_FRAME_INFO *frameInfo));

// Set audio pcm callback
int local_sdk_audio_set_pcm_frame_callback(int chn, int (*callback)(LOCALSDK_G711_FRAME_INFO *frameInfo));

// Start audio
int local_sdk_audio_start();

// Stop audio
int local_sdk_audio_stop();

// Run audio
int local_sdk_audio_run();

// End audio
int local_sdk_audio_end();

// Destory audio
int local_sdk_audio_destory();

/********************
       SPEAKER
********************/

#define LOCALSDK_SPEAKER_SAMPLE_RATE 8000
#define LOCALSDK_SPEAKER_BIT_DEPTH   16
#define LOCALSDK_SPEAKER_TRACK_TYPE  LOCALSDK_PCM_TRACK_TYPE

#if LOCALSDK_SPEAKER_TRACK_TYPE == LOCALSDK_PCM_TRACK_TYPE
    #define LOCALSDK_SPEAKER_BUFFER_SIZE LOCALSDK_PCM_BUFFER_SIZE
#elif LOCALSDK_SPEAKER_TRACK_TYPE == LOCALSDK_G711_TRACK_TYPE
    #define LOCALSDK_SPEAKER_BUFFER_SIZE LOCALSDK_G711_BUFFER_SIZE
#endif

typedef struct {
    uint32_t sample_rate;
    uint32_t bit_depth;
    uint32_t unknown_2; // FIXME: what is it?
    uint32_t track_type;
    uint32_t unknown_4; // FIXME: what is it?
    uint32_t volume;
    uint32_t buffer_size;
    uint32_t unknown_7; // FIXME: what is it?
} LOCALSDK_SPEAKER_OPTIONS;

// Init speaker
int local_sdk_speaker_init();

// Set speaker parameters
int local_sdk_speaker_set_parameters(LOCALSDK_SPEAKER_OPTIONS *options);

// Set volume
int local_sdk_speaker_set_volume(int value);

// Enable mute
int local_sdk_speaker_mute();

// Disable mute
int local_sdk_speaker_unmute();

// Start speaker
int local_sdk_speaker_start();

// Feed g711 data
int local_sdk_speaker_feed_g711_data(void *data, int size);

// Feed PCM data
int local_sdk_speaker_feed_pcm_data(void *data, int size);

// Finish buffer
int local_sdk_speaker_finish_buf_data();

// Clean buffer
int local_sdk_speaker_clean_buf_data();

/********************
        ALARM
********************/

#define LOCALSDK_ALARM_MOTION   1
#define LOCALSDK_ALARM_HUMANOID 7

typedef struct {
    uint32_t unknown_1; // FIXME: what is it?
    uint32_t type; // type (motion/humanoid)
    uint32_t unknown_3; // FIXME: what is it?
    uint32_t state; // state (start/stop)
    uint32_t unknown_5; // FIXME: what is it?
    uint32_t unknown_6; // FIXME: what is it?
    uint32_t unknown_7; // FIXME: what is it?
    uint32_t unknown_8; // FIXME: what is it?
    uint32_t unknown_9; // FIXME: what is it?
    uint32_t unknown_10; // FIXME: what is it?
} LOCALSDK_ALARM_EVENT_INFO;

// Init alarm
int local_sdk_alarm_init(int width, int height);

// Set alarm sensitivity (1...255)
// Type: LOCALSDK_ALARM_MOTION or LOCALSDK_ALARM_HUMANOID
int local_sdk_set_alarm_sensitivity(int type, int value);

// Exit alarm
int local_sdk_alarm_exit();

// Set motor state
int local_sdk_alarm_set_motor_state();

// Set alarm algo module callback
int local_sdk_alarm_algo_module_register_callback();

// Unset alarm algo module callback
int local_sdk_alarm_algo_module_unregister_callback();

// Set alarm state callback
int local_sdk_alarm_state_set_callback(int (*callback)(LOCALSDK_ALARM_EVENT_INFO *eventInfo));

// Clear alarm state callback
int local_sdk_alarm_state_clear_callback(int (*callback)(LOCALSDK_ALARM_EVENT_INFO *eventInfo));

// Set alarm network state
int local_sdk_set_alarm_network_state();

// Set alarm switch
// Type: LOCALSDK_ALARM_MOTION or LOCALSDK_ALARM_HUMANOID
int local_sdk_set_alarm_switch(int type, bool state);

/********************
        LEDS
********************/

int local_sdk_indicator_led_option(bool orange, bool blue);

/********************
       BUTTON
********************/

int local_sdk_setup_keydown_set_callback(int timeout, int (*callback)());

/********************
     NIGHT MODE
********************/

// Color image
int local_sdk_video_set_daytime_mode();

// Grayscale image
int local_sdk_video_set_night_mode();

// Enable auto night mode
int local_sdk_auto_night_light();

// Enable manual night mode
int local_sdk_open_night_light();

// Disable manual night mode
int local_sdk_close_night_light();

// Set night mode state callback
int local_sdk_night_state_set_callback(int (*callback)(int state));

// Opet IR-cut filter
int local_sdk_open_ircut();

// Close IR-cut filter
int local_sdk_close_ircut();

#ifdef __cplusplus
}
#endif

#endif

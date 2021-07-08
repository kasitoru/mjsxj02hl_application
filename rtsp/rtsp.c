#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "./rtsp.h"
#include "./libRtspServer.h"
#include "./../localsdk/localsdk.h"
#include "./../logger/logger.h"
#include "./../configs/configs.h"

static uint32_t primary_session, secondary_session;

// Logger function for libRtspServer
static int librtspserver_logger(const char *format, ...) {
    int result = 0;
    char *message;
    va_list params;
    va_start(params, format);
    if(vasprintf(&message, format, params) > 0) {
        result = logger("rtsp", "rtspserver", LOGGER_LEVEL_INFO, message);
        free(message);
    }
    va_end(params);
    return result;
}

// LocalSDK video type to libRtspServer type
static uint8_t librtspserver_video_type(uint8_t type) {
    switch(type) {
        case LOCALSDK_VIDEO_PAYLOAD_H264: return LIBRTSPSERVER_TYPE_H264;
        case LOCALSDK_VIDEO_PAYLOAD_H265: return LIBRTSPSERVER_TYPE_H265;
        default: return LIBRTSPSERVER_TYPE_NONE;
    }
}

// LocalSDK frame type to libRtspServer type
static uint8_t librtspserver_frame_type(uint8_t type) {
    switch(type) {
        case LOCALSDK_VIDEO_H26X_FRAME_I: return LIBRTSPSERVER_VIDEO_FRAME_I;
        case LOCALSDK_VIDEO_H26X_FRAME_P: return LIBRTSPSERVER_VIDEO_FRAME_P;
        case LOCALSDK_AUDIO_G711_FRAME: return LIBRTSPSERVER_AUDIO_FRAME;
        default: return LIBRTSPSERVER_UNKNOWN_FRAME;
    }
}

// Is enabled
bool rtsp_is_enabled() {
    return APP_CFG.rtsp.enable;
}

// Init RTSP
bool rtsp_init() {
    bool result = false;
    logger("rtsp", "rtsp_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(rtsp_is_enabled()) { // If RTSP enabled
        if(rtspserver_logprintf(librtspserver_logger)) {
            logger("rtsp", "rtsp_init", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_logprintf()");
            if(rtspserver_create(APP_CFG.rtsp.port, APP_CFG.rtsp.username, APP_CFG.rtsp.password)) {
                logger("rtsp", "rtsp_init", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_create()");
                // Primary channel
                char *primary_name = APP_CFG.rtsp.primary_name;
                bool primary_multicast = APP_CFG.rtsp.primary_multicast;
                uint8_t primary_video_type = (APP_CFG.video.primary_enable ? librtspserver_video_type(APP_CFG.video.primary_type) : LIBRTSPSERVER_TYPE_NONE);
                uint32_t primary_framerate = LOCALSDK_VIDEO_FRAMERATE;
                uint8_t primary_audio_type = (APP_CFG.audio.primary_enable ? LIBRTSPSERVER_TYPE_G711A : LIBRTSPSERVER_TYPE_NONE);
                if(primary_session = rtspserver_session(primary_name, primary_multicast, primary_video_type, primary_framerate, primary_audio_type, 0, 0, false)) {
                    logger("rtsp", "rtsp_init", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_session(primary)");
                    // Secondary channel
                    char *secondary_name = APP_CFG.rtsp.secondary_name;
                    bool secondary_multicast = APP_CFG.rtsp.secondary_multicast;
                    uint8_t secondary_video_type = (APP_CFG.video.secondary_enable ? librtspserver_video_type(APP_CFG.video.secondary_type) : LIBRTSPSERVER_TYPE_NONE);
                    uint32_t secondary_framerate = LOCALSDK_VIDEO_FRAMERATE;
                    uint8_t secondary_audio_type = (APP_CFG.audio.secondary_enable ? LIBRTSPSERVER_TYPE_G711A : LIBRTSPSERVER_TYPE_NONE);
                    if(secondary_session = rtspserver_session(secondary_name, secondary_multicast, secondary_video_type, secondary_framerate, secondary_audio_type, 0, 0, false)) {
                        logger("rtsp", "rtsp_init", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_session(secondary)");
                        result = true;
                    } else logger("rtsp", "rtsp_init", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_session(secondary)");
                } else logger("rtsp", "rtsp_init", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_session(primary)");
            } else logger("rtsp", "rtsp_init", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_create()");
        } else logger("rtsp", "rtsp_init", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_logprintf()");
    } else {
        logger("rtsp", "rtsp_init", LOGGER_LEVEL_WARNING, "RTSP server is disabled in the settings.");
        result = true;
    }
    logger("rtsp", "rtsp_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free RTSP
bool rtsp_free() {
    bool result = false;
    logger("rtsp", "rtsp_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(rtsp_is_enabled()) { // If RTSP enabled
        if(result = rtspserver_free(2, primary_session, secondary_session)) {
            logger("rtsp", "rtsp_free", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_free()");
        } else logger("rtsp", "rtsp_free", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_free()");
    } else result = true;
    logger("rtsp", "rtsp_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Send data frame
bool rtsp_media_frame(int chn, signed char *data, size_t size, uint32_t timestamp, uint8_t type) {
    bool result = false;
    if(rtsp_is_enabled()) { // If RTSP enabled
        // Get current timestamp
        if(type == LOCALSDK_AUDIO_G711_FRAME) {
            timestamp = rtspserver_timestamp(LIBRTSPSERVER_TYPE_G711A, 0);
        } else {
            if(
                ((chn == LOCALSDK_VIDEO_PRIMARY_CHANNEL) && (APP_CFG.video.primary_type == LOCALSDK_VIDEO_PAYLOAD_H264))
                ||
                ((chn == LOCALSDK_VIDEO_SECONDARY_CHANNEL) && (APP_CFG.video.secondary_type == LOCALSDK_VIDEO_PAYLOAD_H264))
            ) {
                timestamp = rtspserver_timestamp(LIBRTSPSERVER_TYPE_H264, 0);
            } else if(
                ((chn == LOCALSDK_VIDEO_PRIMARY_CHANNEL) && (APP_CFG.video.primary_type == LOCALSDK_VIDEO_PAYLOAD_H265))
                ||
                ((chn == LOCALSDK_VIDEO_SECONDARY_CHANNEL) && (APP_CFG.video.secondary_type == LOCALSDK_VIDEO_PAYLOAD_H265))
            ) {
                timestamp = rtspserver_timestamp(LIBRTSPSERVER_TYPE_H265, 0);
            }
        }
        // Send frame
        if(rtspserver_frame((chn == LOCALSDK_VIDEO_SECONDARY_CHANNEL ? secondary_session : primary_session), data, librtspserver_frame_type(type), size, timestamp)) {
            result = true;
        }
    } else result = true;
    return result;
}


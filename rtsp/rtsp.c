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
                if(primary_session = rtspserver_session("primary", APP_CFG.rtsp.multicast, APP_CFG.video.type, APP_CFG.video.fps, true)) {
                    logger("rtsp", "rtsp_init", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_session(primary)");
                    // Secondary channel
                    if(secondary_session = rtspserver_session("secondary", APP_CFG.rtsp.multicast, APP_CFG.video.type, APP_CFG.video.fps, true)) {
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
            timestamp = rtspserver_timestamp(RTSP_SERVER_TIMESTAMP_G711);
        } else {
            if(APP_CFG.video.type == LOCALSDK_VIDEO_PAYLOAD_H264) {
                timestamp = rtspserver_timestamp(RTSP_SERVER_TIMESTAMP_H264);
            } else {
                timestamp = rtspserver_timestamp(RTSP_SERVER_TIMESTAMP_H265);
            }
        }
        // Frame type
        switch(type) {
            case LOCALSDK_VIDEO_H26X_FRAME_I:
                type = XOP_VIDEO_FRAME_I;
                break;
            case LOCALSDK_VIDEO_H26X_FRAME_P:
                type = XOP_VIDEO_FRAME_P;
                break;
            case LOCALSDK_AUDIO_G711_FRAME:
                type = XOP_AUDIO_FRAME;
                break;
            default:
                type = 0x00;
        }
        // Send frame
        if(rtspserver_frame((chn == LOCALSDK_VIDEO_SECONDARY_CHANNEL ? secondary_session : primary_session), data, type, size, timestamp)) {
            result = true;
        }
    } else result = true;
    return result;
}


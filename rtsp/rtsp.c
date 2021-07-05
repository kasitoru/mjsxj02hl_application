#include <stdlib.h>
#include <stdio.h>

#include "./rtsp.h"
#include "./libRtspServer.h"
#include "./../localsdk/localsdk.h"
#include "./../logger/logger.h"
#include "./../configs/configs.h"

// Init RTSP
bool rtsp_init() {
    bool result = false;
    logger("rtsp", "rtsp_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    if(rtspserver_create(APP_CFG.rtsp.port, APP_CFG.video.type, APP_CFG.video.fps)) {
        logger("rtsp", "rtsp_init", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_create()");
        result = true;
    } else logger("rtsp", "rtsp_init", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_create()");
    
    logger("rtsp", "rtsp_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free RTSP
bool rtsp_free() {
    bool result = false;
    
    logger("rtsp", "rtsp_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    if(result = rtspserver_free()) {
        logger("rtsp", "rtsp_free", LOGGER_LEVEL_INFO, "%s success.", "rtspserver_free()");
    } else logger("rtsp", "rtsp_free", LOGGER_LEVEL_ERROR, "%s error!", "rtspserver_free()");
    
    logger("rtsp", "rtsp_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Send data frame
bool rtsp_media_frame(int chn, signed char *data, size_t size, uint32_t timestamp, uint8_t type) {
    bool result = false;
    
    // Get session id
    uint32_t session_id;
    if(chn == LOCALSDK_VIDEO_SECONDARY_CHANNEL) {
        session_id = rtspserver_secondary_id();
    } else {
        session_id = rtspserver_primary_id();
    }
    
    // Get current timestamp
    if(type == XOP_AUDIO_FRAME) {
        timestamp = rtspserver_timestamp_g711a();
    } else {
        if(APP_CFG.video.type == LOCALSDK_VIDEO_PAYLOAD_H264) {
            timestamp = rtspserver_timestamp_h264();
        } else {
            timestamp = rtspserver_timestamp_h265();
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
    if(rtspserver_frame(session_id, data, type, size, timestamp)) {
        result = true;
    }
    
    return result;
}


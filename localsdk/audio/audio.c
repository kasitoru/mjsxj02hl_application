#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "./audio.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"
#include "./../../rtsp/rtsp.h"

// Audio capture callback
static int g711_capture_callback(int chn, LOCALSDK_AUDIO_G711_FRAME_INFO *frameInfo) {
    int result = LOCALSDK_OK;
    if(frameInfo && frameInfo->size) {
        // RTSP
        if(rtsp_is_enabled(chn)) {
            if(!rtsp_media_frame(chn, frameInfo->data, frameInfo->size, frameInfo->timestamp, LOCALSDK_AUDIO_G711_FRAME)) {
                result = LOCALSDK_ERROR;
            }
        }
    }
    return result;
}

static int g711_capture_primary_channel(LOCALSDK_AUDIO_G711_FRAME_INFO *frameInfo) {
    if(audio_is_enabled(LOCALSDK_VIDEO_PRIMARY_CHANNEL)) {
        return g711_capture_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL, frameInfo);
    } else return LOCALSDK_ERROR;
}

static int g711_capture_secondary_channel(LOCALSDK_AUDIO_G711_FRAME_INFO *frameInfo) {
    if(audio_is_enabled(LOCALSDK_VIDEO_SECONDARY_CHANNEL)) {
        return g711_capture_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL, frameInfo);
    } else return LOCALSDK_ERROR;
}

// Is enabled
bool audio_is_enabled(int channel) {
    switch(channel) {
        case LOCALSDK_VIDEO_PRIMARY_CHANNEL: return APP_CFG.audio.primary_enable;
        case LOCALSDK_VIDEO_SECONDARY_CHANNEL: return APP_CFG.audio.secondary_enable;
        default: return false;
    }
}

// Init audio
bool audio_init() {
    logger("audio", "audio_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(local_sdk_audio_init() == LOCALSDK_OK) {
        logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_init()");
        // Init channel 0
        if(local_sdk_audio_create(LOCALSDK_AUDIO_CHANNEL) == LOCALSDK_OK) {
            logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_create()");
            LOCALSDK_AUDIO_OPTIONS audio_options = {
                .sample_rate      = LOCALSDK_AUDIO_SAMPLE_RATE,
                .bit_depth        = LOCALSDK_AUDIO_BIT_DEPTH,
                .unknown_2        = 25, // FIXME: what is it?
                .track_type       = LOCALSDK_AUDIO_TRACK_TYPE,
                .unknown_4        = 0, // FIXME: what is it?
                .unknown_5        = 2, // FIXME: what is it?
                .unknown_6        = 1, // FIXME: what is it?
                .unknown_7        = 1, // FIXME: what is it?
                .unknown_8        = 2, // FIXME: what is it?
                .unknown_9        = 20, // FIXME: what is it?
                .volume           = APP_CFG.audio.volume,
                .pcm_buffer_size  = LOCALSDK_AUDIO_PCM_BUFFER_SIZE,
                .g711_buffer_size = LOCALSDK_AUDIO_G711_BUFFER_SIZE,
            };
            if(local_sdk_audio_set_parameters(LOCALSDK_AUDIO_CHANNEL, &audio_options) == LOCALSDK_OK) {
                logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_set_parameters()");
                if(local_sdk_audio_set_encode_frame_callback(LOCALSDK_AUDIO_CHANNEL, g711_capture_primary_channel) == LOCALSDK_OK) {
                    logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_set_encode_frame_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                    if(local_sdk_audio_set_encode_frame_callback(LOCALSDK_AUDIO_CHANNEL, g711_capture_secondary_channel) == LOCALSDK_OK) {
                        logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_set_encode_frame_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                        if(local_sdk_audio_start() == LOCALSDK_OK) {
                            logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_start()");
                            if(local_sdk_audio_run() == LOCALSDK_OK) {
                                logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_run()");
                                
                                logger("audio", "audio_init", LOGGER_LEVEL_DEBUG, "Function completed.");
                                return true;
                            } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_run()");
                        } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_start()");
                    } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_encode_frame_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_encode_frame_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
            } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_parameters()");
        } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_create()");
    } else logger("audio", "audio_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_init()");
    if(audio_free()) {
        logger("audio", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "audio_free()");
    } else logger("audio", "audio_init", LOGGER_LEVEL_WARNING, "%s error!", "audio_free()");
    logger("audio", "audio_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return false;
}

// Free audio
bool audio_free() {
    bool result = true;
    logger("audio", "audio_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    // Stop audio
    if(local_sdk_audio_stop() == LOCALSDK_OK) {
        logger("audio", "audio_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_stop()");
    } else {
        logger("audio", "audio_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_audio_stop()");
        result = false;
    }
    // End audio
    if(local_sdk_audio_end() == LOCALSDK_OK) {
        logger("audio", "audio_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_end()");
    } else {
        logger("audio", "audio_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_audio_end()");
        result = false;
    }
    // Destory audio
    if(local_sdk_audio_destory() == LOCALSDK_OK) {
        logger("audio", "audio_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_destory()");
    } else {
        logger("audio", "audio_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_audio_destory()");
        result = false;
    }
    logger("audio", "audio_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

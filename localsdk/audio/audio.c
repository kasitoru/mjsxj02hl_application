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
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(result &= (local_sdk_audio_init() == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_init()");
        // Init channel 0
        if(result &= (local_sdk_audio_create(LOCALSDK_AUDIO_CHANNEL) == LOCALSDK_OK)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_create()");
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
            if(result &= (local_sdk_audio_set_parameters(LOCALSDK_AUDIO_CHANNEL, &audio_options) == LOCALSDK_OK)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_set_parameters()");
                if(result &= (local_sdk_audio_set_encode_frame_callback(LOCALSDK_AUDIO_CHANNEL, g711_capture_primary_channel) == LOCALSDK_OK)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_set_encode_frame_callback(g711_capture_primary_channel)");
                    if(result &= (local_sdk_audio_set_encode_frame_callback(LOCALSDK_AUDIO_CHANNEL, g711_capture_secondary_channel) == LOCALSDK_OK)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_set_encode_frame_callback(g711_capture_secondary_channel)");
                        if(result &= (local_sdk_audio_start() == LOCALSDK_OK)) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_start()");
                            if(result &= (local_sdk_audio_run() == LOCALSDK_OK)) {
                                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_run()");
                            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_run()");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_start()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_encode_frame_callback(g711_capture_secondary_channel)");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_encode_frame_callback(g711_capture_primary_channel)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_parameters()");
        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_create()");
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_init()");
    
    // Free alarm if error occurred
    if(!result) {
        if(audio_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "audio_free()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "audio_free()");
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free audio
bool audio_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    // Stop audio
    if(result &= (local_sdk_audio_stop() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_stop()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_audio_stop()");
    
    // End audio
    if(result &= (local_sdk_audio_end() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_end()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_audio_end()");
    
    // Destory audio
    if(result &= (local_sdk_audio_destory() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_audio_destory()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_audio_destory()");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "./audio.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"

int g711_capture_callback(LOCALSDK_G711_FRAME_INFO *frameInfo) {
    if(frameInfo && frameInfo->unknown_2) {
        // FIXME: how get data?
        /*
        logger("audio", "g711_capture_callback", LOGGER_LEVEL_DEBUG, "Function is called...");
        logger("audio", "g711_capture_callback", LOGGER_LEVEL_DEBUG, "    frameInfo->size: %u\n", frameInfo->size);
        logger("audio", "g711_capture_callback", LOGGER_LEVEL_DEBUG, "    frameInfo->unknown_2: %u\n", frameInfo->unknown_2);
        logger("audio", "g711_capture_callback", LOGGER_LEVEL_DEBUG, "    frameInfo->index: %u\n", frameInfo->index);
        logger("audio", "g711_capture_callback", LOGGER_LEVEL_DEBUG, "    frameInfo->timestamp: %u\n", frameInfo->timestamp);
        logger("audio", "g711_capture_callback", LOGGER_LEVEL_DEBUG, "\n");
        */
        return LOCALSDK_OK;
    }
    return LOCALSDK_ERROR;
}

// Init audio
bool audio_init() {
    logger("audio", "audio_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    if(local_sdk_audio_init() == LOCALSDK_OK) {
        logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_init()");
        // Init channel 0
        LOCALSDK_AUDIO_OPTIONS audio_options = {
            .unknown_0  = 8000,
            .unknown_1  = 16,
            .unknown_2  = 25,
            .unknown_3  = 1,
            .unknown_4  = 0,
            .unknown_5  = 2,
            .unknown_6  = 1,
            .unknown_7  = 1,
            .unknown_8  = 2,
            .unknown_9  = 20,
            .unknown_10 = 70,
            .unknown_11 = 640,
            .unknown_12 = 320,
        };
        if(local_sdk_audio_create(LOCALSDK_AUDIO_CHANNEL, &audio_options) == LOCALSDK_OK) {
            logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_create()");
            if(local_sdk_audio_set_parameters(LOCALSDK_AUDIO_CHANNEL, &audio_options) == LOCALSDK_OK) {
                logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_set_parameters()");
                if(local_sdk_audio_set_encode_frame_callback(LOCALSDK_AUDIO_CHANNEL, g711_capture_callback) == LOCALSDK_OK) {
                    logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_set_encode_frame_callback()");
                    if(local_sdk_audio_start() == LOCALSDK_OK) {
                        logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_start()");
                        if(local_sdk_audio_run() == LOCALSDK_OK) {
                            logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_audio_run()");
                            
                            logger("audio", "audio_init", LOGGER_LEVEL_DEBUG, "Function completed.");
                            return true;
                        } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_run()");
                    } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_start()");
                } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_encode_frame_callback()");
            } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_set_parameters()");
        } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_create()");
    } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_audio_init()");
    if(audio_free()) {
        logger("alarm", "audio_init", LOGGER_LEVEL_INFO, "%s success.", "audio_free()");
    } else logger("alarm", "alarm_init", LOGGER_LEVEL_WARNING, "%s error!", "audio_free()");
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

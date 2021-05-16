#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "./init.h"
#include "./localsdk.h"
#include "./video/video.h"
#include "./audio/audio.h"
#include "./speaker/speaker.h"
#include "./alarm/alarm.h"
#include "./night/night.h"
#include "./../logger/logger.h"
#include "./../configs/configs.h"

// Log printf function
int logprintf(const char *format, ...) {
    int result = 0;
    if(LOGGER_LEVEL_INFO <= APP_CFG.logger.level) {
        va_list params;
        va_start(params, format);
        result = vprintf(format, params);
        va_end(params);
    }
    return result;
}

// Init all
bool all_init() {
    logger("localsdk-init", "all_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(localsdk_set_logprintf_func(logprintf) == LOCALSDK_OK) {
        logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "localsdk_set_logprintf_func()");
        //if(localsdk_set_shellcall_func(shellcall_func) == LOCALSDK_OK) { // FIXME
            //logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "localsdk_set_shellcall_func()");
            if(localsdk_init() == LOCALSDK_OK) {
                logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "localsdk_init()");
                if(localsdk_get_version() == LOCALSDK_CURRENT_VERSION) {
                    logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "localsdk_get_version()");
                    if(video_init()) {
                        logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "video_init()");
                        if(audio_init()) {
                            logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "audio_init()");
                            if(speaker_init()) {
                                logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "speaker_init()");
                                if(alarm_init()) {
                                    logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "alarm_init()");
                                    if(night_init()) {
                                        logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "night_init()");
                                        
                                        logger("localsdk-init", "all_init", LOGGER_LEVEL_DEBUG, "Function completed.");
                                        return true;
                                    } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "night_init()");
                                } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "alarm_init()");
                            } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "speaker_init()");
                        } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "audio_init()");
                    } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "video_init()");
                } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "localsdk_get_version()");
            } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "localsdk_init()");
        //} else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "localsdk_set_shellcall_func()");
    } else logger("localsdk-init", "all_init", LOGGER_LEVEL_ERROR, "%s error!", "localsdk_set_logprintf_func()");
    if(all_free()) {
        logger("localsdk-init", "all_init", LOGGER_LEVEL_INFO, "%s success.", "all_free()");
    } else logger("localsdk-init", "all_init", LOGGER_LEVEL_WARNING, "%s error!", "all_free()");
    logger("localsdk-init", "all_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return false;
}

// Free all
bool all_free() {
    bool result = true;
    logger("localsdk-init", "all_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Free night mode
    if(night_free()) {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_INFO, "%s success.", "night_free()");
    } else {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_WARNING, "%s error!", "night_free()");
        result = false;
    }
    
    // Free alarm
    if(alarm_free()) {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_INFO, "%s success.", "alarm_free()");
    } else {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_WARNING, "%s error!", "alarm_free()");
        result = false;
    }
    
    // Free speaker
    if(speaker_free()) {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_INFO, "%s success.", "speaker_free()");
    } else {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_WARNING, "%s error!", "speaker_free()");
        result = false;
    }
    
    // Free audio
    if(audio_free()) {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_INFO, "%s success.", "audio_free()");
    } else {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_WARNING, "%s error!", "audio_free()");
        result = false;
    }
    
    // Free video
    if(video_free()) {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_INFO, "%s success.", "video_free()");
    } else {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_WARNING, "%s error!", "video_free()");
        result = false;
    }
    
    // Free localsdk
    if(localsdk_destory() == LOCALSDK_OK) {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_INFO, "%s success.", "localsdk_destory()");
    } else {
        logger("localsdk-init", "all_free", LOGGER_LEVEL_WARNING, "%s error!", "localsdk_destory()");
        result = false;
    }
    
    logger("localsdk-init", "all_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

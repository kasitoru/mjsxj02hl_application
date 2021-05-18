#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "./speaker.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"

// Init speaker
bool speaker_init() {
    logger("speaker", "speaker_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(local_sdk_speaker_init() == LOCALSDK_OK) {
        logger("speaker", "speaker_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_init()");
        LOCALSDK_SPEAKER_OPTIONS speaker_options = {
            .sample_rate = LOCALSDK_SPEAKER_SAMPLE_RATE,
            .bit_depth   = LOCALSDK_SPEAKER_BIT_DEPTH,
            .unknown_2   = 25, // FIXME: what is it?
            .track_type  = 1,
            .unknown_4   = 30, // FIXME: what is it?
            .volume      = APP_CFG.speaker.volume,
            .buffer_size = LOCALSDK_SPEAKER_BUFFER_SIZE,
            .unknown_7   = 1, // FIXME: what is it?
        };
        if(local_sdk_speaker_set_parameters(&speaker_options) == LOCALSDK_OK) {
            logger("speaker", "speaker_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_set_parameters()");
            if(local_sdk_speaker_start() == LOCALSDK_OK) {
                logger("speaker", "speaker_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_start()");
                
                logger("speaker", "speaker_init", LOGGER_LEVEL_DEBUG, "Function completed.");
                return true;
            } else logger("speaker", "speaker_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_start()");
        } else logger("speaker", "speaker_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_set_parameters()");
    } else logger("speaker", "speaker_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_init()");
    if(speaker_free()) {
        logger("speaker", "speaker_init", LOGGER_LEVEL_INFO, "%s success.", "speaker_free()");
    } else logger("speaker", "speaker_init", LOGGER_LEVEL_WARNING, "%s error!", "speaker_free()");
    logger("speaker", "speaker_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return false;
}

// Free speaker
bool speaker_free() {
    logger("speaker", "speaker_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("speaker", "speaker_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return true;
}

// Play media
bool speaker_play_media(char *filename) {
    bool result = true;
    logger("speaker", "speaker_play_media", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Feed file data
    FILE *media = fopen(filename, "rb");
    if(media != NULL) {
        int error_counter = 0;
        logger("speaker", "speaker_play_media", LOGGER_LEVEL_INFO, "%s success.", "fopen()");
        #if LOCALSDK_SPEAKER_TRACK_TYPE == LOCALSDK_PCM_TRACK_TYPE
            fseek(media, 44, SEEK_SET); // Skip head
        #endif
        if(local_sdk_speaker_clean_buf_data() == LOCALSDK_OK) {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_clean_buf_data()");
        } else {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_speaker_clean_buf_data()");
            result = false;
        }
        while(!feof(media)) {
            error_counter = 0;
            char *buffer = (char *) malloc(LOCALSDK_SPEAKER_BUFFER_SIZE);
            size_t length = fread(buffer, 1, LOCALSDK_SPEAKER_BUFFER_SIZE, media);
            while(true) {
                #if LOCALSDK_SPEAKER_TRACK_TYPE == LOCALSDK_PCM_TRACK_TYPE
                    if(local_sdk_speaker_feed_pcm_data(buffer, length) == LOCALSDK_OK || (error_counter >= 300)) break;
                #elif LOCALSDK_SPEAKER_TRACK_TYPE == LOCALSDK_G711_TRACK_TYPE
                    if(local_sdk_speaker_feed_g711_data(buffer, length) == LOCALSDK_OK || (error_counter >= 300)) break;
                #endif
                usleep(98000);
                error_counter++;
            }
            free(buffer);
        }
        if(error_counter >= 300) {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_WARNING, "%s error!", "error_counter (>= 300)");
            result = false;
        }
        if(local_sdk_speaker_finish_buf_data() == LOCALSDK_OK) {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_finish_buf_data()");
        } else {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_speaker_finish_buf_data()");
            result = false;
        }
        fclose(media);
    } else {
        logger("speaker", "speaker_play_media", LOGGER_LEVEL_ERROR, "%s error!", "fopen()");
        result = false;
    }

    logger("speaker", "speaker_play_media", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Set volume
bool speaker_set_volume(int value) {
    bool result = true;
    logger("speaker", "speaker_set_volume", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    if(local_sdk_speaker_set_volume(value) == LOCALSDK_OK) {
        logger("speaker", "speaker_set_volume", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_set_volume()");
        APP_CFG.speaker.volume = value;
    } else {
        logger("speaker", "speaker_set_volume", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_speaker_set_volume()");
        result = false;
    }
    
    logger("speaker", "speaker_set_volume", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Get volume
int speaker_get_volume() {
    logger("speaker", "speaker_get_volume", LOGGER_LEVEL_DEBUG, "Function is called...");
    int value = APP_CFG.speaker.volume;
    logger("speaker", "speaker_get_volume", LOGGER_LEVEL_DEBUG, "Function completed.");
    return value;
}

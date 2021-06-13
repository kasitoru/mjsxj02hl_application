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

int playback_status = SPEAKER_MEDIA_STOPPED;

// Init speaker
bool speaker_init() {
    logger("speaker", "speaker_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(local_sdk_speaker_init() == LOCALSDK_OK) {
        logger("speaker", "speaker_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_init()");
        LOCALSDK_SPEAKER_OPTIONS speaker_options = {
            .sample_rate = LOCALSDK_SPEAKER_SAMPLE_RATE,
            .bit_depth   = LOCALSDK_SPEAKER_BIT_DEPTH,
            .unknown_2   = 25, // FIXME: what is it?
            .track_type  = LOCALSDK_SPEAKER_TRACK_TYPE,
            .unknown_4   = 30, // FIXME: what is it?
            .volume      = APP_CFG.speaker.volume,
            .buffer_size = LOCALSDK_AUDIO_PCM_BUFFER_SIZE,
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

// Play media (WAV, 8000 hz, 16-bit, mono)
bool speaker_play_media(char *filename, int type) {
    bool result = true;
    logger("speaker", "speaker_play_media", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("speaker", "speaker_play_media", LOGGER_LEVEL_DEBUG, "Filename: %s", filename);
    logger("speaker", "speaker_play_media", LOGGER_LEVEL_DEBUG, "Type: %d", type);
    
    // Stop current playing
    if(speaker_status_media() != SPEAKER_MEDIA_STOPPED) {
        if(speaker_stop_media()) {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_INFO, "%s success.", "speaker_stop_media()");
            while(speaker_status_media() != SPEAKER_MEDIA_STOPPED) { usleep(98000); }
        }
    }
    playback_status = SPEAKER_MEDIA_PLAYING;
    
    // Feed file data
    FILE *media = fopen(filename, "rb");
    if(media != NULL) {
        int error_counter = 0;
        logger("speaker", "speaker_play_media", LOGGER_LEVEL_INFO, "%s success.", "fopen()");
        if(type != LOCALSDK_SPEAKER_G711_TYPE) {
            fseek(media, 44, SEEK_SET); // Skip head
        }
        if(local_sdk_speaker_clean_buf_data() == LOCALSDK_OK) {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_speaker_clean_buf_data()");
        } else {
            logger("speaker", "speaker_play_media", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_speaker_clean_buf_data()");
            result = false;
        }
        int buffer_size = LOCALSDK_AUDIO_PCM_BUFFER_SIZE;
        if(type == LOCALSDK_SPEAKER_G711_TYPE) { buffer_size = LOCALSDK_AUDIO_G711_BUFFER_SIZE; }
        while(!feof(media)) {
            error_counter = 0;
            char *buffer = (char *) malloc(buffer_size);
            size_t length = fread(buffer, 1, buffer_size, media);
            while(true) {
                if(type == LOCALSDK_SPEAKER_G711_TYPE) {
                    if(local_sdk_speaker_feed_g711_data(buffer, length) == LOCALSDK_OK || (error_counter >= 300)) break;
                } else {
                    if(local_sdk_speaker_feed_pcm_data(buffer, length) == LOCALSDK_OK || (error_counter >= 300)) break;
                }
                usleep(98000);
                error_counter++;
            }
            free(buffer);
            if(speaker_status_media() == SPEAKER_MEDIA_STOPPED) playback_status = SPEAKER_MEDIA_STOPPING;
            if(speaker_status_media() == SPEAKER_MEDIA_STOPPING) break;
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
    playback_status = SPEAKER_MEDIA_STOPPED;
    return result;
}

// Get playback status
int speaker_status_media() {
    return playback_status;
}

// Stop playback
bool speaker_stop_media() {
    bool result = false;
    logger("speaker", "speaker_stop_media", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(speaker_status_media() != SPEAKER_MEDIA_STOPPED) {
        playback_status = SPEAKER_MEDIA_STOPPING;
        result = true;
    }
    logger("speaker", "speaker_stop_media", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Set volume
bool speaker_set_volume(int value) {
    bool result = true;
    logger("speaker", "speaker_set_volume", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("speaker", "speaker_set_volume", LOGGER_LEVEL_DEBUG, "Volume: %d", value);
    
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

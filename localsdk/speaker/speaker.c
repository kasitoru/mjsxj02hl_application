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
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(result &= (local_sdk_speaker_init() == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_speaker_init()");
        
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
        
        if(result &= (local_sdk_speaker_set_parameters(&speaker_options) == LOCALSDK_OK)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_speaker_set_parameters()");
            if(result &= (local_sdk_speaker_start() == LOCALSDK_OK)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_speaker_start()");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_start()");
        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_set_parameters()");
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_init()");
    
    // Free speaker if error occurred
    if(!result) {
        if(speaker_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_free()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "speaker_free()");
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free speaker
bool speaker_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    LOGGER(LOGGER_LEVEL_DEBUG, "This function is a stub.");
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", "true");
    return true;
}

// Play media (WAV, 8000 hz, 16-bit, mono)
bool speaker_play_media(char *filename, int type) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    LOGGER(LOGGER_LEVEL_INFO, "Filename: %s", filename);
    LOGGER(LOGGER_LEVEL_INFO, "Type: %d", type);
    
    // Stop current playing
    if(speaker_status_media() != SPEAKER_MEDIA_STOPPED) {
        if(speaker_stop_media()) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_stop_media()");
            while(speaker_status_media() != SPEAKER_MEDIA_STOPPED) { usleep(100000); }
        }
    }
    playback_status = SPEAKER_MEDIA_PLAYING;
    
    // Feed file data
    FILE *media = fopen(filename, "rb");
    if(result &= (media != NULL)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "fopen()");
        
        if(type != LOCALSDK_SPEAKER_G711_TYPE) { // Skip head
            fseek(media, 44, SEEK_SET); //-V575
        }
        
        if(result &= (local_sdk_speaker_clean_buf_data() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_speaker_clean_buf_data()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_speaker_clean_buf_data()");
        
        int error_counter = 0;
        int buffer_size = LOCALSDK_AUDIO_PCM_BUFFER_SIZE;
        if(type == LOCALSDK_SPEAKER_G711_TYPE) { buffer_size = LOCALSDK_AUDIO_G711_BUFFER_SIZE; }
        
        while(!feof(media)) {
            error_counter = 0;
            char *buffer = malloc(buffer_size);
            if(buffer != NULL) {
                size_t length = fread(buffer, 1, buffer_size, media);
                while(true) {
                    if(type == LOCALSDK_SPEAKER_G711_TYPE) {
                        if(local_sdk_speaker_feed_g711_data(buffer, length) == LOCALSDK_OK || (error_counter >= 300)) break;
                    } else {
                        if(local_sdk_speaker_feed_pcm_data(buffer, length) == LOCALSDK_OK || (error_counter >= 300)) break;
                    }
                    usleep(100000);
                    error_counter++;
                    if(speaker_status_media() == SPEAKER_MEDIA_STOPPED) playback_status = SPEAKER_MEDIA_STOPPING;
                    if(speaker_status_media() == SPEAKER_MEDIA_STOPPING) break;
                }
                free(buffer);
            } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "malloc(buffer_size)");
            if(speaker_status_media() == SPEAKER_MEDIA_STOPPED) playback_status = SPEAKER_MEDIA_STOPPING;
            if(speaker_status_media() == SPEAKER_MEDIA_STOPPING) break;
        }
        if(error_counter >= 300) {
            LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "error_counter (>= 300)");
            result &= false;
        }
        if(result &= (local_sdk_speaker_finish_buf_data() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_speaker_finish_buf_data()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_speaker_finish_buf_data()");
        fclose(media);
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "fopen()");
    
    playback_status = SPEAKER_MEDIA_STOPPED;
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Get playback status
int speaker_status_media() {
    return playback_status;
}

// Stop playback
bool speaker_stop_media() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(result &= (speaker_status_media() != SPEAKER_MEDIA_STOPPED)) {
        playback_status = SPEAKER_MEDIA_STOPPING;
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Set volume
bool speaker_set_volume(int value) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    LOGGER(LOGGER_LEVEL_INFO, "Volume: %d", value);
    if(result &= (local_sdk_speaker_set_volume(value) == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_speaker_set_volume()");
        APP_CFG.speaker.volume = value;
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_speaker_set_volume()");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Get volume
int speaker_get_volume() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    int value = APP_CFG.speaker.volume;
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (volume = %d).", value);
    return value;
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./init.h"
#include "./localsdk.h"
#include "./osd/osd.h"
#include "./video/video.h"
#include "./audio/audio.h"
#include "./speaker/speaker.h"
#include "./alarm/alarm.h"
#include "./night/night.h"
#include "./../logger/logger.h"
#include "./../configs/configs.h"
#include "./../ipctool/src/tools.h"

// Read file into string
static char *get_file_contents(char *filename) {
    char *file_contents = "";
    FILE *file_stream;
    if(file_stream = fopen(filename, "r")) {
        if(fseek(file_stream, 0, SEEK_END) != -1) {
            long contents_size = ftell(file_stream) + 1;
            if(fseek(file_stream, 0, SEEK_SET) != -1) {
                if(file_contents = malloc(contents_size)) {
                    memset(file_contents, '\0', contents_size);
                    fread(file_contents, 1, contents_size, file_stream);
                    if(ferror(file_stream)) LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "fread()");
                } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "malloc()");
            } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "fseek(SEEK_SET)");
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "fseek(SEEK_END)");
        fclose(file_stream);
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "fopen()");
    return file_contents;
}

// Removes all non-printable characters from string
char *prepare_string(char *string) {
    char *prepared_string = "";
    if(string != NULL) {
        size_t length = strlen(string) + 1;
        if(prepared_string = malloc(length)) {
            memset(prepared_string, '\0', length);
            size_t j = 0;
            for(size_t i = 0; i<length; i++) {
                if(isprint(string[i])) {
                    prepared_string[i-j] = string[i];
                } else j++;
            }
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "malloc()");
    }
    return prepared_string;
}

// Get firmware version
char *firmware_version() {
    char *file_contents = get_file_contents("/usr/app/share/.version");
    char *fw_ver = prepare_string(file_contents);
    free(file_contents);
    return fw_ver;
}

// Get device id
char *device_id() {
    size_t size = 0;
    size_t length = 64;
    char *buffer = malloc(length);
    // https://github.com/OpenIPC/ipctool/blob/5deac9f4f1fd2913b8b34a2d0a0b511e85e4055d/src/hal_hisi.c#L1288-L1300
    for(uint32_t address = 0x12020414; address >= 0x12020400; address -= 4) {
        uint32_t value;
        if(!mem_reg(address, &value, OP_READ)) break;
        size += snprintf(buffer + size, length - size, "%08x", value);
    }
    return buffer;
}

// Log printf function
static int logprintf(const char *format, ...) {
    int result = 0;
    va_list params;
    va_start(params, format);
    char *message = "";
    if(vasprintf(&message, format, params) != -1) {
        result = LOGGER(LOGGER_LEVEL_DEBUG, message);
        free(message);
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "vasprintf(message)");
    va_end(params);
    return result;
}

// Init all
bool all_init() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(result &= (localsdk_set_logprintf_func(logprintf) == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "localsdk_set_logprintf_func()");
        //if(result &= (localsdk_set_shellcall_func(shellcall_func) == LOCALSDK_OK)) { // FIXME
            //LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "localsdk_set_shellcall_func()");
            if(result &= (localsdk_init() == LOCALSDK_OK)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "localsdk_init()");
                if(result &= (localsdk_get_version() == LOCALSDK_CURRENT_VERSION)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "video_init()");
                    if(result &= osd_init()) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "localsdk_get_version()");
                        if(result &= video_init()) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "osd_init()");
                            if(result &= audio_init()) {
                                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "audio_init()");
                                if(result &= speaker_init()) {
                                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_init()");
                                    if(result &= alarm_init()) {
                                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "alarm_init()");
                                        if(result &= night_init()) {
                                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "night_init()");
                                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "night_init()");
                                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "alarm_init()");
                                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "speaker_init()");
                            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "audio_init()");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "video_init()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "osd_init()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "localsdk_get_version()");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "localsdk_init()");
        //} else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "localsdk_set_shellcall_func()");
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "localsdk_set_logprintf_func()");
    
    // Free all if error occurred
    if(!result) {
        if(all_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "all_free()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "all_free()");
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free all
bool all_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    // Free night mode
    if(result &= night_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "night_free()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    // Free alarm
    if(result &= alarm_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "alarm_free()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    // Free speaker
    if(result &= speaker_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_free()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    // Free audio
    if(result &= audio_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "audio_free()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    // Free OSD
    if(result &= osd_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "osd_free()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    // Free video
    if(result &= video_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "video_free()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    // Free localsdk
    if(result &= (localsdk_destory() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "localsdk_destory()");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "localsdk_get_version()");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

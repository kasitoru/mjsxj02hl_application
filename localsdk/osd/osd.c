#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "./osd.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"

static pthread_t datetime_thread;

// One-second timer for pthread
static void *osd_datetime_timer(void *args) {
    while(true) {
        time_t current_time = time(NULL);
        struct tm *timestamp = localtime(&current_time);
        if(local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true, timestamp) != LOCALSDK_OK) {
            LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_timestamp(true)");
        }
        sleep(1);
        pthread_testcancel();
    }
    
    if(local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) != LOCALSDK_OK) {
        LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_timestamp(false)");
    }
}

// Is enabled
bool osd_is_enabled() {
    return APP_CFG.osd.enable;
}

// Init OSD
bool osd_init() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(osd_is_enabled()) {
        // Init primary channel (not work for secondary channel)
        LOCALSDK_OSD_OPTIONS osd_primary_options = {
            .unknown = 67, // FIXME: what is it?
            .datetime_x = APP_CFG.osd.datetime_x,
            .datetime_y = APP_CFG.osd.datetime_y,
            .datetime_reduce = ((APP_CFG.osd.datetime_size < 0) ? abs(APP_CFG.osd.datetime_size)+1 : 1),
            .datetime_increase = ((APP_CFG.osd.datetime_size > 0) ? APP_CFG.osd.datetime_size+1 : 1),
            .oemlogo_x = APP_CFG.osd.oemlogo_x,
            .oemlogo_y = APP_CFG.osd.oemlogo_y,
            .oemlogo_reduce = ((APP_CFG.osd.oemlogo_size < 0) ? abs(APP_CFG.osd.oemlogo_size)+1 : 1),
            .oemlogo_increase = ((APP_CFG.osd.oemlogo_size > 0) ? APP_CFG.osd.oemlogo_size+1 : 1),
        };
        if(result &= (local_sdk_video_osd_set_parameters(LOCALSDK_VIDEO_PRIMARY_CHANNEL, &osd_primary_options) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_osd_set_parameters()");
        else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_osd_set_parameters()");
        
        // Free OSD if error occurred
        if(!result) {
            if(result &= osd_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "osd_free()");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "osd_free()");
        }
    } else LOGGER(LOGGER_LEVEL_INFO, "OSD is disabled in the settings.");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Init OSD after video init
bool osd_postinit() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(osd_is_enabled()) {
        // Display OEM logo (MI)
        if(APP_CFG.osd.oemlogo) {
            if(result &= (local_sdk_video_osd_update_logo(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_osd_update_logo(true)");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_osd_update_logo(true)");
        }
        
        // Display date and time
        if(APP_CFG.osd.datetime) {
            if(result &= (pthread_create(&datetime_thread, NULL, osd_datetime_timer, NULL) == 0)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_create(datetime_thread)");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(datetime_thread)");
        }
        
        // For rectangles see osd_rectangles_callback()
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free OSD
bool osd_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(osd_is_enabled()) {
        // OEM logo
        if(APP_CFG.osd.oemlogo) {
            // Hide
            if(result &= (local_sdk_video_osd_update_logo(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_osd_update_logo(false)");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_logo(false)");
        }
        // Date and time
        if(APP_CFG.osd.datetime) {
            // Stop datetime thread
            if(datetime_thread) {
                if(result &= (pthread_cancel(datetime_thread) == 0)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_cancel(datetime_thread)");
                else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(datetime_thread)");
            }
            // Hide
            if(result &= (local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_osd_update_timestamp(false)");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_timestamp(false)");
        }
        // Rectangle(s)
        if(APP_CFG.osd.motion || APP_CFG.osd.humanoid) {
            // Hide
            if(result &= (local_sdk_video_osd_update_rect_multi(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_osd_update_rect_multi(false)");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_rect_multi(false)");
        }
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Rectangles callback
int osd_rectangles_callback(LOCALSDK_ALARM_EVENT_INFO *eventInfo) {
    if(osd_is_enabled()) {
        if(APP_CFG.osd.motion || APP_CFG.osd.humanoid) {
            LOCALSDK_OSD_RECTANGLES rectangles;
            rectangles.count = 0;
            for(int i=0;i<LOCALSDK_ALARM_MAXIMUM_OBJECTS;i++) {
                if((APP_CFG.osd.motion && eventInfo->objects[i].type == LOCALSDK_ALARM_TYPE_MOTION)
                  ||
                  (APP_CFG.osd.humanoid && eventInfo->objects[i].type == LOCALSDK_ALARM_TYPE_HUMANOID)
                ) {
                    if(eventInfo->objects[i].state) {
                        rectangles.count++;
                        rectangles.objects[rectangles.count-1].x = eventInfo->objects[i].x;
                        rectangles.objects[rectangles.count-1].y = eventInfo->objects[i].y;
                        rectangles.objects[rectangles.count-1].width = eventInfo->objects[i].width;
                        rectangles.objects[rectangles.count-1].height = eventInfo->objects[i].height;
                        rectangles.objects[rectangles.count-1].unknown = 1;
                        if(eventInfo->objects[i].type == LOCALSDK_ALARM_TYPE_HUMANOID) {
                            rectangles.objects[rectangles.count-1].color = LOCALSDK_OSD_COLOR_ORANGE;
                        } else {
                            rectangles.objects[rectangles.count-1].color = LOCALSDK_OSD_COLOR_GREEN;
                        }
                    }
                }
            }
            if(local_sdk_video_osd_update_rect_multi(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true, &rectangles) != LOCALSDK_OK) {
                LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_rect_multi(true)");
            }
        }
    }
    return LOCALSDK_OK;
}

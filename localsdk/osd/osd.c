#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

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
        local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true, timestamp);
        sleep(1);
        pthread_testcancel();
    }
    local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL);
}

// Init OSD
bool osd_init() {
    bool result = false;
    logger("osd", "osd_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(APP_CFG.osd.enable) {
        // Init primary channel (not work for secondary channel)
        LOCALSDK_OSD_OPTIONS osd_primary_options = {
            .unknown = 67, // FIXME: what is it?
            .datetime_x = APP_CFG.osd.datetime_x,
            .datetime_y = APP_CFG.osd.datetime_y,
            .datetime_reduce = APP_CFG.osd.datetime_reduce,
            .datetime_increase = APP_CFG.osd.datetime_increase,
            .oemlogo_x = APP_CFG.osd.oemlogo_x,
            .oemlogo_y = APP_CFG.osd.oemlogo_y,
            .oemlogo_reduce = APP_CFG.osd.oemlogo_reduce,
            .oemlogo_increase = APP_CFG.osd.oemlogo_increase,
        };
        if(local_sdk_video_osd_set_parameters(LOCALSDK_VIDEO_PRIMARY_CHANNEL, &osd_primary_options) == LOCALSDK_OK) {
            logger("osd", "osd_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_set_parameters()");
            
            result = true;
        } else logger("osd", "osd_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_osd_set_parameters()");
        // Free OSD on error(s)
        if(!result) {
            if(osd_free()) {
                logger("osd", "osd_init", LOGGER_LEVEL_INFO, "%s success.", "osd_free()");
            } else logger("osd", "osd_init", LOGGER_LEVEL_WARNING, "%s error!", "osd_free()");
        }
    } else {
        logger("osd", "osd_init", LOGGER_LEVEL_WARNING, "OSD is disabled in the settings.");
        result = true;
    }
    logger("osd", "osd_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Init OSD after video init
bool osd_postinit() {
    bool result = true;
    logger("osd", "osd_postinit", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(APP_CFG.osd.enable) {
        // Display OEM logo (MI)
        if(APP_CFG.osd.oemlogo) {
            if(local_sdk_video_osd_update_logo(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true) == LOCALSDK_OK) {
                logger("osd", "osd_postinit", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_logo(true)");
            } else {
                logger("osd", "osd_postinit", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_logo(true)");
                result = false;
            }
        }
        // Display date and time
        if(APP_CFG.osd.datetime) {
            if(pthread_create(&datetime_thread, NULL, osd_datetime_timer, NULL) == 0) {
                logger("osd", "osd_postinit", LOGGER_LEVEL_INFO, "%s success.", "pthread_create(datetime_thread)");
            } else {
                logger("osd", "osd_postinit", LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(datetime_thread)");
                result = false;
            }
        }
        // For rectangles see osd_rectangles_callback()
    }
    logger("osd", "osd_postinit", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free OSD
bool osd_free() {
    bool result = true;
    logger("osd", "osd_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(APP_CFG.osd.enable) {
        // OEM logo
        if(APP_CFG.osd.oemlogo) {
            // Hide
            if(local_sdk_video_osd_update_logo(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false) == LOCALSDK_OK) {
                logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_logo(false)");
            } else {
                logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_logo(false)");
                result = false;
            }
        }
        // Date and time
        if(APP_CFG.osd.datetime) {
            // Stop datetime thread
            if(datetime_thread) {
                if(pthread_cancel(datetime_thread) == 0) {
                    logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "pthread_cancel(datetime_thread)");
                } else {
                    logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(datetime_thread)");
                    result = false;
                }
            }
            // Hide
            if(local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) == LOCALSDK_OK) {
                logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_timestamp()");
            } else {
                logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_timestamp()");
                result = false;
            }
        }
        // Rectangle(s)
        if(APP_CFG.osd.motion || APP_CFG.osd.humanoid) {
            // Hide
            if(local_sdk_video_osd_update_rect_multi(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) == LOCALSDK_OK) {
                logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_rect_multi()");
            } else {
                logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_rect_multi()");
                result = false;
            }
        }
    }
    logger("osd", "osd_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Rectangles callback
int osd_rectangles_callback(LOCALSDK_ALARM_EVENT_INFO *eventInfo) {
    if(APP_CFG.osd.enable) {
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
            local_sdk_video_osd_update_rect_multi(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true, &rectangles);
        }
    }
    return LOCALSDK_OK;
}

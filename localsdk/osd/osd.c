#include "./osd.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"

// Init OSD
bool osd_init() {
    bool result = false;
    logger("osd", "osd_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    // Init primary channel (not work for secondary channel)
    LOCALSDK_OSD_OPTIONS osd_primary_options = {
        .unknown_0 = 67, // FIXME: what is it?
        .unknown_1 = 48, // FIXME: what is it?
        .unknown_2 = 0, // FIXME: what is it?
        .unknown_3 = 1, // FIXME: what is it?
        .unknown_4 = 1, // FIXME: what is it?
        .unknown_5 = 2, // FIXME: what is it?
        .unknown_6 = 0, // FIXME: what is it?
        .unknown_7 = 1, // FIXME: what is it?
        .unknown_8 = 1, // FIXME: what is it?
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
    logger("osd", "osd_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free OSD
bool osd_free() {
    bool result = true;
    logger("osd", "osd_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    // Hide logo
    if(local_sdk_video_osd_update_logo(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false) == LOCALSDK_OK) {
        logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_logo()");
    } else {
        logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_logo()");
        result = false;
    }
    // Hide timestamp
    if(local_sdk_video_osd_update_timestamp(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) == LOCALSDK_OK) {
        logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_timestamp()");
    } else {
        logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_timestamp()");
        result = false;
    }
    // Hide rectangle(s)
    if(local_sdk_video_osd_update_rect_multi(LOCALSDK_VIDEO_PRIMARY_CHANNEL, false, NULL) == LOCALSDK_OK) {
        logger("osd", "osd_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_osd_update_rect_multi()");
    } else {
        logger("osd", "osd_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_osd_update_rect_multi()");
        result = false;
    }
    logger("osd", "osd_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

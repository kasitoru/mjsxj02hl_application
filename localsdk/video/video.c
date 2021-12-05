#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "./video.h"
#include "./../osd/osd.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"
#include "./../../rtsp/rtsp.h"

// Video capture callback
static int h26x_capture_callback(int chn, LOCALSDK_H26X_FRAME_INFO *frameInfo) {
    int result = LOCALSDK_OK;
    if(frameInfo && frameInfo->size) {
        // RTSP
        if(rtsp_is_enabled(chn)) {
            if(!rtsp_media_frame(chn, frameInfo->data, frameInfo->size, frameInfo->timestamp, frameInfo->type)) {
                //LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "rtsp_media_frame()");
                result = LOCALSDK_ERROR;
            }
        }
    }
    return result;
}

static int h26x_capture_primary_channel(LOCALSDK_H26X_FRAME_INFO *frameInfo) {
    int result = LOCALSDK_OK;
    if(h26x_capture_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL, frameInfo) != LOCALSDK_OK) {
        //LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "h26x_capture_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
        result = LOCALSDK_ERROR;
    }
    return result;
}

static int h26x_capture_secondary_channel(LOCALSDK_H26X_FRAME_INFO *frameInfo) {
    int result = LOCALSDK_OK;
    if(h26x_capture_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL, frameInfo) != LOCALSDK_OK) {
        //LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "h26x_capture_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
        result = LOCALSDK_ERROR;
    }
    return result;
}

// Init video
bool video_init() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(result &= (local_sdk_video_init(LOCALSDK_VIDEO_FRAMERATE) == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_init()");
        // Init channel 0
        int primary_resolution_type;
        LOCALSDK_PICTURE_SIZE primary_picture_size;
        if(result &= (inner_change_resulu_type(LOCALSDK_VIDEO_RESOLUTION_1920x1080, &primary_resolution_type) == LOCALSDK_OK)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "inner_change_resulu_type(LOCALSDK_VIDEO_RESOLUTION_1920x1080)");
            if(result &= (SAMPLE_COMM_SYS_GetPicSize(primary_resolution_type, &primary_picture_size) == LOCALSDK_OK)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "SAMPLE_COMM_SYS_GetPicSize(LOCALSDK_VIDEO_RESOLUTION_1920x1080)");
                LOCALSDK_VIDEO_OPTIONS primary_options = {
                    .bitrate     = APP_CFG.video.primary_bitrate,
                    .fps         = LOCALSDK_VIDEO_FRAMERATE,
                    .resolution  = LOCALSDK_VIDEO_RESOLUTION_1920x1080,
                    .flip        = APP_CFG.video.flip,
                    .mirror      = APP_CFG.video.mirror,
                    .unknown_5   = 0, // FIXME: what is it?
                    .video       = true,
                    .osd         = osd_is_enabled(),
                    .payload     = APP_CFG.video.primary_type,
                    .rcmode      = APP_CFG.video.primary_rcmode,
                    .gop         = APP_CFG.video.gop * LOCALSDK_VIDEO_FRAMERATE,
                    .screen_size = primary_picture_size.width * primary_picture_size.height,
                    .frame_size  = LOCALSDK_VIDEO_PRIMARY_FRAMESIZE,
                    .jpeg        = false,
                    .unknown_14  = 0, // FIXME: what is it?
                };
                if(result &= (local_sdk_video_create(LOCALSDK_VIDEO_PRIMARY_CHANNEL, &primary_options) == LOCALSDK_OK)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_create(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                    if(result &= (local_sdk_video_set_parameters(LOCALSDK_VIDEO_PRIMARY_CHANNEL, &primary_options) == LOCALSDK_OK)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_parameters(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                        if(result &= (local_sdk_video_set_encode_frame_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL, h26x_capture_primary_channel) == LOCALSDK_OK)) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_encode_frame_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                            if(result &= (local_sdk_video_start(LOCALSDK_VIDEO_PRIMARY_CHANNEL) == LOCALSDK_OK)) {
                                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_start(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                                if(result &= (local_sdk_video_run(LOCALSDK_VIDEO_PRIMARY_CHANNEL) == LOCALSDK_OK)) {
                                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_run(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                                    if(result &= osd_postinit()) {
                                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "osd_postinit()");
                                        // Init channel 1
                                        int secondary_resolution_type;
                                        LOCALSDK_PICTURE_SIZE secondary_picture_size;
                                        if(result &= (inner_change_resulu_type(LOCALSDK_VIDEO_RESOLUTION_640x360, &secondary_resolution_type) == LOCALSDK_OK)) {
                                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "inner_change_resulu_type(LOCALSDK_VIDEO_RESOLUTION_640x360)");
                                            if(result &= (SAMPLE_COMM_SYS_GetPicSize(secondary_resolution_type, &secondary_picture_size) == LOCALSDK_OK)) {
                                                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "SAMPLE_COMM_SYS_GetPicSize(LOCALSDK_VIDEO_RESOLUTION_640x360)");
                                                LOCALSDK_VIDEO_OPTIONS secondary_options = {
                                                    .bitrate     = APP_CFG.video.secondary_bitrate,
                                                    .fps         = LOCALSDK_VIDEO_FRAMERATE,
                                                    .resolution  = LOCALSDK_VIDEO_RESOLUTION_640x360,
                                                    .flip        = APP_CFG.video.flip,
                                                    .mirror      = APP_CFG.video.mirror,
                                                    .unknown_5   = 1, // FIXME: what is it?
                                                    .video       = true,
                                                    .osd         = false, // Not work for secondary channel
                                                    .payload     = APP_CFG.video.secondary_type,
                                                    .rcmode      = APP_CFG.video.secondary_rcmode,
                                                    .gop         = APP_CFG.video.gop * LOCALSDK_VIDEO_FRAMERATE,
                                                    .screen_size = secondary_picture_size.width * secondary_picture_size.height,
                                                    .frame_size  = LOCALSDK_VIDEO_SECONDARY_FRAMESIZE,
                                                    .jpeg        = true,
                                                    .unknown_14  = 1, // FIXME: what is it?
                                                };
                                                if(result &= (local_sdk_video_create(LOCALSDK_VIDEO_SECONDARY_CHANNEL, &secondary_options) == LOCALSDK_OK)) {
                                                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_create(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                    if(result &= (local_sdk_video_set_parameters(LOCALSDK_VIDEO_SECONDARY_CHANNEL, &secondary_options) == LOCALSDK_OK)) {
                                                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_parameters(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                        if(result &= (local_sdk_video_set_encode_frame_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL, h26x_capture_secondary_channel) == LOCALSDK_OK)) {
                                                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_encode_frame_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                            if(result &= (local_sdk_video_set_algo_module_register_callback(local_sdk_alarm_algo_module_register_callback) == LOCALSDK_OK)) {
                                                                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_algo_module_register_callback()");
                                                                if(result &= (local_sdk_video_start(LOCALSDK_VIDEO_SECONDARY_CHANNEL) == LOCALSDK_OK)) {
                                                                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_start(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                                    if(result &= (local_sdk_video_run(LOCALSDK_VIDEO_SECONDARY_CHANNEL) == LOCALSDK_OK)) {
                                                                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_run(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_run(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_start(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_algo_module_register_callback()");
                                                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_encode_frame_callback(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_parameters(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_create(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
                                            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "SAMPLE_COMM_SYS_GetPicSize(LOCALSDK_VIDEO_RESOLUTION_640x360)");
                                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "inner_change_resulu_type(LOCALSDK_VIDEO_RESOLUTION_640x360)");
                                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "osd_postinit()");
                                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_run(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_start(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_encode_frame_callback(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_parameters(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_create(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "SAMPLE_COMM_SYS_GetPicSize(LOCALSDK_VIDEO_RESOLUTION_1920x1080)");
        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "inner_change_resulu_type(LOCALSDK_VIDEO_RESOLUTION_1920x1080)");
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_init()");
    
    // Free video if error occurred
    if(!result) {
        if(video_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "video_free()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "video_free()");
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free video
bool video_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    // Stop secondary video
    if(result &= (local_sdk_video_stop(LOCALSDK_VIDEO_SECONDARY_CHANNEL, true) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_stop(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_stop(LOCALSDK_VIDEO_SECONDARY_CHANNEL)");
    
    // Stop primary video
    if(result &= (local_sdk_video_stop(LOCALSDK_VIDEO_PRIMARY_CHANNEL, true) == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_stop(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_stop(LOCALSDK_VIDEO_PRIMARY_CHANNEL)");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

#include <stdlib.h>
#include <string.h>

#include "./configs.h"
#include "./inih/ini.h"
#include "./../localsdk/localsdk.h"
#include "./../logger/logger.h"

// Default values
APPLICATION_CONFIGURATION APP_CFG = {
    // [general]
    .general.name                 = "My Camera",                            // Device name
    .general.led                  = true,                                   // Enable onboard LED indicator

    // [logger]
    .logger.level                 = LOGGER_LEVEL_WARNING,                   // Log level
    .logger.file                  = "",                                     // Write log to file
    
    // [osd]
    .osd.enable                   = false,                                  // Enable On-Screen Display (OSD)
    .osd.oemlogo                  = true,                                   // Display OEM logo (MI)
    .osd.oemlogo_x                = 2,                                      // X position of the OEM logo
    .osd.oemlogo_y                = 0,                                      // Y position of the OEM logo
    .osd.oemlogo_size             = 0,                                      // Size of the OEM logo (can take negative values)
    .osd.datetime                 = true,                                   // Display date and time
    .osd.datetime_x               = 48,                                     // X position of the date and time
    .osd.datetime_y               = 0,                                      // Y position of the date and time
    .osd.datetime_size            = 0,                                      // Size of the date and time (can take negative values)
    .osd.motion                   = false,                                  // Display detected motions in rectangles
    .osd.humanoid                 = false,                                  // Display detected humanoids in rectangles
    
    // [video]
    .video.gop                    = 1,                                      // Group of pictures (GOP) every N*FPS (20)
    .video.flip                   = false,                                  // Flip image (all channels)
    .video.mirror                 = false,                                  // Mirror image (all channels)
    .video.primary_type           = LOCALSDK_VIDEO_PAYLOAD_H264,            // Video compression standard for primary channel
    .video.secondary_type         = LOCALSDK_VIDEO_PAYLOAD_H264,            // Video compression standard for secondary channel
    .video.primary_bitrate        = 1800,                                   // Bitrate for primary channel
    .video.secondary_bitrate      = 900,                                    // Bitrate for secondary channel
    .video.primary_rcmode         = LOCALSDK_VIDEO_RCMODE_VARIABLE_BITRATE, // Rate control mode for primary channel
    .video.secondary_rcmode       = LOCALSDK_VIDEO_RCMODE_VARIABLE_BITRATE, // Rate control mode for secondary channel

    // [audio]
    .audio.volume                 = 70,                                     // Volume (0-100)
    .audio.primary_enable         = true,                                   // Enable audio for primary channel
    .audio.secondary_enable       = true,                                   // Enable audio for secondary channel
    
    // [speaker]
    .speaker.volume               = 70,                                     // Volume (0-100)
    .speaker.type                 = LOCALSDK_SPEAKER_PCM_TYPE,              // Default file format
    
    // [alarm]
    .alarm.motion_sens            = 150,                                    // Motion sensitivity (1-255)
    .alarm.humanoid_sens          = 150,                                    // Humanoid sensitivity (1-255)
    .alarm.motion_timeout         = 60,                                     // Motion timeout (in seconds)
    .alarm.humanoid_timeout       = 60,                                     // Humanoid timeout (in seconds)
    .alarm.motion_detect_exec     = "",                                     // Execute the command when motion is detected
    .alarm.humanoid_detect_exec   = "",                                     // Execute the command when humanoid is detected
    .alarm.motion_lost_exec       = "",                                     // Execute the command when motion is lost
    .alarm.humanoid_lost_exec     = "",                                     // Execute the command when humanoid is lost
    
    // [rtsp]
    .rtsp.enable                  = true,                                   // Enable RTSP server
    .rtsp.port                    = 554,                                    // Port number
    .rtsp.username                = "",                                     // Username (empty for disable)
    .rtsp.password                = "",                                     // Password
    .rtsp.primary_name            = "primary",                              // Name of the primary channel
    .rtsp.secondary_name          = "secondary",                            // Name of the secondary channel
    .rtsp.primary_multicast       = false,                                  // Use multicast for primary channel
    .rtsp.secondary_multicast     = false,                                  // Use multicast for secondary channel
    .rtsp.primary_split_vframes   = true,                                   // Split video frames into separate packets for primary channel
    .rtsp.secondary_split_vframes = true,                                   // Split video frames into separate packets for secondary channel
    
    // [mqtt]
    .mqtt.enable                  = false,                                  // Enable MQTT client
    .mqtt.server                  = "",                                     // Server address
    .mqtt.port                    = 1883,                                   // Port number
    .mqtt.username                = "",                                     // Username (empty for anonimous)
    .mqtt.password                = "",                                     // Password (empty for disable)
    .mqtt.topic                   = "mjsxj02hl",                            // Name of the root topic
    .mqtt.qos                     = 1,                                      // Quality of Service (0, 1 or 2)
    .mqtt.retain                  = true,                                   // Retained messages
    .mqtt.reconnection_interval   = 60,                                     // Reconnection interval (in seconds)
    .mqtt.periodical_interval     = 60,                                     // Interval of periodic message (in seconds)
    
    // [night]
    .night.mode                   = 2,                                      // Night mode (0 = off, 1 = on, 2 = auto)
    .night.gray                   = 2,                                      // Grayscale (0 = off, 1 = on, 2 = auto)
};

// Handler for ini parser
static int parser_handler(void* cfg, const char *section, const char *name, const char *value) {
    bool result = true;

    APPLICATION_CONFIGURATION* config = (APPLICATION_CONFIGURATION*) cfg;
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    #define atob(v) strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "on") == 0 || strcmp(value, "1") == 0
    
    // [general]
    if(MATCH("general", "name")) {
        config->general.name = strdup(value);
    } else if(MATCH("general", "led")) {
        config->general.led = atob(value);
    
    // [logger]
    } else if(MATCH("logger", "level")) {
        config->logger.level = atoi(value);
    } else if(MATCH("logger", "file")) {
        config->logger.file = strdup(value);
    
    // [osd]
    } else if(MATCH("osd", "enable")) {
        config->osd.enable = atob(value);
    } else if(MATCH("osd", "oemlogo")) {
        config->osd.oemlogo = atob(value);
    } else if(MATCH("osd", "oemlogo_x")) {
        config->osd.oemlogo_x = atoi(value);
    } else if(MATCH("osd", "oemlogo_y")) {
        config->osd.oemlogo_y = atoi(value);
    } else if(MATCH("osd", "oemlogo_size")) {
        config->osd.oemlogo_size = atoi(value);
    } else if(MATCH("osd", "datetime")) {
        config->osd.datetime = atob(value);
    } else if(MATCH("osd", "datetime_x")) {
        config->osd.datetime_x = atoi(value);
    } else if(MATCH("osd", "datetime_y")) {
        config->osd.datetime_y = atoi(value);
    } else if(MATCH("osd", "datetime_size")) {
        config->osd.datetime_size = atoi(value);
    } else if(MATCH("osd", "motion")) {
        config->osd.motion = atob(value);
    } else if(MATCH("osd", "humanoid")) {
        config->osd.humanoid = atob(value);
    
    // [video]
    } else if(MATCH("video", "gop")) {
        config->video.gop = atoi(value);
    } else if(MATCH("video", "flip")) {
        config->video.flip = atob(value);
    } else if(MATCH("video", "mirror")) {
        config->video.mirror = atob(value);
    } else if(MATCH("video", "primary_type")) {
        config->video.primary_type = atoi(value);
    } else if(MATCH("video", "secondary_type")) {
        config->video.secondary_type = atoi(value);
    } else if(MATCH("video", "primary_bitrate")) {
        config->video.primary_bitrate = atoi(value);
    } else if(MATCH("video", "secondary_bitrate")) {
        config->video.secondary_bitrate = atoi(value);
    } else if(MATCH("video", "primary_rcmode")) {
        config->video.primary_rcmode = atoi(value);
    } else if(MATCH("video", "secondary_rcmode")) {
        config->video.secondary_rcmode = atoi(value);

    // [audio]
    } else if(MATCH("audio", "volume")) {
        config->audio.volume = atoi(value);
    } else if(MATCH("audio", "primary_enable")) {
        config->audio.primary_enable = atob(value);
    } else if(MATCH("audio", "secondary_enable")) {
        config->audio.secondary_enable = atob(value);

    // [speaker]
    } else if(MATCH("speaker", "volume")) {
        config->speaker.volume = atoi(value);
    } else if(MATCH("speaker", "type")) {
        config->speaker.type = atoi(value);

    // [alarm]
    } else if(MATCH("alarm", "motion_sens")) {
        config->alarm.motion_sens = atoi(value);
    } else if(MATCH("alarm", "humanoid_sens")) {
        config->alarm.humanoid_sens = atoi(value);
    } else if(MATCH("alarm", "motion_timeout")) {
        config->alarm.motion_timeout = atoi(value);
    } else if(MATCH("alarm", "humanoid_timeout")) {
        config->alarm.humanoid_timeout = atoi(value);
    } else if(MATCH("alarm", "motion_detect_exec")) {
        config->alarm.motion_detect_exec = strdup(value);
    } else if(MATCH("alarm", "humanoid_detect_exec")) {
        config->alarm.humanoid_detect_exec = strdup(value);
    } else if(MATCH("alarm", "motion_lost_exec")) {
        config->alarm.motion_lost_exec = strdup(value);
    } else if(MATCH("alarm", "humanoid_lost_exec")) {
        config->alarm.humanoid_lost_exec = strdup(value);

    // [rtsp]
    } else if(MATCH("rtsp", "enable")) {
        config->rtsp.enable = atob(value);
    } else if(MATCH("rtsp", "port")) {
        config->rtsp.port = atoi(value);
    } else if(MATCH("rtsp", "username")) {
        config->rtsp.username = strdup(value);
    } else if(MATCH("rtsp", "password")) {
        config->rtsp.password = strdup(value);
    } else if(MATCH("rtsp", "primary_name")) {
        config->rtsp.primary_name = strdup(value);
    } else if(MATCH("rtsp", "secondary_name")) {
        config->rtsp.secondary_name = strdup(value);
    } else if(MATCH("rtsp", "primary_multicast")) {
        config->rtsp.primary_multicast = atob(value);
    } else if(MATCH("rtsp", "secondary_multicast")) {
        config->rtsp.secondary_multicast = atob(value);
    } else if(MATCH("rtsp", "primary_split_vframes")) {
        config->rtsp.primary_split_vframes = atob(value);
    } else if(MATCH("rtsp", "secondary_split_vframes")) {
        config->rtsp.secondary_split_vframes = atob(value);

    // [mqtt]
    } else if(MATCH("mqtt", "enable")) {
        config->mqtt.enable = atob(value);
    } else if(MATCH("mqtt", "server")) {
        config->mqtt.server = strdup(value);
    } else if(MATCH("mqtt", "port")) {
        config->mqtt.port = atoi(value);
    } else if(MATCH("mqtt", "username")) {
        config->mqtt.username = strdup(value);
    } else if(MATCH("mqtt", "password")) {
        config->mqtt.password = strdup(value);
    } else if(MATCH("mqtt", "topic")) {
        config->mqtt.topic = strdup(value);
    } else if(MATCH("mqtt", "qos")) {
        config->mqtt.qos = atoi(value);
    } else if(MATCH("mqtt", "retain")) {
        config->mqtt.retain = atob(value);
    } else if(MATCH("mqtt", "reconnection_interval")) {
        config->mqtt.reconnection_interval = atoi(value);
    } else if(MATCH("mqtt", "periodical_interval")) {
        config->mqtt.periodical_interval = atoi(value);

    // [night]
    } else if(MATCH("night", "mode")) {
        config->night.mode = atoi(value);
    } else if(MATCH("night", "gray")) {
        config->night.gray = atoi(value);

    // unknown
    } else result &= false;

    if(result) LOGGER(LOGGER_LEVEL_INFO, "Parse success. Section: %s, name: %s, value: %s", section, name, value);
    else LOGGER(LOGGER_LEVEL_WARNING, "Parse error! Section: %s, name: %s, value: %s", section, name, value);
    
    return result;
}

// Init application configs
bool configs_init(char *filename) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    // Read values from config file
    LOGGER(LOGGER_LEVEL_INFO, "Filename: %s", filename);
    if(result &= (ini_parse(filename, parser_handler, &APP_CFG) >= 0)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "ini_parse()");
    else {
        LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "ini_parse()");
        if(result &= configs_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "configs_free()");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "configs_free()");
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free application configs
bool configs_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    LOGGER(LOGGER_LEVEL_DEBUG, "This function is a stub.");
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

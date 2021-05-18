#include <stdlib.h>
#include <string.h>

#include "./configs.h"
#include "./inih/ini.h"
#include "./../logger/logger.h"

// Default values
APPLICATION_CONFIGURATION APP_CFG = {
    // [logger]
    .logger.level               = LOGGER_LEVEL_DEBUG, // Log level
    .logger.file                = "",                 // Write log to file
    
    // [video]
    .video.flip                 = false,              // Flip (true or false)
    .video.mirror               = false,              // Mirror (true or false)

    // [audio]
    .audio.volume               = 70,                 // Volume (0-100)
    
    // [speaker]
    .speaker.volume             = 75,                 // Volume (0-100)
    
    // [alarm]
    .alarm.motion_sens          = 150,                // Motion sensitivity (1-255)
    .alarm.humanoid_sens        = 150,                // Humanoid sensitivity (1-255)
    .alarm.motion_timeout       = 60,                 // Motion timeout (in seconds)
    .alarm.humanoid_timeout     = 60,                 // Humanoid timeout (in seconds)
    .alarm.motion_detect_exec   = "",                 // Execute the command when motion is detected
    .alarm.humanoid_detect_exec = "",                 // Execute the command when humanoid is detected
    .alarm.motion_lost_exec     = "",                 // Execute the command when motion is lost
    .alarm.humanoid_lost_exec   = "",                 // Execute the command when humanoid is lost
    
    // [mqtt]
    .mqtt.server                = "",                 // Address (empty for disable)
    .mqtt.port                  = 1883,               // Port
    .mqtt.username              = "",                 // Username (empty for anonimous)
    .mqtt.password              = "",                 // Password (empty for disable)
    .mqtt.topic                 = "mjsxj02hl",        // Topic name
    .mqtt.qos                   = 1,                  // Quality of Service (0, 1 or 2)
    .mqtt.retain                = false,              // Retained messages
    
    // [night]
    .night.mode                 = 2,                  // Night mode (0 = off, 1 = on, 2 = auto)
    .night.gray                 = 2,                  // Grayscale (0 = off, 1 = on, 2 = auto)
};

// Handler for ini parser
static int parser_handler(void* cfg, const char* section, const char* name, const char* value) {
    bool result = true;

    APPLICATION_CONFIGURATION* config = (APPLICATION_CONFIGURATION*) cfg;
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    
    // [logger]
    if(MATCH("logger", "level")) {
        config->logger.level = atoi(value);
    } else if(MATCH("logger", "file")) {
        config->logger.file = strdup(value);
    
    // [video]
    } else if(MATCH("video", "flip")) {
        config->video.flip = (atoi(value) != 0);
    } else if(MATCH("video", "mirror")) {
        config->video.mirror = (atoi(value) != 0);

    // [audio]
    } else if(MATCH("audio", "volume")) {
        config->audio.volume = atoi(value);

    // [speaker]
    } else if(MATCH("speaker", "volume")) {
        config->speaker.volume = atoi(value);

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

    // [mqtt]
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
        config->mqtt.retain = (atoi(value) != 0);

    // [night]
    } else if(MATCH("night", "mode")) {
        config->night.mode = atoi(value);
    } else if(MATCH("night", "gray")) {
        config->night.gray = atoi(value);

    // unknown
    } else result = false;

    if(result) {
        logger("configs", "configs_init", LOGGER_LEVEL_INFO, "%s success. Section: %s, name: %s, value: %s", "parser_handler()", section, name, value);
    } else {
        logger("configs", "configs_init", LOGGER_LEVEL_WARNING, "%s error! Section: %s, name: %s, value: %s", "parser_handler()", section, name, value);
    }
    
    return result;
}

// Init application configs
bool configs_init(char *filename) {
    bool result = true;
    logger("configs", "configs_init", LOGGER_LEVEL_DEBUG, "Function is called...");

    // Read values from config file
    if(ini_parse(filename, parser_handler, &APP_CFG) >= 0) {
        logger("configs", "configs_init", LOGGER_LEVEL_INFO, "%s success.", "ini_parse()");
    } else {
        logger("configs", "configs_init", LOGGER_LEVEL_ERROR, "%s error!", "ini_parse()");
        logger("configs", "configs_init", LOGGER_LEVEL_ERROR, "Configs filename: %s", filename);
        if(configs_free()) {
            logger("configs", "configs_init", LOGGER_LEVEL_INFO, "%s success.", "configs_free()");
        } else logger("configs", "configs_init", LOGGER_LEVEL_WARNING, "%s error!", "configs_free()");
        result = false;
    }
    
    logger("configs", "configs_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free application configs
bool configs_free() {
    logger("configs", "configs_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("configs", "configs_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return true;
}

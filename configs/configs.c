#include <stdlib.h>
#include <string.h>

#include "./configs.h"
#include "./inih/ini.h"
#include "./../logger/logger.h"

// Default values
APPLICATION_CONFIGURATION APP_CFG = {
    // [logger]
    .logger.level   = LOGGER_LEVEL_DEBUG, // Log level
    
    // [video]
    .video.flip     = false,              // Flip (true or false)
    .video.mirror   = false,              // Mirror (true or false)
    
    // [speaker]
    .speaker.volume = 75,                 // Volume (0-100)
    
    // [alarm]
    .alarm.motion   = 150,                // Sensitivity (1-255)
    .alarm.humanoid = 150,                // Sensitivity (1-255)
    
    // [mqtt]
    .mqtt.server    = "",                 // Address (empty for disable)
    .mqtt.port      = 1883,               // Port
    .mqtt.username  = "",                 // Username (empty for anonimous)
    .mqtt.password  = "",                 // Password (empty for disable)
    .mqtt.topic     = "mjsxj02hl",        // Topic name
    .mqtt.qos       = 1,                  // Quality of Service (0, 1 or 2)
    .mqtt.retain    = false,              // Retained messages
};

// Handler for ini parser
static int parser_handler(void* cfg, const char* section, const char* name, const char* value) {
    bool result = true;

    APPLICATION_CONFIGURATION* config = (APPLICATION_CONFIGURATION*) cfg;
    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    // [video]
    if(MATCH("video", "flip")) {
        config->video.flip = (atoi(value) != 0);
    } else if(MATCH("video", "mirror")) {
        config->video.mirror = (atoi(value) != 0);

    // [speaker]
    } else if(MATCH("speaker", "volume")) {
        config->speaker.volume = atoi(value);

    // [alarm]
    } else if(MATCH("alarm", "motion")) {
        config->alarm.motion = atoi(value);
    } else if(MATCH("alarm", "humanoid")) {
        config->alarm.humanoid = atoi(value);

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

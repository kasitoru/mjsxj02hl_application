#include <stdbool.h>
#include <unistd.h>

#include "./night.h"
#include "./../localsdk.h"
#include "./../alarm/alarm.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"
#include "./../../yyjson/src/yyjson.h"
#include "./../../mqtt/mqtt.h"

// MQTT send info
static bool night_state_mqtt(bool night, bool gray) {
    bool result = false;
    logger("night", "night_state_mqtt", LOGGER_LEVEL_DEBUG, "Function is called...");
    // Send night mode info
    char *topic = mqtt_fulltopic(MQTT_NIGHT_TOPIC);
    // JSON Data
    yyjson_mut_doc *json_doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *json_root = yyjson_mut_obj(json_doc);
    yyjson_mut_doc_set_root(json_doc, json_root);
    // Motion state
    yyjson_mut_obj_add_bool(json_doc, json_root, "state", night);
    // Humanoid state
    yyjson_mut_obj_add_bool(json_doc, json_root, "gray", gray);
    // Current timestamp
    int timestamp = (int) time(NULL);
    yyjson_mut_obj_add_int(json_doc, json_root, "timestamp", timestamp);
    // Send it
    const char *json = yyjson_mut_write(json_doc, 0, NULL);
    if(json) {
        logger("night", "night_state_mqtt", LOGGER_LEVEL_INFO, "%s success.", "yyjson_mut_write()");
        if(mqtt_send(topic, (char *) json)) {
            logger("night", "night_state_mqtt", LOGGER_LEVEL_INFO, "%s success.", "mqtt_send()");
            result = true;
        } else logger("night", "night_state_mqtt", LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send()");
        free((void *)json);
    } else logger("night", "night_state_mqtt", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_mut_write()");
    // Free resources
    yyjson_mut_doc_free(json_doc);
    free(topic);
    logger("night", "night_state_mqtt", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Callback for change state of night mode
static int night_mode_change_callback(int state) {
    int result = LOCALSDK_OK;
    logger("night", "night_mode_change_callback", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("night", "night_mode_change_callback", LOGGER_LEVEL_DEBUG, "state = %d", state);
    switch(state) {
        case NIGHT_MODE_STATE_NIGHTTIME: // night
            // Enable grayscale
            if(APP_CFG.night.gray == 2) { // auto
                if(local_sdk_video_set_night_mode() == LOCALSDK_ERROR) {
                    logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_night_mode()");
                    result = LOCALSDK_ERROR;
                } else logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_set_night_mode()");
            }
            // Open IR-cut filter
            if(local_sdk_open_ircut() == LOCALSDK_ERROR) {
                logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                result = LOCALSDK_ERROR;
            } else logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_open_ircut()");
            // MQTT
            if(mqtt_is_ready()) {
                if(night_state_mqtt(true, (APP_CFG.night.gray == 2 ? true : (APP_CFG.night.gray == 1)))) {
                    logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "night_state_mqtt()");
                } else logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "night_state_mqtt()");
            }
            break;
        case NIGHT_MODE_STATE_DAYTIME: // day
            // Close IR-cut filter
            if(local_sdk_close_ircut() == LOCALSDK_ERROR) {
                logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                result = LOCALSDK_ERROR;
            } else logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_close_ircut()");
            // Disable grayscale
            if(APP_CFG.night.gray == 2) { // auto
                if(local_sdk_video_set_daytime_mode() == LOCALSDK_ERROR) {
                    logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_daytime_mode()");
                    result = LOCALSDK_ERROR;
                } else logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_set_daytime_mode()");
            }
            // MQTT
            if(mqtt_is_ready()) {
                if(night_state_mqtt(false, (APP_CFG.night.gray == 2 ? false : (APP_CFG.night.gray == 1)))) {
                    logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "night_state_mqtt()");
                } else logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "night_state_mqtt()");
            }
            break;
        case NIGHT_MODE_STATE_DISABLE: // Disable alarm system
            if(alarm_switch(false)) {
                logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "alarm_switch(false)");
            } else {
                logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "alarm_switch(false)");
                result = LOCALSDK_ERROR;
            }
            break;
        case NIGHT_MODE_STATE_ENABLE: // Enable alarm system
            if(alarm_switch(true)) {
                logger("night", "night_mode_change_callback", LOGGER_LEVEL_INFO, "%s success.", "alarm_switch(true)");
            } else {
                logger("night", "night_mode_change_callback", LOGGER_LEVEL_ERROR, "%s error!", "alarm_switch(true)");
                result = LOCALSDK_ERROR;
            }
            break;
        default: // unknown
            logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "Unknown value of night mode. Switching");
            result = LOCALSDK_ERROR;
            break;
    }
    logger("night", "night_mode_change_callback", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Init night mode
bool night_init() {
    bool result = false;
    logger("night", "night_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(local_sdk_night_state_set_callback(night_mode_change_callback) == LOCALSDK_OK) {
        logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_night_state_set_callback()");
        // Set night mode
        logger("night", "night_init", LOGGER_LEVEL_DEBUG, "night.mode = %d", APP_CFG.night.mode);
        switch(APP_CFG.night.mode) {
            case 0: // off
                if(local_sdk_open_ircut() == LOCALSDK_OK) {
                    logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_open_ircut()");
                    usleep(98000);
                    if(local_sdk_close_ircut() == LOCALSDK_OK) {
                        logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_close_ircut()");
                        if(local_sdk_close_night_light() == LOCALSDK_OK) {
                            logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_close_night_light()");
                            result = true;
                        } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_night_light()");
                    } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                break;
            case 1: // on
                if(local_sdk_close_ircut() == LOCALSDK_OK) {
                    logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_open_ircut()");
                    usleep(98000);
                    if(local_sdk_open_ircut() == LOCALSDK_OK) {
                        logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_open_ircut()");
                        if(local_sdk_open_night_light() == LOCALSDK_OK) {
                            logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_open_night_light()");
                            result = true;
                        } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_night_light()");
                    } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                break;
            case 2: // auto
                if(local_sdk_open_ircut() == LOCALSDK_OK) {
                    logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_open_ircut()");
                    usleep(98000);
                    if(local_sdk_close_ircut() == LOCALSDK_OK) {
                        logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_close_ircut()");
                        if(local_sdk_auto_night_light() == LOCALSDK_OK) {
                            logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_auto_night_light()");
                            result = true;
                        } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_auto_night_light()");
                    } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                break;
            default: // unknown
                logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "Unknown value of night mode. Switching");
                break;
        }
        // Set grayscale
        if(result) {
            logger("night", "night_init", LOGGER_LEVEL_DEBUG, "night.gray = %d", APP_CFG.night.gray);
            switch(APP_CFG.night.gray) {
                case 0: // off
                    if(local_sdk_video_set_daytime_mode() == LOCALSDK_ERROR) {
                        logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_daytime_mode()");
                        result = false;
                    } else logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_set_daytime_mode()");
                    break;
                case 1: // on
                    if(local_sdk_video_set_night_mode() == LOCALSDK_ERROR) {
                        logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_night_mode()");
                        result = false;
                    } else logger("night", "night_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_set_night_mode()");
                    break;
                case 2: // auto
                    break;
                default: // unknown
                    logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "Unknown value of grayscale mode. Switching");
                    result = false;
                    break;
            }
        }
    } else logger("night", "night_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_night_state_set_callback()");
    logger("night", "night_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free night mode
bool night_free() {
    bool result = false;
    logger("night", "night_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    // Disable night mode
    if(local_sdk_close_ircut() == LOCALSDK_OK) {
        logger("night", "night_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_close_ircut()");
        if(local_sdk_close_night_light() == LOCALSDK_OK) {
            logger("night", "night_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_close_night_light()");
            if(local_sdk_video_set_daytime_mode() == LOCALSDK_OK) {
                logger("night", "night_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_set_daytime_mode()");
                result = true;
            } else logger("night", "night_free", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_daytime_mode()");
        } else logger("night", "night_free", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_night_light()");
    } else logger("night", "night_free", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
    logger("night", "night_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

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
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    // Send night mode info
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
    if(result &= !!json) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_write()");
        
        char *night_topic = mqtt_fulltopic(MQTT_NIGHT_TOPIC);
        if(result &= mqtt_send(night_topic, (char *) json)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_send(MQTT_NIGHT_TOPIC)");
        else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send(MQTT_NIGHT_TOPIC)");
        
        free(night_topic);
        free((void *) json);
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_write()");
    
    // Free resources
    yyjson_mut_doc_free(json_doc);
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Callback for change state of night mode
static int night_mode_change_callback(int state) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    int result = LOCALSDK_OK;
    
    LOGGER(LOGGER_LEVEL_INFO, "State: %s", (state ? "true" : "false"));
    
    switch(state) {
        case NIGHT_MODE_STATE_NIGHTTIME: // night
            
            // Enable grayscale
            if(APP_CFG.night.gray == 2) { // auto
                if(local_sdk_video_set_night_mode() == LOCALSDK_ERROR) {
                    LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_night_mode()");
                    result = LOCALSDK_ERROR;
                } else LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_night_mode()");
            }
            
            // Open IR-cut filter
            if(local_sdk_open_ircut() == LOCALSDK_ERROR) {
                LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                result = LOCALSDK_ERROR;
            } else LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_open_ircut()");
            
            // MQTT
            if(mqtt_is_ready()) {
                if(night_state_mqtt(true, (APP_CFG.night.gray == 2 ? true : (APP_CFG.night.gray == 1)))) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "night_state_mqtt()");
                else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "night_state_mqtt()");
            }
            
            break;
        case NIGHT_MODE_STATE_DAYTIME: // day
            
            // Close IR-cut filter
            if(local_sdk_close_ircut() == LOCALSDK_ERROR) {
                LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                result = LOCALSDK_ERROR;
            } else LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_close_ircut()");
            
            // Disable grayscale
            if(APP_CFG.night.gray == 2) { // auto
                if(local_sdk_video_set_daytime_mode() == LOCALSDK_ERROR) {
                    LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_daytime_mode()");
                    result = LOCALSDK_ERROR;
                } else LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_daytime_mode()");
            }
            
            // MQTT
            if(mqtt_is_ready()) {
                if(night_state_mqtt(false, (APP_CFG.night.gray == 2 ? false : (APP_CFG.night.gray == 1)))) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "night_state_mqtt()");
                else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "night_state_mqtt()");
            }
            
            break;
        case NIGHT_MODE_STATE_DISABLE: // Disable alarm system
            if(alarm_switch(false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "alarm_switch(false)");
            } else {
                LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "alarm_switch(false)");
                result = LOCALSDK_ERROR;
            }
            break;
        case NIGHT_MODE_STATE_ENABLE: // Enable alarm system
            if(alarm_switch(true)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "alarm_switch(true)");
            } else {
                LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "alarm_switch(true)");
                result = LOCALSDK_ERROR;
            }
            break;
        default: // unknown
            LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "Unknown value of night mode. Switching");
            result = LOCALSDK_ERROR;
            break;
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result == LOCALSDK_OK ? "LOCALSDK_OK" : "LOCALSDK_ERROR"));
    return result;
}

// Init night mode
bool night_init() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(result &= (local_sdk_night_state_set_callback(night_mode_change_callback) == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_night_state_set_callback()");
        // Set night mode
        switch(APP_CFG.night.mode) {
            case 0: // off
                if(result &= (local_sdk_open_ircut() == LOCALSDK_OK)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_open_ircut()");
                    usleep(100000);
                    if(result &= (local_sdk_close_ircut() == LOCALSDK_OK)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_close_ircut()");
                        if(result &= (local_sdk_close_night_light() == LOCALSDK_OK)) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_close_night_light()");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_night_light()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                break;
            case 1: // on
                if(result &= (local_sdk_close_ircut() == LOCALSDK_OK)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_open_ircut()");
                    usleep(100000);
                    if(result &= (local_sdk_open_ircut() == LOCALSDK_OK)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_open_ircut()");
                        if(result &= (local_sdk_open_night_light() == LOCALSDK_OK)) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_open_night_light()");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_night_light()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                break;
            case 2: // auto
                if(result &= (local_sdk_open_ircut() == LOCALSDK_OK)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_open_ircut()");
                    usleep(100000);
                    if(result &= (local_sdk_close_ircut() == LOCALSDK_OK)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_close_ircut()");
                        if(result &= (local_sdk_auto_night_light() == LOCALSDK_OK)) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_auto_night_light()");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_auto_night_light()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_close_ircut()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_open_ircut()");
                break;
            default: // unknown
                LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "Unknown value of night mode. Switching");
                break;
        }
        
        // Set grayscale
        if(result) {
            switch(APP_CFG.night.gray) {
                case 0: // off
                    if(result &= (local_sdk_video_set_daytime_mode() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_daytime_mode()");
                    else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_daytime_mode()");
                    break;
                case 1: // on
                    if(result &= (local_sdk_video_set_night_mode() == LOCALSDK_OK)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_night_mode()");
                    else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_set_night_mode()");
                    break;
                case 2: // auto
                    break;
                default: // unknown
                    LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "Unknown value of grayscale mode. Switching");
                    break;
            }
        }
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_night_state_set_callback()");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Free night mode
bool night_free() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    // Disable night mode
    if(result &= (local_sdk_close_ircut() == LOCALSDK_OK)) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_close_ircut()");
        if(result &= (local_sdk_close_night_light() == LOCALSDK_OK)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_close_night_light()");
            if(result &= (local_sdk_video_set_daytime_mode() == LOCALSDK_OK)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_set_daytime_mode()");
            } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_video_set_daytime_mode()");
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_close_night_light()");
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_close_ircut()");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

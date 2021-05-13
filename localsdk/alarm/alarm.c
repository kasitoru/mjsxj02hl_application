#include <stdbool.h>
#include <stdio.h>

#include "./alarm.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"
#include "./../../yyjson/src/yyjson.h"
#include "./../../mqtt/mqtt.h"

// MQTT send info
bool alarm_state_mqtt(bool motion, bool humanoid) {
    bool result = false;
    
    logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Send alarm state info
    char *topic = mqtt_fulltopic(MQTT_ALARM_TOPIC);
    if(topic && topic[0]) {
        logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_INFO, "%s success.", "mqtt_fulltopic()");
    
        // JSON Data
        yyjson_mut_doc *json_doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *json_root = yyjson_mut_obj(json_doc);
        yyjson_mut_doc_set_root(json_doc, json_root);
        
        // Motion state
        yyjson_mut_obj_add_bool(json_doc, json_root, "motion", motion);
        // Humanoid state
        yyjson_mut_obj_add_bool(json_doc, json_root, "humanoid", humanoid);
        // Current timestamp
        int timestamp = (int) time(NULL);
        yyjson_mut_obj_add_int(json_doc, json_root, "timestamp", timestamp);
        
        // Send it
        const char *json = yyjson_mut_write(json_doc, 0, NULL);
        if(json) {
            logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_INFO, "%s success.", "yyjson_mut_write()");
            if(mqtt_send(topic, (char *) json)) {
                logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_INFO, "%s success.", "mqtt_send()");
                result = true;
            } else logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send()");
            free((void *)json);
        } else logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_mut_write()");
        
        // Free resources
        yyjson_mut_doc_free(json_doc);
        free(topic);

    } else logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_ERROR, "%s error!", "mqtt_fulltopic()");
    
    logger("alarm", "alarm_state_mqtt", LOGGER_LEVEL_DEBUG, "Function completed.");
    
    return result;
}

// Alarm callback
int alarm_state_callback(LOCALSDK_ALARM_EVENT_INFO *eventInfo) {
    static bool alarm_state_motion, alarm_state_humanoid;
    switch(eventInfo->type) {
        case LOCALSDK_ALARM_MOTION:
            if((!!eventInfo->state) != alarm_state_motion) {
                alarm_state_motion = !!eventInfo->state;
                logger("alarm", "alarm_state_callback", LOGGER_LEVEL_INFO, "Change %s status: %d", "motion", alarm_state_motion);
                if(mqtt_is_enabled() && mqtt_is_connected()) {
                    if(alarm_state_mqtt(alarm_state_motion, alarm_state_humanoid)) {
                        logger("alarm", "alarm_state_callback", LOGGER_LEVEL_INFO, "%s success.", "alarm_state_mqtt(LOCALSDK_ALARM_MOTION)");
                    } else logger("alarm", "alarm_state_callback", LOGGER_LEVEL_ERROR, "%s error!", "alarm_state_mqtt(LOCALSDK_ALARM_MOTION)");
                }
            }
            break;
        case LOCALSDK_ALARM_HUMANOID:
            if((!!eventInfo->state) != alarm_state_humanoid) {
                alarm_state_humanoid = !!eventInfo->state;
                logger("alarm", "alarm_state_callback", LOGGER_LEVEL_INFO, "Change %s status: %d", "humanoid", alarm_state_humanoid);
                if(mqtt_is_enabled() && mqtt_is_connected()) {
                    if(alarm_state_mqtt(alarm_state_motion, alarm_state_humanoid)) {
                        logger("alarm", "alarm_state_callback", LOGGER_LEVEL_INFO, "%s success.", "alarm_state_mqtt(LOCALSDK_ALARM_HUMANOID)");
                    } else logger("alarm", "alarm_state_callback", LOGGER_LEVEL_ERROR, "%s error!", "alarm_state_mqtt(LOCALSDK_ALARM_HUMANOID)");
                }
            }
            break;
        default:
            logger("alarm", "alarm_state_callback", LOGGER_LEVEL_INFO, "Change %s status: %d", "unknown", eventInfo->state);
    }
    return LOCALSDK_OK;
}

// Init alarm
bool alarm_init() {
    logger("alarm", "alarm_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(local_sdk_alarm_init(LOCALSDK_VIDEO_SECONDARY_WIDTH, LOCALSDK_VIDEO_SECONDARY_HEIGHT) == LOCALSDK_OK) {
        logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_alarm_init()");
        if(local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_MOTION, APP_CFG.alarm.motion) == LOCALSDK_OK) {
            logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_MOTION)");
            if(local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_HUMANOID, APP_CFG.alarm.humanoid) == LOCALSDK_OK) {
                logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_HUMANOID)");
                if(local_sdk_alarm_state_set_callback(alarm_state_callback) == LOCALSDK_OK) {
                    logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_alarm_state_set_callback()");
                    if(local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION, true) == LOCALSDK_OK) {
                        logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION)");
                        if(local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID, true) == LOCALSDK_OK) {
                            logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID)");
                            
                            logger("alarm", "alarm_init", LOGGER_LEVEL_DEBUG, "Function completed.");
                            return true;
                        } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID)");
                    } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION)");
                } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_alarm_state_set_callback()");
            } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_HUMANOID)");
        } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_MOTION)");
    } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_alarm_init()");
    if(alarm_free()) {
        logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "alarm_free()");
    } else logger("alarm", "alarm_init", LOGGER_LEVEL_WARNING, "%s error!", "alarm_free()");
    logger("alarm", "alarm_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return false;
}

// Free alarm
bool alarm_free() {
    bool result = true;
    logger("alarm", "alarm_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Disable alarm for humanoid
    if(local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID, false) == LOCALSDK_OK) {
        logger("alarm", "alarm_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID)");
    } else {
        logger("alarm", "alarm_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID)");
        result = false;
    }
    
    // Disable alarm for motion
    if(local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION, false) == LOCALSDK_OK) {
        logger("alarm", "alarm_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION)");
    } else {
        logger("alarm", "alarm_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION)");
        result = false;
    }
    
    // Clear alarm state callback
    if(local_sdk_alarm_state_clear_callback(alarm_state_callback) == LOCALSDK_OK) {
        logger("alarm", "alarm_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_alarm_state_clear_callback()");
    } else {
        logger("alarm", "alarm_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_alarm_state_clear_callback()");
        result = false;
    }
    
    // Alarm exit
    if(local_sdk_alarm_exit() == LOCALSDK_OK) {
        logger("alarm", "alarm_free", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_alarm_exit()");
    } else {
        logger("alarm", "alarm_free", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_alarm_exit()");
        result = false;
    }
    
    logger("alarm", "alarm_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

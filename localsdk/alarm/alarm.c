#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

#include "./alarm.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"
#include "./../../configs/configs.h"
#include "./../../yyjson/src/yyjson.h"
#include "./../../mqtt/mqtt.h"

static pthread_t timeout_thread;
static int alarm_time_motion, alarm_time_humanoid;

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

// State timeout (for pthread)
void* alarm_state_timeout(void *args) {

    logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_DEBUG, "Function is called...");

    bool alarm_state_motion, alarm_state_humanoid;
    bool alarm_change_motion, alarm_change_humanoid;

    alarm_time_motion = 0;
    alarm_time_humanoid = 0;
    
    alarm_state_motion = false;
    alarm_state_humanoid = false;

    do {
        alarm_change_motion = false;
        alarm_change_humanoid = false;
    
        // Motion
        if(alarm_time_motion > 0) {
            if(alarm_state_motion) {
                if((int) time(NULL) - alarm_time_motion > APP_CFG.alarm.motion_timeout) {
                    alarm_time_motion = 0;
                    alarm_state_motion = false;
                    alarm_change_motion = true;
                }
            } else {
                alarm_state_motion = true;
                alarm_change_motion = true;
            }
        }
        
        // Humanoid
        if(alarm_time_humanoid > 0) {
            if(alarm_state_humanoid) {
                if((int) time(NULL) - alarm_time_humanoid > APP_CFG.alarm.humanoid_timeout) {
                    alarm_time_humanoid = 0;
                    alarm_state_humanoid = false;
                    alarm_change_humanoid = true;
                }
            } else {
                alarm_state_humanoid = true;
                alarm_change_humanoid = true;
            }
        }
        
        // State changed
        if(alarm_change_motion || alarm_change_humanoid) {
        
            // Motion
            if(alarm_change_motion) {
                logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "Change %s status: %d", "motion", alarm_state_motion);
                // Execute the command
                if(alarm_state_motion && APP_CFG.alarm.motion_detect_exec && APP_CFG.alarm.motion_detect_exec[0]) {
                    // Detect
                    if(system(APP_CFG.alarm.motion_detect_exec) == 0) {
                        logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "%s success.", "system(motion_detect_exec)");
                    } else logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_ERROR, "%s error!", "system(motion_detect_exec)");
                } else if(!alarm_state_motion && APP_CFG.alarm.motion_lost_exec && APP_CFG.alarm.motion_lost_exec[0]) {
                    // Lost
                    if(system(APP_CFG.alarm.motion_lost_exec) == 0) {
                        logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "%s success.", "system(motion_lost_exec)");
                    } else logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_ERROR, "%s error!", "system(motion_lost_exec)");
                }
            }
            
            // Humanoid
            if(alarm_change_humanoid) {
                logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "Change %s status: %d", "humanoid", alarm_state_motion);
                // Execute the command
                if(alarm_state_motion && APP_CFG.alarm.humanoid_detect_exec && APP_CFG.alarm.humanoid_detect_exec[0]) {
                    // Detect
                    if(system(APP_CFG.alarm.humanoid_detect_exec) == 0) {
                        logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "%s success.", "system(humanoid_detect_exec)");
                    } else logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_ERROR, "%s error!", "system(humanoid_detect_exec)");
                } else if(!alarm_state_humanoid && APP_CFG.alarm.humanoid_lost_exec && APP_CFG.alarm.humanoid_lost_exec[0]) {
                    // Lost
                    if(system(APP_CFG.alarm.humanoid_lost_exec) == 0) {
                        logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "%s success.", "system(humanoid_lost_exec)");
                    } else logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_ERROR, "%s error!", "system(humanoid_lost_exec)");
                }
            }
            
            // MQTT
            if(mqtt_is_enabled() && mqtt_is_connected()) {
                if(alarm_state_mqtt(alarm_state_motion, alarm_state_humanoid)) {
                    logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_INFO, "%s success.", "alarm_state_mqtt()");
                } else logger("alarm", "alarm_state_timeout", LOGGER_LEVEL_ERROR, "%s error!", "alarm_state_mqtt()");
            }
        }
        
        sleep(1);
        pthread_testcancel();
    } while(true);
}

// Alarm callback
int alarm_state_callback(LOCALSDK_ALARM_EVENT_INFO *eventInfo) {
    if(eventInfo->state) {
        int current_timestamp = (int) time(NULL);
        switch(eventInfo->type) {
            case LOCALSDK_ALARM_MOTION:
                alarm_time_motion = current_timestamp;
                break;
            case LOCALSDK_ALARM_HUMANOID:
                alarm_time_humanoid = current_timestamp;
                break;
            default:
                logger("alarm", "alarm_state_callback", LOGGER_LEVEL_INFO, "Change %s status: %d", "unknown", eventInfo->state);
        }
    }
    return LOCALSDK_OK;
}

// Enable or disable alarm
bool alarm_switch(bool state) {
    bool result = false;
    logger("alarm", "alarm_switch", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Switch alarm for motion
    if(local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION, state) == LOCALSDK_OK) {
        logger("alarm", "alarm_switch", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION)");
        result = true;
    } else logger("alarm", "alarm_switch", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_MOTION)");

    // Switch alarm for humanoid
    if(local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID, state) == LOCALSDK_OK) {
        logger("alarm", "alarm_switch", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID)");
        result = true;
    } else logger("alarm", "alarm_switch", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_set_alarm_switch(LOCALSDK_ALARM_HUMANOID)");
    
    logger("alarm", "alarm_switch", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Init alarm
bool alarm_init() {
    logger("alarm", "alarm_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(local_sdk_alarm_init(LOCALSDK_VIDEO_SECONDARY_WIDTH, LOCALSDK_VIDEO_SECONDARY_HEIGHT) == LOCALSDK_OK) {
        logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_alarm_init()");
        if(local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_MOTION, APP_CFG.alarm.motion_sens) == LOCALSDK_OK) {
            logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_MOTION)");
            if(local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_HUMANOID, APP_CFG.alarm.humanoid_sens) == LOCALSDK_OK) {
                logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_set_alarm_sensitivity(LOCALSDK_ALARM_HUMANOID)");
                if(local_sdk_alarm_state_set_callback(alarm_state_callback) == LOCALSDK_OK) {
                    logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_alarm_state_set_callback()");
                    if(pthread_create(&timeout_thread, NULL, alarm_state_timeout, NULL) == 0) {
                        logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "pthread_create(timeout_thread)");
                        if(alarm_switch(true)) {
                            logger("alarm", "alarm_init", LOGGER_LEVEL_INFO, "%s success.", "alarm_switch(true)");
                        
                            logger("alarm", "alarm_init", LOGGER_LEVEL_DEBUG, "Function completed.");
                            return true;
                        } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "alarm_switch(true)");
                    } else logger("alarm", "alarm_init", LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(timeout_thread)");
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
    
    // Disable alarm
    if(alarm_switch(false)) {
        logger("alarm", "alarm_free", LOGGER_LEVEL_INFO, "%s success.", "alarm_switch(false)");
    } else {
        logger("alarm", "alarm_free", LOGGER_LEVEL_WARNING, "%s error!", "alarm_switch(false)");
        result = false;
    }

    // Stop timeout thread
    if(timeout_thread) {
        if(pthread_cancel(timeout_thread) == 0) {
            logger("alarm", "alarm_free", LOGGER_LEVEL_INFO, "%s success.", "pthread_cancel(timeout_thread)");
        } else {
            logger("alarm", "alarm_free", LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(timeout_thread)");
            result = false;
        }
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

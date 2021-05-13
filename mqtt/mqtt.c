#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "./mqtt.h"
#include "./paho.mqtt.c/src/MQTTClient.h"
#include "./../logger/logger.h"
#include "./../localsdk/localsdk.h"
#include "./../localsdk/speaker/speaker.h"
#include "./../configs/configs.h"
#include "./../yyjson/src/yyjson.h"

MQTTClient MQTTclient;
pthread_t periodical_thread;
pthread_t reconnection_thread;

// Get full topic
char* mqtt_fulltopic(char *topic) {
    char *payload;
    logger("mqtt", "mqtt_fulltopic", LOGGER_LEVEL_DEBUG, "Function is called...");
    if(asprintf(&payload, "%s/%s", APP_CFG.mqtt.topic, topic) > 0) {
        logger("mqtt", "mqtt_fulltopic", LOGGER_LEVEL_INFO, "%s success.", "asprintf()");
    } else logger("mqtt", "mqtt_fulltopic", LOGGER_LEVEL_ERROR, "%s error!", "asprintf()");
    logger("mqtt", "mqtt_fulltopic", LOGGER_LEVEL_DEBUG, "Function completed.");
    return payload;
}

// Send data
bool mqtt_send(char *topic, char *payload) {
    bool result = false;
    logger("mqtt", "mqtt_send", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("mqtt", "mqtt_send", LOGGER_LEVEL_DEBUG, "Topic: %s", topic);
    logger("mqtt", "mqtt_send", LOGGER_LEVEL_DEBUG, "Payload: %s", payload);
    MQTTClient_message message = MQTTClient_message_initializer;
    message.payload = payload;
    message.payloadlen = (int) strlen(payload);
    message.qos = APP_CFG.mqtt.qos;
    message.retained = APP_CFG.mqtt.retain;
    MQTTClient_deliveryToken token;
    if(MQTTClient_publishMessage(MQTTclient, topic, &message, &token) == MQTTCLIENT_SUCCESS) {
        logger("mqtt", "mqtt_send", LOGGER_LEVEL_DEBUG, "Token: %d", token);
        logger("mqtt", "mqtt_send", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_publishMessage()");
        result = true;
    } else logger("mqtt", "mqtt_send", LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_publishMessage()");
    logger("mqtt", "mqtt_send", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Send formatted data
bool mqtt_sendf(char *topic, const char *format, ...) {
    bool result = false;
    logger("mqtt", "mqtt_sendf", LOGGER_LEVEL_DEBUG, "Function is called...");
    va_list params;
    va_start(params, format);
    char *payload;
    if(vasprintf(&payload, format, params) > 0) {
        logger("mqtt", "mqtt_sendf", LOGGER_LEVEL_INFO, "%s success.", "vasprintf()");
        result = mqtt_send(topic, payload);
        free(payload);
    } else logger("mqtt", "mqtt_sendf", LOGGER_LEVEL_ERROR, "%s error!", "vasprintf()");
    va_end(params);
    logger("mqtt", "mqtt_sendf", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Periodic data sending
void* mqtt_periodical(void *arg) {
    bool endless_cycle = (bool) arg;
    logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_DEBUG, "Function is called...");

    bool first = endless_cycle;
    do {
        
        // Home Assistant discovery
        // https://www.home-assistant.io/docs/mqtt/discovery/
        if(first) {
            // TODO
            first = false;
        }
        
        // Send system info
        char *topic = mqtt_fulltopic(MQTT_INFO_TOPIC);
        if(topic && topic[0]) {
            logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "mqtt_fulltopic()");

            // JSON Data
            yyjson_mut_doc *json_doc = yyjson_mut_doc_new(NULL);
            yyjson_mut_val *json_root = yyjson_mut_obj(json_doc);
            yyjson_mut_doc_set_root(json_doc, json_root);
            // SDK version
            int sdk_version = (int) localsdk_get_version();
            yyjson_mut_obj_add_int(json_doc, json_root, "sdk_version", sdk_version);
            // Build time
            int build_time = -1;
            struct tm compile_time;
            if(strptime(__DATE__ ", " __TIME__, "%b %d %Y, %H:%M:%S", &compile_time) != NULL) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "strptime()");
                build_time = (int) mktime(&compile_time);
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "strptime()");
            yyjson_mut_obj_add_int(json_doc, json_root, "build_time", build_time);
            // Startup timestamp
            int startup = (int) -1;
            struct stat proc_self;
            if(stat("/proc/self", &proc_self) == 0) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "stat(\"/proc/self\")");
                startup = (int) proc_self.st_ctime;
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "stat(\"/proc/self\")");
            yyjson_mut_obj_add_int(json_doc, json_root, "startup", startup);
            // Current timestamp
            int timestamp = (int) time(NULL);
            yyjson_mut_obj_add_int(json_doc, json_root, "timestamp", timestamp);
            // IP address
            char *ip_address = "";
            struct ifaddrs *if_list, *if_item;
            if(getifaddrs(&if_list) == 0) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "getifaddrs()");
                for(if_item = if_list; if_item != NULL; if_item = if_item->ifa_next) {
                    if(if_item->ifa_addr == NULL) continue;
                    if(strcmp(if_item->ifa_name, "wlan0") == 0) {
                        struct sockaddr_in * ip_item;
                        ip_item = (struct sockaddr_in *) if_item->ifa_addr;
                        if(ip_item->sin_family == AF_INET) {
                            ip_address = inet_ntoa(ip_item->sin_addr);
                        }
                    }
                }
                freeifaddrs(if_list);
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "getifaddrs()");
            yyjson_mut_obj_add_str(json_doc, json_root, "ip_address", ip_address);
            // RAM
            unsigned long total_ram = 0;
            unsigned long free_ram = 0;
            struct sysinfo ram_info;
            if(sysinfo(&ram_info) == 0) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "sysinfo()");
                total_ram = (unsigned long) ram_info.totalram;
                free_ram = (unsigned long) ram_info.freeram;
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "sysinfo()");
            yyjson_mut_obj_add_uint(json_doc, json_root, "total_ram", total_ram);
            yyjson_mut_obj_add_uint(json_doc, json_root, "free_ram", free_ram);
            // SD-Card memory
            unsigned long total_sdmem = 0;
            unsigned long free_sdmem = 0;
            struct statvfs stat_mmc;
            if(statvfs("/mnt/mmc", &stat_mmc) == 0) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "statvfs(\"/mnt/mmc\")");
                total_sdmem = (unsigned long) stat_mmc.f_frsize * stat_mmc.f_blocks;
                free_sdmem = (unsigned long) stat_mmc.f_bsize * stat_mmc.f_bfree;
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "statvfs(\"/mnt/mmc\")");
            yyjson_mut_obj_add_uint(json_doc, json_root, "total_sdmem", total_sdmem);
            yyjson_mut_obj_add_uint(json_doc, json_root, "free_sdmem", free_sdmem);
            // Configs free memory
            unsigned long total_configs = 0;
            unsigned long free_configs = 0;
            struct statvfs stat_configs;
            if(statvfs("/configs", &stat_configs) == 0) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "statvfs(\"/configs\")");
                total_configs = (unsigned long) stat_configs.f_frsize * stat_configs.f_blocks;
                free_configs = (unsigned long) stat_configs.f_bsize * stat_configs.f_bfree;
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "statvfs(\"/configs\")");
            yyjson_mut_obj_add_uint(json_doc, json_root, "total_configs", total_configs);
            yyjson_mut_obj_add_uint(json_doc, json_root, "free_configs", free_configs);
            // Volume level
            int volume_level = speaker_get_volume();
            yyjson_mut_obj_add_int(json_doc, json_root, "volume_level", volume_level);
            // Image URL
            char *image_url = "";
            if(ip_address && ip_address[0]) {
                if(asprintf(&image_url, "http://%s/cgi/get_image.cgi", ip_address) > 0) {
                    logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "asprintf()");
                } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "asprintf()");
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_WARNING, "%s error!", "ip_address is null so image_url");
            yyjson_mut_obj_add_str(json_doc, json_root, "image_url", image_url);
            free(image_url);

            // Send it
            const char *json = yyjson_mut_write(json_doc, 0, NULL);
            if(json) {
                logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "yyjson_mut_write()");
                if(mqtt_send(topic, (char *) json)) {
                    logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "%s success.", "mqtt_send()");
                } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send()");
                free((void *)json);
            } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_mut_write()");
            
            // Free resources
            yyjson_mut_doc_free(json_doc);
            free(ip_address);
            free(topic);
        } else logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_ERROR, "%s error!", "mqtt_fulltopic()");

        // Sleep
        if(endless_cycle) {
            logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_INFO, "Wait %d seconds until the next sending...", MQTT_PERIODICAL_INTERVAL);
            sleep(MQTT_PERIODICAL_INTERVAL);
            pthread_testcancel();
        }
    } while(endless_cycle);
    
    logger("mqtt", "mqtt_periodical", LOGGER_LEVEL_DEBUG, "Function completed.");
    return 0;
}

// Message received
int mqtt_message_callback(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_DEBUG, "Topic: %s", topicName);
    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_DEBUG, "Payload: %.*s", message->payloadlen, (char *) message->payload);
    
    // Parse JSON
    yyjson_doc *json_doc = yyjson_read((char *) message->payload, message->payloadlen, 0);
    yyjson_val *json_root = yyjson_doc_get_root(json_doc);
    yyjson_val *json_action = yyjson_obj_get(json_root, "action");
    
    if(json_action) {
        logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "yyjson_obj_get(\"action\")");
        
        if(yyjson_is_str(json_action)) {
            logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "yyjson_is_str(json_action)");
            
            // Get image
            if(strcmp(yyjson_get_str(json_action), "get_image") == 0) {
                logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "strcmp(\"get_image\")");
                yyjson_val *json_filename = yyjson_obj_get(json_root, "filename");
                if(yyjson_is_str(json_filename)) {
                    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "yyjson_is_str(json_filename)");
                    if(local_sdk_video_get_jpeg(LOCALSDK_VIDEO_SECONDARY_CHANNEL, (char *) yyjson_get_str(json_filename)) == LOCALSDK_OK) {
                        logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_video_get_jpeg()");
                    } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_get_jpeg()");
                } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_str(json_filename)");
            
            // Set volume
            } else if(strcmp(yyjson_get_str(json_action), "set_volume") == 0) {
                logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "strcmp(\"set_volume\")");
                yyjson_val *json_value = yyjson_obj_get(json_root, "value");
                if(yyjson_is_int(json_value)) {
                    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "yyjson_is_int(json_value)");
                    if(speaker_set_volume(yyjson_get_int(json_value))) {
                        logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "speaker_set_volume()");
                    } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "speaker_set_volume()");
                } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_int(json_value)");
            
            // Play media
            } else if(strcmp(yyjson_get_str(json_action), "play_media") == 0) {
                logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "strcmp(\"play_media\")");
                yyjson_val *json_filename = yyjson_obj_get(json_root, "filename");
                if(yyjson_is_str(json_filename)) {
                    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "yyjson_is_str(json_filename)");
                    if(speaker_play_media((char *) yyjson_get_str(json_filename))) {
                        logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_INFO, "%s success.", "speaker_play_media()");
                    } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "speaker_play_media()");
                } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_str(json_filename)");
            
            // Unknown action
            } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "Unknown action");
            mqtt_periodical((void *) false); // Send new data
        } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_str(json_action)");
    } else logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_ERROR, "%s error!", "yyjson_obj_get(\"action\")");
    yyjson_doc_free(json_doc);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    logger("mqtt", "mqtt_message_callback", LOGGER_LEVEL_DEBUG, "Function completed.");
    return 1;
}

// Delivery confirmed
void mqtt_delivery_callback(void *context, MQTTClient_deliveryToken dt) {
    logger("mqtt", "mqtt_delivery_callback", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("mqtt", "mqtt_delivery_callback", LOGGER_LEVEL_DEBUG, "Message with token value %d delivery confirmed.", dt);
    logger("mqtt", "mqtt_delivery_callback", LOGGER_LEVEL_DEBUG, "Function completed.");
}

// Lost connection
bool mqtt_initialization(bool first_init);
void mqtt_disconnect_callback(void *context, char *cause) {
    logger("mqtt", "mqtt_disconnect_callback", LOGGER_LEVEL_DEBUG, "Function is called...");
    do {
        logger("mqtt", "mqtt_disconnect_callback", LOGGER_LEVEL_WARNING, "The connection to the MQTT server was lost! Wait %d seconds...", MQTT_RECONNECT_INTERVAL);
        mqtt_free();
        sleep(MQTT_RECONNECT_INTERVAL);
        pthread_testcancel();
    } while(mqtt_initialization(false) == false);
    logger("mqtt", "mqtt_disconnect_callback", LOGGER_LEVEL_DEBUG, "Function completed.");
}

// Reconnect (for pthread)
void* mqtt_reconnection(void *args) {
    logger("mqtt", "mqtt_reconnection", LOGGER_LEVEL_DEBUG, "Function is called...");
    mqtt_disconnect_callback(NULL, NULL);
    logger("mqtt", "mqtt_reconnection", LOGGER_LEVEL_DEBUG, "Function completed.");
    return 0;
}

// Is enabled
bool mqtt_is_enabled() {
    logger("mqtt", "mqtt_is_enabled", LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = (APP_CFG.mqtt.server && APP_CFG.mqtt.server[0]);
    logger("mqtt", "mqtt_is_enabled", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Init mqtt (local)
bool mqtt_initialization(bool first_init) {

    logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    if(mqtt_is_enabled()) { // If MQTT enabled
    
        // Get server address
        char *server_address;
        if(asprintf(&server_address, "tcp://%s:%d", APP_CFG.mqtt.server, APP_CFG.mqtt.port) > 0) {

            // Create MQTT client
            if(MQTTClient_create(&MQTTclient, server_address, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL) == MQTTCLIENT_SUCCESS) {
                logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_create()");
                free(server_address);
                    
                // Set callbacks
                if(MQTTClient_setCallbacks(MQTTclient, NULL, mqtt_disconnect_callback, mqtt_message_callback, mqtt_delivery_callback) == MQTTCLIENT_SUCCESS) {
                    logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_setCallbacks()");
                    
                    // Get authorization data
                    MQTTClient_connectOptions connect_options = MQTTClient_connectOptions_initializer;
                    if(APP_CFG.mqtt.username && APP_CFG.mqtt.username[0]) {
                        connect_options.username = APP_CFG.mqtt.username;
                        logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Username: %s", connect_options.username);
                        if(APP_CFG.mqtt.password && APP_CFG.mqtt.password[0]) {
                            connect_options.password = APP_CFG.mqtt.password;
                            logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Password: %s", "<hidden>");
                        } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Password: %s", "<not_set> (shared connection)");
                    } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Username: %s", "<not_set> (anonymous connection)");
                        
                    // Connection to server
                    if(MQTTClient_connect(MQTTclient, &connect_options) == MQTTCLIENT_SUCCESS) {
                        logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_connect()");
                            
                        // Subscribe to topic
                        char *command_topic;
                        if(asprintf(&command_topic, "%s/%s", APP_CFG.mqtt.topic, MQTT_COMMAND_TOPIC) > 0) {
                            logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Command topic: %s", command_topic);
                            if(MQTTClient_subscribe(MQTTclient, command_topic, true) == MQTTCLIENT_SUCCESS) {
                                logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_subscribe()");
                                free(command_topic);
                                
                                // Periodic data sending
                                if(pthread_create(&periodical_thread, NULL, mqtt_periodical, (void *) true) == 0) {
                                    logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_INFO, "%s success.", "pthread_create(periodical_thread)");

                                    logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Function completed.");
                                    return true;
                                } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(periodical_thread)");
                            } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_subscribe()");
                            free(command_topic);
                        } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "asprintf()");
                    } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_connect()");
                } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_setCallbacks()");
            } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_create()");
            free(server_address);
        } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_ERROR, "%s error!", "asprintf()");

        // Reconnect (only for first init)
        if(first_init == true) {
            if(pthread_create(&reconnection_thread, NULL, mqtt_reconnection, NULL) == 0) {
                logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_INFO, "%s success.", "pthread_create(reconnection_thread)");
            } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_WARNING, "%s error!", "pthread_create(reconnection_thread)");
        }

    } else logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_WARNING, "MQTT is disabled because server address not set.");
    logger("mqtt", "mqtt_initialization", LOGGER_LEVEL_DEBUG, "Function completed.");
    return false;
}

// Init mqtt (global)
bool mqtt_init() {
    bool result;
    logger("mqtt", "mqtt_init", LOGGER_LEVEL_DEBUG, "Function is called...");
    result = mqtt_initialization(true);
    logger("mqtt", "mqtt_init", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Check connection
bool mqtt_is_connected() {
    logger("mqtt", "mqtt_is_connected", LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = !!MQTTClient_isConnected(MQTTclient);
    logger("mqtt", "mqtt_is_connected", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}

// Free mqtt
bool mqtt_free() {
    bool result = true;
    logger("mqtt", "mqtt_free", LOGGER_LEVEL_DEBUG, "Function is called...");

    // Stop reconnection
    if(reconnection_thread) {
        if(pthread_cancel(reconnection_thread) == 0) {
            logger("mqtt", "mqtt_free", LOGGER_LEVEL_INFO, "%s success.", "pthread_cancel(reconnection_thread)");
        } else {
            logger("mqtt", "mqtt_free", LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(reconnection_thread)");
            result = false;
        }
    }
    
    // Stop periodic data sending
    if(periodical_thread) {
        if(pthread_cancel(periodical_thread) == 0) {
            logger("mqtt", "mqtt_free", LOGGER_LEVEL_INFO, "%s success.", "pthread_cancel(periodical_thread)");
        } else {
            logger("mqtt", "mqtt_free", LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(periodical_thread)");
            result = false;
        }
    }
    
    // Unsubscribe
    char *command_topic;
    if(asprintf(&command_topic, "%s/%s", APP_CFG.mqtt.topic, MQTT_COMMAND_TOPIC) > 0) {
        if(MQTTClient_unsubscribe(MQTTclient, command_topic) == MQTTCLIENT_SUCCESS) {
            logger("mqtt", "mqtt_free", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_unsubscribe()");
        } else {
            logger("mqtt", "mqtt_free", LOGGER_LEVEL_WARNING, "%s error!", "MQTTClient_unsubscribe()");
            result = false;
        }
        free(command_topic);
    } else {
        logger("mqtt", "mqtt_free", LOGGER_LEVEL_WARNING, "%s error!", "asprintf()");
        result = false;
    }
    
    // Disconnect
    if(MQTTClient_disconnect(MQTTclient, MQTT_DISCONNECT_TIMEOUT) == MQTTCLIENT_SUCCESS) {
        logger("mqtt", "mqtt_free", LOGGER_LEVEL_INFO, "%s success.", "MQTTClient_disconnect()");
    } else {
        logger("mqtt", "mqtt_free", LOGGER_LEVEL_WARNING, "%s error!", "MQTTClient_disconnect()");
        result = false;
    }
    
    // Destroy
    MQTTClient_destroy(&MQTTclient);
    
    logger("mqtt", "mqtt_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return result;
}
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "./mqtt.h"
#include "./homeassistant.h"
#include "./paho.mqtt.c/src/MQTTClient.h"
#include "./../logger/logger.h"
#include "./../localsdk/init.h"
#include "./../localsdk/localsdk.h"
#include "./../localsdk/speaker/speaker.h"
#include "./../configs/configs.h"
#include "./../yyjson/src/yyjson.h"

static MQTTClient MQTTclient;
static pthread_t periodical_thread;
static pthread_t reconnection_thread;
static pthread_t playmedia_thread;

// Prepare string for use as MQTT paths/names
char *mqtt_prepare_string(const char *string) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Device name
    size_t j = 0;
    size_t length = strlen(string) + 1;
    char *device_name = malloc(length);
    if(device_name != NULL) {
        memset(device_name, '\0', length);
        for(size_t i = 0; string[i] != '\0'; i++) {
            char chr = tolower(string[i]);
            if(isspace(chr) || (chr == '/')) {
                device_name[i-j] = '_';
            } else if(isalnum(chr)) {
                device_name[i-j] = chr;
            } else j++;
        }
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "malloc(length)");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
    return device_name;
}

// Get clien id
char *mqtt_client_id() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    char *client_id = "";
    
    char *mqtt_topic = mqtt_prepare_string(APP_CFG.mqtt.topic);
    if(mqtt_topic != NULL) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_prepare_string(APP_CFG.mqtt.topic)");
        char *general_name = mqtt_prepare_string(APP_CFG.general.name);
        if(general_name != NULL) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_prepare_string(APP_CFG.general.name)");
            char *dev_id = device_id();
            if(dev_id != NULL) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "device_id()");
                char *prep_id = mqtt_prepare_string(dev_id);
                if(prep_id != NULL) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_prepare_string(dev_id)");
                    if(asprintf(&client_id, "%s_%s_%s", mqtt_topic, general_name, prep_id) != -1) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "asprintf(client_id)");
                    else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "asprintf(client_id)");
                    free(prep_id);
                } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "mqtt_prepare_string(dev_id)");
                free(dev_id);
            } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "device_id()");
            free(general_name);
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "mqtt_prepare_string(APP_CFG.general.name)");
        free(mqtt_topic);
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "mqtt_prepare_string(APP_CFG.mqtt.topic)");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
    return client_id;
}

// Get full topic
char *mqtt_fulltopic(const char *topic) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    char *payload = "";
    
    char *general_name = mqtt_prepare_string(APP_CFG.general.name);
    if(general_name != NULL) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_prepare_string(APP_CFG.general.name)");
        if(asprintf(&payload, "%s/%s/%s", APP_CFG.mqtt.topic, general_name, topic) != -1) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "asprintf(payload)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "asprintf(payload)");
        free(general_name);
    } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "mqtt_prepare_string(APP_CFG.general.name)");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
    return payload;
}

// Send data
bool mqtt_send(const char *topic, char *payload) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(mqtt_is_ready()) {
        LOGGER(LOGGER_LEVEL_INFO, "Topic: %s", topic);
        LOGGER(LOGGER_LEVEL_INFO, "Payload: %s", payload);
        MQTTClient_message message = MQTTClient_message_initializer;
        message.payload = payload;
        message.payloadlen = (int) strlen(payload);
        message.qos = APP_CFG.mqtt.qos;
        message.retained = APP_CFG.mqtt.retain;
        MQTTClient_deliveryToken token;
        if(result &= (MQTTClient_publishMessage(MQTTclient, topic, &message, &token) == MQTTCLIENT_SUCCESS)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_publishMessage()");
            LOGGER(LOGGER_LEVEL_INFO, "Token: %d", token);
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "MQTTClient_publishMessage()");
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Send formatted data
bool mqtt_sendf(const char *topic, const char *format, ...) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    va_list params;
    va_start(params, format);
    char *payload = "";
    if(result &= (vasprintf(&payload, format, params) != -1)) {
        if(result &= mqtt_send(topic, payload)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_send()");
        else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send()");
        free(payload);
    }
    va_end(params);
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Periodic data sending
static void *mqtt_periodical(void *arg) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    
    bool endless_cycle = (bool) arg;
    bool first = endless_cycle;
    do { //-V1044
        // First iteration
        if(first) {
            first = false;
            
            // Home Assistant Discovery
            
            // info/fw_version
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "fw_version", NULL, NULL, false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, fw_version)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, fw_version)");
            // info/ip_address
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "ip_address", NULL, NULL, false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, ip_address)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, ip_address)");
            // info/total_ram
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "total_ram", NULL, "byte(s)", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, total_ram)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, total_ram)");
            // info/free_ram
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "free_ram", NULL, "byte(s)", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, free_ram)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, free_ram)");
            // info/total_sdmem
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "total_sdmem", NULL, "byte(s)", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, total_sdmem)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, total_sdmem)");
            // info/free_sdmem
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "free_sdmem", NULL, "byte(s)", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, free_sdmem)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, free_sdmem)");
            // info/total_configs
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "total_configs", NULL, "byte(s)", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, total_configs)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, total_configs)");
            // info/free_configs
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "free_configs", NULL, "byte(s)", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, free_configs)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, free_configs)");
            // info/volume_level
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "volume_level", NULL, "%", false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, volume_level)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, volume_level)");
            // info/media_status
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "media_status", NULL, NULL, false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, media_status)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, media_status)");
            // info/image_url
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, "fw_version", NULL, NULL, false)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, fw_version)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_INFO_TOPIC, fw_version)");
            
            // alarm/motion
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_BINARY_SENSOR, MQTT_ALARM_TOPIC, "motion", "motion", NULL, true)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_ALARM_TOPIC, motion)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_ALARM_TOPIC, motion)");
            // alarm/humanoid
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_BINARY_SENSOR, MQTT_ALARM_TOPIC, "humanoid", "motion", NULL, true)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_ALARM_TOPIC, humanoid)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_ALARM_TOPIC, humanoid)");
            
            // night/state
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_BINARY_SENSOR, MQTT_NIGHT_TOPIC, "state", NULL, NULL, true)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_NIGHT_TOPIC, state)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_NIGHT_TOPIC, state)");
            // night/gray
            if(mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_BINARY_SENSOR, MQTT_NIGHT_TOPIC, "gray", NULL, NULL, true)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_NIGHT_TOPIC, gray)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_homeassistant_discovery(MQTT_HOMEASSISTANT_SENSOR, MQTT_NIGHT_TOPIC, gray)");
            
            // Send online state
            char *state_topic = mqtt_fulltopic(MQTT_STATE_TOPIC);
            if(mqtt_send(state_topic, MQTT_STATE_ONLINE)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_send(MQTT_STATE_TOPIC)");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send(MQTT_STATE_TOPIC)");
            free(state_topic);
        }
        
        // Send system info
        yyjson_mut_doc *json_doc = yyjson_mut_doc_new(NULL);
        yyjson_mut_val *json_root = yyjson_mut_obj(json_doc);
        yyjson_mut_doc_set_root(json_doc, json_root);
        // FW version
        char *fw_version = firmware_version();
        if(yyjson_mut_obj_add_str(json_doc, json_root, "fw_version", fw_version)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_str(fw_version)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_str(fw_version)");
        // IP address
        char *ip_address = "";
        struct ifaddrs *if_list, *if_item;
        if(getifaddrs(&if_list) == 0) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "getifaddrs()");
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
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "getifaddrs()");
        if(yyjson_mut_obj_add_str(json_doc, json_root, "ip_address", ip_address)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_str(ip_address)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_str(ip_address)");
        // RAM
        unsigned long total_ram = 0;
        unsigned long free_ram = 0;
        struct sysinfo ram_info;
        if(sysinfo(&ram_info) == 0) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "sysinfo()");
            total_ram = (unsigned long) ram_info.totalram;
            free_ram = (unsigned long) ram_info.freeram;
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "sysinfo()");
        if(yyjson_mut_obj_add_uint(json_doc, json_root, "total_ram", total_ram)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_uint(total_ram)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_uint(total_ram)");
        if(yyjson_mut_obj_add_uint(json_doc, json_root, "free_ram", free_ram)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_uint(free_ram)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_uint(free_ram)");
        // SD-Card memory
        unsigned long total_sdmem = 0;
        unsigned long free_sdmem = 0;
        struct statvfs stat_mmc;
        if(statvfs("/mnt/mmc", &stat_mmc) == 0) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "statvfs(\"/mnt/mmc\")");
            total_sdmem = (unsigned long) stat_mmc.f_frsize * stat_mmc.f_blocks;
            free_sdmem = (unsigned long) stat_mmc.f_bsize * stat_mmc.f_bfree;
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "statvfs(\"/mnt/mmc\")");
        if(yyjson_mut_obj_add_uint(json_doc, json_root, "total_sdmem", total_sdmem)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_uint(total_sdmem)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_uint(total_sdmem)");
        if(yyjson_mut_obj_add_uint(json_doc, json_root, "free_sdmem", free_sdmem)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_uint(free_sdmem)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_uint(free_sdmem)");
        // Configs memory
        unsigned long total_configs = 0;
        unsigned long free_configs = 0;
        struct statvfs stat_configs;
        if(statvfs("/configs", &stat_configs) == 0) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "statvfs(\"/configs\")");
            total_configs = (unsigned long) stat_configs.f_frsize * stat_configs.f_blocks;
            free_configs = (unsigned long) stat_configs.f_bsize * stat_configs.f_bfree;
        } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "statvfs(\"/configs\")");
        if(yyjson_mut_obj_add_uint(json_doc, json_root, "total_configs", total_configs)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_uint(total_configs)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_uint(total_configs)");
        if(yyjson_mut_obj_add_uint(json_doc, json_root, "free_configs", free_configs)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_uint(free_configs)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_uint(free_configs)");
        // Volume level
        int volume_level = speaker_get_volume();
        if(yyjson_mut_obj_add_int(json_doc, json_root, "volume_level", volume_level)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_int(volume_level)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_int(volume_level)");
        // Media status
        bool media_status = speaker_status_media();
        if(yyjson_mut_obj_add_bool(json_doc, json_root, "media_status", media_status)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_bool(media_status)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_bool(media_status)");
        // Image URL
        char *image_url = "";
        if(asprintf(&image_url, "http://%s/cgi/get_image.cgi", ip_address) != -1) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "asprintf(image_url)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "asprintf(image_url)");
        if(yyjson_mut_obj_add_str(json_doc, json_root, "image_url", image_url)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_obj_add_str(image_url)");
        else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "yyjson_mut_obj_add_str(image_url)");
        // Send it
        const char *json = yyjson_mut_write(json_doc, 0, NULL);
        if(json) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_mut_write()");
            char *info_topic = mqtt_fulltopic(MQTT_INFO_TOPIC);
            if(mqtt_send(info_topic, (char *) json)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_send(MQTT_INFO_TOPIC)");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send(MQTT_INFO_TOPIC)");
            free(info_topic);
            free((void *)json);
        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "yyjson_mut_write()");
        // Free resources
        yyjson_mut_doc_free(json_doc);
        free(image_url);
        free(fw_version);
        // Sleep
        if(endless_cycle) {
            LOGGER(LOGGER_LEVEL_INFO, "Wait %d seconds until the next sending...", APP_CFG.mqtt.periodical_interval);
            sleep(APP_CFG.mqtt.periodical_interval);
            pthread_testcancel();
        }
    } while(endless_cycle);
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
    return 0;
}

// Play media (for pthread)
static struct playmedia_args {
    char *filename;
    int type;
} playmedia_args;

static void* mqtt_playmedia(void *args) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    
    struct playmedia_args *arguments = (struct playmedia_args *) args;
    if(speaker_play_media(arguments->filename, arguments->type)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_play_media()");
    else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "speaker_play_media()");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
    return 0;
}

// Message received
static int mqtt_message_callback(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    LOGGER(LOGGER_LEVEL_INFO, "Topic: %s", topicName);
    LOGGER(LOGGER_LEVEL_INFO, "Payload: %.*s", message->payloadlen, (char *) message->payload);
    
    // Parse JSON
    yyjson_doc *json_doc = yyjson_read((char *) message->payload, message->payloadlen, 0);
    yyjson_val *json_root = yyjson_doc_get_root(json_doc);
    yyjson_val *json_action = yyjson_obj_get(json_root, "action");
    if(result &= !!json_action) {
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_obj_get(action)");
        if(result &= yyjson_is_str(json_action)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_is_str(json_action)");
            // Get image
            if(strcmp(yyjson_get_str(json_action), "get_image") == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "strcmp(get_image)");
                yyjson_val *json_filename = yyjson_obj_get(json_root, "filename");
                if(result &= yyjson_is_str(json_filename)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_is_str(json_filename)");
                    if(result &= (local_sdk_video_get_jpeg(LOCALSDK_VIDEO_SECONDARY_CHANNEL, (char *) yyjson_get_str(json_filename)) == LOCALSDK_OK)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_video_get_jpeg()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_video_get_jpeg()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_str(json_filename)");
            // Set volume
            } else if(strcmp(yyjson_get_str(json_action), "set_volume") == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "strcmp(set_volume)");
                yyjson_val *json_value = yyjson_obj_get(json_root, "value");
                if(result &= yyjson_is_int(json_value)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_is_int(json_value)");
                    if(result &= speaker_set_volume(yyjson_get_int(json_value))) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_set_volume()");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "speaker_set_volume()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_int(json_value)");
            // Play media
            } else if(strcmp(yyjson_get_str(json_action), "play_media") == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "strcmp(play_media)");
                yyjson_val *json_filename = yyjson_obj_get(json_root, "filename");
                if(result &= yyjson_is_str(json_filename)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_is_str(json_filename)");
                    playmedia_args.filename = (char *) yyjson_get_str(json_filename);
                    // Set volume level
                    yyjson_val *json_volume = yyjson_obj_get(json_root, "volume");
                    if(result &= yyjson_is_int(json_volume)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_is_int(json_volume)");
                        if(result &= speaker_set_volume(yyjson_get_int(json_volume))) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_set_volume()");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "speaker_set_volume()");
                    }
                    // Get media type
                    playmedia_args.type = APP_CFG.speaker.type;
                    yyjson_val *json_type = yyjson_obj_get(json_root, "type");
                    if(result &= yyjson_is_str(json_type)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "yyjson_is_int(json_type)");
                        if(result &= (strcmp(yyjson_get_str(json_type), "g711") == 0)) {
                            playmedia_args.type = LOCALSDK_SPEAKER_G711_TYPE;
                        } else {
                            playmedia_args.type = LOCALSDK_SPEAKER_PCM_TYPE;
                        }
                    }
                    // Play
                    if(result &= (pthread_create(&playmedia_thread, NULL, mqtt_playmedia, (void *) &playmedia_args) == 0)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_create(playmedia_thread)");
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(playmedia_thread)");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_str(json_filename)");
            // Stop playback
            } else if(strcmp(yyjson_get_str(json_action), "stop_media") == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "strcmp(stop_media)");
                if(result &= speaker_stop_media()) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "speaker_stop_media()");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "speaker_stop_media()");
            // Restart
            } else if(strcmp(yyjson_get_str(json_action), "restart") == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "strcmp(restart)");
                if(result &= (system("restart.sh &") == 0)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "system(restart.sh)");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "system(restart.sh)");
            // Reboot
            } else if(strcmp(yyjson_get_str(json_action), "reboot") == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "strcmp(reboot)");
                if(result &= (system("reboot &") == 0)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "system(reboot)");
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "system(reboot)");
            // Unknown action
            } else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "Unknown action");
            mqtt_periodical((void *) false); // Send new data
        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "yyjson_is_str(json_action)");
    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "yyjson_obj_get(action)");
    
    yyjson_doc_free(json_doc);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Delivery confirmed
static void mqtt_delivery_callback(void *context, MQTTClient_deliveryToken dt) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    LOGGER(LOGGER_LEVEL_INFO, "Message with token value %d delivery confirmed.", dt);
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
}

// Lost connection
static bool mqtt_initialization(bool first_init);
static void mqtt_disconnect_callback(void *context, char *cause) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    
    do {
        LOGGER(LOGGER_LEVEL_INFO, "The connection to the MQTT server was lost! Wait %d seconds...", APP_CFG.mqtt.reconnection_interval);
        mqtt_free(false);
        sleep(APP_CFG.mqtt.reconnection_interval);
        pthread_testcancel();
    } while(mqtt_initialization(false) == false);
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
}

// Reconnect (for pthread)
static void* mqtt_reconnection(void *args) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    mqtt_disconnect_callback(NULL, NULL);
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed.");
    return 0;
}

// Is enabled
bool mqtt_is_enabled() {
    return (APP_CFG.mqtt.enable && APP_CFG.mqtt.server && APP_CFG.mqtt.server[0]);
}

// Init mqtt (local)
static bool mqtt_initialization(bool first_init) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(mqtt_is_enabled()) { // If MQTT enabled
        // Get server address
        char *server_address = "";
        if(result &= (asprintf(&server_address, "tcp://%s:%d", APP_CFG.mqtt.server, APP_CFG.mqtt.port) != -1)) {
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "asprintf(server_address)");
            // Create MQTT client
            char *client_id = mqtt_client_id();
            if(result &= (MQTTClient_create(&MQTTclient, server_address, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL) == MQTTCLIENT_SUCCESS)) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_create()");
                // Set callbacks
                if(result &= (MQTTClient_setCallbacks(MQTTclient, NULL, mqtt_disconnect_callback, mqtt_message_callback, mqtt_delivery_callback) == MQTTCLIENT_SUCCESS)) {
                    LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_setCallbacks()");
                    // Connection options
                    MQTTClient_connectOptions connect_options = MQTTClient_connectOptions_initializer;
                    connect_options.connectTimeout = MQTT_TIMEOUT;
                    // Get authorization data
                    if(APP_CFG.mqtt.username && APP_CFG.mqtt.username[0]) {
                        connect_options.username = APP_CFG.mqtt.username;
                        LOGGER(LOGGER_LEVEL_INFO, "Username: %s", connect_options.username);
                        if(APP_CFG.mqtt.password && APP_CFG.mqtt.password[0]) {
                            connect_options.password = APP_CFG.mqtt.password;
                            LOGGER(LOGGER_LEVEL_INFO, "Password: %s", "<hidden>");
                        } else LOGGER(LOGGER_LEVEL_INFO, "%s success.", "Password: %s", "<not_set> (shared connection)");
                    } else LOGGER(LOGGER_LEVEL_INFO, "%s success.", "Username: %s", "<not_set> (anonymous connection)");
                    // LWT
                    MQTTClient_willOptions lwt_options = MQTTClient_willOptions_initializer;
                    char *state_topic = mqtt_fulltopic(MQTT_STATE_TOPIC);
                    lwt_options.topicName = state_topic;
                    lwt_options.message = MQTT_STATE_OFFLINE;
                    lwt_options.retained = APP_CFG.mqtt.retain;
                    lwt_options.qos = APP_CFG.mqtt.qos;
                    connect_options.will = &lwt_options;
                    // Connection to server
                    if(result &= (MQTTClient_connect(MQTTclient, &connect_options) == MQTTCLIENT_SUCCESS)) {
                        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_connect()");
                        // Subscribe to topic
                        char *command_topic = mqtt_fulltopic(MQTT_COMMAND_TOPIC);
                        if(result &= (MQTTClient_subscribe(MQTTclient, command_topic, true) == MQTTCLIENT_SUCCESS)) {
                            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_subscribe(MQTT_COMMAND_TOPIC)");
                            // Periodic data sending
                            if(result &= (pthread_create(&periodical_thread, NULL, mqtt_periodical, (void *) true) == 0)) {
                                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_create(periodical_thread)");
                            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(periodical_thread)");
                        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_subscribe(MQTT_COMMAND_TOPIC)");
                        free(command_topic);
                    } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_connect()");
                    free(state_topic);
                } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_setCallbacks()");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "MQTTClient_create()");
            free(client_id);
            free(server_address);
        } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "asprintf(server_address)");
        
        // Reconnect (only for first init)
        if((result == false) && (first_init == true)) {
            if(pthread_create(&reconnection_thread, NULL, mqtt_reconnection, NULL) == 0) {
                LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_create(reconnection_thread)");
            } else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "pthread_create(reconnection_thread)");
        }
    } else LOGGER(LOGGER_LEVEL_INFO, "MQTT is disabled in the settings or server address not set.");
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Init mqtt (global)
bool mqtt_init() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    result &= mqtt_initialization(true);
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

// Check connection
bool mqtt_is_connected() {
    return !!MQTTClient_isConnected(MQTTclient);
}

// Check ready
bool mqtt_is_ready() {
    return (mqtt_is_enabled() && mqtt_is_connected());
}

// Free mqtt
bool mqtt_free(bool force) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    bool result = true;
    
    if(mqtt_is_enabled()) { // If MQTT enabled
    
        // Stop reconnection
        if(force) {
            if(reconnection_thread) {
                if(result &= (pthread_cancel(reconnection_thread) == 0)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_cancel(reconnection_thread)");
                else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(reconnection_thread)");
            }
        }
        
        // Stop periodic data sending
        if(periodical_thread) {
            if(result &= (pthread_cancel(periodical_thread) == 0)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "pthread_cancel(periodical_thread)");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "pthread_cancel(periodical_thread)");
        }
        
        if(mqtt_is_connected()) {
            // Unsubscribe
            if(result &= (MQTTClient_unsubscribe(MQTTclient, mqtt_fulltopic(MQTT_COMMAND_TOPIC)) == MQTTCLIENT_SUCCESS)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_unsubscribe(MQTT_COMMAND_TOPIC)");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "MQTTClient_unsubscribe(MQTT_COMMAND_TOPIC)");
            
            // Send offline state
            if(force) {
                char *state_topic = mqtt_fulltopic(MQTT_STATE_TOPIC);
                if(result &= mqtt_send(state_topic, MQTT_STATE_OFFLINE)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_send(MQTT_STATE_TOPIC)");
                else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_send(MQTT_STATE_TOPIC)");
                free(state_topic);
            }
            
            // Disconnect
            if(result &= (MQTTClient_disconnect(MQTTclient, MQTT_TIMEOUT * 1000) == MQTTCLIENT_SUCCESS)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "MQTTClient_disconnect()");
            else LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "MQTTClient_disconnect()");
        }
        // Destroy
        MQTTClient_destroy(&MQTTclient);
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result ? "true" : "false"));
    return result;
}

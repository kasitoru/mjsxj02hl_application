#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdbool.h>

#define MQTT_CLIENT_ID           "MJSXJ02HL"
#define MQTT_DISCONNECT_TIMEOUT  10000
#define MQTT_RECONNECT_INTERVAL  60
#define MQTT_PERIODICAL_INTERVAL 300

#define MQTT_INFO_TOPIC          "info"
#define MQTT_ALARM_TOPIC         "alarm"
#define MQTT_NIGHT_TOPIC         "night"
#define MQTT_COMMAND_TOPIC       "cmd"

// Init mqtt
bool mqtt_init();

// Is enabled
bool mqtt_is_enabled();

// Check connection
bool mqtt_is_connected();

// Check ready
bool mqtt_is_ready();

// Free mqtt
bool mqtt_free(bool force);

// Get full topic
char* mqtt_fulltopic(char *topic);

// Send data
bool mqtt_send(char *topic, char *payload);

// Send formatted data
bool mqtt_sendf(char *topic, const char *format, ...);

#endif

#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdbool.h>

#define MQTT_TIMEOUT             5

#define MQTT_INFO_TOPIC          "info"
#define MQTT_ALARM_TOPIC         "alarm"
#define MQTT_NIGHT_TOPIC         "night"
#define MQTT_COMMAND_TOPIC       "cmd"
#define MQTT_STATE_TOPIC         "state"

#define MQTT_STATE_ONLINE        "online"
#define MQTT_STATE_OFFLINE       "offline"

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
char *mqtt_fulltopic(const char *topic);

// Prepare string for use as MQTT paths/names
char *mqtt_prepare_string(const char *string);

// Get clien id
char *mqtt_client_id();

// Send data
bool mqtt_send(const char *topic, char *payload);

// Send formatted data
bool mqtt_sendf(const char *topic, const char *format, ...);

#endif

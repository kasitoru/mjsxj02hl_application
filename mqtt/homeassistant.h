#ifndef _MQTT_HOMEASSISTANT_H_
#define _MQTT_HOMEASSISTANT_H_

#include <stdbool.h>

#define MQTT_HOMEASSISTANT_DISCOVERY     "homeassistant"
#define MQTT_HOMEASSISTANT_MANUFACTURER  "Xiaomi"
#define MQTT_HOMEASSISTANT_MODEL         "Mi Home Security Camera 1080P (MJSXJ02HL)"

#define MQTT_HOMEASSISTANT_SENSOR        1
#define MQTT_HOMEASSISTANT_BINARY_SENSOR 2

// Add device to Home Assistant over discovery protocol
bool mqtt_homeassistant_discovery(int type, char *topic_name, char *json_field, char *device_class, bool enabled);

#endif

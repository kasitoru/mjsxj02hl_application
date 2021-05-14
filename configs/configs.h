#ifndef _CONFIGS_H_
#define _CONFIGS_H_

#include <stdbool.h>

// All configurations
typedef struct {
    // [logger]
    struct {
        int level;
    } logger;

    // [video]
    struct {
        bool flip;
        bool mirror;
    } video;
    
    // [speaker]
    struct {
        int volume;
    } speaker;
    
    // [alarm]
    struct {
        int motion;
        int humanoid;
        int timeout;
    } alarm;
    
    // [mqtt]
    struct {
        char* server;
        int port;
        char* username;
        char* password; 
        char* topic;
        int qos;
        int retain;
    } mqtt;
} APPLICATION_CONFIGURATION;

// Global configuration variable
extern APPLICATION_CONFIGURATION APP_CFG;

// Init application configs
bool configs_init(char* filename);

// Free application configs
bool configs_free();

#endif

#ifndef _CONFIGS_H_
#define _CONFIGS_H_

#include <stdbool.h>

// All configurations
typedef struct {
    // [logger]
    struct {
        int level;
        char* file;
    } logger;

    // [video]
    struct {
        int type;
        int fps;
        bool flip;
        bool mirror;
    } video;

    // [audio]
    struct {
        int volume;
    } audio;
    
    // [speaker]
    struct {
        int volume;
        int type;
    } speaker;
    
    // [alarm]
    struct {
        int motion_sens;
        int humanoid_sens;
        int motion_timeout;
        int humanoid_timeout;
        char* motion_detect_exec;
        char* humanoid_detect_exec;
        char* motion_lost_exec;
        char* humanoid_lost_exec;
    } alarm;
    
    // [rtsp]
    struct {
        int port;
    } rtsp;
    
    // [mqtt]
    struct {
        char* server;
        int port;
        char* username;
        char* password; 
        char* topic;
        int qos;
        bool retain;
    } mqtt;
    
    // [night]
    struct {
        int mode;
        int gray;
    } night;
    
} APPLICATION_CONFIGURATION;

// Global configuration variable
extern APPLICATION_CONFIGURATION APP_CFG;

// Init application configs
bool configs_init(char* filename);

// Free application configs
bool configs_free();

#endif

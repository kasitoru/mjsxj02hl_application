#ifndef _CONFIGS_H_
#define _CONFIGS_H_

#include <stdbool.h>

// All configurations
typedef struct {

    // [general]
    struct {
        bool led;
    } general;

    // [logger]
    struct {
        int level;
        char* file;
    } logger;

    // [osd]
    struct {
        bool enable;
        bool oem_logo;
        bool date_time;
        bool motion;
        bool humanoid;
    } osd;

    // [video]
    struct {
        bool flip;
        bool mirror;
        bool primary_enable;
        bool secondary_enable;
        int primary_type;
        int secondary_type;
        int primary_bitrate;
        int secondary_bitrate;
        int primary_rcmode;
        int secondary_rcmode;
    } video;

    // [audio]
    struct {
        int volume;
        bool primary_enable;
        bool secondary_enable;
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
        bool enable;
        int port;
        char* username;
        char* password;
        char* primary_name;
        char* secondary_name;
        bool primary_multicast;
        bool secondary_multicast;
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

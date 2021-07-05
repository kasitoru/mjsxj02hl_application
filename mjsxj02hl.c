#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sysexits.h>
#include <string.h>

#include "./logger/logger.h"
#include "./localsdk/localsdk.h"
#include "./localsdk/init.h"
#include "./configs/configs.h"
#include "./mqtt/mqtt.h"
#include "./rtsp/rtsp.h"

// Signal callback
void signal_callback(int signal) {
    logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_DEBUG, "Function is called...");

    // Enable orange pin
    if(local_sdk_indicator_led_option(true, false) == LOCALSDK_OK) {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_indicator_led_option()");
    } else {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_indicator_led_option()");
        signal = EX_SOFTWARE;
    }

    // MQTT free
    if(mqtt_free()) {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_INFO, "%s success.", "mqtt_free()");
    } else {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_WARNING, "%s error!", "mqtt_free()");
        signal = EX_SOFTWARE;
    }

    // All free
    if(all_free()) {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_INFO, "%s success.", "all_free()");
    } else {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_WARNING, "%s error!", "all_free()");
        signal = EX_SOFTWARE;
    }

    // RTSP free
    if(rtsp_free()) {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_INFO, "%s success.", "rtsp_free()");
    } else {
        logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_WARNING, "%s error!", "rtsp_free()");
        signal = EX_SOFTWARE;
    }

    // Exit
    logger("mjsxj02hl_application", "signal_callback", LOGGER_LEVEL_DEBUG, "Function completed.");
    exit(signal);
}

// Factory reset callback
int factory_reset_callback() {
    int result = LOCALSDK_OK;
    logger("mjsxj02hl_application", "factory_reset_callback", LOGGER_LEVEL_DEBUG, "Function is called...");
    
    if(system("mjsxj02hl --factory-reset") == EX_OK) {
        logger("mjsxj02hl_application", "factory_reset_callback", LOGGER_LEVEL_INFO, "%s success.", "system(\"mjsxj02hl --factory-reset\")");
    } else logger("mjsxj02hl_application", "factory_reset_callback", LOGGER_LEVEL_ERROR, "%s error!", "system(\"mjsxj02hl --factory-reset\")");
    
    logger("mjsxj02hl_application", "factory_reset_callback", LOGGER_LEVEL_DEBUG, "Function completed.");
    return LOCALSDK_ERROR;
}

// Main function
int main(int argc, char **argv) {
    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_DEBUG, "Function is called...");

    // Default path of config file
    char *config_filename = "/usr/app/share/mjsxj02hl.conf";

    // Running with arguments
    if(argc > 1) {
        if(strcmp(argv[1], "--config") == 0) { // Set config path
            if(argc == 3) {
                config_filename = argv[2];
            } else {
                printf("Error: missing filename! Use the --help option for more information.\n");
                return EX_USAGE;
            }
        } else if(strcmp(argv[1], "--factory-reset") == 0) { // Factory reset
            printf("Reset to factory settings...\n");
            system("rm -rf /configs/*");
            system("touch /configs/mjsxj02hl.conf");
            system("chmod 644 /configs/mjsxj02hl.conf");
            system("reboot");
            return EX_OK;
        } else if(strcmp(argv[1], "--get-image") == 0) { // Get image
            if(argc == 3) {
                if(system("pidof -o %PPID mjsxj02hl > /dev/null") == EX_OK) {
                    if(local_sdk_video_get_jpeg(LOCALSDK_VIDEO_SECONDARY_CHANNEL, argv[2]) == LOCALSDK_OK) {
                        return EX_OK;
                    } else {
                        printf("Error: local_sdk_video_get_jpeg() failed!\n");
                        return EX_SOFTWARE;
                    }
                } else {
                    printf("Error: main thread of mjsxj02hl application is not running!\n");
                    return EX_UNAVAILABLE;
                }
            } else {
                printf("Error: missing filename! Use the --help option for more information.\n");
                return EX_USAGE;
            }
        } else if(strcmp(argv[1], "--help") == 0) { // Help
            printf("Usage: mjsxj02hl [<action> [options...]]\n");
            printf("\n");
            printf("Running without arguments starts the main thread of the application.\n");
            printf("\n");
            printf("  --config <filename>       Specify the location of the configuration file for the main thread of application.\n");
            printf("\n");
            printf("  --factory-reset           Reset device settings to default values. Attention: this action cannot be undone!\n");
            printf("\n");
            printf("  --get-image <filename>    Output the camera image to a file. Requires a running main thread of mjsxj02hl application.\n");
            printf("\n");
            printf("  --help                    Display this message.\n");
            printf("\n");
            printf("Report an error or help in the development of the project you can on the page %s\n", "https://github.com/avdeevsv91/mjsxj02hl_application");
            return EX_OK;
        } else {
            printf("Error: unknown action! Use the --help option for more information.\n");
            return EX_USAGE;
        }
    }
    
    // Firmware version
    char *fw_version = firmware_version();
    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_FORCED, "Firmware version: %s", fw_version);
    free(fw_version);
    
    // Build time
    struct tm compile_time;
    if(strptime(__DATE__ ", " __TIME__, "%b %d %Y, %H:%M:%S", &compile_time) != NULL) {
        int build_time = (int) mktime(&compile_time);
        logger("mjsxj02hl_application", "main", LOGGER_LEVEL_FORCED, "Build time: %d", build_time);
    } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "strptime()");
    
    // Main thread
    if(configs_init(config_filename)) { // Init configs
        logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "configs_init()");
        if(rtsp_init()) { // Init RTSP
            logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "rtsp_init()");
            if(all_init()) { // Init all systems
                logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "all_init()");

                // Register signals
                if(signal(SIGINT, signal_callback) != SIG_ERR) {
                    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "signal(SIGINT)");
                } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "signal(SIGINT)");
                if(signal(SIGABRT, signal_callback) != SIG_ERR) {
                    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "signal(SIGABRT)");
                } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "signal(SIGABRT)");
                if(signal(SIGTERM, signal_callback) != SIG_ERR) {
                    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "signal(SIGTERM)");
                } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "signal(SIGTERM)");
                
                // Enable blue pin
                if(local_sdk_indicator_led_option(false, true) == LOCALSDK_OK) {
                    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_indicator_led_option()");
                } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_indicator_led_option()");
                
                // Factory reset
                if(local_sdk_setup_keydown_set_callback(3000, factory_reset_callback) == LOCALSDK_OK) {
                    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "local_sdk_setup_keydown_set_callback()");
                } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_setup_keydown_set_callback()");
                
                // MQTT
                if(mqtt_init()) {
                    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_INFO, "%s success.", "mqtt_init()");
                } else logger("mjsxj02hl_application", "main", LOGGER_LEVEL_WARNING, "%s error!", "mqtt_init()");
                
                // Endless waiting
                while(true) {
                    sleep(1);
                }
            } else {
                logger("mjsxj02hl_application", "main", LOGGER_LEVEL_ERROR, "%s error!", "all_init()");
                return EX_SOFTWARE;
            }
        } else {
            logger("mjsxj02hl_application", "main", LOGGER_LEVEL_ERROR, "%s error!", "rtsp_init()");
            return EX_SOFTWARE;
        }
    } else {
        logger("mjsxj02hl_application", "main", LOGGER_LEVEL_ERROR, "%s error!", "configs_init()");
        return EX_CONFIG;
    }
    logger("mjsxj02hl_application", "main", LOGGER_LEVEL_DEBUG, "Function completed.");
    return EX__BASE;
}

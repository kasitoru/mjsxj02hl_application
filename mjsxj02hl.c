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
#include "./ipctool/include/ipchw.h"

// Signal callback
void signal_callback(int signal) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Enable orange LED
    if(APP_CFG.general.led) {
        if(local_sdk_indicator_led_option(true, false) == LOCALSDK_OK) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_indicator_led_option(true, false)");
        else {
            LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "local_sdk_indicator_led_option(true, false)");
            signal = EX_SOFTWARE;
        }
    }
    
    // MQTT free
    if(mqtt_free(true)) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_free(true)");
    else {
        LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "mqtt_free(true)");
        signal = EX_SOFTWARE;
    }
    
    // RTSP free
    if(rtsp_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "rtsp_free()");
    else {
        LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "rtsp_free()");
        signal = EX_SOFTWARE;
    }
    
    // All free
    if(all_free()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "all_free()");
    else {
        LOGGER(LOGGER_LEVEL_WARNING, "%s error!", "all_free()");
        signal = EX_SOFTWARE;
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (signal = %d).", signal);
    exit(signal);
}

// Factory reset callback
int factory_reset_callback() {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    int result = LOCALSDK_OK;
    
    if(system("mjsxj02hl --factory-reset") == EX_OK) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "system(mjsxj02hl --factory-reset)");
    else {
        LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "system(mjsxj02hl --factory-reset)");
        result = LOCALSDK_ERROR;
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (result = %s).", (result == LOCALSDK_OK ? "LOCALSDK_OK" : "LOCALSDK_ERROR"));
    return result;
}

// Main function
int main(int argc, char **argv) {
    LOGGER(LOGGER_LEVEL_DEBUG, "Function is called...");
    
    // Initialize pseudo-random number generator
    srand(time(NULL));
    
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
            printf("Report an error or help in the development of the project you can on the page %s\n", "https://github.com/kasitoru/mjsxj02hl_application");
            return EX_OK;
        } else {
            printf("Error: unknown action! Use the --help option for more information.\n");
            return EX_USAGE;
        }
    }
    
    // Firmware version
    char *fw_ver = firmware_version();
    LOGGER(LOGGER_LEVEL_FORCED, "Firmware version: %s", fw_ver);
    free(fw_ver);
    
    // Device id
    const char *dev_id = getchipid();
    LOGGER(LOGGER_LEVEL_FORCED, "Device ID: %s", dev_id);
    free((char *) dev_id);
    
    // Main thread
    if(configs_init(config_filename)) { // Init configs
        LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "configs_init()");
        if(all_init()) { // Init all systems
            LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "all_init()");
            
            // Register signals
            if(signal(SIGINT, signal_callback) != SIG_ERR) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "signal(SIGINT)"); // SIGINT
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "signal(SIGINT)");
            
            if(signal(SIGABRT, signal_callback) != SIG_ERR) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "signal(SIGABRT)"); // SIGABRT
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "signal(SIGABRT)");
            
            if(signal(SIGTERM, signal_callback) != SIG_ERR) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "signal(SIGTERM)"); // SIGTERM
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "signal(SIGTERM)");
            
            // Onboard LED indicator
            if(APP_CFG.general.led) { // Enable blue LED
                if(local_sdk_indicator_led_option(false, true) == LOCALSDK_OK) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_indicator_led_option(false, true)");
                else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_indicator_led_option()");
            } else { // Disable LEDs
                if(local_sdk_indicator_led_option(false, false) == LOCALSDK_OK) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_indicator_led_option(false, false)");
                else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_indicator_led_option()");
            }
            
            // Factory reset callback
            if(local_sdk_setup_keydown_set_callback(3000, factory_reset_callback) == LOCALSDK_OK) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "local_sdk_setup_keydown_set_callback()");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "local_sdk_setup_keydown_set_callback()");
            
            // RTSP server
            if(rtsp_init()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "rtsp_init()");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "rtsp_init()");
            
            // MQTT client
            if(mqtt_init()) LOGGER(LOGGER_LEVEL_DEBUG, "%s success.", "mqtt_init()");
            else LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "mqtt_init()");
            
            // Endless waiting
            while(true) {
                sleep(1);
            }
            
        } else {
            LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "all_init()");
            return EX_SOFTWARE;
        }
    } else {
        LOGGER(LOGGER_LEVEL_ERROR, "%s error!", "configs_init()");
        return EX_CONFIG;
    }
    
    LOGGER(LOGGER_LEVEL_DEBUG, "Function completed (signal = %d).", EX__BASE);
    return EX__BASE;
}

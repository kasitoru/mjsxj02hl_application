#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "./logger.h"
#include "./../localsdk/init.h"
#include "./../configs/configs.h"

// Write message to log
static int logger_write(const int level, const char *format, ...) {
    int result = 0;
    if(level < 0 || level <= APP_CFG.logger.level) {
        va_list params;
        va_start(params, format);
        result = vprintf(format, params);
        // Write to file
        if(APP_CFG.logger.file && APP_CFG.logger.file[0]) {
            FILE *file;
            if(file = fopen(APP_CFG.logger.file, "a")) {
                vfprintf(file, format, params);
                fclose(file);
            }
        }
        va_end(params);
    }
    return result;
}

// Add message to log
int logger(const char *file, const int line, const char *function, const int level, char *format, ...) {
    int result = 0;
    // Trim non-printable symbols
    char *frmt_buffer = prepare_string(format);
    // Date & Time
    time_t rawtime;
    char datetime[18];
    memset(datetime, '\0', sizeof(datetime));
    if(time(&rawtime) != -1) {
        struct tm *timeinfo;
        if(timeinfo = localtime(&rawtime)) {
            strftime(datetime, sizeof(datetime), "%x %X", timeinfo);
        }
    }
    // Nanoseconds
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    // Print message
    va_list params;
    va_start(params, format);
    char *message = "";
    if(vasprintf(&message, frmt_buffer, params) != -1) {
        result = logger_write(level, "[%s.%09d] [%s:%d][%s]: %s\n", datetime, spec.tv_nsec, file, line, function, message);
        free(frmt_buffer);
        free(message);
    }
    va_end(params);
    
    return result;
}

// Hexdump of memory
int logger_memory(const char *description, const int level, const void *address, const int length, int width) {
    // https://stackoverflow.com/a/7776146
    int result = 0;
    // Check length
    if(length <= 0) {
        result += logger_write(level, "[%s][%s]: incorrect length = %d!\n", "logger", "logger_memory", length);
        return result;
    }
    // Check width
    if(width < 4 || width > 64) width = 16;
    // Output description if given
    if(description != NULL) {
        result += logger_write(level, "%s:\n", description);
    }
    // Process every byte in the data
    int i;
    unsigned char buffer[width+1];
    const unsigned char *chars = (const unsigned char *) address;
    for(i=0;i<length;i++) {
        // Multiple of perLine means new or first line (with line offset)
        if((i % width) == 0) {
            // Only print previous-line ASCII buffer for lines beyond first
            if(i != 0) {
                result += logger_write(level, "  %s\n", buffer);
            }
            // Output the offset of current line
            result += logger_write(level, "  %04x ", i);
        }
        // Now the hex code for the specific character
        result += logger_write(level, " %02x", chars[i]);
        // And buffer a printable ASCII character for later
        if((chars[i] < 0x20) || (chars[i] > 0x7e)) {
            buffer[i % width] = '.';
        } else {
            buffer[i % width] = chars[i];
        }
        buffer[(i % width) + 1] = '\0';
    }
    // Pad out last line if not exactly perLine characters
    while((i % width) != 0) {
        result += logger_write(level, "   ");
        i++;
    }
    // And print the final ASCII buffer
    result += logger_write(level, "  %s\n", buffer);
    return result;
}

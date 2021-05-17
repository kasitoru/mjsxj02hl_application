#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "./logger.h"
#include "./../configs/configs.h"

// Add message to log
int logger(const char *module, const char *function, const int level, const char *format, ...) {
    int result = 0;
    if(level <= APP_CFG.logger.level) {
        char *template;
        if(asprintf(&template, "[%s][%s]: %s\n", module, function, format) > 0) {
            va_list params;
            va_start(params, format);
            result = vprintf(template, params);
            if(APP_CFG.logger.file && APP_CFG.logger.file[0]) {
                FILE *file;
                if(file = fopen(APP_CFG.logger.file, "a")) {
                    vfprintf(file, template, params);
                    fclose(file);
                }
            }
            va_end(params);
            free(template);
        }
    }
    return result;
}

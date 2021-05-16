#include <stdbool.h>

#include "./night.h"
#include "./../localsdk.h"
#include "./../../logger/logger.h"

// Init night mode
bool night_init() {
    return true;
}

// Free night mode
bool night_free() {
    logger("night", "night_free", LOGGER_LEVEL_DEBUG, "Function is called...");
    logger("night", "night_free", LOGGER_LEVEL_DEBUG, "Function completed.");
    return true;
}


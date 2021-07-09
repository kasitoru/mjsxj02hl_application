#ifndef _LOCALSDK_NIGHT_H_
#define _LOCALSDK_NIGHT_H_

#include <stdbool.h>

#define NIGHT_MODE_STATE_NIGHTTIME 0
#define NIGHT_MODE_STATE_DAYTIME   1
#define NIGHT_MODE_STATE_DISABLE   2
#define NIGHT_MODE_STATE_ENABLE    3

// Init night mode
bool night_init();

// Free night mode
bool night_free();

#endif

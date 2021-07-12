#ifndef _LOCALSDK_AUDIO_H_
#define _LOCALSDK_AUDIO_H_

#include <stdbool.h>

// Is enabled
bool audio_is_enabled(int channel);

// Init audio
bool audio_init();

// Free audio
bool audio_free();

#endif

#ifndef _LOCALSDK_VIDEO_H_
#define _LOCALSDK_VIDEO_H_

#include <stdbool.h>

// Is enabled
bool video_is_enabled(int channel);

// Init video
bool video_init();

// Free video
bool video_free();

#endif

#ifndef _LOCALSDK_SPEAKER_H_
#define _LOCALSDK_SPEAKER_H_

#include <stdbool.h>

#define SPEAKER_MEDIA_STOPPED  0
#define SPEAKER_MEDIA_PLAYING  1
#define SPEAKER_MEDIA_STOPPING 2

// Init speaker
bool speaker_init();

// Free speaker
bool speaker_free();

// Play media (WAV, 8000 hz, 16-bit, mono)
bool speaker_play_media(char *filename, int type);

// Get playback status
int speaker_status_media();

// Stop playback
bool speaker_stop_media();

// Set volume
bool speaker_set_volume(int value);

// Get volume
int speaker_get_volume();

#endif

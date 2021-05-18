#ifndef _SPEAKER_H_
#define _SPEAKER_H_

#include <stdbool.h>

// Init speaker
bool speaker_init();

// Free speaker
bool speaker_free();

// Play media (WAV, 8000 hz, 16-bit, mono)
bool speaker_play_media(char *filename);

// Set volume
bool speaker_set_volume(int value);

// Get volume
int speaker_get_volume();

#endif

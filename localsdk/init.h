#ifndef _LOCALSDK_INIT_H_
#define _LOCALSDK_INIT_H_

#include <stdbool.h>

// Get firmware version
char *firmware_version();

// Removes all non-printable characters from string
char *prepare_string(char *string);

// Init all
bool all_init();

// Free all
bool all_free();

#endif

#ifndef _LOCALSDK_OSD_H_
#define _LOCALSDK_OSD_H_

#include <stdbool.h>

#include "./../localsdk.h"


// Init OSD
bool osd_init();

// Init OSD after video init
bool osd_postinit();

// Free OSD
bool osd_free();

// Rectangles callback
int osd_rectangles_callback(LOCALSDK_ALARM_EVENT_INFO *eventInfo);

#endif

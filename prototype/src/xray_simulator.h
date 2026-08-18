#ifndef XRAY_SIMULATOR_H
#define XRAY_SIMULATOR_H

#include <stdbool.h>

typedef struct { unsigned char *pixels; int w,h; } xray_frame_t;

void xray_init();
void xray_shutdown();
void xray_render_current(xray_frame_t *out);

#endif

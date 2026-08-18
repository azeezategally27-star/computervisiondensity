#include "xray_simulator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// produces a grayscale synthetic radiograph 256x256
void xray_init(){ }
void xray_shutdown(){ }

void xray_render_current(xray_frame_t *out){
    int w = 256, h = 256;
    out->w = w; out->h = h;
    out->pixels = malloc(w*h);
    if(!out->pixels) return;
    // simple procedural X-ray: background gradient + random shapes representing items
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            float gx = (float)x / (w-1);
            unsigned char val = (unsigned char)(180 + 30*(0.5f - gx));
            out->pixels[y*w + x] = val;
        }
    }
    // draw a few darker blobs to simulate metal items
    for(int k=0;k<3;k++){
        int cx = 40 + k*60;
        int cy = 60 + k*40;
        int rad = 18 + k*6;
        for(int y=cy-rad;y<=cy+rad;y++){
            if(y<0||y>=h) continue;
            for(int x=cx-rad;x<=cx+rad;x++){
                if(x<0||x>=w) continue;
                int dx = x-cx, dy=y-cy;
                if(dx*dx+dy*dy <= rad*rad){ out->pixels[y*w + x] = 30 + (k*20); }
            }
        }
    }
}

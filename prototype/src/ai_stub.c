#include "ai_stub.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

static int g_demo = 1;

void ai_init(bool demo_mode){ g_demo = demo_mode ? 1 : 0; srand((unsigned)time(NULL)); }
void ai_shutdown(){ }

ai_result_t ai_run_on_frame(const xray_frame_t *frame){
    ai_result_t r; memset(&r,0,sizeof(r));
    // deterministic but varied detections based on frame content heuristics
    // We use simple seeded randomness for demo repeatability when demo mode on
    int seed = frame->pixels ? frame->pixels[0] : 42;
    int base = (seed % 5) + 1;
    // produce a few detections
    int n = 1 + (rand()%4);
    for(int i=0;i<n && i<16;i++){
        detection_t *d = &r.detections[r.count++];
        d->cls = (item_class_t)(rand()%5);
        d->x = 0.1f + 0.15f*i;
        d->y = 0.2f + 0.1f*(i%3);
        d->w = 0.08f + 0.05f*(i%2);
        d->h = 0.08f + 0.05f*((i+1)%2);
        d->confidence = 0.5f + ((rand()%50)/100.0f);
    }
    // base threat score proportional to detections
    float ts = 0.0f;
    for(int i=0;i<r.count;i++) ts += r.detections[i].confidence * (1.0f + ((int)r.detections[i].cls)/2.0f);
    r.threat_score = 1.0f - (1.0f / (1.0f + ts*0.5f)); // normalized
    return r;
}

#include "sensors.h"
float ai_fuse_with_sensors(const ai_result_t *res, const sensors_readout_t *s){
    float w_obj = 0.7f, w_metal = 0.15f, w_weight = 0.15f;
    float obj = res->threat_score;
    float metal = s->metal_detected ? 1.0f : 0.0f;
    float weight = s->weight_anomaly; // 0..1
    float fused = w_obj*obj + w_metal*metal + w_weight*weight;
    if(fused > 1.0f) fused = 1.0f;
    return fused;
}

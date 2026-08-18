#ifndef AI_STUB_H
#define AI_STUB_H

#include <stdbool.h>
#include "xray_simulator.h"

typedef enum { CLASS_FIREARM, CLASS_KNIFE, CLASS_LIQUID, CLASS_BATTERY, CLASS_IED } item_class_t;

typedef struct {
    item_class_t cls;
    float x,y,w,h; // normalized bbox
    float confidence;
} detection_t;

typedef struct {
    detection_t detections[16];
    int count;
    float threat_score;
} ai_result_t;

void ai_init(bool demo_mode);
void ai_shutdown();
ai_result_t ai_run_on_frame(const xray_frame_t *frame);
float ai_fuse_with_sensors(const ai_result_t *res, const struct sensors_readout_t *s);

#endif

#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_logdir[512] = "prototype/evidence";

void logging_init(const char *dir){ if(dir) strncpy(g_logdir, dir, sizeof(g_logdir)-1); mkdir(g_logdir, 0700); }
void logging_shutdown(){ }

void logging_save_evidence(const xray_frame_t *frame, const ai_result_t *res){
    static int id = 0; char path[1024];
    snprintf(path, sizeof(path), "%s/evidence_%03d.pgm", g_logdir, id++);
    FILE *f = fopen(path, "wb");
    if(!f) return;
    fprintf(f, "P5\n%d %d\n255\n", frame->w, frame->h);
    fwrite(frame->pixels, 1, frame->w*frame->h, f);
    fclose(f);
    printf("[LOG] Saved evidence to %s (detections=%d, threat=%.2f)\n", path, res->count, res->threat_score);
    free((void*)frame->pixels);
}

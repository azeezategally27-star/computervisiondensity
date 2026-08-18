#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

static ai_result_t last_results[8];
static int last_count = 0;

void ui_init(){ last_count = 0; }
void ui_shutdown(){ }
void ui_handle_event(SDL_Event *ev){ }

void ui_push_detection(const ai_result_t *res){
    if(last_count < 8) last_results[last_count++] = *res; else { for(int i=1;i<8;i++) last_results[i-1]=last_results[i]; last_results[7]=*res; }
    // simple console logging
    printf("[UI] Detection: %d objects, threat=%.2f\n", res->count, res->threat_score);
}

void ui_frame(){
    // for the demo we just print latest detections to stdout; a full UI would render SDL/OpenGL overlays
    for(int i=0;i<last_count;i++){
        ai_result_t *r = &last_results[i];
        printf("  Entry %d: threat=%.2f, objects=%d\n", i, r->threat_score, r->count);
    }
}

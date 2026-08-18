#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include "ai_stub.h"

void ui_init();
void ui_shutdown();
void ui_frame();
void ui_handle_event(SDL_Event *ev);
void ui_push_detection(const ai_result_t *res);

#endif

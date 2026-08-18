#ifndef LOGGING_H
#define LOGGING_H

#include "xray_simulator.h"
#include "ai_stub.h"

void logging_init(const char *dir);
void logging_shutdown();
void logging_save_evidence(const xray_frame_t *frame, const ai_result_t *res);

#endif

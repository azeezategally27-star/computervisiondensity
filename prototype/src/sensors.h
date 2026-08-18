#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

typedef struct { bool metal_detected; float weight_anomaly; } sensors_readout_t;

void sensors_init();
void sensors_shutdown();
void sensors_update();
sensors_readout_t sensors_get_readout();

#endif

#include "sensors.h"
#include <stdlib.h>
#include <time.h>

static sensors_readout_t g_readout;

void sensors_init(){ srand((unsigned)time(NULL)); g_readout.metal_detected = false; g_readout.weight_anomaly = 0.0f; }
void sensors_shutdown(){ }

void sensors_update(){
    // simple simulated sensors: random metal spikes and occasional weight anomalies
    int r = rand()%1000;
    g_readout.metal_detected = (r < 60); // ~6% chance per frame
    if(rand()%500 < 10) g_readout.weight_anomaly = 0.6f + (rand()%40)/100.0f; else g_readout.weight_anomaly *= 0.9f;
}

sensors_readout_t sensors_get_readout(){ return g_readout; }

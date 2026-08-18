#include "conveyor.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int g_tick = 0;
static int g_bag_pos = 0;
static int g_hold = 0;

void conveyor_init(){ g_tick = 0; g_bag_pos = -100; g_hold = 0; }
void conveyor_shutdown(){ }

void conveyor_load_scenario(const char *path){
    // simple loader: file lines ignored for demo; could parse bag compositions
    FILE *f = fopen(path, "r"); if(!f) return; fclose(f);
}

void conveyor_update(){ if(g_hold) return; g_bag_pos += 2; if(g_bag_pos > 300) g_bag_pos = -200; g_tick++; }

bool conveyor_bag_in_tunnel(){ // return true when bag passes z in [ -20 .. 20 ]
    return (g_bag_pos > -20 && g_bag_pos < 20);
}

void conveyor_hold_current(){ g_hold = 1; }

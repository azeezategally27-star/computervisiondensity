#ifndef CONVEYOR_H
#define CONVEYOR_H

#include <stdbool.h>

void conveyor_init();
void conveyor_shutdown();
void conveyor_update();
void conveyor_load_scenario(const char *path);
bool conveyor_bag_in_tunnel();
void conveyor_hold_current();

#endif

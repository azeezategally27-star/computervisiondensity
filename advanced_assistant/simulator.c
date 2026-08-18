// simulator.c - simulates live flight changes and luggage updates
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "data.h"

static pthread_t sim_thread;
static int sim_running = 0;
static AppWidgets *g_app = NULL;

static void *sim_loop(void *arg){
    g_app = (AppWidgets*)arg;
    srand(time(NULL));
    while(sim_running){
        // randomly pick a flight and change status/gate/luggage
        FlightList *fl = data_get_all();
        if(fl->n>0){
            int i = rand()%fl->n;
            int id = fl->items[i].id;
            int r = rand()%100;
            if(r<20){
                data_update_flight(id, fl->items[i].scheduled, "Delayed", fl->items[i].gate);
            } else if(r<35){
                // gate change
                char newg[4]; snprintf(newg,4, "%c%d", 'A'+(rand()%6), 1 + rand()%9);
                data_update_flight(id, fl->items[i].scheduled, "Gate Changed", newg);
            } else if(r<50){
                // boarding
                data_update_flight(id, fl->items[i].scheduled, "Boarding", fl->items[i].gate);
            } else if(r<65){
                // luggage update
                const char *lbs[] = {"OK","Delayed","Loading","Hold"};
                data_set_luggage(id, lbs[rand()%4]);
            }
            // inform UI to refresh on GTK main loop
            g_idle_add((GSourceFunc)ui_refresh_list, NULL);
        }
        for(int j=0;j<fl->n;j++){ free(fl->items[j].flight_no); free(fl->items[j].dest); free(fl->items[j].scheduled); free(fl->items[j].status); free(fl->items[j].gate); free(fl->items[j].luggage); }
        free(fl->items); free(fl);
        sleep(3);
    }
    return NULL;
}

void simulator_start(AppWidgets *app){
    if(sim_running) return;
    sim_running = 1;
    pthread_create(&sim_thread, NULL, sim_loop, app);
}
void simulator_stop(){
    if(!sim_running) return;
    sim_running = 0; pthread_join(sim_thread, NULL);
}

// main.c - Flight Assistant Panel (GTK3)
#include <gtk/gtk.h>
#include "data.h"
#include "ui.h"
#include "simulator.h"
#include "api_server.h"

int main(int argc, char *argv[]){
    gtk_init(&argc, &argv);

    // initialize DB
    if(!data_init("flight_assistant.db")){
        g_printerr("Failed to initialize DB\n");
        return 1;
    }

    // load or seed flights
    data_seed_if_empty();

    // build UI
    AppWidgets *app = ui_build();

    // start simulator thread (simulates live updates from airport)
    simulator_start(app);

    // start local API server (for rescheduling via network calls)
    api_server_start();

    gtk_main();

    api_server_stop();
    simulator_stop();
    data_close();
    return 0;
}

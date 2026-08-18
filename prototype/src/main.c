#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "renderer.h"
#include "xray_simulator.h"
#include "ai_stub.h"
#include "sensors.h"
#include "conveyor.h"
#include "ui.h"
#include "logging.h"

int main(int argc, char **argv){
    bool demo = true;
    const char *scenario = "assets/sample_scenario.txt";
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--nodemo")==0) demo = false;
        if(strcmp(argv[i],"--scenario")==0 && i+1<argc) scenario = argv[++i];
    }

    // init systems
    if(!renderer_init()){
        fprintf(stderr, "Renderer init failed\n");
        return 1;
    }
    logging_init("prototype/evidence");
    sensors_init();
    conveyor_init();
    xray_init();
    ai_init(demo); // demo mode: simulated detections
    ui_init();

    // load scenario
    conveyor_load_scenario(scenario);

    bool running = true;
    Uint32 last = SDL_GetTicks();
    while(running){
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type == SDL_QUIT) running = false;
            if(ev.type == SDL_KEYDOWN){
                if(ev.key.keysym.sym == SDLK_ESCAPE) running = false;
            }
            ui_handle_event(&ev);
        }

        // update simulated sensors & conveyor
        sensors_update();
        conveyor_update();

        // when bag in tunnel, produce xray and run ai
        if(conveyor_bag_in_tunnel()){
            xray_frame_t frame;
            xray_render_current(&frame);
            ai_result_t res = ai_run_on_frame(&frame);
            // sensor fusion
            sensors_readout_t s = sensors_get_readout();
            res.threat_score = ai_fuse_with_sensors(&res, &s);

            // UI and actions
            ui_push_detection(&res);
            if(res.threat_score > 0.75){
                logging_save_evidence(&frame, &res);
                // simulate countermeasure: stop conveyor
                conveyor_hold_current();
            }
        }

        // render scene and UI
        renderer_frame();
        ui_frame();

        // simple timing
        Uint32 now = SDL_GetTicks();
        Uint32 dt = now - last;
        if(dt < 16) SDL_Delay(16 - dt);
        last = SDL_GetTicks();
    }

    ui_shutdown();
    ai_shutdown();
    xray_shutdown();
    conveyor_shutdown();
    sensors_shutdown();
    logging_shutdown();
    renderer_shutdown();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "renderer.h"
#include "gps.h"

int main(int argc, char **argv){
    bool use_sim = true;
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--gpsd")==0) use_sim = false;
        if(strcmp(argv[i],"--sim")==0) use_sim = true;
    }

    // initialize GPS (simulated by default)
    gps_state_t gps;
    gps_init(&gps);
    if(use_sim){
        if(!gps_start_simulation(&gps, "assets/sample_gps.txt")){
            fprintf(stderr, "Failed to start GPS simulation.\n");
        }
    } else {
        fprintf(stderr, "GPSD mode not implemented in this prototype; falling back to simulation.\n");
        if(!gps_start_simulation(&gps, "assets/sample_gps.txt")){
            fprintf(stderr, "Failed to start GPS simulation.\n");
        }
    }

    // initialize renderer
    if(!renderer_init()){
        fprintf(stderr, "Failed to initialize renderer.\n");
        return 1;
    }

    // main loop
    bool running = true;
    Uint32 last = SDL_GetTicks();
    while(running){
        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type == SDL_QUIT) running = false;
            if(ev.type == SDL_KEYDOWN){
                if(ev.key.keysym.sym == SDLK_ESCAPE) running = false;
            }
        }

        // read GPS position
        double lat, lon;
        gps_get_position(&gps, &lat, &lon);

        // update renderer avatar position
        renderer_set_avatar_geoposition(lat, lon);

        // draw
        renderer_frame();

        // cap ~60fps
        Uint32 now = SDL_GetTicks();
        Uint32 dt = now - last;
        if(dt < 16) SDL_Delay(16 - dt);
        last = SDL_GetTicks();
    }

    renderer_shutdown();
    gps_shutdown(&gps);
    return 0;
}

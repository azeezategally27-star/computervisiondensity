#include "renderer.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

static SDL_Window *win = NULL;
static SDL_GLContext glctx = NULL;

bool renderer_init(){
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    win = SDL_CreateWindow("Xray Security Metaverse - Demo",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           1280, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!win){ fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); return false; }
    glctx = SDL_GL_CreateContext(win);
    if(!glctx){ fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError()); return false; }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.42f, 0.25f, 0.85f, 1.0f); // purple background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1280.0/800.0, 0.1, 2000.0);
    glMatrixMode(GL_MODELVIEW);
    return true;
}

void renderer_shutdown(){ if(glctx) SDL_GL_DeleteContext(glctx); if(win) SDL_DestroyWindow(win); SDL_Quit(); }

// simple demo scene: conveyor and tunnel are drawn by other modules via shared state
void renderer_frame(){
    int w,h; SDL_GetWindowSize(win,&w,&h);
    glViewport(0,0,w,h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(0,200,300, 0,0,0, 0,1,0);

    // ground
    glColor3f(0.95f, 0.95f, 0.98f);
    glBegin(GL_QUADS);
    glVertex3f(-500,-1,-500); glVertex3f(500,-1,-500); glVertex3f(500,-1,500); glVertex3f(-500,-1,500);
    glEnd();

    // conveyor base
    glColor3f(0.2f,0.2f,0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-150,0,-200); glVertex3f(150,0,-200); glVertex3f(150,0,200); glVertex3f(-150,0,200);
    glEnd();

    // X-ray tunnel
    glPushMatrix();
    glTranslatef(0,30,0);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-60, -30, -20); glVertex3f(60, -30, -20); glVertex3f(60, 30, -20); glVertex3f(-60, 30, -20);
    glVertex3f(-60, -30, 20); glVertex3f(60, -30, 20); glVertex3f(60, 30, 20); glVertex3f(-60, 30, 20);
    glEnd();
    glPopMatrix();

    // Note: detailed animated objects are rendered by conveyor/xray modules using OpenGL immediate mode hooks in a full implementation.

    SDL_GL_SwapWindow(win);
}

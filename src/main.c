// trolley_app - Raylib C prototype for Mauritius airport trolley processes
// Build: make in src/ (requires raylib installed)

#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MAX_TROLLEYS 64

typedef enum { STATE_IDLE=0, STATE_MOVING, STATE_DRAGGED, STATE_SERVICE } TrolleyState;

typedef struct {
    int id;
    Vector3 pos;
    Vector3 target;
    Vector3 velocity;
    float speed;
    Color color;
    TrolleyState state;
    float radius;
    float progress; // 0..1 along path
} Trolley;

static Trolley trolleys[MAX_TROLLEYS];
static int trolley_count = 0;

// Simple predefined route waypoints for taxiway
static Vector3 route_points[6] = {
    { -40.0f, 0.0f, -20.0f },
    { -10.0f, 0.0f, -10.0f },
    { 0.0f, 0.0f, 0.0f },
    { 10.0f, 0.0f, 10.0f },
    { 30.0f, 0.0f, 15.0f },
    { 45.0f, 0.0f, 25.0f },
};

static int route_len = 6;

// UI buttons
typedef struct { Rectangle rect; const char *label; } Button;

static Button btn_add = { {10, 10, 120, 28}, "Add trolley" };
static Button btn_remove = { {140, 10, 120, 28}, "Remove trolley" };
static Button btn_pause = { {270, 10, 120, 28}, "Pause" };
static Button btn_reset = { {400, 10, 120, 28}, "Reset" };
static Button btn_opt = { {530, 10, 160, 28}, "Optimize Paths" };
static Button btn_togglecam = { {700, 10, 160, 28}, "Toggle 3D Camera" };

static bool paused = false;
static bool free_cam = false;

static Camera camera = {0};

// Helper: linear interpolation
static Vector3 LerpV3(Vector3 a, Vector3 b, float t) {
    return (Vector3){ a.x + (b.x - a.x)*t, a.y + (b.y - a.y)*t, a.z + (b.z - a.z)*t };
}

static float DistanceV3(Vector3 a, Vector3 b) {
    float dx = a.x-b.x; float dy = a.y-b.y; float dz = a.z-b.z; return sqrtf(dx*dx+dy*dy+dz*dz);
}

static void AddTrolleyAt(Vector3 pos) {
    if (trolley_count >= MAX_TROLLEYS) return;
    Trolley *t = &trolleys[trolley_count];
    t->id = trolley_count;
    t->pos = pos;
    t->target = route_points[0];
    t->velocity = (Vector3){0,0,0};
    t->speed = 3.0f + (rand()%100)/50.0f; // varying speeds
    t->color = LIGHTGRAY;
    t->state = STATE_IDLE;
    t->radius = 1.2f;
    t->progress = 0.0f;
    trolley_count++;
}

static void RemoveLastTrolley(void) {
    if (trolley_count > 0) trolley_count--;
}

static bool CheckButton(const Button *b) {
    Vector2 mp = GetMousePosition();
    return CheckCollisionPointRec(mp, b->rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// Very simple path optimizer: spread trolleys along route evenly
static void OptimizePaths(void) {
    if (trolley_count == 0) return;
    for (int i = 0; i < trolley_count; i++) {
        float t = (float)i / (float)trolley_count;
        int idxf = (int)floorf(t * (route_len-1));
        float local = (t * (route_len-1)) - idxf;
        Vector3 pos = LerpV3(route_points[idxf], route_points[(idxf+1)%(route_len)], local);
        trolleys[i].pos = pos;
        trolleys[i].state = STATE_IDLE;
        trolleys[i].progress = t;
    }
}

// Mouse ray to world plane (y=0)
static Vector3 GetMouseWorldGround(void) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    // plane at y=0: p = ray.origin + t*ray.direction, solve for y=0
    float t = -ray.position.y / ray.direction.y;
    if (t < 0) t = 0;
    Vector3 p = (Vector3){ ray.position.x + ray.direction.x*t,
                           ray.position.y + ray.direction.y*t,
                           ray.position.z + ray.direction.z*t };
    return p;
}

int main(void) {
    // Init
    const int screenW = 1280;
    const int screenH = 720;
    InitWindow(screenW, screenH, "Mauritius Airport - Trolley Processes (Prototype)");
    SetTargetFPS(60);

    camera.position = (Vector3){ 20.0f, 25.0f, 45.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Seed
    srand((unsigned)time(NULL));

    // Setup initial trolleys
    for (int i=0;i<6;i++) {
        float pct = (float)i/6.0f;
        int idx = i % route_len;
        Vector3 pos = route_points[idx];
        pos.x += (rand()%100 - 50)/10.0f;
        pos.z += (rand()%100 - 50)/10.0f;
        AddTrolleyAt(pos);
    }

    int dragged = -1; // trolley id being dragged

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        // Input UI
        if (CheckButton(&btn_add)) AddTrolleyAt((Vector3){ -35 + rand()%10, 0.0f, -18 + rand()%10 });
        if (CheckButton(&btn_remove)) RemoveLastTrolley();
        if (CheckButton(&btn_pause)) paused = !paused;
        if (CheckButton(&btn_reset)) {
            trolley_count = 0; OptimizePaths(); // reset via optimizer placing none
        }
        if (CheckButton(&btn_opt)) OptimizePaths();
        if (CheckButton(&btn_togglecam)) free_cam = !free_cam;

        // Camera control
        if (free_cam) {
            if (IsKeyDown(KEY_W)) camera.position.z -= 20*dt;
            if (IsKeyDown(KEY_S)) camera.position.z += 20*dt;
            if (IsKeyDown(KEY_A)) camera.position.x -= 20*dt;
            if (IsKeyDown(KEY_D)) camera.position.x += 20*dt;
        } else {
            // gentle orbit
            float rotSpeed = 0.2f;
            float angle = GetTime()*rotSpeed;
            camera.position.x = 40.0f * sinf(angle);
            camera.position.z = 40.0f * cosf(angle);
            camera.position.y = 18.0f + sinf(GetTime()*0.3f)*3.0f;
            camera.target = (Vector3){ 5.0f, 0.0f, 4.0f };
        }

        // Dragging logic
        Vector3 mouseWorld = GetMouseWorldGround();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // attempt to pick a trolley
            for (int i=0;i<trolley_count;i++) {
                Trolley *t = &trolleys[i];
                if (DistanceV3(t->pos, mouseWorld) < t->radius*1.5f) {
                    dragged = i; t->state = STATE_DRAGGED; break;
                }
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (dragged >= 0 && dragged < trolley_count) {
                trolleys[dragged].state = STATE_IDLE;
                dragged = -1;
            }
        }
        if (dragged >= 0 && dragged < trolley_count) {
            trolleys[dragged].pos = mouseWorld;
            trolleys[dragged].pos.y = 0.0f;
        }

        if (!paused) {
            // Update trolleys
            for (int i=0;i<trolley_count;i++) {
                Trolley *t = &trolleys[i];
                if (t->state == STATE_DRAGGED) continue;
                // Determine next waypoint target based on progress
                float routePos = fmodf(t->progress + dt*(t->speed/20.0f), 1.0f);
                t->progress = routePos;
                int idxf = (int)floorf(routePos * (route_len-1));
                float local = (routePos * (route_len-1)) - idxf;
                Vector3 wp = LerpV3(route_points[idxf], route_points[(idxf+1)%(route_len)], local);
                // move toward wp
                Vector3 dir = (Vector3){ wp.x - t->pos.x, 0.0f, wp.z - t->pos.z };
                float dist = sqrtf(dir.x*dir.x + dir.z*dir.z);
                if (dist > 0.01f) {
                    dir.x /= dist; dir.z /= dist;
                    float slow = 1.0f;
                    // simple collision avoidance: if too close to other trolley, slow
                    for (int j=0;j<trolley_count;j++) if (j!=i) {
                        float d = DistanceV3(t->pos, trolleys[j].pos);
                        if (d < 3.5f) slow = fminf(slow, 0.3f);
                    }
                    t->pos.x += dir.x * t->speed * slow * dt;
                    t->pos.z += dir.z * t->speed * slow * dt;
                    t->state = STATE_MOVING;
                } else {
                    t->state = STATE_IDLE;
                }
                // bobbing animation
                t->pos.y = sinf((GetTime() + i) * 2.0f) * 0.05f;
                // color state
                if (t->state == STATE_MOVING) t->color = GREEN;
                else if (t->state == STATE_IDLE) t->color = LIGHTGRAY;
                else t->color = YELLOW;
            }
        }

        // render
        BeginDrawing();
            ClearBackground((Color){ 235, 240, 245, 255 }); // light themed background
            BeginMode3D(camera);

                // ground plane
                DrawPlane((Vector3){0,0,0}, (Vector2){200,200}, (Color){200,220,200,255});

                // taxiway strip
                DrawCube((Vector3){0, -0.01f, 0}, 120, 0.02f, 40, (Color){180,180,190,255});
                DrawCube((Vector3){18, 0.0f, 18}, 60, 0.02f, 30, (Color){190,190,200,255});

                // route waypoints
                for (int i=0;i<route_len;i++) DrawSphere(route_points[i], 0.4f, (Color){120,140,220,200});
                // route lines
                for (int i=0;i<route_len-1;i++) DrawLine3D(route_points[i], route_points[i+1], (Color){100,100,140,200});

                // render trolleys
                for (int i=0;i<trolley_count;i++) {
                    Trolley *t = &trolleys[i];
                    // core body
                    DrawCubeV((Vector3){t->pos.x, t->pos.y+0.6f, t->pos.z}, (Vector3){1.6f, 0.6f, 0.9f}, t->color);
                    // small wheels
                    DrawSphere((Vector3){t->pos.x-0.6f, t->pos.y+0.15f, t->pos.z-0.35f}, 0.18f, DARKGRAY);
                    DrawSphere((Vector3){t->pos.x+0.6f, t->pos.y+0.15f, t->pos.z+0.35f}, 0.18f, DARKGRAY);
                    // ID label
                    Vector2 pos2d = GetWorldToScreen((Vector3){t->pos.x, t->pos.y+1.6f, t->pos.z}, camera);
                    DrawText(TextFormat("T#%02d", t->id), (int)pos2d.x-20, (int)pos2d.y, 10, BLACK);
                }

                // terminal as box
                DrawCube((Vector3){-38, 3.0f, -6.0f}, 20, 6, 12, (Color){210,210,220,255});
                DrawCubeWires((Vector3){-38, 3.0f, -6.0f}, 20, 6, 12, (Color){150,150,160,255});

                // small animated lights along taxiway
                for (int i=0;i<60;i++) {
                    float x = -60 + i*2.0f;
                    float z = -2.0f + sinf(GetTime()*2.0f + i*0.3f)*0.4f;
                    float pulse = (sinf(GetTime()*3.0f + i*0.5f)+1.0f)*0.5f;
                    DrawSphere((Vector3){x, 0.15f, z}, 0.08f, (Color){(unsigned char)(200*pulse), 180, 80, 255});
                }

            EndMode3D();

            // Overlay UI
            DrawRectangleRec(btn_add.rect, (Color){240,247,252,230}); DrawText(btn_add.label, btn_add.rect.x+8, btn_add.rect.y+6, 12, BLACK);
            DrawRectangleRec(btn_remove.rect, (Color){240,247,252,230}); DrawText(btn_remove.label, btn_remove.rect.x+8, btn_remove.rect.y+6, 12, BLACK);
            DrawRectangleRec(btn_pause.rect, (Color){240,247,252,230}); DrawText(paused?"Resume":"Pause", btn_pause.rect.x+8, btn_pause.rect.y+6, 12, BLACK);
            DrawRectangleRec(btn_reset.rect, (Color){240,247,252,230}); DrawText(btn_reset.label, btn_reset.rect.x+8, btn_reset.rect.y+6, 12, BLACK);
            DrawRectangleRec(btn_opt.rect, (Color){240,247,252,230}); DrawText(btn_opt.label, btn_opt.rect.x+8, btn_opt.rect.y+6, 12, BLACK);
            DrawRectangleRec(btn_togglecam.rect, (Color){240,247,252,230}); DrawText(btn_togglecam.label, btn_togglecam.rect.x+8, btn_togglecam.rect.y+6, 12, BLACK);

            // HUD info
            DrawText(TextFormat("Trolleys: %d", trolley_count), 10, 48, 14, DARKGRAY);
            DrawText(TextFormat("Drag: Click a trolley to drag. Optimize: spread trolleys. Collision avoidance implemented."), 10, 68, 12, GRAY);

            // show mouse world coordinates
            Vector3 mw = mouseWorld;
            DrawText(TextFormat("MouseWorld: (%.1f, %.1f, %.1f)", mw.x, mw.y, mw.z), 10, screenH-24, 12, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

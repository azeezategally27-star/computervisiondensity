// main.c - ILS Signal Integrity visualizer (GTK3 + Cairo)
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 1000
#define HEIGHT 600
#define N_ARCS 24

typedef struct {
    double x,y; // plane center
    double r; // collision radius
    int dragging;
} Plane;

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing;
    Plane plane;
    double antenna_x, antenna_y;
    double t0;
    double arc_phase[N_ARCS];
    double arc_distort[N_ARCS];
    double beam_angle; // radians
} App;

static App app = {0};

static double now_seconds(){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec*1e-9; }

// utility: linear interpolate
static double lerp(double a, double b, double r){ return a + (b-a)*r; }

// draw runway with simple 3D perspective
static void draw_runway(cairo_t *cr){
    // draw sky
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_set_source_rgb(cr, 0.95,0.98,1.0);
    cairo_fill(cr);
    // runway perspective trapezoid
    double cx = WIDTH/2.0;
    double top_w = WIDTH*0.25, bot_w = WIDTH*0.9;
    double top_y = HEIGHT*0.12, bot_y = HEIGHT*0.9;
    double tlx = cx - top_w/2, trx = cx + top_w/2;
    double blx = cx - bot_w/2, brx = cx + bot_w/2;
    // gradient fill
    cairo_move_to(cr, tlx, top_y);
    cairo_line_to(cr, trx, top_y);
    cairo_line_to(cr, brx, bot_y);
    cairo_line_to(cr, blx, bot_y);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, 0.11,0.12,0.14);
    cairo_fill(cr);
    // runway centerline dashed
    cairo_set_source_rgb(cr, 1,1,1);
    cairo_set_line_width(cr, 4);
    cairo_set_dash(cr, (double[]){30.0, 20.0}, 2, 0);
    double steps = 40;
    for(int i=0;i<steps;i++){
        double y1 = lerp(top_y, bot_y, i/steps);
        double y2 = lerp(top_y, bot_y, (i+0.3)/steps);
        double w1 = lerp(top_w, bot_w, i/steps);
        double w2 = lerp(top_w, bot_w, (i+0.3)/steps);
        double x1 = cx - w1/40.0; double x2 = cx + w1/40.0;
        cairo_move_to(cr, x1, y1); cairo_line_to(cr, x2, y1);
        cairo_move_to(cr, x1, y2); cairo_line_to(cr, x2, y2);
    }
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
}

// draw antenna at runway end
static void draw_antenna(cairo_t *cr, double ax, double ay){
    // antenna base
    cairo_set_source_rgb(cr, 0.2,0.2,0.2);
    cairo_arc(cr, ax, ay, 8, 0, 2*M_PI); cairo_fill(cr);
    // mast
    cairo_set_line_width(cr, 4);
    cairo_move_to(cr, ax, ay-20); cairo_line_to(cr, ax, ay); cairo_stroke(cr);
}

// draw concentric pulsing arcs (sine-wave arcs approximated)
static void draw_arcs(cairo_t *cr, double ax, double ay, double t){
    double speed = 60.0; // pixels per second
    for(int i=0;i<N_ARCS;i++){
        double base_r = fmod((t*speed) + i*22.0, 2000.0);
        double r = base_r + 60; // ensure not zero
        double phase = app.arc_phase[i];
        double distort = app.arc_distort[i];
        double alpha = 0.25 * (1.0 - (i/(double)N_ARCS));
        // draw arc in beam direction only (semi-circles)
        double ang0 = app.beam_angle - M_PI/2; double ang1 = app.beam_angle + M_PI/2;
        cairo_set_line_width(cr, 2 + 0.6*distort);
        cairo_set_source_rgba(cr, 0.1, 0.6, 0.95, alpha + 0.3*distort);
        // to create ripple/phase we will sample along semi-circle and plot small bezier perturbations
        int steps = 180;
        cairo_move_to(cr, ax + (r+sin(phase)*10.0)*cos(ang0), ay + (r+sin(phase)*10.0)*sin(ang0));
        for(int s=1;s<=steps;s++){
            double a = ang0 + (ang1-ang0)*(s/(double)steps);
            // add sinusoidal perturbation shaped by distort factor
            double perturb = sin(a*6.0 + phase + t*2.0) * (3.0 + 15.0*distort) * (1.0 - (i/(double)N_ARCS));
            double rr = r + perturb;
            double x = ax + rr*cos(a);
            double y = ay + rr*sin(a);
            cairo_line_to(cr, x, y);
        }
        cairo_stroke(cr);
    }
}

// draw plane (simple 3D-ish scaling based on y)
static void draw_plane(cairo_t *cr, Plane *p){
    double scale = 1.0 + (p->y / (double)HEIGHT) * 0.6; // further down -> larger
    double w = 48 * scale, h = 24 * scale;
    // shadow
    cairo_set_source_rgba(cr, 0,0,0,0.25);
    cairo_arc(cr, p->x + 6, p->y + h*0.6, w*0.5, 0, 2*M_PI); cairo_fill(cr);
    // body
    cairo_save(cr);
    cairo_translate(cr, p->x, p->y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_rgb(cr, 0.9,0.2,0.2);
    // fuselage
    cairo_move_to(cr, -24, 0); cairo_rel_line_to(cr, 40, 0);
    cairo_set_line_width(cr, 10); cairo_stroke(cr);
    // tail
    cairo_set_source_rgb(cr, 0.14,0.2,0.5);
    cairo_move_to(cr, -20, -6); cairo_line_to(cr, -28, -14); cairo_line_to(cr, -20, 6); cairo_close_path(cr); cairo_fill(cr);
    // cockpit
    cairo_set_source_rgb(cr, 0.8,0.9,1.0); cairo_arc(cr, 16, 0, 6, 0, 2*M_PI); cairo_fill(cr);
    cairo_restore(cr);
}

// compute distortions if plane intersects an arc: increase arc_distort and create reflected-origin ripples
static void compute_interactions(Plane *p, double ax, double ay){
    for(int i=0;i<N_ARCS;i++){
        // compute nominal radius for this arc at current time
        double t = now_seconds() - app.t0; double speed = 60.0;
        double base_r = fmod((t*speed) + i*22.0, 2000.0);
        double r = base_r + 60;
        // distance between plane center and arc circle
        double dcenter = hypot(p->x - ax, p->y - ay);
        double diff = fabs(dcenter - r);
        if(diff < p->r + 8){
            // intersection -> increase distortion
            app.arc_distort[i] = fmin(1.0, app.arc_distort[i] + 0.08);
            // spawn reflected ripple by nudging a neighboring arc phase
            int reflected_idx = (i + 3) % N_ARCS;
            app.arc_phase[reflected_idx] += 0.6; // phase shift
        } else {
            // decay distortion slowly
            app.arc_distort[i] *= 0.96;
            app.arc_phase[i] *= 0.999;
        }
    }
}

// mitigation actions
static void auto_correct_beam(GtkButton *btn, gpointer user_data){
    // slowly align beam away from plane to reduce intersections
    double target = atan2(app.antenna_y - app.plane.y, app.antenna_x - app.plane.x);
    // pick angle slightly offset
    double new_angle = lerp(app.beam_angle, target + 0.3, 0.35);
    app.beam_angle = new_angle;
}
static void apply_nulling(GtkButton *btn, gpointer user_data){
    // reduce distortions by damping arc_distort
    for(int i=0;i<N_ARCS;i++) app.arc_distort[i] *= 0.2;
}
static void reset_scene(GtkButton *btn, gpointer user_data){
    for(int i=0;i<N_ARCS;i++){ app.arc_phase[i]=0; app.arc_distort[i]=0; }
    app.beam_angle = -M_PI/2;
    app.plane.x = WIDTH/2; app.plane.y = HEIGHT*0.7; app.plane.r = 18; app.plane.dragging=0;
}

// drawing callback
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data){
    double t = now_seconds() - app.t0;
    draw_runway(cr);
    draw_arcs(cr, app.antenna_x, app.antenna_y, t);
    draw_antenna(cr, app.antenna_x, app.antenna_y);
    draw_plane(cr, &app.plane);
    // HUD
    cairo_set_source_rgb(cr, 0.0,0.0,0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    char buf[128]; snprintf(buf,sizeof(buf), "Plane: (%.0f, %.0f)  Beam angle: %.2f rad", app.plane.x, app.plane.y, app.beam_angle);
    cairo_move_to(cr, 14, 18); cairo_show_text(cr, buf);
    return FALSE;
}

// tick to update animation
static gboolean tick(gpointer data){
    double t = now_seconds() - app.t0;
    // advance arc phases for subtle motion
    for(int i=0;i<N_ARCS;i++) app.arc_phase[i] += 0.02 + 0.004*i;
    // compute interactions
    compute_interactions(&app.plane, app.antenna_x, app.antenna_y);
    gtk_widget_queue_draw(app.drawing);
    return TRUE;
}

// mouse events for dragging
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data){
    double dx = event->x - app.plane.x; double dy = event->y - app.plane.y;
    if(hypot(dx,dy) < app.plane.r*2){ app.plane.dragging = 1; return TRUE; }
    return FALSE;
}
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data){ app.plane.dragging = 0; return TRUE; }
static gboolean on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data){ if(app.plane.dragging){ app.plane.x = event->x; app.plane.y = event->y; } return TRUE; }

int main(int argc, char **argv){
    gtk_init(&argc, &argv);
    app.t0 = now_seconds();
    app.antenna_x = WIDTH/2.0; app.antenna_y = HEIGHT*0.12; app.beam_angle = -M_PI/2;
    // init plane
    app.plane.x = WIDTH/2.0; app.plane.y = HEIGHT*0.72; app.plane.r = 18; app.plane.dragging = 0;
    for(int i=0;i<N_ARCS;i++){ app.arc_phase[i] = ((double)rand()/(double)RAND_MAX)*2*M_PI; app.arc_distort[i]=0.0; }

    // build UI
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), WIDTH, HEIGHT);
    gtk_window_set_title(GTK_WINDOW(app.window), "ILS Signal Integrity — Prototype");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(app.window), box);

    // header with buttons
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(lbl), "<span weight='bold' size='large'>ILS Signal Integrity Visualizer (Prototype)</span>");
    gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 6);
    GtkWidget *btn_auto = gtk_button_new_with_label("Auto-correct Beam"); GtkWidget *btn_null = gtk_button_new_with_label("Apply Nulling"); GtkWidget *btn_reset = gtk_button_new_with_label("Reset");
    gtk_box_pack_end(GTK_BOX(hbox), btn_reset, FALSE, FALSE, 6);
    gtk_box_pack_end(GTK_BOX(hbox), btn_null, FALSE, FALSE, 6);
    gtk_box_pack_end(GTK_BOX(hbox), btn_auto, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 4);

    app.drawing = gtk_drawing_area_new(); gtk_widget_set_size_request(app.drawing, WIDTH, HEIGHT-60);
    gtk_box_pack_start(GTK_BOX(box), app.drawing, TRUE, TRUE, 0);

    g_signal_connect(app.drawing, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(app.drawing, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(app.drawing, "button-release-event", G_CALLBACK(on_button_release), NULL);
    g_signal_connect(app.drawing, "motion-notify-event", G_CALLBACK(on_motion), NULL);
    gtk_widget_set_events(app.drawing, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

    g_signal_connect(btn_auto, "clicked", G_CALLBACK(auto_correct_beam), NULL);
    g_signal_connect(btn_null, "clicked", G_CALLBACK(apply_nulling), NULL);
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(reset_scene), NULL);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(app.window);
    g_timeout_add(33, tick, NULL); // ~30 FPS

    gtk_main();
    return 0;
}

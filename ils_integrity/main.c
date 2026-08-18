// main.c - ILS Signal Integrity visualizer (GTK3 + Cairo)
// Extended: frequency/phase sliders, phasor renderer, sprite decoding, frame capture
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <stdio.h>

#define WIDTH 1000
#define HEIGHT 600
#define N_ARCS 48

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
    // controls
    double frequency_hz; // simulated frequency (Hz)
    double global_phase;
    int phasor_mode;
    GdkPixbuf *plane_pix;
    int capturing;
    int capture_frame;
} App;

static App app = {0};

static double now_seconds(){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec*1e-9; }
static double lerp(double a, double b, double r){ return a + (b-a)*r; }

// decode embedded base64 plane sprite to plane.png on first run
static void write_plane_png_if_missing(){
    const char *path = "ils_integrity/plane.png";
    if(g_file_test(path, G_FILE_TEST_EXISTS)) return;
    // read base64
    FILE *f = fopen("ils_integrity/plane_base64.txt","r");
    if(!f) return;
    fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    char *b64 = malloc(sz+1); fread(b64,1,sz,f); b64[sz]=0; fclose(f);
    // decode with g_base64_decode
    gsize out_len=0; gchar *out = g_base64_decode(b64, &out_len);
    free(b64);
    if(out && out_len>0){
        FILE *of = fopen(path, "wb"); if(of){ fwrite(out,1,out_len,of); fclose(of); }
        g_free(out);
    }
}

static void load_plane_sprite(){
    write_plane_png_if_missing();
    GError *err = NULL;
    app.plane_pix = gdk_pixbuf_new_from_file("ils_integrity/plane.png", &err);
    if(err){ g_clear_error(&err); app.plane_pix = NULL; }
}

// draw runway
static void draw_runway(cairo_t *cr){
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_set_source_rgb(cr, 0.95,0.98,1.0);
    cairo_fill(cr);
    double cx = WIDTH/2.0;
    double top_w = WIDTH*0.25, bot_w = WIDTH*0.9;
    double top_y = HEIGHT*0.12, bot_y = HEIGHT*0.9;
    double tlx = cx - top_w/2, trx = cx + top_w/2;
    double blx = cx - bot_w/2, brx = cx + bot_w/2;
    cairo_move_to(cr, tlx, top_y);
    cairo_line_to(cr, trx, top_y);
    cairo_line_to(cr, brx, bot_y);
    cairo_line_to(cr, blx, bot_y);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, 0.11,0.12,0.14);
    cairo_fill(cr);
    // centerline
    cairo_set_source_rgb(cr, 1,1,1);
    cairo_set_line_width(cr, 4);
    cairo_set_dash(cr, (double[]){30.0, 20.0}, 2, 0);
    int steps = 40;
    for(int i=0;i<steps;i++){
        double y1 = lerp(top_y, bot_y, i/(double)steps);
        double w1 = lerp(top_w, bot_w, i/(double)steps);
        double x1 = cx - w1/40.0; double x2 = cx + w1/40.0;
        cairo_move_to(cr, x1, y1); cairo_line_to(cr, x2, y1);
    }
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
}

static void draw_antenna(cairo_t *cr, double ax, double ay){
    cairo_set_source_rgb(cr, 0.2,0.2,0.2);
    cairo_arc(cr, ax, ay, 8, 0, 2*M_PI); cairo_fill(cr);
    cairo_set_line_width(cr, 4);
    cairo_move_to(cr, ax, ay-20); cairo_line_to(cr, ax, ay); cairo_stroke(cr);
}

// phasor-based interference renderer
static void draw_phasor_arcs(cairo_t *cr, double ax, double ay, double t){
    // parameters
    double c = 3e8; // speed of light (m/s) - scale abstract
    double freq = app.frequency_hz > 1 ? app.frequency_hz : 1e9; // default 1 GHz if not set
    // map freq slider to an effective wavelength in pixels
    double lambda = 300000000.0 / freq; // meters; then scale to pixels by arbitrary factor
    double pix_per_meter = 0.0005 * freq; // playful scaling so higher freq -> denser arcs
    double k = 2.0*M_PI / fmax(1.0, lambda*pix_per_meter);
    int samples = 360;
    for(int i=0;i<N_ARCS;i++){
        double base_r = 80 + i*16; // base radius for arc i
        // we'll draw a semi-circle from beam_angle - pi/2 to +pi/2
        double ang0 = app.beam_angle - M_PI/2; double ang1 = app.beam_angle + M_PI/2;
        int steps = samples/2;
        // compute amplitude samples using phasor sum of direct and reflected
        double amplitudes[1024];
        for(int s=0;s<=steps;s++){
            double a = ang0 + (ang1-ang0)*(s/(double)steps);
            double x = ax + base_r * cos(a);
            double y = ay + base_r * sin(a);
            // direct path length
            double d_direct = hypot(x - ax, y - ay);
            double complex sum = 0.0 + 0.0*I;
            // source contribution (antenna)
            double phase_src = k * d_direct + app.global_phase;
            double attenuation = 1.0 / (1.0 + 0.001*d_direct);
            sum += attenuation * cexp(I * phase_src);
            // reflection from plane: approximate plane as a point scatterer at plane center
            double d1 = hypot(app.plane.x - ax, app.plane.y - ay); // antenna->plane
            double d2 = hypot(x - app.plane.x, y - app.plane.y); // plane->point
            double d_ref = d1 + d2 + 1e-6;
            double phase_ref = k * d_ref + app.global_phase + 0.5; // add small phase shift
            double att_ref = 0.6 / (1.0 + 0.001*d_ref);
            sum += att_ref * cexp(I * phase_ref);
            double mag = cabs(sum);
            amplitudes[s] = mag;
        }
        // normalize amplitudes for rendering
        double maxv = 0; for(int s=0;s<=steps;s++) if(amplitudes[s]>maxv) maxv=amplitudes[s];
        if(maxv < 1e-6) maxv = 1.0;
        // draw polyline with radial perturbation based on amplitude
        cairo_set_line_width(cr, 1.5);
        for(int s=0;s<=steps;s++){
            double a = ang0 + (ang1-ang0)*(s/(double)steps);
            double amp = amplitudes[s]/maxv; // 0..1
            double perturb = (amp*18.0) * (1.0 - (i/(double)N_ARCS));
            double rr = base_r + perturb;
            double x = ax + rr*cos(a);
            double y = ay + rr*sin(a);
            double hue = 0.55 - 0.4*amp; // color varies
            cairo_set_source_rgba(cr, 0.1 + 0.5*amp, 0.6 - 0.3*amp, 1.0 - 0.5*amp, 0.6);
            if(s==0) cairo_move_to(cr, x, y); else cairo_line_to(cr, x, y);
        }
        cairo_stroke(cr);
    }
}

// draw plane with perspective; use sprite if available
static void draw_plane(cairo_t *cr, Plane *p){
    double scale = 0.8 + (p->y / (double)HEIGHT) * 1.0; // further -> larger
    double w = 64 * scale, h = 32 * scale;
    // shadow ellipse
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0,0,0,0.28);
    cairo_translate(cr, p->x + 8, p->y + h*0.6);
    cairo_scale(cr, w*0.009, h*0.03);
    cairo_arc(cr, 0, 0, 40, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_restore(cr);
    if(app.plane_pix){
        // draw pixbuf centered
        GdkPixbuf *pb = app.plane_pix;
        int pw = gdk_pixbuf_get_width(pb); int ph = gdk_pixbuf_get_height(pb);
        double iw = pw; double ih = ph;
        double sx = w / iw; double sy = h / ih;
        cairo_save(cr);
        cairo_translate(cr, p->x - w/2, p->y - h/2);
        cairo_scale(cr, sx, sy);
        gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    } else {
        // fallback vector plane
        cairo_save(cr);
        cairo_translate(cr, p->x, p->y);
        cairo_scale(cr, scale, scale);
        cairo_set_source_rgb(cr, 0.9,0.2,0.2);
        cairo_set_line_width(cr, 8);
        cairo_move_to(cr, -24, 0); cairo_rel_line_to(cr, 40, 0); cairo_stroke(cr);
        cairo_set_source_rgb(cr, 0.14,0.2,0.5);
        cairo_move_to(cr, -20, -6); cairo_line_to(cr, -28, -14); cairo_line_to(cr, -20, 6); cairo_close_path(cr); cairo_fill(cr);
        cairo_restore(cr);
    }
}

static void compute_interactions(Plane *p, double ax, double ay){
    // damping & phase nudges
    for(int i=0;i<N_ARCS;i++){
        // simple proximity check but less important in phasor mode
        double base_r = 80 + i*16;
        double dcenter = hypot(p->x - ax, p->y - ay);
        double diff = fabs(dcenter - base_r);
        if(diff < p->r + 12){ app.arc_distort[i] = fmin(1.0, app.arc_distort[i] + 0.08); }
        else app.arc_distort[i] *= 0.97;
    }
}

static void draw_old_arcs(cairo_t *cr, double ax, double ay, double t){
    double speed = 60.0;
    for(int i=0;i<N_ARCS;i++){
        double base_r = fmod((t*speed) + i*22.0, 2000.0);
        double r = base_r + 60;
        double phase = app.arc_phase[i];
        double distort = app.arc_distort[i];
        double alpha = 0.25 * (1.0 - (i/(double)N_ARCS));
        double ang0 = app.beam_angle - M_PI/2; double ang1 = app.beam_angle + M_PI/2;
        cairo_set_line_width(cr, 2 + 0.6*distort);
        cairo_set_source_rgba(cr, 0.1, 0.6, 0.95, alpha + 0.3*distort);
        int steps = 180;
        cairo_move_to(cr, ax + (r+sin(phase)*10.0)*cos(ang0), ay + (r+sin(phase)*10.0)*sin(ang0));
        for(int s=1;s<=steps;s++){
            double a = ang0 + (ang1-ang0)*(s/(double)steps);
            double perturb = sin(a*6.0 + phase + t*2.0) * (3.0 + 15.0*distort) * (1.0 - (i/(double)N_ARCS));
            double rr = r + perturb;
            double x = ax + rr*cos(a);
            double y = ay + rr*sin(a);
            cairo_line_to(cr, x, y);
        }
        cairo_stroke(cr);
    }
}

static void auto_correct_beam(GtkButton *btn, gpointer user_data){
    double target = atan2(app.antenna_y - app.plane.y, app.antenna_x - app.plane.x);
    double new_angle = lerp(app.beam_angle, target + 0.3, 0.35);
    app.beam_angle = new_angle;
}
static void apply_nulling(GtkButton *btn, gpointer user_data){ for(int i=0;i<N_ARCS;i++) app.arc_distort[i] *= 0.2; }
static void reset_scene(GtkButton *btn, gpointer user_data){ for(int i=0;i<N_ARCS;i++){ app.arc_phase[i]=0; app.arc_distort[i]=0; } app.beam_angle = -M_PI/2; app.plane.x = WIDTH/2; app.plane.y = HEIGHT*0.72; app.plane.r = 18; app.plane.dragging=0; }

// capture frames
static void capture_frame_to_png(){
    if(!app.capturing) return;
    char dir[] = "ils_integrity/frames";
    g_mkdir_with_parents(dir, 0755);
    int idx = app.capture_frame++;
    char fname[256]; snprintf(fname,256, "%s/frame_%04d.png", dir, idx);
    // render to image surface
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT-60);
    cairo_t *cr = cairo_create(surface);
    double t = now_seconds() - app.t0;
    draw_runway(cr);
    if(app.phasor_mode) draw_phasor_arcs(cr, app.antenna_x, app.antenna_y, t); else draw_old_arcs(cr, app.antenna_x, app.antenna_y, t);
    draw_antenna(cr, app.antenna_x, app.antenna_y);
    draw_plane(cr, &app.plane);
    cairo_destroy(cr);
    cairo_surface_write_to_png(surface, fname);
    cairo_surface_destroy(surface);
}

// UI draw
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data){
    double t = now_seconds() - app.t0;
    draw_runway(cr);
    if(app.phasor_mode) draw_phasor_arcs(cr, app.antenna_x, app.antenna_y, t); else draw_old_arcs(cr, app.antenna_x, app.antenna_y, t);
    draw_antenna(cr, app.antenna_x, app.antenna_y);
    draw_plane(cr, &app.plane);
    cairo_set_source_rgb(cr, 0.0,0.0,0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    char buf[128]; snprintf(buf,sizeof(buf), "Freq: %.1f (slider)  Phase: %.2f  Mode: %s", app.frequency_hz, app.global_phase, app.phasor_mode?"Phasor":"Legacy");
    cairo_move_to(cr, 14, 18); cairo_show_text(cr, buf);
    return FALSE;
}

static gboolean tick(gpointer data){
    for(int i=0;i<N_ARCS;i++) app.arc_phase[i] += 0.02 + 0.004*i;
    compute_interactions(&app.plane, app.antenna_x, app.antenna_y);
    gtk_widget_queue_draw(app.drawing);
    if(app.capturing){ capture_frame_to_png(); if(app.capture_frame>240) app.capturing=0; }
    return TRUE;
}

// mouse events
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data){ double dx = event->x - app.plane.x; double dy = event->y - app.plane.y; if(hypot(dx,dy) < app.plane.r*2){ app.plane.dragging = 1; return TRUE; } return FALSE; }
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data){ app.plane.dragging = 0; return TRUE; }
static gboolean on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data){ if(app.plane.dragging){ app.plane.x = event->x; app.plane.y = event->y; } return TRUE; }

// control callbacks
static void on_freq_changed(GtkRange *range, gpointer user_data){ app.frequency_hz = gtk_range_get_value(range); }
static void on_phase_changed(GtkRange *range, gpointer user_data){ app.global_phase = gtk_range_get_value(range); }
static void on_mode_toggled(GtkToggleButton *tb, gpointer user_data){ app.phasor_mode = gtk_toggle_button_get_active(tb); }
static void on_capture_clicked(GtkButton *btn, gpointer user_data){ app.capturing = 1; app.capture_frame = 0; }

int main(int argc, char **argv){
    gtk_init(&argc, &argv);
    app.t0 = now_seconds(); app.antenna_x = WIDTH/2.0; app.antenna_y = HEIGHT*0.12; app.beam_angle = -M_PI/2;
    app.plane.x = WIDTH/2.0; app.plane.y = HEIGHT*0.72; app.plane.r = 18; app.plane.dragging=0;
    for(int i=0;i<N_ARCS;i++){ app.arc_phase[i] = ((double)rand()/(double)RAND_MAX)*2*M_PI; app.arc_distort[i]=0.0; }
    app.frequency_hz = 1e9; app.global_phase = 0.0; app.phasor_mode = 1; app.capturing=0;
    load_plane_sprite();

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL); gtk_window_set_default_size(GTK_WINDOW(app.window), WIDTH, HEIGHT);
    gtk_window_set_title(GTK_WINDOW(app.window), "ILS Signal Integrity — Advanced Prototype");
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); gtk_container_add(GTK_CONTAINER(app.window), vbox);
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(lbl), "<span weight='bold' size='large'>ILS Signal Integrity — Advanced Prototype</span>");
    gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *freq_label = gtk_label_new("Frequency (1e8..5e9)"); GtkWidget *freq_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1e8, 5e9, 1e8);
    gtk_range_set_value(GTK_RANGE(freq_scale), app.frequency_hz);
    g_signal_connect(freq_scale, "value-changed", G_CALLBACK(on_freq_changed), NULL);

    GtkWidget *phase_label = gtk_label_new("Phase"); GtkWidget *phase_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 6.2831, 0.05);
    gtk_range_set_value(GTK_RANGE(phase_scale), app.global_phase);
    g_signal_connect(phase_scale, "value-changed", G_CALLBACK(on_phase_changed), NULL);

    GtkWidget *mode_toggle = gtk_check_button_new_with_label("Phasor Mode"); gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mode_toggle), app.phasor_mode);
    g_signal_connect(mode_toggle, "toggled", G_CALLBACK(on_mode_toggled), NULL);
    GtkWidget *btn_auto = gtk_button_new_with_label("Auto-correct Beam"); GtkWidget *btn_null = gtk_button_new_with_label("Apply Nulling"); GtkWidget *btn_reset = gtk_button_new_with_label("Reset"); GtkWidget *btn_capture = gtk_button_new_with_label("Capture");
    g_signal_connect(btn_auto, "clicked", G_CALLBACK(auto_correct_beam), NULL); g_signal_connect(btn_null, "clicked", G_CALLBACK(apply_nulling), NULL); g_signal_connect(btn_reset, "clicked", G_CALLBACK(reset_scene), NULL); g_signal_connect(btn_capture, "clicked", G_CALLBACK(on_capture_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(controls), freq_label, FALSE, FALSE, 4); gtk_box_pack_start(GTK_BOX(controls), freq_scale, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(controls), phase_label, FALSE, FALSE, 4); gtk_box_pack_start(GTK_BOX(controls), phase_scale, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(controls), mode_toggle, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(controls), btn_auto, FALSE, FALSE, 4); gtk_box_pack_start(GTK_BOX(controls), btn_null, FALSE, FALSE, 4); gtk_box_pack_start(GTK_BOX(controls), btn_reset, FALSE, FALSE, 4); gtk_box_pack_start(GTK_BOX(controls), btn_capture, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), controls, FALSE, FALSE, 4);

    app.drawing = gtk_drawing_area_new(); gtk_widget_set_size_request(app.drawing, WIDTH, HEIGHT-120); gtk_box_pack_start(GTK_BOX(vbox), app.drawing, TRUE, TRUE, 0);
    g_signal_connect(app.drawing, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(app.drawing, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(app.drawing, "button-release-event", G_CALLBACK(on_button_release), NULL);
    g_signal_connect(app.drawing, "motion-notify-event", G_CALLBACK(on_motion), NULL);
    gtk_widget_set_events(app.drawing, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(app.window);
    g_timeout_add(33, tick, NULL);
    gtk_main();
    return 0;
}

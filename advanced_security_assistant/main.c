// main.c - Security Assistant UI (simulation + assistant retrieval)
#include <gtk/gtk.h>
#include <cairo.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "kb.h"

#define WIDTH 1100
#define HEIGHT 720
#define N_ENTITIES 24

typedef struct Entity { double x,y; double vx,vy; int luggage; int flagged; int id; } Entity;

static Entity entities[N_ENTITIES];
static double t0;
static GtkWidget *window;
static GtkWidget *drawing;
static GtkWidget *assistant_view;
static GtkWidget *kb_status_label;
static int db_ready = 0;

static double now(){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec*1e-9; }

static void init_entities(){ srand(time(NULL)); for(int i=0;i<N_ENTITIES;i++){ entities[i].x = 100 + rand()%800; entities[i].y = 120 + rand()%420; entities[i].vx = ((rand()%100)/100.0 - 0.5) * 1.6; entities[i].vy = ((rand()%100)/100.0 - 0.5) * 1.0; entities[i].luggage = (rand()%100) < 70; entities[i].flagged = 0; entities[i].id = i+1; } }

static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer d){
    cairo_set_source_rgb(cr, 0.98,0.99,1.0); cairo_paint(cr);
    // draw checkpoint rectangle
    cairo_set_source_rgb(cr, 0.95,0.97,1.0); cairo_rectangle(cr, 40, 80, 760, 520); cairo_fill(cr);
    // draw scanner
    cairo_set_source_rgb(cr, 0.2,0.5,0.9); cairo_rectangle(cr, 420, 90, 30, 520); cairo_fill(cr);
    // draw entities
    for(int i=0;i<N_ENTITIES;i++){
        Entity *e = &entities[i];
        // body
        if(e->flagged) cairo_set_source_rgb(cr, 0.9,0.25,0.25); else cairo_set_source_rgb(cr, 0.1,0.2,0.35);
        cairo_arc(cr, e->x, e->y, 10, 0, 2*M_PI); cairo_fill(cr);
        // luggage box
        if(e->luggage){ cairo_set_source_rgb(cr, 0.7,0.5,0.2); cairo_rectangle(cr, e->x-12, e->y+12, 24, 12); cairo_fill(cr); }
        // id label
        cairo_set_source_rgb(cr, 1,1,1); cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); cairo_set_font_size(cr, 10);
        char buf[16]; snprintf(buf,16, "%d", e->id);
        cairo_move_to(cr, e->x-4, e->y+4); cairo_show_text(cr, buf);
        if(e->flagged){ // pulsing ring
            double s = 1.0 + 0.15 * sin((now()-t0)*6.0);
            cairo_set_source_rgba(cr, 0.9,0.25,0.25, 0.6);
            cairo_set_line_width(cr, 2);
            cairo_arc(cr, e->x, e->y, 18*s, 0, 2*M_PI); cairo_stroke(cr);
        }
    }
    return FALSE;
}

static gboolean tick(gpointer d){
    for(int i=0;i<N_ENTITIES;i++){
        entities[i].x += entities[i].vx; entities[i].y += entities[i].vy;
        if(entities[i].x < 60 || entities[i].x > 780) entities[i].vx *= -1;
        if(entities[i].y < 100 || entities[i].y > 580) entities[i].vy *= -1;
    }
    gtk_widget_queue_draw(drawing);
    return TRUE;
}

// simple heuristic: if luggage crosses near scanner x ~ 420, random chance of oddity -> flag
static void scan_frame_and_flag(){
    for(int i=0;i<N_ENTITIES;i++){
        Entity *e = &entities[i];
        if(e->luggage && fabs(e->x - 440) < 30){
            // compute a score influenced by position and random
            double score = (1.0 - fabs(e->y - 300)/260.0) * ((rand()%100)/100.0);
            if(score > 0.6){ e->flagged = 1; }
        }
    }
}

static void clear_flags(){ for(int i=0;i<N_ENTITIES;i++) entities[i].flagged = 0; }

// assistant: query KB with context of flagged entity id
static void show_assistant_for_entity(int ent_id){
    char q[64]; snprintf(q, sizeof(q), "suspicious luggage OR luggage indicators");
    int n=0; char **res = kb_query(q, 4, &n);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(assistant_view)), "", -1);
    GtkTextIter end; GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(assistant_view)); gtk_text_buffer_get_end_iter(tb, &end);
    if(n==0){ gtk_text_buffer_insert(tb, &end, "No relevant guidance in KB. Train KB with local SOPs.", -1); }
    else {
        char header[128]; snprintf(header, sizeof(header), "Entity %d flagged — recommended guidance:\n", ent_id);
        gtk_text_buffer_insert(tb, &end, header, -1);
        for(int i=0;i<n;i++){
            gtk_text_buffer_insert(tb, &end, "- ", -1);
            gtk_text_buffer_insert(tb, &end, res[i], -1);
            gtk_text_buffer_insert(tb, &end, "\n", -1);
        }
        gtk_text_buffer_insert(tb, &end, "\nSuggested actions:\n1) Isolate item in secure room\n2) Notify security supervisor & open an incident log\n3) Use non-destructive inspection tools where available\", -1);
    }
    kb_free_results(res, n);
}

// UI callbacks
static void on_train_clicked(GtkButton *b, gpointer u){
    gtk_label_set_text(GTK_LABEL(kb_status_label), "Training KB...");
    kb_close(); // re-open to clear
    if(!kb_init("advanced_security_assistant/security_kb.db")){
        gtk_label_set_text(GTK_LABEL(kb_status_label), "KB init failed"); return;
    }
    if(kb_train_from_dir("advanced_security_assistant/kb")){
        db_ready = 1; gtk_label_set_text(GTK_LABEL(kb_status_label), "KB trained: ready");
    } else {
        gtk_label_set_text(GTK_LABEL(kb_status_label), "KB training failed");
    }
}

static void on_scan_clicked(GtkButton *b, gpointer u){
    scan_frame_and_flag(); gtk_widget_queue_draw(drawing);
}

static void on_clear_clicked(GtkButton *b, gpointer u){ clear_flags(); gtk_widget_queue_draw(drawing); }

static void on_quarantine_clicked(GtkButton *b, gpointer u){
    // find first flagged entity
    for(int i=0;i<N_ENTITIES;i++) if(entities[i].flagged){
        entities[i].flagged = 0; // simulate quarantine
        show_assistant_for_entity(entities[i].id);
        break;
    }
}

int main(int argc, char **argv){
    gtk_init(&argc, &argv);
    t0 = now();
    init_entities();
    kb_init("advanced_security_assistant/security_kb.db");

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL); gtk_window_set_default_size(GTK_WINDOW(window), WIDTH, HEIGHT); gtk_window_set_title(GTK_WINDOW(window), "Security Assistant — Mauritius (Prototype)");
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8); gtk_container_add(GTK_CONTAINER(window), hbox);

    // left: canvas
    drawing = gtk_drawing_area_new(); gtk_widget_set_size_request(drawing, 820, 640); gtk_box_pack_start(GTK_BOX(hbox), drawing, FALSE, FALSE, 6);
    g_signal_connect(drawing, "draw", G_CALLBACK(on_draw), NULL);

    // right: assistant & controls
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); gtk_box_pack_start(GTK_BOX(hbox), right, TRUE, TRUE, 6);
    GtkWidget *hdr = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(hdr), "<span weight='bold' size='large'>Security Assistant (Prototype)</span>"); gtk_box_pack_start(GTK_BOX(right), hdr, FALSE, FALSE, 2);
    kb_status_label = gtk_label_new("KB status: ready (or train)"); gtk_box_pack_start(GTK_BOX(right), kb_status_label, FALSE, FALSE, 2);

    GtkWidget *btn_train = gtk_button_new_with_label("Train KB"); GtkWidget *btn_scan = gtk_button_new_with_label("Scan Frame"); GtkWidget *btn_quarantine = gtk_button_new_with_label("Quarantine First Flagged"); GtkWidget *btn_clear = gtk_button_new_with_label("Clear Flags");
    gtk_box_pack_start(GTK_BOX(right), btn_train, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(right), btn_scan, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(right), btn_quarantine, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(right), btn_clear, FALSE, FALSE, 2);
    g_signal_connect(btn_train, "clicked", G_CALLBACK(on_train_clicked), NULL);
    g_signal_connect(btn_scan, "clicked", G_CALLBACK(on_scan_clicked), NULL);
    g_signal_connect(btn_quarantine, "clicked", G_CALLBACK(on_quarantine_clicked), NULL);
    g_signal_connect(btn_clear, "clicked", G_CALLBACK(on_clear_clicked), NULL);

    assistant_view = gtk_text_view_new(); gtk_text_view_set_editable(GTK_TEXT_VIEW(assistant_view), FALSE); gtk_widget_set_size_request(assistant_view, 320, 360); gtk_box_pack_end(GTK_BOX(right), assistant_view, FALSE, FALSE, 2);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    g_timeout_add(40, tick, NULL); // ~25 FPS
    gtk_main();
    kb_close();
    return 0;
}

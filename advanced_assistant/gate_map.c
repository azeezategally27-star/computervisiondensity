// gate_map.c - simple animated gate map using GTK drawing area and Cairo
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "data.h"

typedef struct Plane { double x,y; double vx, vy; int gate; } Plane;

static GtkWidget *gm_win = NULL;
static GtkWidget *gm_area = NULL;
static Plane planes[8];
static int nplanes = 0;

static gboolean gm_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data){
    GtkAllocation alloc; gtk_widget_get_allocation(widget, &alloc);
    int w = alloc.width, h = alloc.height;
    // background
    cairo_set_source_rgb(cr, 0.96, 0.99, 1.0);
    cairo_paint(cr);
    // draw gates as boxes
    int ngates = 8; int cols = 4;
    for(int i=0;i<ngates;i++){
        int gx = 80 + (i%cols)*220; int gy = 60 + (i/cols)*160;
        cairo_set_source_rgb(cr, 0.94,0.94,0.97);
        cairo_rectangle(cr, gx-50, gy-30, 100, 60);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0.06,0.24,0.57);
        char buf[16]; snprintf(buf,16,"Gate %c%d", 'A', i+1);
        cairo_set_source_rgb(cr, 0.06,0.24,0.57);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12);
        cairo_move_to(cr, gx-30, gy);
        cairo_show_text(cr, buf);
    }
    // draw planes
    for(int i=0;i<nplanes;i++){
        cairo_set_source_rgb(cr, 0.2, 0.5, 0.9);
        cairo_arc(cr, planes[i].x, planes[i].y, 10, 0, 2*M_PI);
        cairo_fill(cr);
        // tail
        cairo_set_source_rgb(cr, 0.9, 0.2, 0.2);
        cairo_rectangle(cr, planes[i].x-12, planes[i].y-2, 6, 4);
        cairo_fill(cr);
    }
    return FALSE;
}

static gboolean gm_tick(gpointer data){
    // move planes
    for(int i=0;i<nplanes;i++){
        planes[i].x += planes[i].vx; planes[i].y += planes[i].vy;
        // bounce within area
        if(planes[i].x < 20 || planes[i].x > 760) planes[i].vx *= -1;
        if(planes[i].y < 20 || planes[i].y > 520) planes[i].vy *= -1;
    }
    if(gm_area) gtk_widget_queue_draw(gm_area);
    return TRUE;
}

void gate_map_open_window(GtkWidget *parent){
    if(gm_win){ gtk_window_present(GTK_WINDOW(gm_win)); return; }
    gm_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(gm_win), 800, 560);
    gtk_window_set_title(GTK_WINDOW(gm_win), "Gate Map — Animated");
    gm_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(gm_area, 800, 560);
    gtk_container_add(GTK_CONTAINER(gm_win), gm_area);
    g_signal_connect(gm_area, "draw", G_CALLBACK(gm_on_draw), NULL);
    // initialize planes
    srand(time(NULL));
    nplanes = 6;
    for(int i=0;i<nplanes;i++){
        planes[i].x = 100 + rand()%600; planes[i].y = 80 + rand()%400;
        planes[i].vx = ((rand()%100)/100.0 - 0.5) * 2.0;
        planes[i].vy = ((rand()%100)/100.0 - 0.5) * 2.0;
    }
    g_timeout_add(40, gm_tick, NULL);
    gtk_widget_show_all(gm_win);
}

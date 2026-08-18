// baggage.c - conveyor visualization with simple animated dots
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <time.h>

typedef struct Bag { double x,y; double speed; int color; } Bag;
static Bag bags[64]; static int nbag = 0;
static GtkWidget *bg_win = NULL; static GtkWidget *bg_area = NULL;

static gboolean bg_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data){
    GtkAllocation alloc; gtk_widget_get_allocation(widget, &alloc);
    int w = alloc.width, h = alloc.height;
    cairo_set_source_rgb(cr, 0.98, 0.98, 0.99); cairo_paint(cr);
    // draw conveyor belt
    cairo_set_source_rgb(cr, 0.85, 0.85, 0.9);
    cairo_rectangle(cr, 40, h/2 - 30, w-80, 60); cairo_fill(cr);
    // draw bags
    for(int i=0;i<nbag;i++){
        double x = bags[i].x; double y = bags[i].y;
        cairo_set_source_rgb(cr, (bags[i].color&1)?0.2:0.9, (bags[i].color&2)?0.4:0.7, (bags[i].color&4)?0.8:0.3);
        cairo_rectangle(cr, x, y, 18, 12); cairo_fill(cr);
    }
    return FALSE;
}

static gboolean bg_tick(gpointer data){
    for(int i=0;i<nbag;i++){
        bags[i].x += bags[i].speed;
        if(bags[i].x > 760){ bags[i].x = 40; }
    }
    if(bg_area) gtk_widget_queue_draw(bg_area);
    return TRUE;
}

void baggage_open_window(GtkWidget *parent){
    if(bg_win){ gtk_window_present(GTK_WINDOW(bg_win)); return; }
    bg_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(bg_win), 820, 260);
    gtk_window_set_title(GTK_WINDOW(bg_win), "Baggage — Conveyor Simulation");
    bg_area = gtk_drawing_area_new(); gtk_widget_set_size_request(bg_area, 820, 260);
    gtk_container_add(GTK_CONTAINER(bg_win), bg_area);
    g_signal_connect(bg_area, "draw", G_CALLBACK(bg_on_draw), NULL);
    // initialize bags
    srand(time(NULL)); nbag = 20;
    for(int i=0;i<nbag;i++){
        bags[i].x = 40 + rand()%720; bags[i].y = 100 + (rand()%20 - 10);
        bags[i].speed = 1.0 + (rand()%100)/200.0;
        bags[i].color = rand()%8;
    }
    g_timeout_add(30, bg_tick, NULL);
    gtk_widget_show_all(bg_win);
}

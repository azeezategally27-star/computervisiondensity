// scheduler.c - simple Gantt-like resource scheduler visualization
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <time.h>

static GtkWidget *sch_win = NULL; static GtkWidget *sch_area = NULL;

static gboolean sch_on_draw(GtkWidget *widget, cairo_t *cr, gpointer data){
    GtkAllocation alloc; gtk_widget_get_allocation(widget, &alloc);
    int w = alloc.width, h = alloc.height;
    cairo_set_source_rgb(cr, 0.99, 0.99, 1.0); cairo_paint(cr);
    // time axis
    int rows = 4; int rowh = (h-40)/rows;
    for(int r=0;r<rows;r++){
        cairo_set_source_rgb(cr, 0.95,0.95,0.97);
        cairo_rectangle(cr, 40, 20 + r*rowh, w-80, rowh-8); cairo_fill(cr);
        // draw few sample tasks with animated position
        double t = (double)time(NULL);
        double offset = fmod(t*0.3 + r*20, w-120);
        cairo_set_source_rgb(cr, 0.2+0.1*r, 0.5-0.05*r, 0.9-0.1*r);
        cairo_rectangle(cr, 60 + offset, 30 + r*rowh, 120, rowh-24); cairo_fill(cr);
        cairo_set_source_rgb(cr, 1,1,1);
        cairo_move_to(cr, 80 + offset, 30 + r*rowh + (rowh/2)); cairo_show_text(cr, "Crew A");
    }
    return FALSE;
}

void scheduler_open_window(GtkWidget *parent){
    if(sch_win){ gtk_window_present(GTK_WINDOW(sch_win)); return; }
    sch_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(sch_win), 900, 340);
    gtk_window_set_title(GTK_WINDOW(sch_win), "Resource Scheduler — Animated Gantt");
    sch_area = gtk_drawing_area_new(); gtk_widget_set_size_request(sch_area, 900, 340);
    gtk_container_add(GTK_CONTAINER(sch_win), sch_area);
    g_signal_connect(sch_area, "draw", G_CALLBACK(sch_on_draw), NULL);
    g_timeout_add(80, (GSourceFunc)gtk_widget_queue_draw, sch_area);
    gtk_widget_show_all(sch_win);
}

// ui.c - builds GTK UI and updates UI on data changes (extended with panel buttons)
#include <gtk/gtk.h>
#include <string.h>
#include "data.h"

// declarations for new panels
void assistant_open_window(GtkWidget *parent);
void gate_map_open_window(GtkWidget *parent);
void baggage_open_window(GtkWidget *parent);
void scheduler_open_window(GtkWidget *parent);

typedef struct AppWidgets {
    GtkWidget *window;
    GtkWidget *listbox;
    GtkWidget *chat_view;
    guint refresh_timer;
} AppWidgets;

AppWidgets *global_app = NULL;

static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);

AppWidgets* ui_build(){
    AppWidgets *app = g_malloc0(sizeof(AppWidgets));

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 700);
    gtk_window_set_title(GTK_WINDOW(app->window), "Mauritius Airport — Flight Assistant Panel");

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css, "advanced_assistant/resources.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_add(GTK_CONTAINER(app->window), hbox);

    // left: flight list
    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(hbox), left, TRUE, TRUE, 6);

    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<span size='x-large' weight='bold' class='header-label'>Flight Operations — Mauritius (MRU)</span>");
    gtk_box_pack_start(GTK_BOX(left), header, FALSE, FALSE, 4);

    app->listbox = gtk_list_box_new();
    gtk_widget_set_vexpand(app->listbox, TRUE);
    gtk_box_pack_start(GTK_BOX(left), app->listbox, TRUE, TRUE, 4);

    // right: assistant/chat and details
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(hbox), right, FALSE, FALSE, 6);
    gtk_widget_set_size_request(right, 420, -1);

    GtkWidget *chat_label = gtk_label_new("Assistant"); gtk_box_pack_start(GTK_BOX(right), chat_label, FALSE, FALSE, 2);
    app->chat_view = gtk_text_view_new(); gtk_text_view_set_editable(GTK_TEXT_VIEW(app->chat_view), FALSE);
    gtk_widget_set_size_request(app->chat_view, 420, 220);
    gtk_box_pack_start(GTK_BOX(right), app->chat_view, FALSE, FALSE, 2);

    // controls
    GtkWidget *btn_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *btn_res = gtk_button_new_with_label("Reschedule...");
    GtkWidget *btn_export = gtk_button_new_with_label("Export CSV");
    GtkWidget *btn_gate = gtk_button_new_with_label("Gate Map");
    GtkWidget *btn_baggage = gtk_button_new_with_label("Baggage Panel");
    GtkWidget *btn_scheduler = gtk_button_new_with_label("Scheduler");
    gtk_container_add(GTK_CONTAINER(btn_box), btn_res);
    gtk_container_add(GTK_CONTAINER(btn_box), btn_export);
    gtk_container_add(GTK_CONTAINER(btn_box), btn_gate);
    gtk_container_add(GTK_CONTAINER(btn_box), btn_baggage);
    gtk_container_add(GTK_CONTAINER(btn_box), btn_scheduler);
    gtk_box_pack_end(GTK_BOX(right), btn_box, FALSE, FALSE, 4);

    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // populate list
    FlightList *fl = data_get_all();
    for(int i=0;i<fl->n;i++){
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *col1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        char buf[256]; snprintf(buf,256,"<span class='flight-no'>%s</span> <span class='dest'>%s</span>", fl->items[i].flight_no, fl->items[i].dest);
        GtkWidget *lbl = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(lbl), buf);
        gtk_box_pack_start(GTK_BOX(col1), lbl, FALSE, FALSE, 0);
        char sub[128]; snprintf(sub,128,"Gate %s • %s", fl->items[i].gate, fl->items[i].scheduled);
        GtkWidget *lbl2 = gtk_label_new(sub); gtk_box_pack_start(GTK_BOX(col1), lbl2, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(h), col1, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(row), h);
        gtk_container_add(GTK_CONTAINER(app->listbox), row);
        g_object_set_data(G_OBJECT(row), "flight_id", GINT_TO_POINTER(fl->items[i].id));
    }
    free(fl->items); free(fl);

    g_signal_connect(app->listbox, "row-activated", G_CALLBACK(on_row_activated), app);

    // button handlers open new windows
    g_signal_connect(btn_gate, "clicked", G_CALLBACK((GCallback)gate_map_open_window), app->window);
    g_signal_connect(btn_baggage, "clicked", G_CALLBACK((GCallback)baggage_open_window), app->window);
    g_signal_connect(btn_scheduler, "clicked", G_CALLBACK((GCallback)scheduler_open_window), app->window);
    g_signal_connect(btn_res, "clicked", G_CALLBACK(on_row_activated), app->listbox);
    g_signal_connect(btn_export, "clicked", G_CALLBACK(on_row_activated), app->listbox);

    gtk_widget_show_all(app->window);
    global_app = app;
    return app;
}

static void open_reschedule_dialog(GtkWindow *parent, int flight_id){
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Reschedule Flight", parent, GTK_DIALOG_MODAL, "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new(); gtk_container_add(GTK_CONTAINER(content), grid);
    GtkWidget *entry_time = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(entry_time), "2026-08-18 12:00");
    GtkWidget *entry_gate = gtk_entry_new(); gtk_entry_set_text(GTK_ENTRY(entry_gate), "A1");
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("New time (YYYY-MM-DD HH:MM):"),0,0,1,1);
    gtk_grid_attach(GTK_GRID(grid), entry_time,1,0,1,1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("New Gate:"),0,1,1,1);
    gtk_grid_attach(GTK_GRID(grid), entry_gate,1,1,1,1);
    gtk_widget_show_all(dialog);
    if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT){
        const char *ntime = gtk_entry_get_text(GTK_ENTRY(entry_time));
        const char *ngate = gtk_entry_get_text(GTK_ENTRY(entry_gate));
        data_update_flight(flight_id, ntime, "Rescheduled", ngate);
        g_idle_add((GSourceFunc)ui_refresh_list, NULL);
    }
    gtk_widget_destroy(dialog);
}

static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data){
    // if activated from button, we open reschedule for selected or first item
    int id = 0;
    if(GTK_IS_LIST_BOX(row)){
        // called with listbox as user_data: pick first active
        GList *children = gtk_container_get_children(GTK_CONTAINER(box));
        if(children){
            GtkListBoxRow *r = GTK_LIST_BOX_ROW(children->data);
            id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(r), "flight_id"));
            g_list_free(children);
        }
    } else {
        id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "flight_id"));
    }
    if(id>0) open_reschedule_dialog(GTK_WINDOW(global_app->window), id);
}

// basic UI refresh helper (called by simulator via idle)
void ui_refresh_list(){
    // clear and repopulate
    GList *children = gtk_container_get_children(GTK_CONTAINER(global_app->listbox));
    for(GList *l=children;l;l=l->next){ gtk_widget_destroy(GTK_WIDGET(l->data)); }
    g_list_free(children);
    FlightList *fl = data_get_all();
    for(int i=0;i<fl->n;i++){
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *col1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        char buf[256]; snprintf(buf,256,"<span class='flight-no'>%s</span> <span class='dest'>%s</span>", fl->items[i].flight_no, fl->items[i].dest);
        GtkWidget *lbl = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(lbl), buf);
        gtk_box_pack_start(GTK_BOX(col1), lbl, FALSE, FALSE, 0);
        char sub[128]; snprintf(sub,128,"Gate %s • %s • %s", fl->items[i].gate, fl->items[i].scheduled, fl->items[i].status);
        GtkWidget *lbl2 = gtk_label_new(sub); gtk_box_pack_start(GTK_BOX(col1), lbl2, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(h), col1, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(row), h);
        gtk_container_add(GTK_CONTAINER(global_app->listbox), row);
        g_object_set_data(G_OBJECT(row), "flight_id", GINT_TO_POINTER(fl->items[i].id));
    }
    free(fl->items); free(fl);
    gtk_widget_show_all(global_app->window);
}

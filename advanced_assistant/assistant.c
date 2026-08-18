// assistant.c - simple local rule-based assistant with typing animation
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static GtkWidget *assistant_win = NULL;
static GtkWidget *assistant_view = NULL;
static GtkWidget *assistant_entry = NULL;

static char *make_response(const char *query){
    // naive rule-based responses; keep varied and realistic
    char *q = strdup(query); for(char *p=q; *p; ++p) *p = tolower(*p);
    if(strstr(q,"delay") || strstr(q,"delayed")){
        free(q);
        return strdup("I see several flights with potential delays. I recommend reallocating ground crew and offering meal vouchers if delay exceeds 90 minutes.");
    }
    if(strstr(q,"resched") || strstr(q,"reschedule") || strstr(q,"change time")){
        free(q);
        return strdup("You can reschedule directly from the flight list. I can propose optimal times avoiding gate conflicts — here are three options: +15min, +45min, +90min.");
    }
    if(strstr(q,"baggage")){
        free(q);
        return strdup("Baggage loading status: 3 flights have delayed baggage. I suggest prioritizing flight MK123 luggage to reduce passenger disruption.");
    }
    if(strstr(q,"status")){
        free(q);
        return strdup("Overall system status: 7 flights on schedule, 1 delayed, 1 boarding. Gate utilization at 68%.");
    }
    free(q);
    return strdup("I'm the local assistant prototype. I can suggest reschedules, check baggage, or highlight critical alerts. Try asking about delays, rescheduling, or baggage.");
}

static gboolean append_text_chunk(gpointer data){
    // data is a struct with buffer pointer and offset; for simplicity, just append whole string gradually
    char *buf = (char*)data;
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(assistant_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(tb, &end);
    gtk_text_buffer_insert(tb, &end, buf, -1);
    g_free(buf);
    return FALSE;
}

static void on_send(GtkButton *btn, gpointer user_data){
    const char *q = gtk_entry_get_text(GTK_ENTRY(assistant_entry));
    if(!q || strlen(q)==0) return;
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(assistant_view));
    GtkTextIter end; gtk_text_buffer_get_end_iter(tb, &end);
    char out[1024]; snprintf(out,1024, "You: %s\n", q);
    gtk_text_buffer_insert(tb, &end, out, -1);
    // produce response with typing animation (chunks)
    char *resp = make_response(q);
    // simulate typing by scheduling small chunks
    int len = strlen(resp);
    int chunk = 60; int i=0;
    while(i < len){
        int take = (len - i) < chunk ? (len - i) : chunk;
        char *part = g_strndup(resp + i, take);
        g_idle_add(append_text_chunk, part);
        i += take;
        // insert slight delay between chunks
        g_usleep(80000);
    }
    g_idle_add(append_text_chunk, g_strdup("\n"));
    free(resp);
    gtk_entry_set_text(GTK_ENTRY(assistant_entry), "");
}

void assistant_open_window(GtkWidget *parent){
    if(assistant_win){ gtk_window_present(GTK_WINDOW(assistant_win)); return; }
    assistant_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(assistant_win), 480, 320);
    gtk_window_set_title(GTK_WINDOW(assistant_win), "Local Assistant — Prototype");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(assistant_win), vbox);

    assistant_view = gtk_text_view_new(); gtk_text_view_set_editable(GTK_TEXT_VIEW(assistant_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), assistant_view, TRUE, TRUE, 4);

    assistant_entry = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(assistant_entry), "Ask about delays, reschedule, baggage...");
    gtk_box_pack_start(GTK_BOX(vbox), assistant_entry, FALSE, FALSE, 2);

    GtkWidget *btn = gtk_button_new_with_label("Send"); gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_send), NULL);

    gtk_widget_show_all(assistant_win);
}

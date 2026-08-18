// main.c - assistant chat UI and response composer
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "kb.h"

#define DB_PATH "advanced_assistant_chat/assistant_chat.db"
#define KB_DIR "advanced_assistant_chat/kb"

typedef struct Message { char *text; int from_bot; double opacity; struct Message *next; } Message;

static GtkWidget *win;
static GtkWidget *entry;
static GtkWidget *vbox;
static Message *msg_head = NULL, *msg_tail = NULL;
static guint anim_timer = 0;

// append message to list
static void add_message(const char *text, int from_bot){
    Message *m = malloc(sizeof(Message)); m->text = strdup(text); m->from_bot = from_bot; m->opacity = 0.0; m->next = NULL;
    if(!msg_tail){ msg_head = msg_tail = m; } else { msg_tail->next = m; msg_tail = m; }
}

// render messages as labels inside vbox, with opacity applied via style context
static gboolean refresh_ui(gpointer data){
    // clear children
    GList *kids = gtk_container_get_children(GTK_CONTAINER(vbox));
    for(GList *l=kids;l;l=l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(kids);
    // add each message with animation: opacity maps to alpha on label via css? GTK labels don't support alpha easily; instead use RGBA in markup
    Message *m = msg_head; while(m){
        char *escaped = g_markup_escape_text(m->text, -1);
        char buf[4096]; if(m->from_bot){
            // bot bubble
            double alpha = fmin(1.0, m->opacity);
            int a = (int)(alpha*255);
            snprintf(buf, sizeof(buf), "<span foreground='#0b3d91' size='medium'>%s</span>", escaped);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
            GtkWidget *lbl = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(lbl), buf); gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 6);
        } else {
            snprintf(buf, sizeof(buf), "<span foreground='white' background='#0b74de' size='medium'>%s</span>", escaped);
            GtkWidget *frame = gtk_frame_new(NULL);
            gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
            GtkWidget *lbl = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(lbl), buf); gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
            gtk_container_add(GTK_CONTAINER(frame), lbl);
            gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 6);
        }
        g_free(escaped);
        m->opacity = fmin(1.0, m->opacity + 0.12);
        m = m->next;
    }
    gtk_widget_show_all(win);
    return TRUE;
}

// typing animation: add response in small chunks
static void type_out_response(const char *resp){
    int len = strlen(resp); int pos = 0; char buf[512];
    while(pos < len){
        int take = (len - pos) > 60 ? 60 : (len - pos);
        strncpy(buf, resp + pos, take); buf[take]=0;
        add_message(buf, 1);
        pos += take;
        // refresh UI and sleep briefly to simulate typing
        refresh_ui(NULL);
        usleep(90000);
    }
}

// compose response: retrieve top KB matches and create answer
static char *compose_response(const char *query){
    int n=0; char **res = kb_query(query, 4, &n);
    // build answer
    char *out = malloc(8192); out[0]=0;
    if(n==0){
        snprintf(out,8192, "I couldn't find dedicated docs in KB. Suggest checking SOPs or provide more detail.\nQuick suggestion: to reschedule a flight, check gate availability, notify ground crew, and update passenger notifications.");
    } else {
        strcat(out, "I found the following relevant guidance:\n");
        for(int i=0;i<n;i++){
            strcat(out, "- "); strcat(out, res[i]); strcat(out, "\n");
        }
        strcat(out, "\nSuggested next steps:\n1) Confirm gate & crew availability\n2) Update schedule in system\n3) Notify stakeholders\nIf you want, I can draft the reschedule message or propose specific times.");
    }
    kb_free_results(res, n);
    return out;
}

// handle send
static void on_send_clicked(GtkButton *btn, gpointer user_data){
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry)); if(!text || strlen(text)==0) return;
    add_message(text, 0);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
    refresh_ui(NULL);
    // compose reply
    char *reply = compose_response(text);
    // simulate typing in background
    type_out_response(reply);
    free(reply);
}

int main(int argc, char **argv){
    gtk_init(&argc, &argv);
    int do_train = 0;
    for(int i=1;i<argc;i++){ if(strcmp(argv[i],"--train")==0) do_train=1; }
    if(!kb_init(DB_PATH)){ fprintf(stderr, "Failed to open KB DB\n"); return 1; }
    if(do_train){
        printf("Training KB from %s ...\n", KB_DIR);
        kb_train_from_dir(KB_DIR);
        printf("Training complete. Run './assistant_chat' to start the app.\n");
        kb_close(); return 0;
    }
    // UI
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL); gtk_window_set_default_size(GTK_WINDOW(win), 800, 600); gtk_window_set_title(GTK_WINDOW(win), "Advanced Assistant — Offline (Mauritius)");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); gtk_container_add(GTK_CONTAINER(win), box);
    GtkWidget *hdr = gtk_label_new(NULL); gtk_label_set_markup(GTK_LABEL(hdr), "<span weight='bold' size='large'>Advanced Assistant (Offline)</span>"); gtk_box_pack_start(GTK_BOX(box), hdr, FALSE, FALSE, 6);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); GtkWidget *sc = gtk_scrolled_window_new(NULL, NULL); gtk_container_add(GTK_CONTAINER(sc), vbox); gtk_widget_set_vexpand(sc, TRUE); gtk_box_pack_start(GTK_BOX(box), sc, TRUE, TRUE, 4);
    GtkWidget *entry_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6); entry = gtk_entry_new(); gtk_box_pack_start(GTK_BOX(entry_box), entry, TRUE, TRUE, 4); GtkWidget *btn = gtk_button_new_with_label("Send"); gtk_box_pack_start(GTK_BOX(entry_box), btn, FALSE, FALSE, 4); gtk_box_pack_start(GTK_BOX(box), entry_box, FALSE, FALSE, 4);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_send_clicked), NULL);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    // initial prompt
    add_message("Hello — I'm the local Advanced Assistant for Mauritius airport. Ask about rescheduling, baggage, or operations.", 1);
    refresh_ui(NULL);
    gtk_widget_show_all(win);
    gtk_main();
    kb_close();
    return 0;
}

// api_server.c - simple local HTTP API for rescheduling (uses POSIX sockets simplified)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "data.h"

static pthread_t server_thread;
static int server_running = 0;
static int server_fd = -1;

static void handle_client(int client){
    char buf[4096]; int r = read(client, buf, sizeof(buf)-1); if(r<=0){ close(client); return; }
    buf[r]=0;
    // naive HTTP parsing - look for POST /reschedule?id=...&time=...&gate=...
    if(strstr(buf, "POST /reschedule")!=NULL){
        // parse query from first line
        char *line = strtok(buf, "\r\n");
        char *q = strchr(line, '?');
        int id=0; char time[64]={0}; char gate[16]={0};
        if(q){
            char *p = q+1; while(p && *p && *p!=' '){
                char key[64], val[256]; if(sscanf(p, "%63[^=&]=%255[^&]", key, val)==2){
                    if(strcmp(key,"id")==0) id = atoi(val);
                    else if(strcmp(key,"time")==0) strncpy(time, val, 63);
                    else if(strcmp(key,"gate")==0) strncpy(gate, val, 15);
                }
                char *amp = strchr(p,'&'); if(!amp) break; p = amp+1;
            }
        }
        if(id>0){
            data_update_flight(id, time[0]?time:"TBD", "Rescheduled", gate[0]?gate:"TBD");
            const char *resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK";
            write(client, resp, strlen(resp));
        } else {
            const char *resp = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing id";
            write(client, resp, strlen(resp));
        }
    } else {
        const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n";
        write(client, resp, strlen(resp));
    }
    close(client);
}

static void *server_loop(void *arg){
    int port = 8888;
    struct sockaddr_in addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(port);
    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr))<0){ perror("bind"); return NULL; }
    listen(server_fd, 8);
    while(server_running){
        int client = accept(server_fd, NULL, NULL);
        if(client>=0) handle_client(client);
    }
    close(server_fd); return NULL;
}

void api_server_start(){ if(server_running) return; server_running=1; pthread_create(&server_thread,NULL,server_loop,NULL); }
void api_server_stop(){ if(!server_running) return; server_running=0; shutdown(server_fd, SHUT_RDWR); pthread_join(server_thread,NULL); }

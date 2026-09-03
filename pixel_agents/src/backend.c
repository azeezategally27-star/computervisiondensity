/*
 * pixel_agents backend (C)
 * Lightweight TCP server for a demo pixel-agents room.
 * Listens on 127.0.0.1:9191 and accepts simple newline-terminated commands.
 * Commands:
 *   LIST_AGENTS\n
 *   TASKS <agent_id>\n
 *   PERFORM_TASK <agent_id> <task_index>\n
 * Responses are simple JSON-like strings. All behavior is deterministic and
 * implemented plainly in C for readability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 9191
#define BACKLOG 8
#define MAXLINE 2048

typedef struct {
    int id;
    const char *name;
    const char *role;
    int x, y; // world coordinates (for frontend placement)
} agent_t;

typedef struct {
    int agent_id;
    const char *tasks[8];
    int task_count;
} agent_tasks_t;

static agent_t agents[] = {
    {1, "Aisha", "Check-in Agent", 200, 150},
    {2, "Ravi", "Security Officer", 400, 120},
    {3, "Marie", "Baggage Handler", 600, 180},
    {4, "Jean", "Customs Officer", 800, 200},
    {5, "Fatima", "Gate Agent", 1000, 140},
    {6, "Olivier", "Ground Crew", 300, 400},
    {7, "Selena", "Customer Service", 500, 380},
    {8, "Kumar", "Immigration", 700, 360},
    {9, "Lina", "Cleaning Crew", 900, 420},
    {10, "Paul", "Fire Safety", 1100, 320},
    {11, "Rashid", "Ramp Controller", 1300, 280},
    {12, "Noel", "Maintenance", 1500, 450},
    {13, "Iris", "Pilot Liaison", 1700, 220},
    {14, "Chen", "Security K9", 1900, 200},
    {15, "Maya", "Medical", 2100, 400}
};
static const int AGENT_COUNT = sizeof(agents)/sizeof(agents[0]);

static agent_tasks_t tasks_map[] = {
    {1, {"Issue Boarding Pass", "Verify Documents", "Upgrade Seat", NULL}, 3},
    {2, {"X-ray Scan Bag", "Manual Inspection", "Flag Suspicious", NULL}, 3},
    {3, {"Route Luggage", "Load into Cart", "Report Missing Tag", NULL}, 3},
    {4, {"Inspect Goods", "Stamp Clearance", "Refer to Security", NULL}, 3},
    {5, {"Call Boarding", "Assign Standby", "Close Gate", NULL}, 3},
    {6, {"Tow Aircraft", "Refuel Assist", "Position Vehicle", NULL}, 3},
    {7, {"Handle Complaint", "Issue Voucher", "Lookup Reservation", NULL}, 3},
    {8, {"Verify Passport", "Check Visa", "Record Entry", NULL}, 3},
    {9, {"Clean Spill", "Sanitize Seat", "Dispose Trash", NULL}, 3},
    {10,{"Run Fire Drill", "Inspect Alarm", "Evacuate Zone", NULL}, 3},
    {11,{"Clear Taxiway", "Communicate with Tower", "Coordinate Pushback", NULL}, 3},
    {12,{"Repair Belt", "Replace Part", "Report Fault", NULL}, 3},
    {13,{"Confirm Flight Plan", "Relay ETA", "Coordinate Crew", NULL}, 3},
    {14,{"Search Area", "Handler Assist", "Stand Down", NULL}, 3},
    {15,{"Attend Passenger", "Provide Aid", "Call Ambulance", NULL}, 3}
};
static const int TASKMAP_COUNT = sizeof(tasks_map)/sizeof(tasks_map[0]);

static pthread_mutex_t loglock = PTHREAD_MUTEX_INITIALIZER;

static void ensure_logs_dir(){ mkdir("pixel_agents/logs", 0700); }

static void append_log(const char *entry){
    pthread_mutex_lock(&loglock);
    ensure_logs_dir();
    FILE *f = fopen("pixel_agents/logs/events.txt","a");
    if(f){ fprintf(f, "%s\n", entry); fclose(f); }
    pthread_mutex_unlock(&loglock);
}

static agent_tasks_t *find_tasks_for_agent(int agent_id){
    for(int i=0;i<TASKMAP_COUNT;i++) if(tasks_map[i].agent_id == agent_id) return &tasks_map[i];
    return NULL;
}

static void handle_list_agents(int client){
    char out[MAXLINE]; int n = 0;
    n += snprintf(out+n, sizeof(out)-n, "AGENTS [");
    for(int i=0;i<AGENT_COUNT;i++){
        n += snprintf(out+n, sizeof(out)-n, "{\"id\":%d,\"name\":\"%s\",\"role\":\"%s\",\"x\":%d,\"y\":%d}%s",
            agents[i].id, agents[i].name, agents[i].role, agents[i].x, agents[i].y, (i+1<AGENT_COUNT)?",":"");
    }
    n += snprintf(out+n, sizeof(out)-n, "]\n");
    send(client, out, strlen(out), 0);
}

static void handle_tasks_for_agent(int client, int agent_id){
    agent_tasks_t *t = find_tasks_for_agent(agent_id);
    char out[MAXLINE]; if(!t){ snprintf(out, sizeof(out), "TASKS_FAIL agent_not_found\n"); send(client,out,strlen(out),0); return; }
    int n = snprintf(out, sizeof(out), "TASKS %d [", agent_id);
    for(int i=0;i<t->task_count;i++){
        n += snprintf(out+n, sizeof(out)-n, "\"%s\"%s", t->tasks[i], (i+1<t->task_count)?",":"");
    }
    n += snprintf(out+n, sizeof(out)-n, "]\n");
    send(client, out, strlen(out), 0);
}

static void handle_perform_task(int client, int agent_id, int task_index){
    char out[MAXLINE];
    agent_tasks_t *t = find_tasks_for_agent(agent_id);
    if(!t || task_index < 0 || task_index >= t->task_count){ snprintf(out,sizeof(out),"TASK_FAIL invalid_task\n"); send(client,out,strlen(out),0); return; }
    // simulate performing the task (deterministic pseudo-delay)
    int delay_ms = 500 + ((agent_id * 37 + task_index * 97) % 1200);
    // respond immediately with a TASK_STARTED, then after delay append log (frontend can consider it done)
    snprintf(out, sizeof(out), "TASK_STARTED %d %d %d\n", agent_id, task_index, delay_ms);
    send(client, out, strlen(out), 0);
    // log event
    char entry[512]; snprintf(entry, sizeof(entry), "Agent %d performed task '%s' (index %d) — duration %dms", agent_id, t->tasks[task_index], task_index, delay_ms);
    append_log(entry);
}

static void *client_thread(void *arg){
    int client = *(int*)arg; free(arg);
    char buf[MAXLINE]; ssize_t n;
    while((n = recv(client, buf, sizeof(buf)-1, 0)) > 0){
        buf[n] = '\0'; // trim newlines
        char *nl = strchr(buf,'\n'); if(nl) *nl='\0';
        if(strlen(buf)==0) continue;
        if(strcmp(buf, "LIST_AGENTS") == 0){ handle_list_agents(client); }
        else if(strncmp(buf, "TASKS ",6) == 0){ int aid = atoi(buf+6); handle_tasks_for_agent(client, aid); }
        else if(strncmp(buf, "PERFORM_TASK ",13) == 0){ int aid, tid; if(sscanf(buf+13, "%d %d", &aid, &tid) == 2) handle_perform_task(client, aid, tid); else { char out[128]; snprintf(out,sizeof(out),"TASK_FAIL bad_format\n"); send(client,out,strlen(out),0);} }
        else { char out[128]; snprintf(out,sizeof(out),"ERR unknown_command\n"); send(client,out,strlen(out),0); }
    }
    close(client); return NULL;
}

int main(){
    mkdir("pixel_agents/logs", 0700);
    int sockfd, newfd; struct sockaddr_in serv, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){ perror("socket"); return 1; }
    memset(&serv,0,sizeof(serv)); serv.sin_family = AF_INET; serv.sin_addr.s_addr = inet_addr("127.0.0.1"); serv.sin_port = htons(PORT);
    int yes = 1; setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if(bind(sockfd, (struct sockaddr*)&serv, sizeof(serv))<0){ perror("bind"); return 1; }
    if(listen(sockfd, BACKLOG) < 0){ perror("listen"); return 1; }
    printf("Pixel backend listening on 127.0.0.1:%d\n", PORT);
    while(1){ socklen_t cli_len = sizeof(cli); newfd = accept(sockfd, (struct sockaddr*)&cli, &cli_len); if(newfd < 0){ perror("accept"); continue; } int *pclient = malloc(sizeof(int)); *pclient = newfd; pthread_t tid; pthread_create(&tid, NULL, client_thread, pclient); pthread_detach(tid); }
    close(sockfd); return 0;
}

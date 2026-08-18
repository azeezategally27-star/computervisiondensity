// data.c - sqlite-backed flight store and helpers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "data.h"

static sqlite3 *db = NULL;

int data_init(const char *path){
    if(sqlite3_open(path, &db) != SQLITE_OK) return 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS flights("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "flight_no TEXT, dest TEXT, scheduled TEXT, status TEXT, gate TEXT, luggage TEXT, pax INTEGER, priority INTEGER, last_update INTEGER);";
    char *err = NULL;
    if(sqlite3_exec(db, sql, 0,0,&err) != SQLITE_OK){
        fprintf(stderr, "DB init error: %s\n", err); sqlite3_free(err); return 0;
    }
    return 1;
}

void data_close(){ if(db) sqlite3_close(db); }

int data_count_flights(){
    const char *sql = "SELECT COUNT(*) FROM flights;";
    sqlite3_stmt *stmt; int cnt=0;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)==SQLITE_OK){
        if(sqlite3_step(stmt)==SQLITE_ROW) cnt = sqlite3_column_int(stmt,0);
    }
    sqlite3_finalize(stmt); return cnt;
}

void data_seed_if_empty(){
    if(data_count_flights()>0) return;
    // seed with sample flights tuned for Mauritius (MRU)
    const char *flights[][5] = {
        {"MK123","Mauritius (MRU)","2026-08-18 09:40","On Time","A12"},
        {"QR450","Doha (DOH)","2026-08-18 10:05","On Time","B3"},
        {"AF789","Paris (CDG)","2026-08-18 11:20","Delayed","C1"},
        {"ET555","Addis Ababa (ADD)","2026-08-18 12:00","Boarding","A5"},
        {"SA200","Johannesburg (JNB)","2026-08-18 12:45","On Time","D2"},
        {"BA099","London (LHR)","2026-08-18 14:10","On Time","C4"},
        {"MU321","Hong Kong (HKG)","2026-08-18 16:30","Scheduled","E1"},
        {"AI777","New Delhi (DEL)","2026-08-18 17:05","Scheduled","B7"}
    };
    const char *sql = "INSERT INTO flights(flight_no,dest,scheduled,status,gate,luggage,pax,priority,last_update) VALUES(?,?,?,?,?,?,?, ?,?);";
    sqlite3_stmt *stmt;
    for(int i=0;i<8;i++){
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)==SQLITE_OK){
            sqlite3_bind_text(stmt,1,flights[i][0],-1,SQLITE_STATIC);
            sqlite3_bind_text(stmt,2,flights[i][1],-1,SQLITE_STATIC);
            sqlite3_bind_text(stmt,3,flights[i][2],-1,SQLITE_STATIC);
            sqlite3_bind_text(stmt,4,flights[i][3],-1,SQLITE_STATIC);
            sqlite3_bind_text(stmt,5,flights[i][4],-1,SQLITE_STATIC);
            sqlite3_bind_text(stmt,6,"OK",-1,SQLITE_STATIC);
            sqlite3_bind_int(stmt,7,150 + (i*10)%90);
            sqlite3_bind_int(stmt,8,(i%3==0)?1:0);
            sqlite3_bind_int(stmt,9,(int)time(NULL));
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }
}

FlightList *data_get_all(){
    const char *sql = "SELECT id,flight_no,dest,scheduled,status,gate,luggage,pax,priority,last_update FROM flights ORDER BY scheduled;";
    sqlite3_stmt *stmt;
    FlightList *list = malloc(sizeof(FlightList)); list->n=0; list->items=NULL;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)==SQLITE_OK){
        while(sqlite3_step(stmt)==SQLITE_ROW){
            Flight f = {0};
            f.id = sqlite3_column_int(stmt,0);
            f.flight_no = strdup((const char*)sqlite3_column_text(stmt,1));
            f.dest = strdup((const char*)sqlite3_column_text(stmt,2));
            f.scheduled = strdup((const char*)sqlite3_column_text(stmt,3));
            f.status = strdup((const char*)sqlite3_column_text(stmt,4));
            f.gate = strdup((const char*)sqlite3_column_text(stmt,5));
            f.luggage = strdup((const char*)sqlite3_column_text(stmt,6));
            f.pax = sqlite3_column_int(stmt,7);
            f.priority = sqlite3_column_int(stmt,8);
            f.last_update = sqlite3_column_int(stmt,9);
            list->items = realloc(list->items, sizeof(Flight)*(list->n+1));
            list->items[list->n++] = f;
        }
    }
    sqlite3_finalize(stmt);
    return list;
}

int data_update_flight(int id, const char *scheduled, const char *status, const char *gate){
    const char *sql = "UPDATE flights SET scheduled=?, status=?, gate=?, last_update=? WHERE id=?;";
    sqlite3_stmt *stmt; int rc=0;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)==SQLITE_OK){
        sqlite3_bind_text(stmt,1,scheduled,-1,SQLITE_STATIC);
        sqlite3_bind_text(stmt,2,status,-1,SQLITE_STATIC);
        sqlite3_bind_text(stmt,3,gate,-1,SQLITE_STATIC);
        sqlite3_bind_int(stmt,4,(int)time(NULL));
        sqlite3_bind_int(stmt,5,id);
        if(sqlite3_step(stmt) == SQLITE_DONE) rc=1;
    }
    sqlite3_finalize(stmt);
    return rc;
}

int data_set_luggage(int id, const char *l){
    const char *sql = "UPDATE flights SET luggage=?, last_update=? WHERE id=?;";
    sqlite3_stmt *stmt; int rc=0;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)==SQLITE_OK){
        sqlite3_bind_text(stmt,1,l,-1,SQLITE_STATIC);
        sqlite3_bind_int(stmt,2,(int)time(NULL));
        sqlite3_bind_int(stmt,3,id);
        if(sqlite3_step(stmt) == SQLITE_DONE) rc=1;
    }
    sqlite3_finalize(stmt);
    return rc;
}

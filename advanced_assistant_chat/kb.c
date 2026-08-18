// kb.c - simple SQLite FTS5 KB implementation
#include "kb.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static sqlite3 *db = NULL;

int kb_init(const char *dbpath){
    if(sqlite3_open(dbpath, &db) != SQLITE_OK) return 0;
    const char *sql = "CREATE VIRTUAL TABLE IF NOT EXISTS docs USING fts5(filename, content);";
    char *err = NULL;
    if(sqlite3_exec(db, sql, 0,0,&err) != SQLITE_OK){ fprintf(stderr, "KB init error: %s\n", err); sqlite3_free(err); return 0; }
    return 1;
}

static char *read_file(const char *path){
    FILE *f = fopen(path, "rb"); if(!f) return NULL; fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    char *buf = malloc(sz+1); if(!buf){ fclose(f); return NULL; }
    fread(buf,1,sz,f); buf[sz]=0; fclose(f); return buf;
}

int kb_train_from_dir(const char *dir_path){
    DIR *d = opendir(dir_path); if(!d) return 0;
    struct dirent *ent; char path[4096];
    sqlite3_stmt *stmt = NULL; const char *ins = "INSERT INTO docs(filename, content) VALUES(?, ?);";
    while((ent = readdir(d))){
        if(ent->d_type == DT_REG){
            snprintf(path, sizeof(path), "%s/%s", dir_path, ent->d_name);
            char *txt = read_file(path); if(!txt) continue;
            if(sqlite3_prepare_v2(db, ins, -1, &stmt, NULL)==SQLITE_OK){
                sqlite3_bind_text(stmt, 1, ent->d_name, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, txt, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            free(txt);
        }
    }
    closedir(d);
    return 1;
}

char **kb_query(const char *query, int max_results, int *out_count){
    char sql[4096]; snprintf(sql, sizeof(sql), "SELECT filename || '::' || snippet(docs, 1, '[', ']', '...', -1) AS s FROM docs WHERE docs MATCH ? LIMIT %d;", max_results);
    sqlite3_stmt *stmt; char **res = NULL; int n=0;
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)==SQLITE_OK){
        sqlite3_bind_text(stmt, 1, query, -1, SQLITE_STATIC);
        while(sqlite3_step(stmt)==SQLITE_ROW){
            const unsigned char *t = sqlite3_column_text(stmt, 0);
            res = realloc(res, sizeof(char*)*(n+1)); res[n++] = strdup((const char*)t);
        }
    }
    sqlite3_finalize(stmt);
    *out_count = n; return res;
}

void kb_free_results(char **rows, int n){ for(int i=0;i<n;i++) free(rows[i]); free(rows); }
void kb_close(){ if(db) sqlite3_close(db); }

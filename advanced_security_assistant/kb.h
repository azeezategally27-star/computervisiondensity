// kb.h - simple KB interface (reused pattern)
#ifndef KB_H
#define KB_H

int kb_init(const char *dbpath);
int kb_train_from_dir(const char *dir_path);
char **kb_query(const char *query, int max_results, int *out_count);
void kb_free_results(char **rows, int n);
void kb_close();

#endif

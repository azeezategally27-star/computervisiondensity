// data.h
#ifndef DATA_H
#define DATA_H

typedef struct {
    int id;
    char *flight_no;
    char *dest;
    char *scheduled;
    char *status;
    char *gate;
    char *luggage;
    int pax;
    int priority;
    int last_update;
} Flight;

typedef struct { int n; Flight *items; } FlightList;

int data_init(const char *path);
void data_close();
void data_seed_if_empty();
FlightList *data_get_all();
int data_update_flight(int id, const char *scheduled, const char *status, const char *gate);
int data_set_luggage(int id, const char *l);
int data_count_flights();

#endif

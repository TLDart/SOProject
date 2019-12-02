#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include "Parser.h"

extern int landing_time, min_hold, max_hold, landing_delta, takeoff_time, takeoff_delta;

extern pthread_mutex_t mutex_time, mutex_command;
extern pthread_cond_t time_var, command_var;
extern int running, mq_id, max_takeoffs, max_landings, showStats, showVerbose;
extern shared_mem *airport;

struct list_arrival *arrival_list;
struct list_departure *departure_list;
pthread_cond_t flight_type_var;
pthread_mutex_t flight_type_mutex;
sem_t *mutex;
struct list_arrival{
    int priority;//0-EMERGENCY 1-NORMAL
    int eta;
    int fuel;
    int shared_memory_index; //Saves the shared memory corresponding iD
    struct list_arrival *next;
};


//Struct used to save departure flights
struct list_departure{
    int takeoff;
    int shared_memory_index;
    struct list_departure *next;
};


struct CT_info* create_ct_info();
void fuel_decrement(void* arg);
void add_ct_info(struct CT_info* node, struct CT_info* head);
void *get_messages(void *arg);
int index_shm();
void print_ct(char* name, struct CT_info* list);
void showStatistics(int signum);
struct list_arrival *create_arrival_list();
struct list_arrival *create_node_arrival(struct CT_info *information);
void add_arrival(struct list_arrival *header, struct list_arrival *node);
void pop_arrival(struct list_arrival *header, struct list_arrival *node);
void remove_arrival(struct list_arrival *header, struct list_arrival *nodo);
void choose_flights_to_hold(struct list_arrival *header);
struct list_departure* create_node_departure(struct CT_info *information);
void add_departure(struct list_departure *header, struct list_departure *node);
void remove_departure(struct list_departure *header, struct list_departure *node);
void choose_flights_to_work(struct list_arrival *header_arrival, struct list_departure *header_departure);
void *decrement_eta(void* arg);
struct list_departure* create_departure_list();
struct timespec timedwait_time(struct wt time_given);
int compare_time(struct timespec begin, struct wt takeoff);
void print_arrivals();
void print_departures();

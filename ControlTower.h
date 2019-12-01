#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include "Parser.h"

extern int landing_time, min_hold, max_hold, landing_time;

struct list_arrival{
    int priority;//prioridade do voo (0 -> prioritario | 1 -> nao prioritario)
    int eta;//tempo estimado para a chegada do voo ao aeroporto
    int fuel;//fuel do voo
    int shared_memory_index; //guarda o id da shared memory correspondente ao voo
    struct list_arrival *next;
};


//struct usada na lista que vai servir para guardar os voos DERPARTURE
struct list_departure{
    int takeoff;
    int shared_memory_index;
    struct list_departure *next;
};

extern pthread_mutex_t mutex_time, mutex_command;
extern pthread_cond_t time_var, command_var;
extern int running, mq_id, max_takeoffs, max_landings, showStats, showVerbose;
extern shared_mem *airport;
extern struct CT_info* arrival_list, *departure_list;

struct CT_info* create_ct_info();
void fuel_decrement(void* arg);
void add_ct_info(struct CT_info* node, struct CT_info* head);
void *get_messages(void *arg);
int index_shm();
void print_ct(char* name, struct CT_info* list);
void showStatistics(int signum);
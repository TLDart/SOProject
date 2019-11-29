#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include "Parser.h"

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
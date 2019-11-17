//
// Created by TLDart on 11/11/2019.
//
#include "Thread.h"
#ifndef PROJECTWORK_THEADCT_H
#define PROJECTWORK_THEADCT_H

#endif //PROJECTWORK_THEADCT_H


extern pthread_mutex_t mutex_time;
extern pthread_cond_t time_var, command_var;
extern int condition, mq_id, max_takeoffs, max_landings;
extern shared_mem *airport;


struct message_array{
    struct message* msg;
    struct message_array* next;
};
struct CT_info{
    int fuel;
    int time_to_track;
    int id;
    int pos;
    struct CT_info* next;
};

struct message_array* create_msg_array();
void fuel_decrement(void* arg);
void add_msg_array(struct message_array* node, struct message_array* head);
void *get_messages(void *arg);
int index_shm();



//
// Created by TLDart on 11/11/2019.
//
#include "Thread.h"
#ifndef PROJECTWORK_THEADCT_H
#define PROJECTWORK_THEADCT_H

#endif //PROJECTWORK_THEADCT_H


extern pthread_mutex_t mutex_time;
extern pthread_cond_t time_var;
extern condition;


struct message_array{
    struct message* msg;
    struct message_array* next;
};



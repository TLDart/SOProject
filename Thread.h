//
// Created by TLDart on 08/11/2019.
//

#ifndef PROJECTWORK_THEADBEHAVIOUR_H
#define PROJECTWORK_THEADBEHAVIOUR_H

#endif //PROJECTWORK_THEADBEHAVIOUR_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include "Parsing.h"

#define BUFFER_SIZE 250
#define MSGTYPE_DEFAULT 2
#define MSGTYPE_PRIORITY 1

struct message{
    long msgtype;
    int mode; //1 - ARRIVAL, 0 - DEPARTURE
    int fuel;
    int time_to_track;//IF MODE 1 THEN ETA, IF MODE 0 THEN TAKEOFF
    int id;//ID that the control tower will use to message
};


struct args_threads{
    int id;
    p_node node;
};

struct sharedmem_info{//Struct used to send the position of the shared memory from the control tower to the flight
    long msgtype;
    int position;
};

void *time_counter(void *arg);
void *create_flights(void *pointer);
void *get_message_from_pipe(void *arg);
void *departure(void *arg);
void *arrival(void *arg);
void print_msg(struct message* node);
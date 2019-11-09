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
#include "Parsing.h"

#define BUFFER_SIZE 250

struct args_threads{
    int id;
    p_node node;
};

void *time_counter(void *arg);
void *create_flights(void *pointer);
void *get_message_from_pipe(void *arg);
void *departure(void *arg);
void *arrival(void *arg);
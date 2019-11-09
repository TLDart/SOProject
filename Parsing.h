//
// Created by TLDart on 08/11/2019.
//

#ifndef PROJECTWORK_PARSING_H
#define PROJECTWORK_PARSING_H

#endif //PROJECTWORK_PARSING_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LinkedList.h"
#include <time.h>
#include <pthread.h>
#define SIZE 200

typedef struct{
    int total_flights, total_landed, total_takeoff, redirected_flights, rejected_flights;
    double avg_ETA, avg_takeoff, avg_man_holding, avg_man_emergency;
    int time ;

    // Landing Tracks
    // Takeoff tracks

}shared_mem;

int n_palavras(char *string);
p_node parsing(char *string);
void write_to_log(char * msg);
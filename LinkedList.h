//
// Created by TLDart on 08/11/2019.
//

#ifndef PROJECTWORK_LINKED_LIST_H
#define PROJECTWORK_LINKED_LIST_H

#endif //PROJECTWORK_LINKED_LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct node* p_node;
struct node{
    char *mode;
    char *flight_code;
    int init;
    int takeoff;
    int fuel;
    int eta;
    p_node next;
};


p_node create_list();

void add_flight(p_node node, p_node list);
p_node pop_flight(p_node list);
void print_list(p_node list);
void print_node(p_node node);
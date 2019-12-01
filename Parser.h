#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Structures.h"
#include <time.h>
#include <pthread.h>
#include <math.h>
#define SIZE 200

extern int time_unit;
extern struct timespec begin;
extern float time_sec;
extern p_node head;


int n_palavras(char *string);
p_node parsing(char *string);
void write_to_log(char * msg);

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Structures.h"
#include <time.h>
#include <pthread.h>

#define SIZE 200

int n_palavras(char *string);
p_node parsing(char *string);
void write_to_log(char * msg);
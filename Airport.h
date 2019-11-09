#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <sys/msg.h>
#include "Thread.h"

#define PIPE_NAME "Input_pipe"
#define MSGTYPE_DEFAULT 2
#define MSGTYPE_PRIORITY 1

struct message{
    long msgtype;
    int mode; //1 - ARRIVAL, 0 - DEPARTURE
    int fuel;
    int time_to_track;//IF MODE 1 THEN ETA, IF MODE 0 THEN TAKEOFF
    int id;//ID that the control tower will use to message
};

struct shm_information{//Struct used to send the position of the shared memory from the control tower to the flight
    long msgtype;
    int posicao;
};


void simulation_manager(char* config_path);
int load_config(char* path);
void showStats(int signum);
void exit_program(int signum);
void control_tower();
void clean_log();


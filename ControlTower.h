#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <sys/fcntl.h>
#include "Parser.h"

#define  CAN_SEND "CAN_SEND"
#define  CAN_HOLD "CAN_HOLD"


extern int mq_id, max_landings, max_takeoffs;
extern shared_mem *airport;

int runningCT = 1,
        counter_arr = 0, // Flights
        counter_dep = 0,// at the same time
        new_message = 0;


pthread_t messenger;
pthread_mutex_t flight_verifier = PTHREAD_MUTEX_INITIALIZER;

sem_t *can_send,
      *can_hold;


//Functions
void control_tower();
void *get_messages(void *arg);
int index_shm();
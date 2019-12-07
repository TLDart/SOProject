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

struct list_arrival{
    int priority;//0-EMERGENCY 1-NORMAL
    int eta;
    int fuel;
    int shared_memory_index; //Saves the shared memory corresponding iD
    int number_of_nodes;

    struct list_arrival *next;
};

struct list_departure{
    int takeoff;
    int shared_memory_index;
    int number_of_nodes;
    struct list_departure *next;
};

extern pthread_cond_t command_var;
extern int mq_id, max_landings, max_takeoffs, takeoff_time, takeoff_delta, max_hold, min_hold, landing_delta;
extern shared_mem *airport;

int runningCT = 1,
        counter_arr = 0, // Flights
        counter_dep = 0,// at the same time
        new_message = 0;

struct list_departure *header_departure;
struct list_arrival *header_arrival;
pthread_t messenger;
pthread_mutex_t flight_verifier = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t awake_holder_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t awake_holder_var = PTHREAD_COND_INITIALIZER;



sem_t *can_send,
      *can_hold;


//Functions
void control_tower();
void *get_messages(void *arg);
int index_shm();
void flight_handler();
int compare_time(struct timespec begin, struct wt takeoff);
struct list_arrival *create_arrival_list();
struct list_arrival *create_node_arrival(struct message *information, int position);
void add_arrival(struct list_arrival *header, struct list_arrival *node);
struct list_arrival *pop_arrival(struct list_arrival *header, struct list_arrival *node);
void remove_arrival(struct list_arrival *header, struct list_arrival *node);
struct list_departure* create_departure_list();
struct list_departure* create_node_departure(struct message* information, int position);
void add_to_departure(struct list_departure *header, struct list_departure * node);
void remove_departure(struct list_departure *header, struct list_departure *node);

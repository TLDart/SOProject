#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#define SIZE 200
#define MSGTYPE_DEFAULT 2
#define MSGTYPE_PRIORITY 1
#define MSGTYPE_EXIT 3
#define RED "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW   "\x1B[33m"
#define BLUE   "\x1B[34m"
#define MAGENTA  "\x1B[35m"
#define CYAN   "\x1B[36m"
#define WHITE   "\x1B[37m"
#define RESET "\x1B[0m"

extern pthread_cond_t time_var;
extern int list_element, showVerbose;

typedef struct{
    int total_flights,
        total_landed,
        total_takeoff,
        total_emergency,
        redirected_flights,
        rejected_flights,
        total_holding_man,
        total_emergency_holding_man,
        total_time_landing,
        total_time_takeoff,
        stop_condition,
        flights_arrived;
        pthread_cond_t command_var;
        pthread_condattr_t cattr;
        pthread_mutex_t mutex_command;
        pthread_mutexattr_t mattr;


    int max_flights[];
}shared_mem;


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

struct message{
    long msgtype;
    int mode; //1 - ARRIVAL, 0 - DEPARTURE
    int fuel;
    int flight_code; // the stuct hold a maximium on TP+7 digits
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

struct CT_info{
    int fuel;
    int time_to_track;
    int id;
    int pos;
    struct CT_info* next;
};

struct wt{
    int secs;
    int nsecs;

};

p_node create_list();

void add_flight(p_node node, p_node list);
p_node pop_flight(p_node list);
void print_list(p_node list);
void print_node(p_node node);
int now_in_tm(struct timespec begin,int time_unit);
struct wt convert_to_wait(int time, int time_unit);
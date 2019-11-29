#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define SIZE 200
#define MSGTYPE_DEFAULT 2
#define MSGTYPE_PRIORITY 1
#define RED "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW   "\x1B[33m"
#define BLUE   "\x1B[34m"
#define MAGENTA  "\x1B[35m"
#define CYAN   "\x1B[36m"
#define WHITE   "\x1B[37m"
#define RESET "\x1B[0m"


typedef struct{
    int total_flights, total_landed, total_takeoff, redirected_flights, rejected_flights;
    double avg_ETA, avg_takeoff, avg_man_holding, avg_man_emergency;
    int time;
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

struct message_array{
    struct message* msg;
    struct message_array* next;
};
struct CT_info{
    int fuel;
    int time_to_track;
    int id;
    int pos;
    struct CT_info* next;
};


p_node create_list();

void add_flight(p_node node, p_node list);
p_node pop_flight(p_node list);
void print_list(p_node list);
void print_node(p_node node);
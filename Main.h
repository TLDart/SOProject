#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include "Parser.h"
#include <sys/time.h>
#include <semaphore.h>
#define PIPE_NAME "Pipe"
#define BUFFER_SIZE 250

/*Argument variables*/
int showVerbose = 0;
FILE* logfile;
char* filename = "config.txt";

/*Config Variables */
int time_unit,
    takeoff_time,
    takeoff_delta,
    landing_time,
    landing_delta,
    min_hold,
    max_hold,
    max_takeoffs,
    max_landings;

/*Structure Variables */
int shmid, /*Shared memory ID*/
    pipe_fd, /*Pipe ID*/
    mq_id, /*Message Queue ID*/
    ids = 10, /*Thread IDS*/
    running = 1;/*Closing condition*/
    shared_mem *airport; // Shared memory variable
    p_node head; // head of the linked list

//struct list_arrival *arrival_list;
//struct list_departure *departure_list;

/*Other Variables*/
struct timespec begin; //Records the beginning of the program
int list_element = 0; // Keeps track of the supposed beginning of the list
/*Conditional Variables and Mutexes*/
pthread_cond_t time_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_time = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t exitor_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t exitor_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_exit;


void clean_log();
void parse_arguments(int argc, char **argv);
void config_test(char *token);
void control_tower();
void exit_handler(int signum);

int load_config(char *path);
void simulation_manager(char *config_path);
void get_message_from_pipe(int fd);
void *create_flights(void *pointer);
void *departure(void *arg);
void *arrival(void *arg);
void print_msg(struct message * node);
void* exit_thread(void* arg);
int get_flight_code(char *flight_code);
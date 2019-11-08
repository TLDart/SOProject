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
#include "Parsing.h"


#define PATH "/Users/tldart/Documents/Semester 3/SO/Project/config.txt"
#define BUFFER_SIZE 250
#define PIPE_NAME "Input_pipe"
#define MSGTYPE_DEFAULT 2
#define MSGTYPE_PRIORITY 1

/*typedef struct{
  long msgtype;
  char *message;
}message;
*/
struct message{
    long msgtype;
    int mode; //se for 1 e arrival, se for 0 e departure
    int fuel;
    int time_to_track;//se o mode for 1, e o eta, se o mode for 0, e o takeoff
    int id;//numero que a control tower vai usar para mandar a mensgem ao voo
};


struct args_threads{
    int id;
    p_node nodo;
};

struct shm_information{//struct usada para enviar a posicao da shared memory correspondente, da control tower para o voo
    long msgtype;
    int posicao;
};


void simulation_manager(char* config_path);
int load_config(char* path);
void showStats(int signum);
void* get_message_from_pipe(void* arg);
void terminate(int signum);
void write_to_log(char*msg);
void* time_counter(void* arg);
void control_tower();


void* create_flights(void* pointer);
void* departure(void * arg);
void* arrival(void* arg);

void clean_log();
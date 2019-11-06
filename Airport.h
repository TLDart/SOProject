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



#define PATH "/Users/tldart/Documents/Semester 3/SO/Project/config.txt"
#define BUFFER_SIZE 250
#define PIPE_NAME "Input_pipe"
typedef struct{
    int total_flights, total_landed, total_takeoff, redirected_flights, rejected_flights;
    double avg_ETA, avg_takeoff, avg_man_holding, avg_man_emergency;
    int time ;

	// Landing Tracks
    // Takeoff tracks

}shared_mem;

typedef struct{
  long msgtype;
  char *message;
}message;

void simulation_manager(char* config_path);
int load_config(char* path);
void showStats(int signum);
void* get_message_from_pipe(void* arg);
void terminate(int signum);
void write_to_log(char*msg);
void* time_counter(void* arg);
void control_tower();

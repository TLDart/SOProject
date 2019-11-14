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

#include "ThreadCT.h"

#define PIPE_NAME "Input_pipe"

void simulation_manager(char* config_path);
int load_config(char* path);
void showStats(int signum);
void exit_program(int signum);
void control_tower();
void clean_log();
void config_test(char* token);


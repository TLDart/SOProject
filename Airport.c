// Compile by using ./filename <configPath>
#include "Airport.h"
#include <string.h>
int time_unit, timer;
int takeoff_time,takeoff_delta,landing_time,landing_delta, min_hold, max_hold;
int max_takeoffs, max_landings;
int shmid;
shared_mem* airport;
int fd;
int mq_id;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv){
    simulation_manager(argv[1]);
}

void simulation_manager(char* config_path) {
    //Handle signals
    pthread_t time_thread, pipe_reader;
    signal(SIGUSR1,showStats);
    signal(SIGINT, terminate);
    //Load initial config
    if(load_config(config_path) < 0){
        //Loads initial simulation config
        perror("Could not load config, exiting");
	exit(-1);
    }
    printf("%d",getpid());
    //Create Shared Memory Volume
    shmid = shmget(IPC_PRIVATE, sizeof(shared_mem),IPC_CREAT|0777);
    if (shmid < 0){
        perror("Failed to Create shared memory block");
        exit(1);
    }
    airport = (shared_mem*) shmat(shmid, NULL,0);
    
    //Create MSQ
	if((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1){
    	perror("Failed to create message queue\n");
	exit(-1);
  }


   //Create Control Tower
        if(fork() == 0){
        control_tower(); 
        exit(0);
	}

    //Create time thread
	pthread_create(&time_thread, NULL,time_counter,airport); 

    //Create named pipe and thread
        unlink(PIPE_NAME);
        mkfifo(PIPE_NAME,O_CREAT|O_EXCL|0666);
        fd = open(PIPE_NAME, O_RDONLY|O_NONBLOCK);
	pthread_create(&pipe_reader, NULL,get_message_from_pipe,&fd); 
	
	pthread_join(time_thread, NULL);
	pthread_join(pipe_reader,NULL);

}

int load_config(char* path) {
    //Load config from PATH, returns 1 if successful, -1 if not successful
    int counter = 0;
    FILE *fp;

    if ((fp = fopen(path, "r")) == NULL) return -1;
    char buffer[BUFFER_SIZE], *token;
    const char delimiter[2] = ",";
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; //Removes the trailing ‘\n’
	//printf("%s \n", buffer);
      if (counter == 0) {
            time_unit = atoi(buffer);
        }
        if (counter == 1) {
            token = strtok(buffer,delimiter);
	    takeoff_time = atoi(token);
            token = strtok(NULL,delimiter);
            takeoff_delta = atoi(token);
        }
        if (counter == 2) {
            token = strtok(buffer,delimiter);
            landing_time = atoi(token);
            token = strtok(NULL,delimiter);
            landing_delta = atoi(token);

        }
        if (counter == 3) {
            token = strtok(buffer,delimiter);
            min_hold = atoi(token);
            token = strtok(NULL,delimiter);
            max_hold = atoi(token);

        }
        if (counter == 4) {
            max_takeoffs = atoi(buffer);

        }
        if(counter == 5){
            max_landings = atoi(buffer);
        }
	memset(buffer, 0, strlen(buffer));//Resets the buffer
        counter++;
    }
    return 1;
}

void showStats(int signum){
	//Shows user stats
    signal(SIGUSR1, showStats);
    //We will need a mutex here
    printf("Timer %d", airport->time);
    printf("Total number of flights : %d\n", airport->total_flights);
    printf("Total flights that Landed: %d\n", airport->total_landed);
    printf("Estimated wait time : %lf\n", airport->avg_ETA);
    printf("Total Flights that TookOff: %d\n", airport->total_takeoff);
    printf("Average TakeOff Time : %lf\n", airport->avg_takeoff);
    printf("Average number of holding maneuvers per landing : %lf\n", airport->avg_man_holding);
    printf("Average number of maneuvers per Emergency Time : %lf\n", airport->avg_man_emergency);
    printf("Total redirected flights : %d\n", airport->redirected_flights);
    printf("Total rejected flights : %d\n", airport->rejected_flights);
    //And another here

}

void* get_message_from_pipe(void* arg){
	//Reads a message from pipe into de buffer;
	int fd = *((int*)arg);
	char buffer[BUFFER_SIZE];
	fd_set read_set;
	int nread;

	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);
	while(1){
		if(select(fd + 1, &read_set, NULL,NULL,NULL) > 0){
			if (FD_ISSET(fd, &read_set)){
                                nread = read(fd,buffer, BUFFER_SIZE);
                                if (nread > 0){
                                        buffer[nread] = '\0';
                                        printf("%s", buffer); 
					//Do the parsing functionm

				}
			}
		}
	}
}

void terminate(int signum){
	//Terminates the program,TODO:  NEEDS TO CLEAN RESOURCES
	exit(0);	
}

void write_to_log(char * msg){
	//Writes msg to lopg
	FILE * fp;
	time_t ctime;
	struct tm *parsed_time;

	//Handle time
	time(&ctime);
	parsed_time = localtime(&ctime);
	fp = fopen ("log.txt","a");
	fprintf(fp, "%d:%d:%d %s\n",parsed_time->tm_hour, parsed_time->tm_min, parsed_time->tm_sec, msg);	
}

void* time_counter(void* arg){
	shared_mem *airport = (shared_mem*)arg;
	while(1){
		pthread_mutex_lock(&mutex);
		airport->time++;
		//printf("%d TIME UNIT: %f\n", airport->time,time_unit*1000);
		pthread_mutex_unlock(&mutex);
		usleep(time_unit*1000);
		}
	}
void control_tower(){
		
}

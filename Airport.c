// Compile by using ./filename <configPath>
#include "Airport.h"
//TODO: Review Parsing and LinkedList
//TODO: Add A ReadME
int time_unit, takeoff_time, takeoff_delta, landing_time, landing_delta, min_hold, max_hold, max_takeoffs, max_landings;// Global Variables relative to config
int shmid, fd, mq_id, ids = 1, condition = 1;// shared memory id , pipe id, messaqe queue id, thread id
shared_mem *airport; // Shared memory variable
p_node head; // head of the linked list

pthread_cond_t time_var = PTHREAD_COND_INITIALIZER,command_var = PTHREAD_COND_INITIALIZER;;
pthread_mutex_t mutex_time = PTHREAD_MUTEX_INITIALIZER, mutex_write = PTHREAD_MUTEX_INITIALIZER, mutex_stats = PTHREAD_MUTEX_INITIALIZER, mutex_command = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char **argv) {
    if(argv[1] != NULL) {
        simulation_manager(argv[1]);
    }
    else{
        simulation_manager("config.txt");
    }
}

void simulation_manager(char *config_path) {
    /* The simulation Manager will handle all creating and managing all other "sub-parts " of the program
     * Parameters:
     *      config_path - specifies the config path used for initial config
     */
    pthread_t time_thread, pipe_reader, flight_creator;

    //Handle Signals
    signal(SIGUSR1, showStats);
    signal(SIGINT, exit_program);

    //CLean "log.txt" if necessary
    clean_log();

    write_to_log("PROGRAMS STARTS");

    //Load initial config
    if (load_config(config_path) < 0) {
        perror("CONFIG LOAD ERROR\n");
        exit(0);
    }
    printf("MAIN MAX TAKEOFFS %d\n", (max_takeoffs));


    //Create Linked List
    head = create_list();

    //Create Shared Memory Volume
    shmid = shmget(IPC_PRIVATE, sizeof(shared_mem) +(max_landings +max_takeoffs) * sizeof(int), IPC_CREAT | 0777);
    if (shmid < 0) {
        perror("Failed to Create shared memory block");
        exit(1);
    }
    airport = (shared_mem *) shmat(shmid, NULL, 0);
    memset(airport->max_flights,0,(max_landings + max_takeoffs) * sizeof(int));
    //if(airport->max_flights[837] == 0) puts("SUCCESS:))");

    //Create MSQ
    if ((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        perror("Failed to create message queue\n");
        exit(-1);
    }

    //Create Control Tower
    if (fork() == 0) {
        control_tower();
        exit(0);
    }

    //Create named pipe
    unlink(PIPE_NAME);
    mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0666);
    if ((fd = open(PIPE_NAME, O_RDWR)) < 0) perror("Pipe Error");

    //Create Flight_Creator_Thread
    pthread_create(&flight_creator, NULL, create_flights, head);

    //Create Time_Thread
    pthread_create(&time_thread, NULL, time_counter, airport);

    //Create Pipe_reader Thread
    pthread_create(&pipe_reader, NULL, get_message_from_pipe, &fd);

    //TODO: Correct Undefined Behavior
    //while (1) {};
   pthread_join(flight_creator, NULL);


}

int load_config(char *path) {
    /*Load initial config into the variables written above
     *
     * Parameters :
     *      path - Specifies the path from which the config is being loaded
     *
     * Returns:
     *      1 - Successful load
     *     -1 - Unsuccessful Load
     */
    //TODO: Create some protection functions
    int counter = 0;
    FILE *fp;

    if ((fp = fopen(path, "r")) == NULL){perror("CONFIG ERROR. ABORTING\n"); return -1;}
    char buffer[BUFFER_SIZE], *token;
    const char delimiter[2] = ",";
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; //Removes the trailing ‘\n’
        if (counter == 0) {
            if((time_unit = atoi(buffer)) == 0 && buffer[0] !='0'){ return -1;}
        }
        if (counter == 1) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if((takeoff_time = atoi(token)== 0 && buffer[0] !='0')){ return -1;}
            token = strtok(NULL, delimiter);
            config_test(token);
            if((takeoff_delta = atoi(token) == 0 && buffer[0] !='0')){ return -1;}
        }
        if (counter == 2) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if((landing_time = atoi(token) == 0 && buffer[0] !='0')){return -1;}
            token = strtok(NULL, delimiter);
            config_test(token);
            if((landing_delta = atoi(token) == 0 && buffer[0] !='0')){return -1;}

        }
        if (counter == 3) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if((min_hold = atoi(token) == 0 && buffer[0] !='0')){ return -1;}
            token = strtok(NULL, delimiter);
            config_test(token);
            if((max_hold = atoi(token) == 0 && buffer[0] !='0')){ return -1;}

        }
        if (counter == 4) {
            if((max_takeoffs = atoi(buffer)) == 0 && buffer[0] !='0'){ return -1;}
        }
        if (counter == 5) {
            if((max_landings = atoi(buffer) == 0) && buffer[0] !='0'){ return -1;}
        }
        memset(buffer, 0, strlen(buffer));//Resets the buffer
        counter++;
    }
    if(counter != 6){
        perror("COULD NOT LOAD CONFIG");
        return -1;
    }
    fclose(fp);
    return 1;
}

void showStats(int signum) {
    /* Prints Stats to the user
     *
     *
     * Parameters:
     *      signum = signal number
     */
    //TODO: sigprockmask instead of this
    signal(SIGUSR1, showStats);
    pthread_mutex_lock(&mutex_stats);
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
    pthread_mutex_unlock(&mutex_stats);

}



void exit_program(int signum){
    /* Terminates the current program freeing all the resource
     *
     * Parameter
     *      signum - Number to the signal
     */
    /*
    wait(NULL);//fica à espera que a control tower acabe aquilo que tem a fazer
    //Remove mutexes and condition variables
    pthread_cond_destroy(&command_var);
    pthread_cond_destroy(&time_var);
    pthread_mutex_destroy(&mutex_time);
    pthread_mutex_destroy(&mutex_command);
    pthread_mutex_destroy(&mutex_stats);

    //remover a message queue
    msgctl(mq_id, IPC_RMID, NULL);

    //remover a shared memory
    shmdt(airport);
    shmctl(shmid, IPC_RMID, NULL);

    //Free flight list
    free(head);

    */
    puts("[PROGRAM ENDING]");
    write_to_log("PROGRAM ENDING");
    //remover o mutex_write usado no write_to_log, para ficarem todos juntos, podem mudar-se todos os outros para depois desta instrução
    pthread_mutex_destroy(&mutex_write);

    exit(0);
}

void control_tower() {
    /* Handles Flight Threads, shared memory communication, and Message Queue Messaging
     *
     */
    pthread_t msg_reader;
    puts("CONTROL TOWER CREATED");
    // Insert Control Tower Code
    pthread_create(&msg_reader, NULL, get_messages,NULL);
    pthread_join(msg_reader, NULL);
}

void clean_log() {
    /*Opens a file for writing and closes it, therefore erasing it*/
    FILE *fp = fopen("log.txt", "w");
    fclose(fp);
}

void write_to_log(char * msg){
    //Writes msg to log
    FILE * fp;
    time_t ctime;
    struct tm *parsed_time;

    //Handle time
    time(&ctime);
    parsed_time = localtime(&ctime);
    pthread_mutex_lock(&mutex_write);
    fp = fopen ("log.txt","a");
    fprintf(fp, "%2d:%2d:%2d %s\n",parsed_time->tm_hour, parsed_time->tm_min, parsed_time->tm_sec, msg);
    fclose(fp);
    pthread_mutex_unlock(&mutex_write);
}

void config_test(char* token){
    if(token == NULL){
        perror("CONFIG ERROR. ABORTING\n");
        exit(-1);
    }
}
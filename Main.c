/*
 * Duarte Dias 2018293526
 * Gabriel Fernandes 2018288117
 */
#include "Main.h"

int main(int argc, char **argv) {
    parse_arguments(argc, argv);
    simulation_manager(filename);
}

void simulation_manager(char *config_path) {
    /* The simulation Manager will handle all creating and managing all other "sub-parts " of the program
     * Parameters:
     *      config_path - specifies the config path used for initial config
     */
    clock_gettime(CLOCK_REALTIME,&begin);

    pthread_t flight_creator;

    signal(SIGUSR1, SIG_IGN);   /*Handle Signals*/
    signal(SIGINT, exit_handler);

    clean_log(); /*Clean "log.txt" if necessary*/
    logfile = fopen("log.txt", "a"); /*Opening log global pointer*/

    write_to_log("[PROGRAMS STARTS]");

    if (load_config(config_path) < 0) { /*Load initial config*/
        perror("Failed to load config\n");
        exit(0);
    }

    if ((head = create_list()) == NULL) {  /*Create Linked List*/
        perror("Failed to create linked lists");
        exit(-1);
    }

    /*Create Shared Memory Volume*/
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_mem) + (max_landings + max_takeoffs) * sizeof(int),
                        IPC_CREAT | 0777)) < 0) {
        perror("Failed to Create shared memory block\n");
        exit(1);
    }

    if(showVerbose == 1 ) printf("%s [SHARED MEMORY CREATED]%s\n", BLUE, RESET);

    /*Attaching and defining shared memory Values*/
    airport = shmat(shmid, NULL, 0);
    memset(airport->max_flights, 0, (max_landings + max_takeoffs) * sizeof(int));

    pthread_condattr_setpshared(&airport->cattr,PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&airport->command_var, &airport->cattr);

    pthread_mutexattr_setpshared(&airport->mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&airport->mutex_command, &airport->mattr);

    airport->total_flights = 0;
    airport->total_landed = 0;
    airport->total_takeoff = 0;
    airport->total_emergency = 0;
    airport->redirected_flights = 0;
    airport->rejected_flights = 0;
    airport->total_holding_man = 0;
    airport->total_emergency_holding_man = 0;
    airport->total_time_landing = 0;
    airport->total_time_takeoff = 0;
    airport->flights_arrived = 0;

    /*Create MSQ*/
    if ((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        perror("Failed to create message queue\n");
        exit(-1);
    }

    if(showVerbose == 1 ) printf("%s [MESSAGE QUEUE CREATED]%s\n", BLUE, RESET);

    if(showVerbose == 1 ) printf(" _>>>takeoff time %d takeoffDelta %d Time unit %d min_hold %d\n", takeoff_time,takeoff_delta,time_unit, min_hold);
    /*Create Control Tower*/
    if (fork() == 0) {
        control_tower();
        if(showVerbose == 1 ) puts(" CT EXITED");
        exit(0);
    }

    /*Create named pipe*/
    unlink(PIPE_NAME);
    mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0666);
    if ((pipe_fd = open(PIPE_NAME, O_RDWR)) < 0) perror("Pipe Error");

    if(showVerbose == 1 ) printf("%s [NAMED PIPE CREATED]%s\n", BLUE, RESET);

    pthread_create(&flight_creator, NULL, create_flights, head); /*Create Flight_Creator_Thread */
    pthread_create(&thread_exit, NULL, exit_thread, NULL); /*Create Flight_Creator_Thread */

    if(showVerbose == 1 ) printf("THREAD CREATES\n");

    if (showVerbose == 1) printf("Correctly initiated Time Thread\n");

    /*Run getting a message from the pipe*/
    get_message_from_pipe(pipe_fd);
    if(showVerbose == 1 ) puts("PIPE CLOSED");
    /*Cleaning The system*/
    pthread_join(thread_exit,NULL);
    if(showVerbose == 1 ) printf("THREAD EXITED\n");
    pthread_join(flight_creator, NULL);
    if(showVerbose == 1 ) printf("FLIGHT CREATOR CLOSED\n");
    wait(NULL);
    if(showVerbose == 1 ) printf("CONTROL TOWER DOWN\n");

    /*Cleaning*/
    /*Shared Memory*/
    pthread_cond_destroy(&airport->command_var);
    pthread_condattr_destroy(&airport->cattr);
    pthread_mutex_destroy(&airport->mutex_command);
    pthread_mutexattr_destroy(&airport->mattr);
    shmdt(airport);
    shmctl(shmid, IPC_RMID, 0);

    /*Mutexes and cond variables*/
    pthread_cond_destroy(&time_var);
    pthread_cond_destroy(&exitor_var);
    pthread_mutex_destroy(&mutex_time);
    pthread_mutex_destroy(&exitor_mutex);

    /*Headers and linked lists*/
    free(head);
    /*Files pipes and message queue*/
    write_to_log("[PROGRAMS ENDS]");
    fclose(logfile);
    unlink(PIPE_NAME);
    msgctl(mq_id,IPC_RMID,0);
    /*Exiting*/

    exit(0);
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
    if(showVerbose == 1 ) printf("%s [LOADING CONFIG]%s\n", BLUE, RESET);

    int counter = 0;
    char buffer[BUFFER_SIZE], *token;
    const char delimiter[2] = ",";
    FILE* fp;

    if ((fp = fopen(path, "r")) == NULL) {
        perror("CONFIG ERROR. ABORTING\n");
        return -1;
    }

    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; //Removes the trailing '\n'
        if (counter == 0) {
            if ((time_unit = atoi(buffer)) == 0 && buffer[0] != '0') { return -1; }
        }
        if (counter == 1) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if ((takeoff_time = atoi(token)) == 0 && buffer[0] != '0') { return -1; }
            token = strtok(NULL, delimiter);
            config_test(token);
            if ((takeoff_delta = atoi(token)) == 0 && buffer[0] != '0') { return -1; }
        }
        if (counter == 2) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if ((landing_time = atoi(token)) == 0 && buffer[0] != '0') { return -1; }
            token = strtok(NULL, delimiter);
            config_test(token);
            if ((landing_delta = atoi(token)) == 0 && buffer[0] != '0') { return -1; }

        }
        if (counter == 3) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if ((min_hold = atoi(token)) == 0 && buffer[0] != '0') { return -1; }
            token = strtok(NULL, delimiter);
            config_test(token);
            if ((max_hold = atoi(token)) == 0 && buffer[0] != '0') { return -1; }

        }
        if (counter == 4) {
            if ((max_takeoffs = atoi(buffer)) == 0 && buffer[0] != '0') { return -1; }
        }
        if (counter == 5) {
            if ((max_landings = atoi(buffer)) == 0 && buffer[0] != '0') { return -1; }
        }
        memset(buffer, 0, strlen(buffer));//Resets the buffer
        counter++;
    }
    if (counter != 6) {
        perror("COULD NOT LOAD CONFIG");
        return -1;
    }
    fclose(fp);
    return 1;
}


void exit_handler(int signum) {
    /* Changes some flags and prepares the program to exit
     *
     * Parameter
     *      signum - Number to the signal
     */
    write_to_log("[CTRL-C RECEIVED STARTING END SEQUENCE]");
    /*Creates the last thread*/


    running = 0;

    /*Setting the pipe to non block*/
    fcntl(pipe_fd, F_SETFL, O_NONBLOCK);

}


void config_test(char *token) {
    /*Small function that aids the config loader to verify if the argument is valid*/
    if (token == NULL) {
        perror("CONFIG ERROR. ABORTING\n");
        exit(-1);
    }
}

void parse_arguments(int argc, char **argv) {
    /*
     * Parameters:
     *      ArgC - Number of arguments in argV
     *      ArgV - Parameters passed to the program via console
     *
     * */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            showVerbose = 1;
        }
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 <= argc) {
                filename = malloc(strlen(argv[i + 1]));
                if(showVerbose == 1 ) printf("%s", argv[i + 1]);
                strcpy(filename, argv[i + 1]);
            }
        }
    }
}

void clean_log() {
    /*Opens a file for writing and closes it, therefore erasing it*/
    logfile = fopen("log.txt", "w");
    fclose(logfile);
}
void write_to_log(char *msg) {
    /*Writes message to both log and stdout
     * Parameter:
     *      Message to write
     *
     * */
    time_t ctime;
    struct tm *parsed_time;

    /*Display time correctly*/
    time(&ctime);
    parsed_time = localtime(&ctime);
    fflush(logfile);
    fprintf(logfile, "%2d:%2d:%2d %s\n", parsed_time->tm_hour, parsed_time->tm_min, parsed_time->tm_sec, msg);
    fflush(logfile);
    printf("%2d:%2d:%2d %s\n", parsed_time->tm_hour, parsed_time->tm_min, parsed_time->tm_sec, msg);

}

void get_message_from_pipe(int file_d) {
    /* Reads message coming from the Named Pipe
     *
     * Parameter :
     *      file_d = Indication of the file descriptor of the pipe
     *
     */
    if(showVerbose == 1 ) puts("[PIPE HANDLER THREAD CREATED]");
    char buffer[BUFFER_SIZE];
    int nread;
    p_node parsed_data;

    while (running) {
        nread = read(file_d, buffer, BUFFER_SIZE);
        if (errno != EINTR) {
            if (nread > 0) {
                buffer[nread] = '\0';
                if (buffer[nread - 1] == '\n') //Remove trailing '\n' if necessary
                    buffer[nread - 1] = '\0';
                parsed_data = parsing(buffer); // Handle the buffer
                if (parsed_data != NULL) {
                    if(showVerbose == 1 ) printf("%sPARSER ADDED %s", CYAN,RESET);
                    add_flight(parsed_data, head);
                    airport->total_flights++;
                }
            }
        }
    }
     /*Read last bytes from the disk and send them the log file*/
    if(showVerbose == 1 )puts("Cleaning Pipe");
    while((nread = read(file_d, buffer, BUFFER_SIZE)) > 0){
        write_to_log(buffer);
    }
    if(showVerbose == 1 ) puts("Cleaned Pipe");
    pthread_cond_broadcast(&time_var);
}

/*Threaded Functions*/

    void *create_flights(void *pointer) {
        /* Thread Function That handle flight Creation. Verifies the heads of the Linked list and checks if it is time for a flight to be created
         *
         * Parameters:
         *      pointer - Pointer to struct of type p_node which is the head of the list
         */
        if(showVerbose == 1 ) printf("FLIGHT CREATOR\n");
        p_node list = head, flight;
        pthread_t thread; //New thread to be Created
        struct args_threads *args;// Pointer to struct that holds flight information, this struct will be handed to the flight on creation

        pthread_mutex_lock(&mutex_time);
        while (running == 1 || list->next != NULL) {
            flight = list->next;
            if(flight == NULL){
                if(showVerbose == 1 ) printf("%sNULL ELEMENT%s\n", RED,RESET);
                pthread_cond_wait(&time_var, &mutex_time);
            }
            else{
                struct timespec waiting_time;
                clock_gettime(CLOCK_REALTIME,&waiting_time);
                struct wt t = convert_to_wait(flight->init - now_in_tm(begin,time_unit), time_unit);
                //printf(" ->>>NSECS %ld \n",  waiting_time.tv_nsec);
                waiting_time.tv_sec += t.secs;
                if(waiting_time.tv_nsec + t.nsecs >= 1000000000){ //If time nanosecs overflow max value we add 1 second to tv_sec and add the modulus to the nsec
                    waiting_time.tv_sec += 1;
                    waiting_time.tv_nsec = (waiting_time.tv_nsec + t.nsecs) % 1000000000;
                    //waiting_time.tv_nsec = 0;
                    //printf("NANSEC OVEFLOW  %ld\n", waiting_time.tv_nsec);
                }
                else{
                    waiting_time.tv_nsec += t.nsecs; // if it does not overflow just add the nano ecs
                }
                //printf(" NSECS %ld\n",  waiting_time.tv_nsec);
                pthread_cond_timedwait(&time_var, &mutex_time, &waiting_time); //to the correct timed wait

            }
            //printf("%sPROGRAM TIME : %d %s \n", RED,now_in_tm(begin,time_unit),RESET);
            //Verify, with mutual exclusion and condition variable, if it is time for the flight to be created
            while (flight != NULL && flight->init <= now_in_tm(begin, time_unit)) {
                args = (struct args_threads *) malloc(sizeof(struct args_threads)); //Needs to be freed from within the departure/ arrival function;
                args->id = ids;
                args->node = flight;//TODO: This pointer needs to be freed

                /*Create a thread*/
                if (strcmp(flight->mode, "DEPARTURE") == 0) {
                    pthread_create(&thread, NULL, departure, args);

                    list_element++;
                } else if (strcmp(flight->mode, "ARRIVAL") == 0) {
                    pthread_create(&thread, NULL, arrival, args);
                    list_element++;
                } else {
                    write_to_log("[THREAD CREATION ERROR]");
                }
                ids++; //Increment Thread Unique ID
                list->next = list->next->next; //Removes from list without destroying node
                flight = list->next;
                //print_list(head);
            }
        }
        pthread_mutex_unlock(&mutex_time);
        if(showVerbose == 1 ) printf("REACHED END\n");
        pthread_cond_broadcast(&exitor_var);
        pthread_exit(NULL);
    }

    void *departure(void *arg) {
        /* Handles all functionality for the Departure-Type Threads
         *
         * Parameters
         *      arg - Struct which holds a struct that contains flight info
         *
         */
        struct args_threads *data = (struct args_threads *) arg;
        char *aux = malloc(sizeof(char) * BUFFER_SIZE);
        struct sharedmem_info temp;//Holds the message returned by the control tower
        struct message msg;
        int command = 1;
        char track[3];

        /*Arrival Activity*/
        sprintf(aux, "[DEPARTURE THREAD CREATED] [FLIGHT CODE] : %s [TAKEOFF]: %d", data->node->flight_code,
                data->node->takeoff);
        write_to_log(aux);
        free(aux);

        /*Create the message*/
        msg.msgtype = MSGTYPE_DEFAULT;
        msg.mode = 0;
        msg.fuel = -1;//Departing planes has no fuel
        msg.time_to_track = data->node->takeoff;
        msg.id = data->id;
        msg.flight_code = get_flight_code(data->node->flight_code);
        if(showVerbose == 1 ) printf("%s[THREAD][MSG SENT][ID] %d %s\n", CYAN,msg.id, RESET);

        if (msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {/*Sends the message to the control tower*/
            perror("SENDING MESSAGE ERROR");
            exit(-1);
        }
        if(showVerbose == 1 ) printf("%s-------------[THREAD][MSG SENT SUCESSFULLY][ID] %d %s\n", WHITE,msg.id, RESET);

        if (msgrcv(mq_id, &temp, sizeof(temp) - sizeof(long), msg.id, 0) == -1) {/*Reads the message from the control Tower*/
            perror("RECEIVING MESSAGE ERROR");
            exit(-1);
        }
        if(showVerbose == 1 ) printf("%s[THREAD][RECEIVED MSG SUCCESSFULLY] [MYID] %ld [POS] %d\n%s",CYAN,temp.msgtype,temp.position, RESET);

        /*
        for(int i  = 0; i < 10; i++){
            printf("%d ", airport->max_flights[i]);
            puts("");
         }*/

        if(temp.position == -1){
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] %s", data->node->flight_code);
            write_to_log(aux);
            free(aux);
            pthread_exit(NULL);//Exited due to being rejected
        }

        //Post Thread Activity
        pthread_mutex_lock(&airport->mutex_command);
        while (command == 1) {//colocar a condicao uma vez a shared memory criada com as posicoes para os comandos
            if(showVerbose == 1 ) printf("%s[THREAD][WAITING FOR COMMAND] [MYID] %ld [POS] %d%s\n",YELLOW,temp.msgtype,temp.position, RESET);
            pthread_cond_wait(&airport->command_var, &airport->mutex_command);
            command = airport->max_flights[temp.position];
            if(showVerbose == 1 ) printf("%s READ NEW COMMAND SUCCESSFULLY %d %s\n", RED,command, RESET);

        }
        pthread_mutex_unlock(&airport->mutex_command);;
        if (command == 2) {
            strcpy(track, "R1");
        } else if (command == 3) {
            strcpy(track, "R2");
        } else {
            puts("WRONG COMMAND UNEXPECTED BEHAVIOR. EXITING");
            exit(-1);
        }

        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s DEPARTURE %s started", data->node->flight_code, track);
        write_to_log(aux);
        free(aux);

        usleep((takeoff_time* time_unit) * 1000); //external global var

        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s DEPARTURE %s concluded", data->node->flight_code, track);
        write_to_log(aux);
        free(aux);

        //Final Thread Activity
        airport->max_flights[temp.position] = 0;
        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] %s", data->node->flight_code);
        write_to_log(aux);
        free(aux);
        free(data->node);
        free(data);
        if(showVerbose == 1 ) printf("%s THREAD EXITED SUCCESSFULLY \n%s", MAGENTA,RESET);
        pthread_detach(pthread_self());
        pthread_exit(NULL);
    }


    void *arrival(void *arg) {
        /* Handles all functionality for the Arrival-Type Threads
         *
         * Parameters
         *      arg - Struct which holds a struct that contains flight info
         */
        struct args_threads *data = (struct args_threads *) arg;
        struct sharedmem_info temp; //Holds the message that the that the control tower sends to the thread with the position in shared memory
        char *aux = malloc(sizeof(char) * BUFFER_SIZE);;//Writing to log temporary variable
        int emergency_condition = 4 + data->node->eta + landing_time;//Priority flight condition
        struct message msg;
        int command = 1;
        char track[3];

        // Initial Thread Behavior
        sprintf(aux, "[ARRIVAL THREAD CREATED] [FLIGHT CODE] : %s [ETA]: %d [FUEL]: %d", data->node->flight_code,data->node->eta, data->node->fuel);
        write_to_log(aux);
        free(aux);

        /*MSQ Messaging*/
        if (data->node->fuel <= emergency_condition) {
            msg.msgtype = MSGTYPE_PRIORITY;
            airport->total_emergency++;
            aux = malloc(sizeof(char) * BUFFER_SIZE);;//Writing to log temporary variable
            sprintf(aux, "[%s] REQUESTED EMERGENCY LANDING", data->node->flight_code);
            write_to_log(aux);
            free(aux);

        } else {
            msg.msgtype = MSGTYPE_DEFAULT;
        }
        msg.mode = 1;
        msg.fuel = data->node->fuel;
        msg.time_to_track = data->node->eta;
        msg.id = data->id;
        msg.flight_code = get_flight_code(data->node->flight_code);


        if(showVerbose == 1 ) printf("%s[THREAD][MSG SENT] [ID] %d%s\n",CYAN, msg.id, RESET);

        if (msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {/*Sends message to the control tower*/
            perror("ERROR SENDING MESSAGE");
            exit(-1);
        }
        if(showVerbose == 1 ) printf("%s-------------[THREAD][MSG SENT SUCESSFULLY][ID] %d %s\n", WHITE,msg.id, RESET);

        if (msgrcv(mq_id, &temp, sizeof(temp) - sizeof(long), msg.id, 0) == -1) {/*Receives message from control tower*/
            perror("ERROR RECEIVING MESSAGE");
            exit(-1);
        }
        if(showVerbose == 1 ) printf("%s[THREAD][RECEIVED MSG SUCCESSFULLY] [MYID] %ld [POS] %d\n%s",CYAN,temp.msgtype,temp.position, RESET);

        //Wait for command loop

        if(temp.position == -1){\
            aux = malloc(sizeof(char) * BUFFER_SIZE);
            sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] : %s [THREAD REJECTED]", data->node->flight_code);
            write_to_log(aux);
            free(aux);
            pthread_exit(NULL);//Exited due to being rejected
        }

        pthread_mutex_lock(&airport->mutex_command);
        while (airport->max_flights[temp.position] == 1){//verifica se recebeu o comando para aterrar ou para ir para outro aeroporto, se receber sai

            pthread_cond_wait(&airport->command_var, &airport->mutex_command);
            command = airport->max_flights[temp.position];
            if(showVerbose == 1 ) printf("%s[THREAD][WAITING FOR COMMAND] [MYID] %s [POS] %d [COMMAND] %d\n%s",YELLOW,data->node->flight_code,temp.position,command, RESET);
            //printf("READ NEW COMMAND SUCESSFULLY %d\n", command);
            if (airport->max_flights[temp.position] == 7) {//se receber um holding escreve essa informacao no log
                //sprintf(aux, "%s HOLDING", data->node->flight_code);//TODO:RECHECK THIS
                //write_to_log function "{fligth_code} HOLDING {tempo de holding}"
                //A hora nao sei bem como fazer, usamos o tempo do pc? Ou temos um inicializador separado? O nosso timer nao serve para aquilo
                //o fligth_code vai buscar-se usando info -> nodo -> fligth_code
                //tempo de holding vai ser o airport -> avg_man_holding
                airport->max_flights[temp.position] = 1; //Set slot back to listening state
            }
        }
        pthread_mutex_unlock(&airport->mutex_command);

        //Command receiving behaviour

        if (command == 5 || command == 6) {//If the flight is going to land
            if (command == 5)strcpy(track, "L1");
            else strcpy(track, "L2");
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "%s LANDING %s started", data->node->flight_code, track);
            write_to_log(aux);
            free(aux);
            usleep((landing_time * time_unit) * 1000);
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "%s LANDING %s concluded", data->node->flight_code, track);
            write_to_log(aux);
            free(aux);
        } else if (command == 8) {//Order to fly away to another airport
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "%s LEAVING TO OTHER AIRPORT => FUEL = %d", data->node->flight_code, data->node->fuel);
            write_to_log(aux);
            free(aux);
        }


        //Deleting Thread and freeing memory
        airport->max_flights[temp.position] = 0;
        aux = malloc(sizeof(char) * BUFFER_SIZE);
        sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] : %s", data->node->flight_code);
        write_to_log(aux);
        free(aux);

        free(data->node);
        free(data);
        if(showVerbose == 1 ) printf("%s THREAD EXITED SUCCESSFULLY \n%s", MAGENTA,RESET);
        pthread_detach(pthread_self());
        pthread_exit(NULL);

    }
void *exit_thread(void *arg){
    struct message msg;

    if(showVerbose == 1 ) printf("LAST FLIGHT\n");
    pthread_mutex_lock(&exitor_mutex);
    pthread_cond_wait(&exitor_var, &exitor_mutex);
    pthread_mutex_unlock(&exitor_mutex);
    sleep(1);
    if(showVerbose == 1 ) printf("SENT LAST FLIGHT\n");
    msg.msgtype = MSGTYPE_EXIT;
    msg.mode = -1;
    msg.fuel = -1;
    msg.time_to_track = -1;
    msg.id = -1;
    msg.flight_code = 0;

    if (msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {/*Sends message to the control tower*/
        perror("ERROR SENDING MESSAGE");
        exit(-1);
    }

    pthread_exit(NULL);
}

int get_flight_code(char *flight_code){
        return atoi(&flight_code[2]);
    }

    void print_msg(struct message *node) {
        if (node != NULL) {
            puts("-------STARTING TO PRINT NODE-----------");
            printf("MSG TYPE %ld\n"
                   "MODE %d\n"
                   "FUEL %d\n"
                   "TTOTRACK %d \n"
                   "ID %d\n", node->msgtype = MSGTYPE_DEFAULT, node->mode, node->fuel, node->time_to_track, node->id);
            puts("-------ENDING PRINT NODE-----------");
        } else {
            puts("NULL ERROR");
        }

    }

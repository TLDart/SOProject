// Compile by using ./filename <configPath>
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
    main_pid = getpid();
    pthread_t time_thread, flight_creator;

    pthread_condattr_setpshared(&cattr,PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&command_var, &cattr);

    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex_command, &mattr);

    signal(SIGUSR1, showStats);   /*Handle Signals*/
    signal(SIGINT, exit_handler);

    clean_log(); /*ClLean "log.txt" if necessary*/
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

    airport->total_flights = 0;
    airport->total_landed = 0;
    airport->total_takeoff = 0;
    airport->redirected_flights = 0;
    airport->rejected_flights = 0;
    airport->avg_ETA = 0;
    airport->avg_takeoff = 0;
    airport->avg_man_holding = 0;
    airport->avg_man_emergency = 0;

    /*Create MSQ*/
    if ((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        perror("Failed to create message queue\n");
        exit(-1);
    }

    if(showVerbose == 1 ) printf("%s [MESSAGE QUEUE CREATED]%s\n", BLUE, RESET);
    /*Create Control Tower*/
    if (fork() == 0) {
        control_tower();
        exit(0);
    }


    /*Create named pipe*/
    unlink(PIPE_NAME);
    mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0666);
    if ((pipe_fd = open(PIPE_NAME, O_RDWR)) < 0) perror("Pipe Error");

    if(showVerbose == 1 ) printf("%s [NAMED PIPE CREATED]%s\n", BLUE, RESET);

    pthread_create(&flight_creator, NULL, create_flights, head); /*Create Flight_Creator_Thread */

    pthread_create(&time_thread, NULL, time_counter, airport);  /*Create Time_Thread*/

    if (showVerbose == 1) printf("Correctly initiated Time Thread\n");

    /*Run getting a message from the pipe*/
    get_message_from_pipe(pipe_fd);

    /*Cleaning The system*/
    shmdt(airport);
    shmctl(shmid, IPC_RMID, 0);
    unlink(PIPE_NAME);
    msgctl(shmid,IPC_RMID,0);

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
            if ((takeoff_time = atoi(token) == 0 && buffer[0] != '0')) { return -1; }
            token = strtok(NULL, delimiter);
            config_test(token);
            if ((takeoff_delta = atoi(token) == 0 && buffer[0] != '0')) { return -1; }
        }
        if (counter == 2) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if ((landing_time = atoi(token) == 0 && buffer[0] != '0')) { return -1; }
            token = strtok(NULL, delimiter);
            config_test(token);
            if ((landing_delta = atoi(token) == 0 && buffer[0] != '0')) { return -1; }

        }
        if (counter == 3) {
            token = strtok(buffer, delimiter);
            config_test(token);
            if ((min_hold = atoi(token) == 0 && buffer[0] != '0')) { return -1; }
            token = strtok(NULL, delimiter);
            config_test(token);
            if ((max_hold = atoi(token) == 0 && buffer[0] != '0')) { return -1; }

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

void showStats(int signum) {
    /* Prints the stats to stdout
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


void exit_handler(int signum) {
    /* Changes some flags and prepares the program to exit
     *
     * Parameter
     *      signum - Number to the signal
     */
    write_to_log("[PROGRAM ENDING]");

    running = 0;

    /*Setting the pipe to non block*/
    fcntl(pipe_fd, F_SETFL, O_NONBLOCK);

    //remover o mutex_write usado no write_to_log, para ficarem todos juntos, podem mudar-se todos os outros para depois desta instrução
    pthread_mutex_destroy(&mutex_write);

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
        if (strcmp(argv[i], "-t") == 0) {
            showTime = 1;
        }
        if (strcmp(argv[i], "-v") == 0) {
            showVerbose = 1;
        }
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                free(filename);
                filename = malloc(sizeof(argv[i + 1]));
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

    fprintf(logfile, "%2d:%2d:%2d %s\n", parsed_time->tm_hour, parsed_time->tm_min, parsed_time->tm_sec, msg);
    printf("%2d:%2d:%2d %s\n", parsed_time->tm_hour, parsed_time->tm_min, parsed_time->tm_sec, msg);
}

void get_message_from_pipe(int file_d) {
    /* Reads message coming from the Named Pipe
     *
     * Parameter :
     *      file_d = Indication of the file descriptor of the pipe
     *
     */
    puts("[PIPE HANDLER THREAD CREATED]");
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
                    if (showVerbose == 1) print_node(parsed_data);
                    add_flight(parsed_data, head);
                }
            }
        }
    }
     /*Read last bytes from the disk and send them the log file*/
    puts("Cleaning Pipe");
    while((nread = read(file_d, buffer, BUFFER_SIZE)) > 0){
        write_to_log(buffer);
    }
}

/*Threaded Functions*/

    void *time_counter(void *arg) {
        /* Count time according to the time Unit variable read from config
         *
         *
         */
        puts("TIME-COUNTER THREAD CREATED");
        while (1) {
            pthread_mutex_lock(&mutex_time);
            airport->time++;
            if (showTime == 1) printf("[TIME] : %d\n", airport->time);
            pthread_mutex_unlock(&mutex_time);
            pthread_cond_broadcast(&time_var);

            /*Converting ms to us*/
            usleep(time_unit * 1000);
        }
    }

    void *create_flights(void *pointer) {
        /* Thread Function That handle flight Creation. Verifies the heads of the Linked list and checks if it is time for a flight to be created
         *
         * Parameters:
         *      pointer - Pointer to struct of type p_node which is the head of the list
         */
        printf("FLIGHT CREATOR");
        p_node list = head, flight;
        pthread_t thread; //New thread to be Created
        struct args_threads *args;// Pointer to struct that holds flight information, this struct will be handed to the flight on creation

        while (running || list->next != NULL) {
            flight = list->next;
            //Verify, with mutual exclusion and condition variable, if it is time for the flight to be created
            while (flight != NULL && flight->init <= airport->time) {
                args = (struct args_threads *) malloc(sizeof(struct args_threads)); //Needs to be freed from within the departure/ arrival function;
                args->id = ids;
                args->node = flight;//TODO: This pointer needs to be freed

                /*Create a thread*/
                if (strcmp(flight->mode, "DEPARTURE") == 0) {
                    pthread_create(&thread, NULL, departure, args);
                    airport->total_flights++;
                } else if (strcmp(flight->mode, "ARRIVAL") == 0) {
                    pthread_create(&thread, NULL, arrival, args);
                    airport->total_flights++;

                } else {
                    puts("[THREAD CREATION ERROR]");
                    write_to_log("[THREAD CREATION ERROR]");
                }
                ids++; //Increment Thread Unique ID
                list->next = list->next->next; //Removes from list without destroying node
                flight = list->next;
            }
            usleep(time_unit * 1000);
        }
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
        printf("%s\n", aux);
        write_to_log(aux);
        free(aux);

        /*Create the message*/
        msg.msgtype = MSGTYPE_DEFAULT;
        msg.mode = 0;
        msg.fuel = -1;//Departing planes have no fuel
        msg.time_to_track = data->node->takeoff;
        msg.id = data->id;

        printf("%s[THREAD][MSG SENT][ID] %d %s\n", CYAN,msg.id, RESET);

        if (msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {/*Sends the message to the control tower*/
            perror("SENDING MESSAGE ERROR");
            exit(-1);
        }
        if (msgrcv(mq_id, &temp, sizeof(temp) - sizeof(long), msg.id, 0) == -1) {/*Reads the message from the control Tower*/
            perror("RECEIVING MESSAGE ERROR");
            exit(-1);
        }
        printf("%s[THREAD][RECEIVED MSG SUCCESSFULLY] [MYID] %ld [POS] %d\n%s",CYAN,temp.msgtype,temp.position, RESET);

        for(int i  = 0; i < 10; i++){
            printf("%d ", airport->max_flights[i]);
        }
        puts("");
        //Post Thread Activity
        pthread_mutex_lock(&mutex_command);
        while (command == 1) {//colocar a condicao uma vez a shared memory criada com as posicoes para os comandos
            printf("%s[THREAD][WAITING FOR COMMAND] [MYID] %ld [POS] %d%s\n",YELLOW,temp.msgtype,temp.position, RESET);
            pthread_cond_wait(&command_var, &mutex_command);
            printf("%s READ NEW COMMAND SUCCESSFULLY %d %s\n", RED,command, RESET);
            command = airport->max_flights[temp.position];

        }
        pthread_mutex_unlock(&mutex_command);
        puts("HERE");
        if (command == 2) {
            strcpy(track, "R1");
        } else if (command == 3) {
            strcpy(track, "R2");
        } else {
            puts("WRONG COMMAND UNEXPECTED BEHAVIOR. EXITING");
            exit(-1);
        }

        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s DEPARTURE %s started", data->node->flight_code, track); // TODO: Complete with departure track
        write_to_log(aux);
        free(aux);

        sleep(takeoff_time); //external global var

        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s DEPARTURE %s concluded", data->node->flight_code, track);// TODO:Complete with departure track
        write_to_log(aux);
        free(aux);

        //Final Thread Activity
        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] %s", data->node->flight_code);
        write_to_log(aux);
        free(data->node);
        free(data);
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
        } else {
            msg.msgtype = MSGTYPE_DEFAULT;
        }
        msg.mode = 1;
        msg.fuel = data->node->fuel;
        msg.time_to_track = data->node->eta;
        msg.id = data->id;


        printf("%s[THREAD][MSG SENT] [ID] %d%s\n",CYAN, msg.id, RESET);

        if (msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {/*Sends message to the control tower*/
            perror("ERROR SENDING MESSAGE");
            exit(-1);
        }

        if (msgrcv(mq_id, &temp, sizeof(temp) - sizeof(long), msg.id, 0) == -1) {/*Receives message from control tower*/
            perror("ERROR RECEIVING MESSAGE");
            exit(-1);
        }

        printf("%s[THREAD][RECEIVED MSG SUCCESSFULLY] [MYID] %ld [POS] %d\n%s",CYAN,temp.msgtype,temp.position, RESET);
        //Wait for command loop

        pthread_mutex_lock(&mutex_command);
        while (airport->max_flights[temp.position] == 1){//verifica se recebeu o comando para aterrar ou para ir para outro aeroporto, se receber sai
            printf("%s[THREAD][WAITING FOR COMMAND] [MYID] %ld [POS] %d\n%s",YELLOW,temp.msgtype,temp.position, RESET);
            pthread_cond_wait(&command_var, &mutex_command);
            command = airport->max_flights[temp.position];
            printf("READ NEW COMMAND SUCESSFULLY %d", command);
            if (airport->max_flights[temp.position] == 7) {//se receber um holding escreve essa informacao no log
                aux = (char *) malloc(sizeof(char) * SIZE);
                sprintf(aux, "%s HOLDING %2.lf", data->node->flight_code, airport->avg_man_holding);//TODO:RECHECK THIS
                //write_to_log function "{fligth_code} HOLDING {tempo de holding}"
                //A hora nao sei bem como fazer, usamos o tempo do pc? Ou temos um inicializador separado? O nosso timer nao serve para aquilo
                //o fligth_code vai buscar-se usando info -> nodo -> fligth_code
                //tempo de holding vai ser o airport -> avg_man_holding
                airport->max_flights[temp.position] = 1; //Set slot back to listening state
                write_to_log(aux);
                free(aux);
            }
        }
        pthread_mutex_unlock(&mutex_command);

        //Command receiving behaviour

        if (command == 5 || command == 6) {//If the flight is going to land
            if (command == 5)strcpy(track, "L1");
            else strcpy(track, "L2");
            aux = (char *) malloc(sizeof(char) * SIZE);
            printf("%s\n", aux);
            sprintf(aux, "%s LANDING %s started", data->node->flight_code, track);
            write_to_log(aux);
            free(aux);
            sleep(landing_time);//Waits landing time
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "%s LANDING %s concluded", data->node->flight_code, track);
            printf("%s\n", aux);
            write_to_log(aux);
            free(aux);
        } else if (command == 8) {//Order to fly away to another airport
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "%s LEAVING TO OTHER AIRPORT => FUEL = %d", data->node->flight_code, data->node->fuel);
            write_to_log(aux);
            free(aux);
        }


        //Deleting Thread and freeing memory
        aux = malloc(sizeof(char) * BUFFER_SIZE);
        sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] : %s", data->node->flight_code);
        write_to_log(aux);

        free(data->node);
        free(data);
        pthread_exit(NULL);
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
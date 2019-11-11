
#include "Thread.h"
extern p_node head;
extern pthread_mutex_t mutex_time, mutex_command;
extern pthread_cond_t time_var, command_var;
extern int ids, time_unit, mq_id, takeoff_time, landing_time;
extern shared_mem* airport;

void *time_counter(void *arg) {
    /* Count time according to the time Unit variable read from config
     *
     *
     */
    puts("TIME-COUNTER THREAD CREATED");
    while (1) {
        pthread_mutex_lock(&mutex_time);
        airport->time++;
        pthread_mutex_unlock(&mutex_time);
        pthread_cond_broadcast(&time_var);
        usleep(time_unit * 1000); //Converting ms to us
    }
}


void *get_message_from_pipe(void *arg) {
    /* Reads message coming from the Named Pipe
     *
     * Parameter :
     *      arg = pointer to the pipe file descriptor
     *
     *
     *
     */
    puts("PIPE HANDLER THREAD CREATED");
    int fd = *((int *) arg);
    char buffer[BUFFER_SIZE];
    fd_set read_set;
    int nread;
    p_node parsed_data;

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);
    while (1) {
        if (select(fd + 1, &read_set, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(fd, &read_set)) {
                nread = read(fd, buffer, BUFFER_SIZE);
                if (nread > 0) {
                    buffer[nread] = '\0';
                    if (buffer[nread - 1] == '\n') //Remove trailing '\n' if necessary
                        buffer[nread - 1] = '\0';
                    parsed_data = parsing(buffer); // Handle the buffer
                    if (parsed_data != NULL) {
                        //print_node(parsed_data);
                        if (head == NULL) puts("NULLHEAD"); // Verify if list lead exists
                        add_flight(parsed_data, head);
                    }
                }
            }
        }
    }
}

void *create_flights(void *pointer) {
    /* Thread Function That handle flight Creation. Verifies the heads of the Linked list and checks if it is time for a flight to be created
     *
     * Parameters:
     *      pointer - Pointer to struct of type p_node which is the head of the list
     */
    puts("FLIGHT CREATOR THREAD CREATED");
    p_node list = head, flight;

    pthread_t thread; //New thread to be Created
    struct args_threads *args;// Pointer to struct that holds flight information, this struct will be handed to the flight on creation

    while (1) {
        flight = list->next;
        //Verify, with mutual exclusion and condition variable, if it is time for the flight to be created
        pthread_mutex_lock(&mutex_time);
        while (flight == NULL || flight->init > airport->time) {
            pthread_cond_wait(&time_var, &mutex_time); // Changed by Timer Thread
            flight = list->next;
        }
        pthread_mutex_unlock(&mutex_time);

        args = (struct args_threads *) malloc(sizeof(struct args_threads)); //Needs to be freed from within the departure/ arrival function;
        args->id = ids;
        args->node = flight;//TODO: This pointer needs to be freed

        //Functions to create the thread TODO: Functions that actually manipulate the shared memory
        if (strcmp(flight->mode, "DEPARTURE") == 0) {
            pthread_create(&thread, NULL, departure, args);
        } else if (strcmp(flight->mode, "ARRIVAL") == 0) {
            pthread_create(&thread, NULL, arrival, args);
        } else {
            puts("[THREAD CREATION ERROR]");
            write_to_log("[THREAD CREATION ERROR]");
        }
        ids++; //Increment Thread Unique ID
        list->next = list->next->next; //Removes from list without destroying node
    }
    /*
    TODO:Handle this thread after closure
    */
}


void *departure(void *arg) {
    /* Handles all functionality for the Departure-Type Threads
     *
     * Parameters
     *      arg - Struct which holds a struct that contains flight info
     *
     *
     */
    struct args_threads *data = (struct args_threads *) arg;
    char *aux = malloc(sizeof(char) * BUFFER_SIZE);
    struct sharedmem_info temp;//Holds the message returned by the control tower
    //int position;

    //Arrival Activity
    sprintf(aux, "[DEPARTURE THREAD CREATED] [FLIGHT CODE] : %s [TAKEOFF]: %d", data->node->flight_code,
            data->node->takeoff);
    printf("%s\n", aux);
    write_to_log(aux);
    free(aux);
    //Initial Tower Messaging
    /*
    struct message *msg = malloc(sizeof(struct message));
    msg -> msgtype = MSGTYPE_DEFAULT;
    msg -> mode = 0;
    msg -> fuel = -1;//Departing planes have no fuel
    msg -> time_to_track = data -> node -> takeoff;
    msg -> id = data -> id;

    if(msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1){
        perror("SENDING MESSAGE ERROR");
        exit(-1);
    }

    if(msgrcv(mq_id, &temp, sizeof(temp) - sizeof(long), data -> id, 0) == -1){ //PARA QUE ISTO FUNCIONE A CONTROL TOWER TEM DE MANDAR A MENSAGEM COM MSGTYPE IGUAL AO ID QUE LHE FOI PASSADO NA MENSAGEM PARA ESTA ENVIADA
        perror ("RECEIVING MESSAGE ERROR");
        exit(-1);
    }

    position = temp.position;

    //Post Thread Activity

    pthread_mutex_lock(&mutex_command);
    while(){//colocar a condicao uma vez a shared memory criada com as posicoes para os comandos
        pthread_cond_wait(&command_var, &mutex_command);
    }
    pthread_mutex_unlock(&mutex_command);

    aux = (char *) malloc(sizeof(char) * SIZE);
    sprintf(aux, "%s DEPARTURE {pista em que o voo vai partir} started", data->node->flight_code); // TODO: Complete with departure track
    write_to_log(aux);
    free(aux);
    sleep(takeoff_time); //external global var
    aux = (char *) malloc(sizeof(char) * SIZE);
    sprintf(aux, "%s DEPARTURE {colocar a pista} concluded",data->node->flight_code);// TODO:Complete with departure track
    write_to_log(aux);
    free(aux);
    */
    //Final Thread Activity
    aux = (char *) malloc(sizeof(char) * SIZE);
    sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] %s", data->node->flight_code);
    printf("%s\n", aux);
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
    int position; //Defined shared memory position for the flight
    char *aux = malloc(sizeof(char) * BUFFER_SIZE);;//Writing to log temporary variable
    int emergency_condition = 4 + data -> node -> eta + landing_time;//Priority flight condition

    // Initial Thread Behavior
    sprintf(aux, "[ARRIVAL THREAD CREATED] [FLIGHT CODE] : %s [ETA]: %d [FUEL]: %d", data->node->flight_code,
            data->node->eta, data->node->fuel);
    printf("%s\n", aux);
    write_to_log(aux);
    free(aux);

    //Post Thread Activity
    /*
    struct message *msg = malloc(sizeof(struct message));

    if(data -> node -> fuel == emergency_condition){
        msg -> msgtype = MSGTYPE_PRIORITY;
    }
    else{
        msg -> msgtype = MSGTYPE_DEFAULT;
    }
    msg -> mode = 1;
    msg -> fuel = data -> node -> fuel;
    msg -> time_to_track = data -> node -> eta;
    msg -> id = data -> id;

    if(msgsnd(mq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1){
        perror("ERROR SENDING MESSAGE");
        exit(-1);
    }

    if(msgrcv(mq_id, &temp, sizeof(temp) - sizeof(long), data -> id, 0) == -1){ //PARA QUE ISTO FUNCIONE A CONTROL TOWER TEM DE MANDAR A MENSAGEM COM MSGTYPE IGUAL AO ID QUE LHE FOI PASSADO NA MENSAGEM PARA ESTA ENVIADA
        perror("ERROR RECEIVING MESSAGE");
        exit(-1);
    }

    position = temp.position;

    //Wait for command loop
    pthread_mutex_lock(&mutex_command);
    while(){//verifica se recebeu o comando para aterrar ou para ir para outro aeroporto, se receber sai
        pthread_cond_wait(&command_var, &mutex_command);
        if(){//se receber um holding escreve essa informacao no log
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "%s HOLDING %2.lf", data -> node -> flight_code, airport -> avg_man_holding);//TODO:RECHECK THIS
            //write_to_log function "{fligth_code} HOLDING {tempo de holding}"
            //A hora nao sei bem como fazer, usamos o tempo do pc? Ou temos um inicializador separado? O nosso timer nao serve para aquilo
            //o fligth_code vai buscar-se usando info -> nodo -> fligth_code
            //tempo de holding vai ser o airport -> avg_man_holding
            write_to_log(aux);
            free(aux);
        }
    }
    pthread_mutex_unlock(&mutex_command);

    //Command receiving behaviour
    TODO:DECIDE COMMANDS AND WHAT THEY DO
    if(){//se o voo for aceite para aterrar
        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s LANDING {pista em que esta a aterrar} started", data -> node -> flight_code);//TODO:Track
        write_to_log(aux);
        free(aux);
        sleep(landing_time);//Waits landing time
        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s LANDING {pista em que esta a aterrar} concluded", data->node->flight_code);//TODO:Track
        write_to_log(aux);
        free(aux);
    }
    else if(){//se o comando recevido for para este ir para outro aeroporto
        aux = (char *) malloc(sizeof(char) * SIZE);
        sprintf(aux, "%s LEAVING TO OTHER AIRPORT => FUEL = %d", data -> node -> flight_code, data -> node -> fuel);
        write_to_log(aux);
        free(aux);
    }*/


    //Deleting Thread and freeing memory
    aux = malloc(sizeof(char) * BUFFER_SIZE);
    sprintf(aux, "[THREAD DELETED] [FLIGHT CODE] : %s", data->node->flight_code);
    printf("%s\n", aux);
    write_to_log(aux);
    free(data -> node);
    free(data);
    pthread_exit(NULL);
}

#include "Thread.h"
extern p_node head;
extern pthread_mutex_t mutex_time;
extern pthread_cond_t time_var;
extern int ids, time_unit;
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
                        print_node(parsed_data);
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
    puts("FLIGHT CREATOR THREAD CRATED");
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

    srand(time(NULL)); //Temporary
    struct args_threads *data = (struct args_threads *) arg;
    char temp[250];
    pthread_mutex_lock(&mutex_time);
    sprintf(temp, "[DEPARTURE THREAD CREATED] [FLIGHT CODE] : %s [TAKEOFF]: %d", data->node->flight_code,
            data->node->takeoff);
    printf("%s\n", temp);
    write_to_log(temp);
    pthread_mutex_unlock(&mutex_time);
    //Post Thread Activity
    sleep(rand() % 3); //Temporary
    sprintf(temp, "[THREAD DELETED] [FLIGHT CODE] %s", data->node->flight_code);
    printf("%s\n", temp);
    write_to_log(temp);
    //TODO: Free Memory?
    return NULL;
}


void *arrival(void *arg) {
    /* Handles all functionality for the Arrival-Type Threads
     *
     * Parameters
     *      arg - Struct which holds a struct that contains flight info
     */
    srand(time(NULL));//Temporary
    struct args_threads *data = (struct args_threads *) arg;
    char temp[250];
    pthread_mutex_lock(&mutex_time);
    sprintf(temp, "[ARRIVAL THREAD CREATED] [FLIGHT CODE] : %s [ETA]: %d [FUEL]: %d", data->node->flight_code,
            data->node->eta, data->node->fuel);
    printf("%s\n", temp);
    write_to_log(temp);
    pthread_mutex_unlock(&mutex_time);
    //Post Thread Activity
    sleep(rand() % 3);//Temporary
    sprintf(temp, "[THREAD DELETED] [FLIGHT CODE] : %s", data->node->flight_code);
    printf("%s\n", temp);
    write_to_log(temp);
    //TODO:Free Memory
    return NULL;
}

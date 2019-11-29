#include "ControlTower.h"

void control_tower() {
    /* Handles Flight Threads, shared memory communication, and Message Queue Messaging
     *
     */
    signal(SIGINT, SIG_IGN); /*Ignore SIGINT*/

    arrival_list = create_ct_info();
    departure_list = create_ct_info();
    pthread_t msg_reader;
    puts("CONTROL TOWER CREATED");

    // Insert Control Tower Code
    pthread_create(&msg_reader, NULL, get_messages, NULL);

    sleep(10);
    printf("%s MSG SENT %s\n", RED, RESET);
    pthread_mutex_lock(&mutex_command);
    airport->max_flights[0] = 2;
    pthread_mutex_lock(&mutex_command);
    pthread_cond_broadcast(&command_var);

    pthread_join(msg_reader, NULL);
}


struct CT_info* create_ct_info(){
    /* Creates a linked list with header
     *
     * Returns:
     *      Return an element of type message_array with default elements and NULL values, this will be the head
     */
    struct CT_info* head = (struct CT_info*) malloc(sizeof(struct CT_info));
    head->fuel = -1;
    head->time_to_track = -1;
    head->id = -1;
    head->pos = -1;
    return head;

}

void fuel_decrement(void* arg){
    /* Thread function that decrements fuel every time unit
     *
     *
     * */
    struct CT_info* head = (struct CT_info*) arg;
    pthread_mutex_lock(&mutex_time);
    while(running != 0) {
        pthread_cond_wait(&time_var, &mutex_time);
        while (head->next != NULL) {
            head->fuel--;
        }
    }
    pthread_mutex_unlock(&mutex_time);
}

void add_ct_info(struct CT_info* node, struct CT_info* head) {
    /* Adds message_array type element to the  end of the list
     * TODO: Scheduler
     *
     * Parameters:
     *      node - p_node type element that we want to add to our list
     *      head - specifies the head of a message type list
     */
    while(head->next != NULL){
        head = head->next;
    }
    head->next = node;
}

void *get_messages(void *arg){
    /*  Thead Function that process messages from the incoming message queue.
     * It both reads the message as well as it searches for an available spot in the array, and finally sends a message to the thread with that info;
    *
    */

    struct message aux;
    struct CT_info* element;
    struct message_array *temp;
    struct sharedmem_info info;
    int pos;

    printf("[CONTROL TOWER NOW RECEIVING MESSAGES]\n");

    while(running){//Reads messages until it receives a termination condition
        if(msgrcv(mq_id, &aux, sizeof(aux) - sizeof(long), -MSGTYPE_DEFAULT, 0) == -1){
            printf("ERROR RECEIVING MSQ MSG\n");
        }
        /*if(showStats == 1)*/ printf("%s -->>[CT][RECEIVED MSG SUCCESSFULLY][ID] %d [MODE] %d%s\n",CYAN,aux.id,aux.mode, RESET);

        element = (struct CT_info *) malloc(sizeof(struct CT_info));//Used to store the message received by the threads;
        element->fuel = aux.fuel;//Departing planes have no fuel
        element->time_to_track = aux.time_to_track;
        element->id = aux.id;
        element->next = NULL;
        if((pos = index_shm()) != -1){
            element->pos = pos;
        }

        if(aux.mode == 1){//Arrival type flight
            add_ct_info(element,arrival_list);
            if(showVerbose == 1) print_ct("Arrival",  arrival_list);
            if(showVerbose == 1) printf("ADDED TO ARRIVAL LIST\n");
        }
        else if(aux.mode == 0){//Departure type flight
            add_ct_info(element, departure_list);
            if(showVerbose == 1) print_ct("Departure",  departure_list);

            if(showVerbose == 1) printf("ADDED TO DEPARTURE LIST\n");
        }
        else if(aux.mode == -1){
            running = 0;// -1 means that it receives order to terminate the program
        }
        else{ puts("ERROR ADDING FLIGHT TO ARRIVAL/DEPARTURE ARRAY");}

        //Reply to the thread with the correct shared memory position
        info.msgtype = aux.id;//Select the correct thread ID
        info.position = pos;

        airport->max_flights[element->pos] = 1;
        if(msgsnd(mq_id, &info, sizeof(info) - sizeof(long), 0) == -1){ /*Fill the position in the shared memory*/
            puts("ERROR SENDING MESSAGE (CT)");
        }
        /*if(showStats == 1)*/ printf("%s -->>[CT][SENT MSG SUCCESSFULLY][ID] %ld [POS] %d%s\n",CYAN,info.msgtype,info.position, RESET);
    }
    if(showVerbose == 1) printf("%s [CT] Get messages thread ended %s", RED,RESET);
    pthread_exit(NULL);
}
int index_shm(){
    /*
    *Searches the Shared memory for an empty slot (Non 0 slot), in which the flight is gonna be put
    *
    * Return
    *   -1 In case of error
    *
     *   Else : Returns the shared memory slot given to the flight
    */
    int i = 0;
    //printf("TOTAL SIZE %d",max_takeoffs+ max_landings);
    for (i = 0; i < (max_takeoffs+ max_landings) ; i++){ //para encontrar uma posicao no vetor para atribuir ao voo
        if( airport->max_flights[i] == 0){
            return i;
        }
    }
    puts("[UNAVAILABLE SPACE FOR FLIGHT THREAD]");
    return -1;
}

void print_ct(char* name, struct CT_info* list) {
    printf("------PRINTING %s------\n", name);
    list = list->next;
    while (list) {
        printf("ID %d FUEL %d\n", list->id, list->fuel);
        list = list->next;
    }
}

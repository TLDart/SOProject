
#include "ThreadCT.h"
#include <stdio.h>
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
    while(condition != 0) {
        pthread_cond_wait(&time_var, &mutex_time);
        //TODO NEEDS MUTEX?
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
    pthread_cond_broadcast(&command_var);
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
    struct CT_info *arrival_list = create_ct_info();
    struct CT_info *departure_list = create_ct_info();
    printf("[CONTROL TOWER NOW RECEIVING MESSAGES]\n");

    while(condition){//Reads messages until it receives a termination condition
        //aux = (struct message *) malloc(sizeof(struct message));//Used to store the message received by the threads;
        if(msgrcv(mq_id, &aux, sizeof(aux) - sizeof(long), MSGTYPE_DEFAULT, 0) == -1){
            printf("ERROR RECEIVING MSQ MSG\n");
        }
        printf("-->>[CT][RECEIVED MSG SUCCESSFULLY][ID] %d [MODE] %d\n",aux.id,aux.mode);
        //print_msg(&aux);
        element = (struct CT_info *) malloc(sizeof(struct CT_info));//Used to store the message received by the threads;
        element->fuel = aux.fuel;//Departing planes have no fuel
        element->time_to_track = aux.time_to_track;
        element->id = aux.id;
        if((pos = index_shm()) != -1){
            element->pos = pos;
        }

        if(aux.mode == 1){//Arrival type flight
            add_ct_info(element,arrival_list);
            //printf("ADDED TO ARRIVAL LIST\n");
        }
        else if(aux.mode == 0){//Departure type flight
            add_ct_info(element, departure_list);
            //printf("ADDED TO DEPARTURE LIST\n");
        }
        else if(aux.mode == -1){
            condition = 0;// -1 means that it receives order to terminate the program
        }
        else{ puts("ERROR ADDING FLIGHT TO ARRIVAL/DEPARTURE ARRAY");}

        //Reply to the thread with the correct shared memory position
        //info = (struct sharedmem_info *) malloc(sizeof(struct sharedmem_info));
        info.msgtype = aux.id;//Select the correct thread ID
        info.position = pos;
        //printf(" MSG TYPE %d POS %d\n", aux.id, info.position);
        //Fill the position in the shared memory
        airport->max_flights[element->pos] = 1;
        if(msgsnd(mq_id, &info, sizeof(info) - sizeof(long), 0) == -1){
            puts("ERROR SENDING MESSAGE (CT)");
        }
    }
    //acho que nao e preciso escrever que esta thread acabou no log
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

#include "ThreadCT.h"
#include <stdio.h>
struct message_array* create_msg_array(){
    /* Creates a linked list with header
     *
     * Returns:
     *      Return an element of type message_array with default elements and NULL values, this will be the head
     */
    struct message_array* head = (struct message_array*) malloc(sizeof(struct message_array));
    head-> msg = NULL;
    head-> next = NULL;
    return head;

}

void fuel_decrement(void* arg){
    /* Thread function that decrements fuel every time unit
     *
     *
     * */
    struct message_array* head = (struct message_array*) arg;
    pthread_mutex_lock(&mutex_time);
    while(condition != 0) {
        pthread_cond_wait(&time_var, &mutex_time);
        while (head->next != NULL) {
            head->msg->fuel--;
        }

    }
}

void add_msg_array(struct message_array* node, struct message_array* head) {
    /* Adds message_array type element to the  end of thelist
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

    struct message *aux;
    struct message_array *temp;
    struct sharedmem_info *info;
    int pos;
    struct message_array *arrival_list = create_msg_array();
    struct message_array *departure_list = create_msg_array();
    printf("[CONTROL TOWER NOW RECEIVING MESSAGES]\n");

    while(condition){//Reads messages until it receives a termination condition
        //printf("LOOP\n");
        aux = (struct message *) malloc(sizeof(struct message));//Used to store the message received by the threads;
        if(msgrcv(mq_id, aux, sizeof(aux) - sizeof(long), MSGTYPE_DEFAULT, 0) == -1){
            printf("ERROR RECEIVING MSQ MSG\n");
        }
        printf("[CT][RECEIVED MSG SUCCESSFULLY][ID] %d [MODE] %d\n",aux->id,aux->mode);


        if(aux -> mode == 1){//Arrival type flight
            temp = (struct message_array *) malloc(sizeof(struct message_array));
            temp -> msg = aux;
            temp -> next = NULL;
            add_msg_array(temp,arrival_list);
        }
        else if(aux -> mode == 0){//Departure type flight
            temp = (struct message_array *) malloc(sizeof(struct message_array));
            temp -> msg = aux;
            temp -> next = NULL;
            add_msg_array(temp, departure_list);
        }
        else if(aux -> mode == -1){
            condition = 0;// -1 means that it receives order to terminate the program
        }
        else{
            puts("ERROR ADDING FLIGHT TO ARRIVAL/DEPARTURE ARRAY");
        }

        //Reply to the thread with the correct shared memory position
        info = (struct sharedmem_info *) malloc(sizeof(struct sharedmem_info));
        info -> msgtype = aux -> id;//Select the correct thread ID
        if((pos = index_shm()) != -1){
            info -> position = pos;
        }
        else{
            puts("ERROR FINDING SHARED MEMORY SLOT");
        }
         printf(" MSG TYPE %d POS %d\n", aux -> id, info->position);
        //Fill the position in the shared memory
        airport->max_flights[info->position] = 1;
        if(msgsnd(mq_id, info, sizeof(info) - sizeof(long), 0) == -1){
            puts("ERROR SENDING MESSAGE (CT)");
        }
        free(info);

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

    for (i = 0; i < (max_takeoffs+ max_landings) ; i++){ //para encontrar uma posicao no vetor para atribuir ao voo
        if( airport->max_flights[i] == 0){
             return i;
        }
    }
    puts("[UNAVAILABLE SPACE FOR FLIGHT THREAD]");
    return -1;
}
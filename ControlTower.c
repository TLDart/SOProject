#include "ControlTower.h"


void control_tower(){
    sem_unlink(CAN_HOLD);
    can_hold = sem_open(CAN_HOLD,O_CREAT| O_EXCL,0700,0);
    sem_unlink(CAN_SEND);

    header_departure = create_departure_list();
    header_arrival = create_arrival_list();

    can_hold = sem_open(CAN_SEND,O_CREAT | O_EXCL,0700,0);
    pthread_create(&messenger,NULL, get_messages, NULL);
    pthread_join(messenger,NULL);
}


void *get_messages(void *arg) {
    /*  Thread Function that process messages from the incoming message queue.
     * It both reads the message as well as it searches for an available spot in the array, and finally sends a message to the thread with that info;
    *
    */
    struct message msg_rcv;
    struct sharedmem_info msg_sent;

    while(runningCT){
        if(msgrcv(mq_id, &msg_rcv, sizeof(struct message)- sizeof(long), -MSGTYPE_EXIT,0) < 0){
            printf("There as an error sending the message");
        }
        printf("Message Received\n");
        //If the message was received
        new_message = 1;

        //Verify if the flight can't received
        if(msg_rcv.mode == 1) { //Arrival
            if(counter_arr == max_landings){//Flight rejected
                airport->rejected_flights++; //Increment stats
                msg_sent.position = -1;
            }
            else{
                pthread_mutex_lock(&flight_verifier);
                msg_sent.position = index_shm(); // Finds an available index the shared_mem
                new_message = 1;
                pthread_cond_broadcast(&awake_holder); // Broadcasts to awake the thread
                //sem_wait(can_send);
                add_arrival(header_arrival, create_node_arrival(&msg_rcv, msg_sent.position));
                header_arrival ->number_of_nodes++;
                //sem_post(can_hold);
                new_message = 0;
                pthread_mutex_unlock(&flight_verifier);
            }

        }
        else if(msg_rcv.mode == 0){// Departure

            if(counter_dep == max_takeoffs){
                airport->rejected_flights++;
                msg_sent.position = -1;
            }
            else{
                pthread_mutex_lock(&flight_verifier);
                msg_sent.position = index_shm();
                new_message = 1;
                //sem_wait(can_send);
                add_to_departure(header_departure, create_node_departure(&msg_rcv, msg_sent.position));
                header_departure ->number_of_nodes++;
                //sem_post(can_hold);
                new_message = 0;
                pthread_mutex_unlock(&flight_verifier);

            }

        }
        //Send the message
        msg_sent.msgtype = msg_rcv.id;

            if(msg_sent.position == -1){
                printf("HERE\n");
            }
        if(msgsnd(mq_id, &msg_sent, sizeof(struct sharedmem_info)- sizeof(long), 0) < 0){
            printf("Error sending the messsage\n");//TODO MIGHT be a problem here

        }

    }
    pthread_exit(NULL);
}
int index_shm(){
    int i = 0;

    for(i = 0; i < max_landings + max_takeoffs ; i++){
        if(airport -> max_flights[i] == 0){
            airport -> max_flights[i] = 1;
            return i;//returns the index of the shared memory designated to certain flight
        }
    }
    return -1;//if an error occurs, -1 is the return value
}

void flight_handler(){




}
struct list_arrival *create_arrival_list(){
    /* Creates the arrival list
     * Number of nodes is only used by the head of the linked list
     */
    struct list_arrival *header;
    //Creating the list
    if((header = (struct list_arrival *) malloc(sizeof(struct list_arrival))) == NULL){
        printf("ERRO ao criar a lista de voos arrival.\n");
        return NULL;
    }
    else {
        header->priority = -1;
        header->eta = -1;
        header->fuel = -1;
        header->shared_memory_index = -1;
        header->number_of_nodes = 0;
        header->next = NULL;
    }
    return header;
}
struct list_arrival *create_node_arrival(struct message *information, int position){
    /*
     * Creates an arrival type node using the de CT info from the message
     * Parameters
     *      information - information that derives from the CT that was read from the message queue
     *
     * Returns
     *      Returns a struct node to be added to the arrivals list
     * */
    struct list_arrival *node;
    int priority = 0;

    if((node = (struct list_arrival *) malloc(sizeof(struct list_arrival))) == NULL){
        perror("Error creating node.\n");
        return NULL;
    }
    //verifica se o voo e prioritario
    if(information -> fuel == information -> time_to_track + landing_time + 4){
        priority = 1;
    }

    node -> priority = priority;
    node -> eta = information -> time_to_track;
    node -> fuel = information -> fuel;
    node -> shared_memory_index = position;
    node -> number_of_nodes = -1;
    node -> next = NULL;

    return node;
}
void add_arrival(struct list_arrival *header, struct list_arrival *node){
    /* Adds the arrival in the correct place in the arrival list, takes in consideration both priority and then fuel (in this order)
     * Parameters
     *      Header - pointer to the head of the list to be added
     *      Node - Node to be added
     *
     * */
    struct list_arrival * last, *current;

    if(header != NULL){//Verify if the list exists
        last = header;
        current = header -> next;

        while(current != NULL && node -> priority <= current -> priority && node -> eta > current -> eta){//While the priority is equal or lower than the current priority and the node's ETA is greater than the node in the list
            last = current;
            current = current -> next;

        }

        node -> next = current;
        last -> next = node;
    }
    else{
        printf("A lista de arrivals passada nao existe!!!!!!!\n");
    }
}
struct list_arrival *pop_arrival(struct list_arrival *header, struct list_arrival *node){
    /* Remover a node element of the list
     * Parameters
     *      Header - Head of the list to be verified
     *      Node - node to be popped
     *
*/
    struct list_arrival *last, *current = NULL;

    if(header != NULL && header -> next != NULL){
        last = header;
        current = header->next;

        while(current != NULL && current != node){
            last = current;
            current = current -> next;
        }

        if(current == NULL){
            printf("The inserted node does not belong to the list\n");
        }
        else{
            last -> next = current -> next; // removes the node from the list
        }
    }
    return current;
}
void remove_arrival(struct list_arrival *header, struct list_arrival *node){
    /* Same functionality as pop_flight() but this one removes from memory (frees pointer)
     Parameters
     *      Header - Head of the list to be verified
     *      Node - node to be removed
     *
     * */

    struct list_arrival *last, *current;

    if(header != NULL){

        last = header;
        current = header -> next;

        while(current != NULL && current != node){
            last = current;
            current = current -> next;
        }

        if(current == NULL){
            printf("O nodo que inseriu nao existe na lista de arrivals.\n");//nao sei queres fazer algo especial para o caso disto acontecer
        }
        else{
            last->next = current -> next;//Takes the node off the list
            free(current); // Frees pointer from memory
        }
    }
    else{
        printf("The List does not exist\n");
    }
}
struct list_departure* create_departure_list(){
    /* Creates the departure list
     */
    struct list_departure *header;
    //Creating the list
    if((header = (struct list_departure *) malloc(sizeof(struct list_departure))) == NULL){
        printf("Erro ao criar a lista de voos departure.\n");
        return NULL;
    }

    header -> takeoff = -1;
    header -> shared_memory_index = -1;
    header ->number_of_nodes = 0;
    header -> next = NULL;

    return header;
}
struct list_departure* create_node_departure(struct message* information, int position){
    /*Strips info from the struct received by the message queue and create a departure type item
     * Parameters
     *      information - Struct with the flight info, from the message queue
     * */

    struct list_departure *node;

    node = (struct list_departure *) malloc(sizeof(struct list_departure));

    node -> takeoff = information -> time_to_track;
    node -> shared_memory_index = position;
    node ->number_of_nodes = -1;
    node -> next = NULL;

    return node;
}
void add_to_departure(struct list_departure *header, struct list_departure * node){
    /* Adds the departure in the correct place in the departure list, takes in consideration both priority and then fuel (in this order)
     * Parameters
     *      Header - pointer to the head of the list to be added
     *      Node - Node to be added
     *
     * */


    struct list_departure *last, *current;

    if(header != NULL){
        last = header;
        current = header -> next;

        while(current != NULL && node -> takeoff > current -> takeoff){
            last = current;
            current = current -> next;
        }

        node -> next = current;
        last -> next = node;

    }
    else{
        printf("The inserted list does not exist (Departure list)\n");//nao sei se queres que o programa pare ou nao
    }
}


void remove_departure(struct list_departure *header, struct list_departure *node){
    /* Take node from the departure list and removes it permanently (freeing memory);
     * Parameters
     *      header - the head of the list
     *      node - the node to be removed
     * */
    struct list_departure *last, *current;

    if(header != NULL){

        last = header;
        current = header -> next;

        while(current != NULL && current != node){
            last = current;
            current = current -> next;
        }

        if(current == NULL){
            printf("The inserted node does not exist exist in the departures list\n");//nao sei se queres que o programa acabe ou nao se isto acontecer
        }
        else{
            last->next = current -> next;//Removes the node from the list
            free(current); //Free the memory associated with the node
        }
    }
    else{
        printf("The Inserted list does not exist\n");//nao sei se queres que o programa acabe ou nao se isto acontecer
    }

}
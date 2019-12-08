#include "ControlTower.h"


void control_tower(){
    signal(SIGINT, SIG_IGN);
    sem_unlink(CAN_HOLD);
    can_hold = sem_open(CAN_HOLD,O_CREAT| O_EXCL,0700,0);
    sem_unlink(CAN_SEND);

    header_departure = create_departure_list();
    header_arrival = create_arrival_list();

    can_send = sem_open(CAN_SEND,O_CREAT | O_EXCL,0700,0);
    pthread_create(&messenger,NULL, get_messages, NULL);

    flight_handler();

    pthread_join(messenger,NULL);

    /*Cleaning resources*/
    /*Conditional variables and threads*/
    pthread_mutex_destroy(&flight_verifier);
    pthread_mutex_destroy(&awake_holder_mutex);
    pthread_cond_destroy(&awake_holder_var);

    /*Semaphores*/
    sem_close(can_send);
    sem_close(can_hold);
    sem_unlink(CAN_SEND);
    sem_unlink(CAN_HOLD);

    /*Linked lists*/
    free(header_arrival);
    free(header_departure);
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
                pthread_mutex_unlock(&flight_verifier);
                new_message = 1;
                pthread_cond_broadcast(&awake_holder_var); // Broadcasts to awake the thread
                pthread_mutex_unlock(&flight_verifier);
                sem_wait(can_send); //waits for the flight handler to put the node on the list
                add_arrival(header_arrival, create_node_arrival(&msg_rcv, msg_sent.position));
                header_arrival ->number_of_nodes++;
                sem_post(can_hold); //Says to the flight holder that it can hold
                pthread_mutex_lock(&flight_verifier);
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
                msg_sent.position = index_shm();
                pthread_mutex_lock(&flight_verifier);
                new_message = 1;
                pthread_cond_broadcast(&awake_holder_var); // Broadcasts to awake the thread
                pthread_mutex_unlock(&flight_verifier);
                sem_wait(can_send);
                add_to_departure(header_departure, create_node_departure(&msg_rcv, msg_sent.position));
                header_departure -> number_of_nodes++;
                sem_post(can_hold);
                pthread_mutex_lock(&flight_verifier);
                new_message = 0;
                pthread_mutex_unlock(&flight_verifier);

            }
        }
        else if(msg_rcv.mode == -1){
            printf("RECEIVED END MESSAGE\n");
            runningCT = 0;
            new_message = 1;
            pthread_cond_broadcast(&awake_holder_var);
            sem_wait(can_send);
            sem_post(can_hold);
            pthread_mutex_lock(&flight_verifier);
            new_message = 0;
            pthread_mutex_unlock(&flight_verifier);
        }
        //Send the message
        msg_sent.msgtype = msg_rcv.id;

        if(msg_sent.position == -1){
            printf("HERE\n");
        }
        if(msg_rcv.mode != -1) {
            if (msgsnd(mq_id, &msg_sent, sizeof(struct sharedmem_info) - sizeof(long), 0) < 0) {
                printf("Error sending the messsage\n");//TODO MIGHT be a problem here

            }
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
    /* This will hopefully handle the flights
     *
     * */
    srand(getppid());
    struct list_arrival *node_arrival;
    struct list_arrival *hold_temp;
    struct list_arrival *current_element;
    char *buffer;
    int random_number;
    int sleeptime = 0;
    int counter = 0;
    while(runningCT == 1 || header_departure->number_of_nodes != 0 || header_arrival->number_of_nodes != 0) {
        pthread_mutex_lock(&awake_holder_mutex);
        while(new_message != 1 && header_departure->number_of_nodes == 0 && header_arrival->number_of_nodes == 0){ // Stop if the both lists are empty and there is no new message
            pthread_cond_wait(&awake_holder_var,&awake_holder_mutex);

        }
        pthread_mutex_unlock(&awake_holder_mutex);

        sleeptime = 0;
        pthread_mutex_lock(&flight_verifier);
        if (new_message == 1) {
            pthread_mutex_unlock(&flight_verifier);
            sem_post(can_send);
            printf("PODE ADICIONAR\n");
            sem_wait(can_hold);
            printf("PODE CONTINUAR\n");

        } else {
            pthread_mutex_unlock(&flight_verifier);
        }
        //printf("NUMBER OF NODES %d\n", header_arrival->number_of_nodes);
        /*Decrementing Fuel*/
        if (header_arrival->number_of_nodes > 0) {
            node_arrival = header_arrival->next;
            while (node_arrival->next != NULL) {
                if (compare_time(begin, convert_to_wait(node_arrival->eta, time_unit)) == -1) {
                    node_arrival->fuel--;
                    //printf("DECREMENTED FUEL\n");

                }
                node_arrival = node_arrival->next;
            }
        }
        /*Chosing holding*/
        if (header_arrival->number_of_nodes > 5) {
            //printf("HOLDING\n");
            hold_temp = header_arrival->next;
            for (int i = 0; i < 5; i++) {
                hold_temp = hold_temp->next;
            }
            while (hold_temp != NULL && compare_time(begin, convert_to_wait(hold_temp->eta, time_unit)) == 1) {
                random_number = rand() % (max_hold - min_hold) + min_hold; //Calculates the random hold value
                if ((hold_temp->fuel - landing_time - random_number) > 0) {
                    current_element = hold_temp;
                    hold_temp = hold_temp->next;
                    current_element->eta += random_number;
                    airport->total_holding_man++;
                    airport->total_time_landing += random_number;
                    if (current_element->priority == 1) airport->total_emergency_holding_man++;
                    airport->max_flights[current_element->shared_memory_index] = 7;
                    buffer = (char *) malloc(sizeof(char) * SIZE);
                    sprintf(buffer, "TP%d HOLDING [%d]",current_element->flight_code,random_number);
                    write_to_log(buffer);
                    free(buffer);
                    pthread_cond_broadcast(&airport->command_var);
                    pop_arrival(header_arrival, current_element);
                    add_arrival(header_arrival, current_element);
                } else {
                    current_element = hold_temp;
                    hold_temp = hold_temp->next;
                    airport->max_flights[current_element->shared_memory_index] = 8;
                    airport->redirected_flights++;
                    header_arrival->number_of_nodes--;
                    pthread_cond_broadcast(&airport->command_var);
                    remove_arrival(header_arrival, current_element);
                }
            }
        }
        /*Choose flight to work)*/
        if(counter % 2 == 0){
            if (header_arrival->number_of_nodes > 0) {
                if(header_arrival->number_of_nodes > 1){
                    if (compare_time(begin, convert_to_wait(header_arrival->next->next->eta, time_unit)) == 1) {// If it is time to shedule the flight
                        //printf("CHOOSES 2\n");
                        airport->max_flights[header_arrival->next->next->shared_memory_index] = 6;
                        //pthread_cond_broadcast(&airport->command_var);
                        header_arrival->number_of_nodes--;
                        if(header_arrival->next->next->priority == 1) airport->total_emergency++;
                        airport->total_landed++;
                        remove_arrival(header_arrival, header_arrival->next->next);
                    }
                }

                if (compare_time(begin, convert_to_wait(header_arrival->next->eta, time_unit)) == 1) {// If it is time to shedule the flight
                    //printf("CHOOSES 1\n");
                    airport->max_flights[header_arrival->next->shared_memory_index] = 5;
                    pthread_cond_broadcast(&airport->command_var);
                    header_arrival->number_of_nodes--;
                    airport->total_landed++;
                    if(header_arrival->next->priority == 1) airport->total_emergency++;
                    remove_arrival(header_arrival,header_arrival->next);
                    sleeptime = landing_time + landing_delta;
                    usleep(sleeptime * 1000 * time_unit);
                }
            }
            //printf("TRIED\n");
        }
        else{
            if(header_departure->number_of_nodes > 0){
                if(header_departure->number_of_nodes > 1) {
                    if (compare_time(begin, convert_to_wait(header_departure->next->next->takeoff, time_unit)) ==
                        1) {// If it is time to shedule the flight
                        airport->max_flights[header_departure->next->next->shared_memory_index] = 3; //Depart on R1
                        //pthread_cond_broadcast(&airport->command_var);
                        header_departure->number_of_nodes--;
                        airport->total_takeoff++;
                        remove_departure(header_departure, header_departure->next->next);
                    }
                }
                if(compare_time(begin, convert_to_wait(header_departure->next->takeoff, time_unit)) == 1) {// If it is time to shedule the flight
                    airport->max_flights[header_departure->next->shared_memory_index] = 2; //Depart on R2
                    pthread_cond_broadcast(&airport->command_var);
                    header_departure->number_of_nodes--;
                    airport->total_takeoff++;
                    remove_departure(header_departure,header_departure->next);
                    sleeptime = takeoff_time + takeoff_delta;
                    usleep(sleeptime * 1000 * time_unit);

                }
            }
        }
        if (header_arrival->number_of_nodes > 0) {
            //printf("NODES\n");
            node_arrival = header_arrival->next;
            while (node_arrival->next != NULL) {
                if (compare_time(begin, convert_to_wait(node_arrival->eta, time_unit)) == -1) {
                    //printf("SLEEPTIME %d\n", sleeptime);
                    node_arrival->fuel -= sleeptime;
                }
                node_arrival = node_arrival->next;
            }
        }
        counter++;
        //printf("ENDS LOOP\n");
        usleep(time_unit *1000); // Sleeping a time unit
    }
}
int compare_time(struct timespec begin, struct wt takeoff){
    struct timespec temp;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    temp.tv_sec = begin.tv_sec;
    temp.tv_nsec = begin.tv_nsec;

    if(begin.tv_nsec + takeoff.nsecs > 1000000000 ){
        temp.tv_sec += 1;
        temp.tv_nsec = (begin.tv_nsec + takeoff.nsecs) % 1000000000;
    }
    temp.tv_sec += takeoff.secs;

    if(now.tv_sec >= temp.tv_sec){
        return 1;
    }
    else{
        return -1;
    }
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
        header->flight_code = 0;
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
    node->flight_code = information->flight_code;
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

        while(current != NULL && node -> eta > current -> eta){//While the priority is equal or lower than the current priority and the node's ETA is greater than the node in the list
            last = current;
            current = current -> next;

        }
        while(current != NULL && node -> priority < current -> priority){//While the priority is equal or lower than the current priority and the node's ETA is greater than the node in the list
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
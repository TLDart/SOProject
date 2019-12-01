#include "ControlTower.h"

void control_tower() {
    /* Handles Flight Threads, shared memory communication, and Message Queue Messaging
     *
     */
    signal(SIGINT, SIG_IGN); /*Ignore SIGINT*/
    signal(SIGUSR1, showStatistics);   /*Handle Signals*/
    printf(("PID %d\n"), getpid());

    arrival_list = create_ct_info();
    departure_list = create_ct_info();
    pthread_t msg_reader;
    puts("CONTROL TOWER CREATED");

    // Insert Control Tower Code
    pthread_create(&msg_reader, NULL, get_messages, NULL);
    pthread_join(msg_reader, NULL);
}

void showStatistics(int signum) {
    /* Prints the stats to stdout, TU refers to time_units
     *
     * Parameters:
     *      signum = signal number
     */
    //TODO: sigprockmask instead of this
    signal(SIGUSR1, showStatistics);
    printf("Total number of flights : %d\n", airport->total_flights);
    printf("Total flights that Landed: %d\n", airport->total_landed);
    printf("Average Landing ETA: %.2lf TU\n ", (airport->total_time_landing * 1.0) / airport->total_landed);
    printf("Total Flights that TookOff: %d\n", airport->total_takeoff);
    printf("Average Takeoff Time : %.2lf TU\n ", (airport->total_time_takeoff * 1.0) / airport->total_takeoff);
    printf("Average number of holding maneuvers per Regular Flight : %lf\n", (airport->total_holding_man *1.0) / (airport->total_landed));
    printf("Average number of maneuvers per Emergency Flight : %lf\n", (airport->total_emergency_holding_man *1.0) / (airport->total_emergency));
    printf("Total redirected flights : %d\n", airport->redirected_flights);
    printf("Total rejected flights : %d\n", airport->rejected_flights);
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
    /*  Thread Function that process messages from the incoming message queue.
     * It both reads the message as well as it searches for an available spot in the array, and finally sends a message to the thread with that info;
    *
    */

    struct message aux;
    struct CT_info* element;
    struct sharedmem_info info;
    int pos, counter_departs = 0, counter_arriv = 0;

    printf("[CONTROL TOWER NOW RECEIVING MESSAGES]\n");

    while(running) {//Reads messages until it receives a termination condition
        if (msgrcv(mq_id, &aux, sizeof(aux) - sizeof(long), -MSGTYPE_DEFAULT, 0) == -1) {
            printf("ERROR RECEIVING MSQ MSG\n");
        }
        /*if(showStats == 1)*/ printf("%s -->>[CT][RECEIVED MSG SUCCESSFULLY][ID] %d [MODE] %d%s\n", CYAN, aux.id,
                                      aux.mode, RESET);

        element = (struct CT_info *) malloc(sizeof(struct CT_info));//Used to store the message received by the threads;
        element->fuel = aux.fuel;//Departing planes have no fuel
        element->time_to_track = aux.time_to_track;
        element->id = aux.id;
        element->next = NULL;

        if ((aux.mode == 1 && counter_arriv != max_landings) || (aux.mode == 0 && counter_departs != max_takeoffs)) {
            if ((pos = index_shm()) != -1) {
                element->pos = pos;
            }

            if (aux.mode == 1) {//Arrival type flight
                add_ct_info(element, arrival_list);//TODO CHANGE TO CORRECT LIST

                if (showVerbose == 1) print_ct("Arrival", arrival_list);
                if (showVerbose == 1) printf("ADDED TO ARRIVAL LIST\n");
            } else if (aux.mode == 0) {//Departure type flight
                add_ct_info(element, departure_list);//TODO CHANGE TO CORRECT LIST
                if (showVerbose == 1) print_ct("Departure", departure_list);

                if (showVerbose == 1) printf("ADDED TO DEPARTURE LIST\n");
            } else if (aux.mode == -1) {
                running = 0;// -1 means that it receives order to terminate the program
            } else { puts("ERROR ADDING FLIGHT TO ARRIVAL/DEPARTURE ARRAY"); }

            //Reply to the thread with the correct shared memory position
            info.msgtype = aux.id;//Select the correct thread ID
            info.position = pos;
            airport->max_flights[element->pos] = 1;

        }
        else{
            info.msgtype = aux.id; //Covers the case where there is not enough space
            info.position = -1;
            airport->rejected_flights++;
        }
            if (msgsnd(mq_id, &info, sizeof(info) - sizeof(long), 0) == -1) { /*Fill the position in the shared memory*/
                puts("ERROR SENDING MESSAGE (CT)");
            }
            /*if(showStats == 1)*/ printf("%s -->>[CT][SENT MSG SUCCESSFULLY][ID] %ld [POS] %d%s\n", CYAN, info.msgtype,
                                          info.position, RESET);
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

/*DEBUG FUNCTION*/
void print_ct(char* name, struct CT_info* list) {
    printf("------PRINTING %s------\n", name);
    list = list->next;
    while (list) {
        printf("ID %d FUEL %d\n", list->id, list->fuel);
        list = list->next;
    }
}

struct list_arrival *create_arrival_list(){
    struct list_arrival *header;
    //verificar se a lista foi criada ou nao
    if((header = (struct list_arrival *) malloc(sizeof(struct list_arrival))) == NULL){
        printf("ERRO ao criar a lista de voos arrival.\n");
        exit(0);
        //nao sei se queres que o programa acabe ou se queres que isto volte a tentar criar a lista
        return NULL;
    }
    else {
        header->priority = -1;
        header->eta = -1;
        header->fuel = -1;
        header->shared_memory_index = -1;
        header->next = NULL;
    }
    return header;
}

struct list_arrival *create_node_arrival(struct CT_info *information){
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
        printf("Error creating node.\n");
        exit(0);
        //nao sei se queres que o programa acabe ou se queres que isto volte a tentar criar a lista
        return NULL;
    }
    //verifica se o voo e prioritario
    if(information -> fuel == information -> time_to_track + landing_time + 4){
        priority = 1;
    }

    node -> priority = priority;
    node -> eta = information -> time_to_track;
    node -> fuel = information -> fuel;
    node -> shared_memory_index = information -> pos;
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


    struct list_arrival * ant, *atual;

    if(header != NULL){//Verify if the list exists
        ant = header;
        atual = header -> next;

        while(atual != NULL && node -> priority <= atual -> priority && node -> eta > atual -> eta){//While the priority is equal or lower than the current priority and the node's ETA is greater than the node in the list

            ant = atual;
            atual = atual -> next;

        }

        node -> next = atual;
        ant -> next = node;
    }
    else{
        printf("A lista de arrivals passada nao existe!!!!!!!\n");//nao sei se queres que de erro, ou se queres levar isto em consideracao, feel free para apagar
    }

}

void pop_arrival(struct list_arrival *header, struct list_arrival *node){
    /* Remover a node element of the list
     * Parameters
     *      Header - Head of the list to be verified
     *      Node - node to be popped
     *
*/
    struct list_arrival *last, *current;

    if(header != NULL && header -> next != NULL){
        last = header;
        current = header->next;

        while(current != NULL && current != node){//TODO might not work
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
}

void remove_arrival(struct list_arrival *header, struct list_arrival *nodo){
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

        while(current != NULL && current != nodo){//TODO Verify here 2
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

void choose_flights_to_hold(struct list_arrival *header){
    /* This function takes arrivals list and chooses the flight to hold, informs then and reorders the array
     * Parameters:
     *      Header - head of the arrival list;
     *
     * */
    struct list_arrival *list;
    struct list_arrival *temp; //serve para eu guardar o pointer para o nodo que esta a ser ordenado
    int i, time_to_hold;

    srand(getpid()); // Randomizer

    if(header != NULL){

        if((list = header -> next) != NULL){// First 5 five flights do not need to hold, therefore they are skipped
            for (i = 0; i < 5  && list != NULL; i++){
                list = list -> next;
            }

            while(list != NULL && list -> eta == 0){
                time_to_hold = rand()%(max_hold - min_hold) + min_hold;//Randomize holding times

                if( (list -> fuel - landing_time) >= time_to_hold){ //Verify if hold is possible
                    list -> eta = time_to_hold;
                    //Reorder array
                    temp = list;
                    list = list -> next; //passo logo para o nodo seguinte, porque se ficasse no mesmo, o proximo nodo nao seria o correto, visto que vai mudar de posicao na lista
                    airport -> max_flights[temp -> shared_memory_index] = 7;//Notify the thread that it need to hold
                    airport->total_holding_man++;
                    airport->total_time_landing += time_to_hold;
                    if(list->priority == 1) airport->total_emergency_holding_man = 0;
                    pthread_cond_broadcast(&command_var);//notifica a thread para esta ver a mensagem

                    pop_arrival(header, temp);//Removes node from the list
                    add_arrival(header, temp);//adiciona o nodo na posicao certa da lista

                }
                else{
                    //If holding is not possible, Redirect flight
                    temp = list;//guardo o nodo para poder remove-lo da lista
                    airport -> max_flights[temp -> shared_memory_index] = 8;
                    pthread_cond_broadcast(&command_var);
                    airport->redirected_flights++;
                    list = list -> next;//passa para o proximo nodo
                    pop_arrival(header, temp);//Pop node from the list
                }
            }

        }
        else{
            printf("Error past list does not have elements\n");
        }
    }
    else{
        printf("Error, past list does not exist\n");
        exit(0);
    }
}


struct list_departure* create_node_departure(struct CT_info *information){
    /*Strips info from the struct received by the message queue and create a departure type item
     * Parameters
     *      information - Struct with the flight info, from the message queue
     *
     *
     * */

    struct list_departure *node;

    node = (struct list_departure *) malloc(sizeof(struct list_departure));

    node -> takeoff = information -> time_to_track;
    node -> shared_memory_index = information -> pos;
    node -> next = NULL;

    return node;
}


void add_departure(struct list_departure *header, struct list_departure *node){
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
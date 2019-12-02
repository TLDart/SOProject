#include "ControlTower.h"


void control_tower() {
    /* Handles Flight Threads, shared memory communication, and Message Queue Messaging
     *
     */
    //signal(SIGINT, SIG_IGN); /*Ignore SIGINT*/
    signal(SIGUSR1, showStatistics);   /*Handle Signals*/
    printf(("PID %d\n"), getpid()); // TODO CORRECT RUNNING CONDITION

    arrival_list = create_arrival_list();
    departure_list = create_departure_list();
    pthread_t msg_reader, dec_fuel;
    puts("CONTROL TOWER CREATED");
    // Insert Control Tower Code
    pthread_create(&msg_reader, NULL, get_messages, NULL);
    pthread_create(&dec_fuel, NULL, decrement_eta,arrival_list);
    choose_flights_to_work(arrival_list,departure_list);
    //sleep(10);
    //pthread_join(msg_reader, NULL);
    //pthread_join(dec_fuel, NULL);
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

void add_ct_info(struct CT_info* node, struct CT_info* head) {
    /* Adds message_array type element to the  end of the list
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
                add_arrival(arrival_list, create_node_arrival(element));
                print_arrivals();
                if (showVerbose == 1) printf("ADDED TO ARRIVAL LIST\n");
            } else if (aux.mode == 0) {//Departure type flight

                add_departure(departure_list, create_node_departure(element));
                print_departures();
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
        printf("A lista de arrivals passada nao existe!!!!!!!\n");
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
                    if(list->priority == 1) airport->total_emergency_holding_man++;
                    pthread_cond_broadcast(&airport->command_var);//notifica a thread para esta ver a mensagem

                    pop_arrival(header, temp);//Removes node from the list
                    add_arrival(header, temp);//adiciona o nodo na posicao certa da lista

                }
                else{
                    //If holding is not possible, Redirect flight
                    temp = list;//guardo o nodo para poder remove-lo da lista
                    airport -> max_flights[temp -> shared_memory_index] = 8;
                    pthread_cond_broadcast(&airport-> command_var);
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

struct list_departure* create_departure_list(){
    struct list_departure *header;

    if((header = (struct list_departure *) malloc(sizeof(struct list_departure))) == NULL){
        printf("Erro ao criar a lista de voos departure.\n");
        //nao sei se queres que o programa acabe ou tente criar outra vez a lista
        return NULL;
    }

    header -> takeoff = -1;
    header -> shared_memory_index = -1;
    header -> next = NULL;

    return header;
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

void choose_flights_to_work(struct list_arrival *header_arrival, struct list_departure *header_departure){

    struct wt time_to_process;//e o eta ou o takeoff, e o tempo que precisa de ser processado para o timedwait
    struct timespec time_for_timedwait;

    struct list_arrival *arrival;
    struct list_departure *departure;




    int aux = 0;
    int temp = 0;
    int counter = 0; //simula a variavel global que quero colocar:
    printf("STARTED CHOOSING FLIGHTS\n");
    //colocar um mutex nesta variavel porque quando estou a colocar outro voo tenho de o fazer sem esta estar a ser lida pela control tower
    if(header_arrival == NULL && header_departure == NULL){
        printf("Arrival and departure list were nor created with success\n");
    }


    //colocar algo que pare a thread se nao houverem voos para executar

    while(header_arrival -> next != NULL || header_departure -> next != NULL || running == 1){ //condition e a tal variavel que e alterada para dizer a control tower que o programa vai acabar

        counter = 0;
        //muda o flight type para arrival
        //Assim a thread so e perturbada se o voo que chegar a control tower for arrival



        pthread_mutex_lock(&flight_type_mutex);
        while(header_arrival -> next != NULL && (aux < 2 || temp == ETIMEDOUT) ){//verifica se ocorreu o time out, se ocorreu a vez dos arrivals passou, tem de esperar pela proxima vez
            //se o aux cheagar a 2, quer dizer que foram executados dois arrivals e chegou a vez dos departures

            arrival = header_arrival -> next;//seleciona o voo a executar
            if(arrival != NULL && arrival -> eta == 0){

                if((arrival -> next != NULL && arrival -> next -> eta != 0) || arrival -> next == NULL){//executa apenas um da lista de arrivals
                    counter += 1;

                    time_to_process = convert_to_wait(landing_time + landing_delta, time_unit);
                    time_for_timedwait = timedwait_time(time_to_process);

                    //manda o voo aterrar
                    if(aux == 0){
                        airport -> max_flights[arrival -> shared_memory_index] = 5;//diz qual e a pista a utilizar pelo voo
                    }
                    else if(aux == 1){
                        airport -> max_flights[arrival -> shared_memory_index] = 6;
                    }

                    pthread_cond_broadcast(&airport->command_var);

                    aux ++;
                    //retira o voo do array
                    remove_arrival(header_arrival, arrival);

                    nanosleep(&time_for_timedwait,NULL);
                    //temp = pthread_cond_timedwait(&flight_type_var, &flight_type_mutex, &time_for_timedwait);
                }
                else if(arrival -> next != NULL && arrival -> next -> eta == 0){//executa dois da lista de arrivals
                    counter = 2;

                    time_to_process = convert_to_wait(landing_time + landing_delta, time_unit);
                    time_for_timedwait = timedwait_time(time_to_process);

                    //manda o voo aterrar

                    airport -> max_flights[arrival -> shared_memory_index] = 5;//diz qual e a pista a utilizar pelo voo

                    airport -> max_flights[arrival -> next -> shared_memory_index] = 6;

                    pthread_cond_broadcast(&airport->command_var);
                    aux = 2;
                    //retira o voo do array
                    remove_arrival(header_arrival, arrival -> next);
                    remove_arrival(header_arrival, arrival);

                    nanosleep(&time_for_timedwait,NULL);
                    //temp = pthread_cond_timedwait(&flight_type_var, &flight_type_mutex, &time_for_timedwait);
                }

            }
            else{
                aux = 2;//se o eta nao for igual a 0 quero que passe para ir ver se pode fazer alguma departure
            }

        }
        pthread_mutex_unlock(&flight_type_mutex);


        aux = 0; //reset da variavel aux para poder ser usada com o mesmo proposito nos departure
        temp = 0; //reset da variavel temp
        counter = 0;

        //muda o flight type para departure
        //Assim a thread so e perturbada se o voo que chegar a control tower for departure


        pthread_mutex_lock(&flight_type_mutex);
        while(header_departure -> next != NULL && (aux < 2 || temp == ETIMEDOUT)){

            departure = header_departure -> next;

            if(departure != NULL && compare_time(begin, convert_to_wait(departure -> takeoff, time_unit)) == 1 ){


                if((departure -> next != NULL && compare_time(begin, convert_to_wait(departure -> next -> takeoff, time_unit)) == -1) || departure -> next == NULL){
                    counter += 1;

                    //avisa a thread que pode aterrar
                    if(aux == 0){
                        airport -> max_flights[departure -> shared_memory_index] = 2;
                    }
                    else if(aux == 1){
                        airport -> max_flights[departure -> shared_memory_index] = 3;
                    }

                    pthread_cond_broadcast(&airport->command_var);

                    aux ++;

                    //retirar o voo do array
                    remove_departure(header_departure, departure);

                    time_to_process = convert_to_wait(takeoff_time + takeoff_delta, time_unit);
                    //time_for_timedwait = timedwait_time(time_to_process);
                    time_for_timedwait.tv_sec = time_to_process.secs;
                    time_for_timedwait.tv_nsec = time_to_process.nsecs;

                    //usleep(takeoff_time + takeoff_delta))
                    //sleep(10);
                    //nanosleep(&time_for_timedwait,NULL);
                    //pthread_cond_timedwait(&flight_type_var, &flight_type_mutex, &time_for_timedwait);

                }
                else if(departure -> next != NULL && compare_time(begin, convert_to_wait(departure -> next -> takeoff, time_unit)) == 1){
                    counter = 2;

                    airport -> max_flights[departure -> shared_memory_index] = 2;
                    airport -> max_flights[departure -> next -> shared_memory_index] = 3;

                    pthread_cond_broadcast(&airport->command_var);

                    aux = 2;

                    //retirar os dois voos do array
                    remove_departure(header_departure, departure -> next);
                    remove_departure(header_departure, departure);

                    time_to_process = convert_to_wait(takeoff_time + takeoff_delta, time_unit);
                    //time_for_timedwait = timedwait_time(time_to_process);

                    time_for_timedwait.tv_sec = time_to_process.secs;
                    time_for_timedwait.tv_nsec = time_to_process.nsecs;
                    //sleep(10);
                    nanosleep(&time_for_timedwait,NULL);
                    //pthread_cond_timedwait(&flight_type_var, &flight_type_mutex, &time_for_timedwait);//nao sei se e a melhor approach de fazer a thread esperar, sleep aqui tambem nao ficava mal
                }
            }
            else{
                aux = 2;//passa a frente as departures porque nao ha nenhuma que possa ser executada
            }

        }
        pthread_mutex_unlock(&flight_type_mutex);
        //reset as variaveis
        aux = 0;
        temp = 0;
        counter = 0;

    }
}

//funcao que decrementa o eta dos arrivals
void * decrement_eta(void* arg){
    int time_unit_in_ns = time_unit * 1000;//time_unit in ms to time_unit in ns for the usleep function
    struct list_arrival *arrival;
    struct list_arrival *header_arrival = (struct list_arrival *) arg;

    //printf("STARTED DECREMENT\n");
    if(header_arrival != NULL){
        //decrementa o ETA
        //puts("NOT_NULL");
        while(header_arrival -> next != NULL || running == 1){
            if(header_arrival -> next != NULL){
                arrival = header_arrival -> next;

                while(arrival != NULL){
                    if(arrival -> eta > 0){
                        arrival -> eta --;
                        arrival ->fuel--;
                        //puts("DECREMENTED SUCCESSFULLY");
                    }
                    arrival = arrival->next;
                }
                choose_flights_to_hold(header_arrival);

                if(header_arrival -> next -> eta == 0){
                    //se eu tiver tempo de implementar tudo como deve ser, aqui vai ficar um broadcast para uma variavel de condicao, para avisar que está um voo pronto para ser executado
                }
            }
            usleep(time_unit_in_ns);//espera uma time_unit para decrementar os eta's
        }
    }
    pthread_exit(NULL);

}

//retorna o momento absoluto ate quando a thread deve esperar (struct timespec)
//recebe o landing time e intervalo de landing ou o takeoff time e o intervalo de takeoff em sec + nsec
struct timespec timedwait_time(struct wt time_given){
    struct timespec now_time;
    struct timespec time_to_wait;
    clock_gettime(CLOCK_REALTIME, &now_time);

    time_to_wait.tv_sec = now_time.tv_sec;
    time_to_wait.tv_nsec = now_time.tv_nsec;

    if(now_time.tv_nsec + time_given.nsecs > 1000000000 ){
        time_to_wait.tv_sec += 1;
        time_to_wait.tv_nsec = (now_time.tv_nsec + time_given.nsecs) % 1000000000;
    }
    time_to_wait.tv_sec += time_given.secs;

    return time_to_wait;
}


//se for o tempo de executar o departure retorn a 1, caso contrario -1
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

    if(now.tv_sec >= temp.tv_sec && now.tv_nsec >= temp.tv_nsec){
        return 1;
    }
    else{
        return -1;
    }
}


void print_arrivals(){
    struct list_arrival *element = arrival_list;
    for(int i = 0; element != NULL; i++){
        printf("NUMBER %d SHARED MEMORY INDEX %d \n", i, element->shared_memory_index);
        element = element->next;
    }
    puts("--------------------");
}

void print_departures(){
    struct list_departure *element = departure_list;
    for(int i = 0; element != NULL; i++){
        printf("NUMBER %d SHARED MEMORY INDEX %d \n", i, element->shared_memory_index);
        element = element->next;
    }
    puts("--------------------");


};


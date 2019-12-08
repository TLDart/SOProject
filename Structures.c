/*
 * Duarte Dias 2018293526
 * Gabriel Fernandes 2018288117
 */
#include "Structures.h"

p_node create_list() {
    /* Creates a linked list with header
     *
     * Returns:
     *      Return an element of type p_node with default elements and NULL values, this will be the head
     */
    p_node list = (p_node) malloc(sizeof(struct node));
    list->mode = NULL;
    list->flight_code = NULL;
    list->init = 0;
    list->eta = 0;
    list->takeoff = 0;
    list->fuel = 0;
    list->next = NULL;

    return list;
}

void add_flight(p_node node, p_node lhead) {
    /* A p_node type element to the list according to init time,meaning that the lowest element will have position 2 (position 1 is the head of the list
     *
     * Parameters:
     *      node - p_node type element that we want to add to our list
     *      head - specifies the head of a p_node type list
     */
    int i = 0;
    p_node temp = lhead;
    p_node ant, actual;

    ant = temp;
    actual = temp->next; //passa o header da lista a frente
    node->next = NULL;

    for(i = 0; actual != NULL && node->init > actual->init; i++) {
        ant = actual;
        actual = actual->next;
    }

    ant->next = node;
    node->next = actual;

    if((i == 0) || i - 1 == list_element ) {
        pthread_cond_signal(&time_var);
        if (showVerbose ==1) printf("%s SIGNALING%s\n", WHITE,RESET);
    }

}


p_node pop_flight(p_node lhead) {
    /*Removes the first element of the list and returns it (without freeing)
     *
     * Parameters:
     *      head - head of the p_node function
     *
     * Returns:
     *      Return the element just removed
     */
    p_node res = NULL;
    if (lhead->next != NULL) {
        res = lhead->next;
        lhead->next = lhead->next->next;
    }
    return res;
}

void print_list(p_node lhead) {
    /* Loops through every element and print a string of information containg the mode and init
     * Parameters:
     *      head - Head of the p_node list;
     *
     */
    p_node temp = lhead -> next;
    while (temp != NULL) {
        //print_node(head->next);
        printf("MODE:[%s] INIT: [%d]\n", temp->mode, temp->init);
        temp = temp->next;
    }
}

void print_node(p_node node) {
    /* Prints a single p_node element info
     *
     * Parameters:
     *      node - node element of type p_node to be printed;
     */

    puts("----------------------------------");
    printf("[MODE] : %s\n"
           "[FLIGHT CODE] : %s\n"
           "[INIT] : %d\n", node->mode, node->flight_code, node->init);
    if (strcmp("DEPARTURE", node->mode) == 0) {
        printf("[TAKEOFF]: %d\n", node->takeoff);
    } else {
        printf("[ETA]: %d\n"
               "[FUEL] : %d\n", node->eta, node->fuel);
    }
    puts("----------------------------------");
}

struct wt convert_to_wait(int time, int time_unit){
    /*Takes a time_unit value and converts to wait value of seconds + nano seconds. Example: time_unit: 0.5, time = 3, 1 sec, 500000000 nano secs
     * Parameters
     *      time - Current time in time units
     *      time_unit - Time unit used (must be in milliseconds)
     *
     * */
    struct wt wt;
    wt.secs = (int) (floor(time* (1.0)*(1.0 *time_unit/1000)));
    wt.nsecs = (time* (1.0)*(1.0 *time_unit/1000) - floor(time* (1.0)*(1.0 *time_unit/1000))) * 1000000000;
    //printf(" WAIT in secs%d, %d nanosecs", wt.secs, wt.nsecs);

    return wt;
}

int now_in_tm(struct timespec begin,int time_unit){
    /*Converts the current time into time units.
     * Parameters
     *      Begin - struct that contains the reference to when the program began
     *      Time_unit - Time unit used (must be in milliseconds)
     * */
    struct timespec now;
    clock_gettime(CLOCK_REALTIME,&now);
    int secs = ((now.tv_sec - begin.tv_sec) /( 1.0 * time_unit/1000));
    int nsecs = ((now.tv_nsec * 1.0 - begin.tv_nsec * 1.0)/(time_unit*1000000));
    return secs + nsecs;
};
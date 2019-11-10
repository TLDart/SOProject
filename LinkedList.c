
#include "LinkedList.h"

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

void add_flight(p_node node, p_node head) {
    /* A p_node type element to the list according to init time,meaning that the lowest element will have position 2 (position 1 is the head of the list
     *
     * Parameters:
     *      node - p_node type element that we want to add to our list
     *      head - specifies the head of a p_node type list
     */
    p_node temp = head;
    p_node ant, actual;

    ant = temp;
    actual = temp->next; //passa o header da lista a frente
    node->next = NULL;

        while (actual != NULL && node->init > actual->init) {
            ant = actual;
            actual = actual->next;
        }

    ant->next = node;
    node->next = actual;
}


p_node pop_flight(p_node head) {
    /*Removes the first element of the list and returns it (without freeing)
     *
     * Parameters:
     *      head - head of the p_node function
     *
     * Returns:
     *      Return the element just removed
     */
    p_node res = NULL;
    if (head->next != NULL) {
        res = head->next;
        head->next = head->next->next;
    }
    return res;
}

void print_list(p_node head) {
    /* Loops through every element and print a string of information containg the mode and init
     * Parameters:
     *      head - Head of the p_node list;
     *
     *
     */
    p_node temp = head -> next;
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
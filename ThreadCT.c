
#include "ThreadCT.h"
#include <stdio.h>
struct message_array* create_msg_array(){
    /* Creates a linked list with header
     *
     * Returns:
     *      Return an element of type message_arry with default elements and NULL values, this will be the head
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
        pthread_cond_wait(&time_var, NULL);
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
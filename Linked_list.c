
#include "Linked_list.h"

p_node create_list() {
    //cria uma lista com header de nodos do tipo node
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

void add_flight(p_node node, p_node list) { //passa-se um nodo da struct node como argumento

    //adiciona um nodo do tipo node a uma lista do tipo node previamente criada, ordena a lista por ordem descrescente de init

    p_node temp = list;
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


p_node pop_flight(p_node list) {
//Remove o primeiro nodo da lista e retorna o endereco desse nodo

    p_node res = NULL;
    if (list->next != NULL) {
        res = list->next;
        list->next = list->next->next;
    }
    return res;
}

void print_list(p_node head) {
    p_node temp = head -> next;
    while (temp != NULL) {
        //print_node(head->next);
       printf("MODE:[%s] INIT: [%d]\n", temp->mode, temp->init);
        temp = temp->next;
    }
}

void print_node(p_node node) {
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
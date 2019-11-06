#include <stdlib.h>
#include <stdio.h>

typedef struct node* p_node;

struct node{
  char *mode;
  char *flight_code;
  int init;
  int takeoff;
  int fuel;
  p_node next;
};


typedef struct{
  long msgtype;
  p_node info;
}message;


//cria uma lista com header de nodos do tipo node
p_node create_list(){
  p_node *list = (p_node *) malloc(sizeof(node));

  list -> mode = NULL;
  list -> flight_code = NULL;
  list -> init = 0;
  list -> takeoff = 0;
  list -> fuel = 0;
  list -> p_node = NULL;

  return list;
}


void add_flight(p_node node, p_node list){ //passa-se um nodo da struct node como argumento

  //adiciona um nodo do tipo node a uma lista do tipo node previamente criada, ordena a lista por ordem descrescente de init

  p_node temp = list;
  p_node ant, actual;

  ant = temp;
  actual = temp -> next; //passa o header da lista a frente
  node -> next = NULL;


  while(actual -> next != NULL || node -> init > actual -> init){
    ant = actual;
    actual = actual -> next;
  }

  ant -> next = node;
  node -> next = actual;
}


p_node pop_flight(p_node list){
//Remove o primeiro nodo da lista e retorna o endereco desse nodo

  p_node res = NULL;
  if(list -> next != NULL){
    res = list -> next;
    list -> next = list -> next -> next;
  }
  return res;
}

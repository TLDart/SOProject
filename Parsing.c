#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SIZE 200

typedef struct node *p_node;

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


void erro(char *string){
    //perror(string);
    printf("%s\n", string);
    //exit(-1);
}

int n_palavras(char *string){
    char *token, *temp;
    char del[2] = " ";
    int res = 0;
    temp = (char *) malloc(sizeof(char) * (strlen(string) + 1));
    strcpy(temp, string);


    token = strtok(temp, del);
    while(token != NULL){
        token = strtok(NULL, del);
        res++;
    }
    return res;
}


p_node parsing(char *string){
    char *temp, *token, *aux;
    char del [2] = " ";
    int i = 0;
    int e,j, numero_palavras;
    char **vector = (char **) malloc(sizeof(char *) * 8);
    int flag = 0; //flag para verificar se o comando e aceite

    p_node nodo = (p_node) malloc(sizeof(struct node));//nodo que vai ser retornado caso o comando seja aceite

    temp = (char *) malloc(sizeof(char) * strlen(string) + 1); //ver se preciso de alocar para o '\0'
    strcpy(temp, string);

    //colocar aqui a funcao para contar as palavras e depois voltar a colocar a a string no temp
    numero_palavras = n_palavras(temp);
    if(numero_palavras == 6 || numero_palavras == 8) {


        //copia o mode para a primeira posicao do array de strings e aloca a memoria para essa string dinamicamente
        token = strtok(temp, del);
        aux = (char *) malloc(sizeof(char) * strlen(token) + 1);
        strcpy(aux, token);
        vector[0] = aux;

        if (strcmp(aux, "DEPARTURE") == 0) {
            //codigo para a DEPARTURE
            //aloca memoria dinamicamente para cada token e coloca essa string no local correto do array de strings
            for (i = 1; token != NULL; i++) {
                token = strtok(NULL, del);
                if (token != NULL) {
                    aux = (char *) malloc(sizeof(char) * (strlen(token) + 1));
                    strcpy(aux, token);
                    vector[i] = aux;
                }
            }
            //coloca na variavel aux a string de controlo devolvida por
            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "DEPARTURE %s init: %s takeoff: %s", vector[1], vector[3], vector[5]);
            //printf("%s\n", aux);
            if (strcmp(aux, string) != 0) {

                flag = 1;
            }

            //verifica o flight_code
            temp = vector[1];
            if (temp[0] != 'T') {
                flag = 1;
            }
            if (temp[1] != 'P') {
                flag = 1;
            }


            for (e = 2; e < strlen(temp); e++) {
                //printf("%c\n", temp[e]);
                if ((temp[e]) > 57 || (temp[e]) < 48) {
                    flag = 1;
                }
            }
            //verifica o tempo inicial e o takeoff
            for (j = 3; j <= 5; j += 2) {
                temp = vector[j];
                for (e = 0; e < strlen(temp); e++) {
                    if (temp[e] > 57 || temp[e] < 48) {
                        flag = 1;
                    }
                }
            }


            if (flag == 0) {
                nodo->mode = vector[0];
                nodo->flight_code = vector[1];
                nodo->init = atoi(vector[3]);
                nodo->takeoff = atoi(vector[5]);
                nodo->fuel = -1;
                nodo->next = NULL;
            } else {
                erro("O comando que introduziu nao satisfaz a estrutura dos comandos.\n");
                return NULL;
            }
            free(vector);
        } else if (strcmp(token, "ARRIVAL") == 0) {
            //codgigo para a ARRIVAL
            for (i = 1; token != NULL; i++) {
                token = strtok(NULL, del);
                if (token != NULL) {
                    aux = (char *) malloc(sizeof(char) * strlen(token) + 1);
                    strcpy(aux, token);
                    vector[i] = aux;
                }
            }

            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "ARRIVAL %s init: %s eta: %s fuel: %s", vector[1], vector[3], vector[5], vector[7]);
            if (strcmp(aux, string) != 0) {
                flag = 1;
            }

            //verifica o flight_code
            temp = vector[1];
            if (temp[0] != 'T') {
                flag = 1;
            }
            if (temp[1] != 'P') {
                flag = 1;
            }
            for (e = 2; e < strlen(temp); e++) {
                if ((temp[e]) > 57 || (temp[e]) < 48) {
                    flag = 1;
                }
            }
            //verifica o tempo inicial e o takeoff
            for (j = 3; j <= 7; j += 2) {
                temp = vector[j];
                for (e = 0; e < strlen(temp); e++) {
                    if (temp[e] > 57 || temp[e] < 48) {
                        flag = 1;
                    }
                }
            }
            if (flag == 0) {
                nodo->mode = vector[0];
                nodo->flight_code = vector[1];
                nodo->init = atoi(vector[3]);
                nodo->takeoff = atoi(vector[5]);
                nodo->fuel = atoi(vector[7]);
                nodo->next = NULL;
            } else {
                erro("O comando que introduziu nao satisfaz a estrutura dos comandos.\n");
                return NULL;
            }
            free(vector);
        } else {
            free(nodo);
            free(vector);
            erro("O comando que introduziu nao satisfaz a estrutura dos comandos.\n");
            return NULL;
        }
        return nodo;
    }
    else{
        free(vector);
        free(nodo);
        erro("O comando que introduziu nao satisfaz a estrutura dos comandos.\n");
        return NULL;
    }
}





int main(){
    p_node nodo = parsing("DEPARTURE TP440 init: 0 takeoff: 100");
    printf("%s, %s, %d, %d, %d\n", nodo -> mode, nodo -> flight_code, nodo -> init, nodo -> takeoff, nodo -> fuel);
    nodo = parsing("ARRIVAL TP437 init: 0 eta: 100 fuel: 1000");
    printf("%s, %s, %d, %d, %d\n", nodo -> mode, nodo -> flight_code, nodo -> init, nodo -> takeoff, nodo -> fuel);
    nodo = parsing("DEPARTURE TP440 init: 10 takeoff: 100");
    printf("%s, %s, %d, %d, %d\n", nodo -> mode, nodo -> flight_code, nodo -> init, nodo -> takeoff, nodo -> fuel);
    return 0;
}

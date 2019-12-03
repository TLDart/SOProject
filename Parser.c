#include <math.h>
#include "Parser.h"
extern pthread_mutex_t mutex_write, mutex_time;
extern shared_mem* airport;

int n_palavras(char *string){
    /* Counts the number of words in a string
     *
     * Parameters:
     *      string - char* type to be counted;
     *
     * Returns:
     *      Returns the number of words in a string
     */
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
    /* transforms the given string into a p_node type element
     *
     * Parameters:
     *      string - char* type that will serve as the message to be parsed
     *
     * Returns:
     *    p_node type element if string is valid, according to the function
     *    NULL in case of invalid parsing
     */
    char *temp, *token, *aux, msgwrong[128],msgright[128], msgtimeout[128];
    char del [2] = " ";
    int i = 0;
    int e,j, numero_palavras;
    char **vector = (char **) malloc(sizeof(char *) * 8);
    int flag = 0; //flag para verificar se o comando e aceite

    sprintf(msgwrong,"WRONG COMMAND [PARSING ERROR] => %s", string);
    sprintf(msgright,"NEW COMMAND => %s", string);
    sprintf(msgtimeout,"WRONG COMMAND [TIME IS GREATER THAN INIT] => %s", string);

    p_node nodo = (p_node) malloc(sizeof(struct node));//Node that will be return if the command gets accepted

    temp = (char *) malloc(sizeof(char) * strlen(string) + 1);
    strcpy(temp, string);

    numero_palavras = n_palavras(temp);
    if(numero_palavras == 6 || numero_palavras == 8) {

        //Copies mode to the first position of the array e allocates memory dynamically
        token = strtok(temp, del);
        aux = (char *) malloc(sizeof(char) * strlen(token) + 1);
        strcpy(aux, token);
        vector[0] = aux;

        if (strcmp(aux, "DEPARTURE") == 0) {
            //Behaviour for departure
            //Allocates memory dinamically for each token and puts that string correctly in the array of strings.
            for (i = 1; token != NULL; i++) {
                token = strtok(NULL, del);
                if (token != NULL) {
                    aux = (char *) malloc(sizeof(char) * (strlen(token) + 1));
                    strcpy(aux, token);
                    vector[i] = aux;
                }
            }

            aux = (char *) malloc(sizeof(char) * SIZE);
            sprintf(aux, "DEPARTURE %s init: %s takeoff: %s", vector[1], vector[3], vector[5]);
            //printf("%s\n", aux);
            if (strcmp(aux, string) != 0) {

                flag = 1;
            }

            //Verifies flight code
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
            //Verifies initial time and takeoff
            for (j = 3; j <= 5; j += 2) {
                temp = vector[j];
                for (e = 0; e < strlen(temp); e++) {
                    if (temp[e] > 57 || temp[e] < 48) {
                        flag = 1;
                    }
                }
            }
            if(atoi(vector[3]) > atoi(vector[5])){
                flag = 1;
            }



            if (flag == 0) {
                nodo->mode = vector[0];
                nodo->flight_code = vector[1];
                nodo->init = atoi(vector[3]);
                nodo->takeoff = atoi(vector[5]);
                nodo->eta = -1;
                nodo->fuel = -1;
                nodo->next = NULL;
            } else {
                printf("WRONG COMMAND => %s\n", string );
                write_to_log(msgwrong);
                return NULL;
            }
            free(vector);
        } else if (strcmp(token, "ARRIVAL") == 0) {
            //Behavior for Arrival code
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

            //Verifies Flight code
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
            //Verifies initial time and takeoff
            for (j = 3; j <= 7; j += 2) {
                temp = vector[j];
                for (e = 0; e < strlen(temp); e++) {
                    if (temp[e] > 57 || temp[e] < 48) {
                        flag = 1;
                    }
                }
            }

            if(atoi(vector[7]) < atoi(vector[5]) + 4 + landing_time){
                flag = 1;
            }

            if (flag == 0) {
                nodo->mode = vector[0];
                nodo->flight_code = vector[1];
                nodo->init = atoi(vector[3]);
                nodo->takeoff = -1;
                nodo->eta = atoi(vector[5]);
                nodo->fuel = atoi(vector[7]);
                nodo->next = NULL;
            } else {
                //printf("WRONG COMMAND =>%s\n", string );
                write_to_log(msgwrong);
                return NULL;
            }
            free(vector);
        } else {
            free(nodo);
            free(vector);
            //printf("WRONG COMMAND => %s\n", string );
            write_to_log(msgwrong);
            return NULL;
        }
        //printf("CURRENT TIME %lf\n", gettime());
        if(nodo->init < now_in_tm(begin, time_unit)){
            //printf("WRONG COMMAND [TIME IS GREATER THAN INIT] => %s\n", string );
            write_to_log(msgtimeout);
            return NULL;
        }
        //printf("NEW COMMAND => %s\n", string );
        write_to_log(msgright);
        return nodo;
    }
    else{
        free(vector);
        free(nodo);
        //printf("WRONG COMMAND => %s\n", string );
        write_to_log(msgwrong);
        return NULL;
    }
}



/* DEBUG
int main(){
    p_node node = parsing("DEPARTURE TP440 init: 0 takeoff: 100");
    printf("%s, %s, %d, %d, %d\n", node -> mode, node -> flight_code, node -> init, node -> takeoff, node -> fuel);
    node = parsing("ARRIVAL TP437 init: 0 eta: 100 fuel: 1000");
    printf("%s, %s, %d, %d, %d\n", node -> mode, node -> flight_code, node -> init, node -> eta, node -> fuel);
    node = parsing("DEPARTURE TP440 init: 10 takeoff: 100");
    printf("%s, %s, %d, %d, %d\n", node -> mode, node -> flight_code, node -> init, node -> takeoff, node -> fuel);
    return 0;
}*/

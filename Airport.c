// Compile by using ./filename <configPath>
#include "Airport.h"

int time_unit, timer;
int takeoff_time,takeoff_delta,landing_time,landing_delta, min_hold, max_hold;
int max_takeoffs, max_landings;
int shmid;
shared_mem* airport;
int fd;
int mq_id;
p_node head;
int ids; //Our very own thread unique identifier

pthread_cond_t time_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_time = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_write = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv){
    simulation_manager(argv[1]);
}

void simulation_manager(char* config_path) {
    //Handle signals
    pthread_t time_thread, pipe_reader, flight_creator;
    signal(SIGUSR1,showStats);
    signal(SIGINT, terminate);

    clean_log();

    write_to_log("PROGRAMS STARTS");

    //Load initial config
    if(load_config(config_path) < 0){
        //Loads initial simulation config
        perror("Could not load config, exiting");
	exit(-1);
    }
    //printf("%d",getpid());

    head = create_list();
    //Create Shared Memory Volume
    shmid = shmget(IPC_PRIVATE, sizeof(shared_mem),IPC_CREAT|0777);
    if (shmid < 0){
        perror("Failed to Create shared memory block");
        exit(1);
    }
    airport = (shared_mem*) shmat(shmid, NULL,0);
    
    //Create MSQ
	if((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1){
    	perror("Failed to create message queue\n");
	exit(-1);
  }

   //Create Control Tower
        if(fork() == 0){
        control_tower(); 
        exit(0);
	}

    //Create named pipe
    unlink(PIPE_NAME);
    mkfifo(PIPE_NAME,O_CREAT|O_EXCL|0666);
    if ((fd = open(PIPE_NAME, O_RDWR)) < 0) perror("Pipe Error");

    //Create Flight_Creator_Thread
    pthread_create(&flight_creator,NULL, create_flights, head);

    //Create Time_Thread
	pthread_create(&time_thread, NULL,time_counter,airport);

	//Create Pipe_reader Thread
	pthread_create(&pipe_reader, NULL,get_message_from_pipe,&fd);

	while(1){};

}

int load_config(char* path) {
    //Load config from PATH, returns 1 if successful, -1 if not successful
    int counter = 0;
    FILE *fp;

    if ((fp = fopen(path, "r")) == NULL) return -1;
    char buffer[BUFFER_SIZE], *token;
    const char delimiter[2] = ",";
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; //Removes the trailing ‘\n’
	//printf("%s \n", buffer);
      if (counter == 0) {
            time_unit = atoi(buffer);
        }
        if (counter == 1) {
            token = strtok(buffer,delimiter);
	    takeoff_time = atoi(token);
            token = strtok(NULL,delimiter);
            takeoff_delta = atoi(token);
        }
        if (counter == 2) {
            token = strtok(buffer,delimiter);
            landing_time = atoi(token);
            token = strtok(NULL,delimiter);
            landing_delta = atoi(token);

        }
        if (counter == 3) {
            token = strtok(buffer,delimiter);
            min_hold = atoi(token);
            token = strtok(NULL,delimiter);
            max_hold = atoi(token);

        }
        if (counter == 4) {
            max_takeoffs = atoi(buffer);

        }
        if(counter == 5){
            max_landings = atoi(buffer);
        }
	memset(buffer, 0, strlen(buffer));//Resets the buffer
        counter++;
    }
    return 1;
}

void showStats(int signum){
	//Shows user stats
    signal(SIGUSR1, showStats);
    //We will need a mutex here
    printf("Timer %d", airport->time);
    printf("Total number of flights : %d\n", airport->total_flights);
    printf("Total flights that Landed: %d\n", airport->total_landed);
    printf("Estimated wait time : %lf\n", airport->avg_ETA);
    printf("Total Flights that TookOff: %d\n", airport->total_takeoff);
    printf("Average TakeOff Time : %lf\n", airport->avg_takeoff);
    printf("Average number of holding maneuvers per landing : %lf\n", airport->avg_man_holding);
    printf("Average number of maneuvers per Emergency Time : %lf\n", airport->avg_man_emergency);
    printf("Total redirected flights : %d\n", airport->redirected_flights);
    printf("Total rejected flights : %d\n", airport->rejected_flights);
    //And another here

}

void* get_message_from_pipe(void* arg){
	//Reads a message from pipe into de buffer;
	puts("PIPE HANDLER THREAD CREATED");
	int fd = *((int*)arg);
	char buffer[BUFFER_SIZE];
	fd_set read_set;
	int nread;
	p_node parsed_data;

	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);
	while(1){
		if(select(fd + 1, &read_set, NULL,NULL,NULL) > 0){
			if (FD_ISSET(fd, &read_set)){
                    nread = read(fd,buffer, BUFFER_SIZE);
                    if (nread > 0){
                            buffer[nread] = '\0';
                            if(buffer[nread - 1]  == '\n')
                                buffer[nread -1] = '\0';
                            //printf("%s", buffer);
                            //Do the parsing function
                            parsed_data = parsing(buffer);
                            if(parsed_data != NULL){
                                print_node(parsed_data);
                                if(head == NULL) puts("NULLHEAD");
                                add_flight(parsed_data, head);
                                //print_list(head);
                            }
                    }
			}
		}
	}
}

void terminate(int signum){
	//Terminates the program,TODO: NEEDS TO CLEAN RESOURCES
	puts("[PROGRAM ENDING]");
    write_to_log("PROGRAM ENDING");
	exit(0);
}

void* time_counter(void* arg){
    puts("TIME-COUNTER THREAD CREATED");
	while(1){
		pthread_mutex_lock(&mutex_time);
		airport->time++;
		//printf("[TIME] :%d\n", airport->time);
		pthread_mutex_unlock(&mutex_time);
		pthread_cond_broadcast(&time_var);
		usleep(time_unit*1000);
		}
	}
void control_tower(){
    puts("CONTROL TOWER CREATED");
    // Insert Control Tower Code

}
void* create_flights(void* pointer){
    puts("FLIGHT CREATOR THREAD CRATED");
    //p_node nodo = (p_node) pointer; //typecast para p_node
    p_node list = head;
    p_node flight;

    pthread_t thread;//para colocar no pthread_create
    struct args_threads *args;//pointer para uma struct que vai ser passado aos voos com os seus respetivos argumentos


    while(1){

        flight = list -> next;
        //Parte que vai verificar se o tempo para tratar do voo ja chegou (uso de uma variavel de condicao e um mutex)
        pthread_mutex_lock(&mutex_time);
        while(flight == NULL || flight -> init > airport -> time){
            pthread_cond_wait(&time_var, &mutex_time);
            flight = list -> next;
        }
        pthread_mutex_unlock(&mutex_time);
        //#############
        args = (struct args_threads *) malloc(sizeof(struct args_threads)); //dar free deste pointer dentro da funcao departure ou arrival
        args -> id = ids;
        args -> nodo = flight;//depois tenho de dar free deste pointer

        //funcao para iniciar a thread
        if(strcmp(flight -> mode, "DEPARTURE") == 0){
            pthread_create(&thread, NULL, departure, args);
        }

        else if(strcmp(flight -> mode, "ARRIVAL") == 0){
            pthread_create(&thread, NULL, arrival, args);
        }

        else{
            //da erro e escreve no log talvez
        }

        ids++; //incrementa a variavel que da os ids para as threads, assim garantimos que e sempre diferente

        list -> next = list -> next -> next; //Passa a frente o voo que acabou de ser tratado
    }
    /*
    Fazer alguma coisa para dar handle desta thread uma vez que ja nao seja necessaria
    */
}

void* departure(void * arg){
    srand(time(NULL));
    struct args_threads* data = (struct args_threads*) arg;
    char temp[250];
    pthread_mutex_lock(&mutex_time);
    sprintf(temp,"[DEPARTURE THREAD CREATED] [FLIGHT CODE] : %s [TAKEOFF]: %d", data->nodo->flight_code, data->nodo->takeoff);
    printf("%s\n",temp);
    write_to_log(temp);
    pthread_mutex_unlock(&mutex_time);
    //Post Thread Activity
    sleep(rand()%3);
    sprintf(temp,"[THREAD DELETED] [FLIGHT CODE] %s", data->nodo->flight_code);
    printf("%s\n",temp);
    write_to_log(temp);
    // DO we need to free??
    return NULL;
}
void* arrival(void* arg){
    srand(time(NULL));
    struct args_threads* data = (struct args_threads*) arg;
    char temp[250];
    pthread_mutex_lock(&mutex_time);
    sprintf(temp,"[ARRIVAL THREAD CREATED] [FLIGHT CODE] : %s [ETA]: %d [FUEL]: %d", data->nodo->flight_code, data->nodo->eta, data->nodo->fuel);
    printf("%s\n",temp);
    write_to_log(temp);
    pthread_mutex_unlock(&mutex_time);
    //Post Thread Activity
    sleep(rand()%3);
    sprintf(temp,"[THREAD DELETED] [FLIGHT CODE] : %s",data->nodo->flight_code);
    printf("%s\n",temp);
    write_to_log(temp);
    return NULL;
}

void clean_log(){
    FILE* fp = fopen("log.txt", "w");
    fclose(fp);
}


#include "ControlTower.h"


void control_tower(){
    sem_unlink(CAN_HOLD);
    can_hold = sem_open(CAN_HOLD,O_CREAT| O_EXCL,0700,0);
    sem_unlink(CAN_SEND);

    can_hold = sem_open(CAN_SEND,O_CREAT | O_EXCL,0700,0);
    pthread_create(&messenger,NULL, get_messages, NULL);
    pthread_join(messenger,NULL);
}


void *get_messages(void *arg) {
    /*  Thread Function that process messages from the incoming message queue.
     * It both reads the message as well as it searches for an available spot in the array, and finally sends a message to the thread with that info;
    *
    */
    struct message msg_rcv;
    struct sharedmem_info msg_sent;

    while(runningCT){
        if(msgrcv(mq_id, &msg_rcv, sizeof(struct message)- sizeof(long), -MSGTYPE_EXIT,0) < 0){
            printf("There as an error sending the message");
        }
        printf("Message Received\n");
        //If the message was received
        new_message = 1;

        //Verify if the flight can't received
        if(msg_rcv.mode == 1) { //Arrival
            if(counter_arr == max_landings){//Flight rejected
                airport->rejected_flights++; //Increment stats
                msg_sent.position = -1;
            }
            else{
                pthread_mutex_lock(&flight_verifier);
                msg_sent.position = index_shm(); // Finds an available index the shared_mem
                new_message = 1;
                //sem_wait(can_send);
                //add_to_arrival(&msg_rcv);
                //sem_post(can_hold);
                new_message = 0;
                pthread_mutex_unlock(&flight_verifier);
            }

        }
        else if(msg_rcv.mode == 0){// Departure

            if(counter_dep == max_takeoffs){
                airport->rejected_flights++;
                msg_sent.position = -1;
            }
            else{
                pthread_mutex_lock(&flight_verifier);
                msg_sent.position = index_shm();
                new_message = 1;
                //sem_wait(can_send);
                //add_to_departure(&msg_rcv);
                //sem_post(can_hold);
                new_message = 0;
                pthread_mutex_unlock(&flight_verifier);

            }

        }
        //Send the message
        msg_sent.msgtype = msg_rcv.id;

            if(msg_sent.position == -1){
                printf("HERE\n");
            }
        if(msgsnd(mq_id, &msg_sent, sizeof(struct sharedmem_info)- sizeof(long), 0) < 0){
            printf("Error sending the messsage\n");//TODO MIGHT be a problem here

        }

    }
    pthread_exit(NULL);
}
int index_shm(){
    int i = 0;

    for(i = 0; i < max_landings + max_takeoffs ; i++){
        if(airport -> max_flights[i] == 0){
            airport -> max_flights[i] = 1;
            return i;//returns the index of the shared memory designated to certain flight
        }
    }
    return -1;//if an error occurs, -1 is the return value
}
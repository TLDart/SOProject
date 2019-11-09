//
// Created by TLDart on 08/11/2019.
//

#include "TheadBehaviour.h"
extern p_node head;
extern BUFFER_SIZE;

void *get_message_from_pipe(void *arg) {
    /* Reads message coming from the Named Pipe
     *
     * Parameter :
     *      arg = pointer to the pipe file descriptor
     *
     *
     *
     */
    puts("PIPE HANDLER THREAD CREATED");
    int fd = *((int *) arg);
    char buffer[BUFFER_SIZE];
    fd_set read_set;
    int nread;
    p_node parsed_data;

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);
    while (1) {
        if (select(fd + 1, &read_set, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(fd, &read_set)) {
                nread = read(fd, buffer, BUFFER_SIZE);
                if (nread > 0) {
                    buffer[nread] = '\0';
                    if (buffer[nread - 1] == '\n') //Remove trailing '\n' if necessary
                        buffer[nread - 1] = '\0';
                    parsed_data = parsing(buffer); // Handle the buffer
                    if (parsed_data != NULL) {
                        print_node(parsed_data);
                        if (head == NULL) puts("NULLHEAD"); // Verify if list lead exists
                        add_flight(parsed_data, head);
                    }
                }
            }
        }
    }
}
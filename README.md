# Operating Systems Project
    This project is based in the language C and makes uses of thread, semaphores, condition variables, shared memory, processes, etc
    This is a unix based project and made to run on UNIX system (Testing unit was a Ubuntu 18.04).

## Project Goal 
    Control and handle flights in San Francisco Airport
    The San Francisco airport contains 2 2-pair lanes in which planes land and depart.
    The Simulation Manager generates all the resources needed to the rest of the project.
    Upon command flight received (from the pipe), we will make sure that the command is inded correct and if soo it will be added to the current to create list.
    There a thread will wait for the correct init time to create the flight (which will be repesented by a Thread);
    Upon Creation a flight information the control tower of the status and the control tower responds to the flight with the slot in which the flight is going to listen to commands (Using a message queue); 
    The control Tower will then Schedule and order landing / departure the flight through that slot (using our own scheduler), making no flight are departing and landing at the same time;
    Upon decision the flight departs/lands/leaves after a delta time, deleting the thread.
    On SIGUSR1 the program prints runtime stats to stdout.
    On SIGINT the program closes the pipe waits for all flights allready instanciated to leave and then clean resources and leaves.  
    
## General usage
    To compile and run there is an included makefile
    There is also an included tester bash script that input multiple commands into the machine (written using the sh shell);
    
## Files and more
    Alongside this ReadME this project includes:
    
### Airport.c and Airport.h
    Main file that handles creating, handling and deleting other processes

### Thread.c and Thread.h
    File that contains all thread functionality of the airport, from creating flights, to counting time or even reading from the pipe

### Parsing.c and Parsing.h
    Function that handles parsing the commands from the pipe and returning a system readable struct

### LinkedList.c and LinkedList.h
    Backend file that handles some structures required for the program

### config.txt
    File that contains a number of required variables necessary to the program
    
### tester.sh
    Simples script that automatically inputs commands to the pipe
 
#### makefile
    File that generats the executable
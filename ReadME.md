# Operating Systems Project
    This project is based in the language C and it make uses of:
    * Threads
    * Semaphores
    * Condition variables
    * Shared memory
    * Processes
    * Pipes
    
   This Project is meant to work on Linux 19.04

## Lore 
    Based on the San Francisco's Airport in real life, a Simation Manager runs a system where flights are controled by a Control Tower. Each flight can be declined, received, or be holded. There is also support for emergency flights.
   
## Flow
     1. The Simulation Manager starts the program.
     In the simulation Manager:
     * Commands are read from an input *Pipe*, and proceed to be parsed, meaning they can be accepted or denied.
     * Accepted commmands are moved to a linked list where they wait to be scheduled.
     * A Thread using conditional Timed wait schedules the flights, aka creates threads, to start in the correct time delta.
     2. After a flight is created:
     * Sends a message using a *mssage queue* to the control Tower waiting for an available slot.
     3. Control Tower:
     * Receives and Responds to incoming messages from the queue
     * Schedules flights to land, hold (6th and above on the queue), or sends them wait (either in case of no fuel or if the airport is full). Supports Emergency flights.
  
### Additional notes
    * Upon a flight as landed, its respective thread is released.\
    * On SIGUSR1 the program prints runtime stats to stdout.
    * On SIGINT the program closes the pipe, waits for all flights allready instanciated to leave and then clean resources and leaves. 
    
## General usage
    To compile and run there it is included a makefile
    There is also an included tester bash script that will input multiple commands into the program (written using the sh shell);
    
## Files and more
    Alongside this ReadME this project includes:
    
### Main.c and Main.h
    Contains our Simualtion Manager, it creates every resource needed and then handles reading from the pipe and creates flights in the current eta

### ControlTower.c and ControlTower.h
    The control tower is a secondary process that control what flights do, they can either land/depart, hold, be rejected or redirected 

### Parser.c and Parser.h
    Function that handles parsing the commands from the pipe and returning a system readable struct

### Structures.c and Structures.h
    Backend file that handles some structures required for the program

### config.txt
    File that contains a number of required variables necessary to the program
    
### tester.sh
    Simples script that automatically inputs commands to the pipe
 
#### makefile
    File that generates the executable
    
## Known Bugs/Error
    * Signals (like SIGBUS/ SIGALARM where not treated not to disrupt the program and, if used, will kill the program

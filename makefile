CC = gcc
OBJS = Airport.o LinkedList.o Parsing.o Thread.o
PROG = airport
FLAGS = -Wall -O -g -pthread
MATH = -lm

################## GENERIC #################

all: ${PROG}

clean:
	rm ${OBJS} ${PROG}

${PROG}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
	${CC} ${FLAGS} $< -c -o $@

################ DEPENDECIES ###############

Airport.o: Airport.c Airport.h LinkedList.o Thread.o

LinkedList.o: LinkedList.c LinkedList.h

Parsing.o: Parsing.c Parsing.h LinkedList.h

Thread.o : Parsing.h Thread.c Thread.h

ThreadCT.o: ThreadCT.h ThreadCT.c Thread.h



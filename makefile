CC = gcc
OBJS = Airport.o Linked_list.o Parsing.o
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

################ DEPENDECIES ##############


Airport.o: Airport.c Airport.h Linked_list.o Parsing.o Parsing.h

Linked_list.o: Linked_list.c Linked_list.h

Parsing.o: Parsing.c Parsing.h Linked_list.h



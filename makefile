CC = gcc
OBJS = Main.o Parser.o Structures.o ControlTower.o
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

Main.o: Main.c Main.h Parser.o ControlTower.o

Parser.o: Parser.c Parser.h Structures.o

Structures.o: Structures.c Structures.h

ControlTower.o: ControlTower.c ControlTower.h





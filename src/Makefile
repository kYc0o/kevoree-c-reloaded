
SRC=$(shell ls *.c) 

SRC+=lib/list.c

OBJ=$(SRC:%.c=%.o)

all: $(OBJ)
	gcc -o main $(OBJ)	
	echo "done"

%.o: %.c
	gcc -o $@ -c $<

clean:
	rm -f *.o
	rm -f lib/*.o


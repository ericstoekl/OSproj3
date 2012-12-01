
SRC = pr3.c
INC = storage.h f_system.h dir_stack.h
LIB = storage.c f_system.c dir_stack.c


pr3: $(SRC) $(INC) $(LIB)
	gcc -std=c99 -Wall -Wextra -g -o pr3 $(SRC) $(INC) $(LIB) -lm

clean:
	rm pr3

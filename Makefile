
SRC = pr3.c
INC = storage.h
LIB = storage.c


pr3: $(SRC) $(INC) $(LIB)
	gcc -std=c99 -Wall -Wextra -g -o pr3 $(SRC) $(INC) $(LIB) -lm

clean:
	rm pr3

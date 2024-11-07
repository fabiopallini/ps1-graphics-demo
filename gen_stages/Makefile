EXEC = gen_stages 
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRC = main.c include/cJSON.c

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

run: $(EXEC)
	./$(EXEC)
	mv STAGES.BIN ../data/stages/

clean:
	rm -f $(EXEC)

# all clean and run are not file names
.PHONY: all clean run

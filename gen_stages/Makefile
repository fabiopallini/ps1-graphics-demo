EXEC = gen_stages$(EXE)
EXE =
ifeq ($(OS),Windows_NT)
    EXE = .exe
    MOVE = move
else
    MOVE = mv
endif

CC = gcc
CFLAGS = -m32 -Wall -Wextra -g -Iinclude -I../src
SRC = main.c include/cJSON.c

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

run: $(EXEC)
	./$(EXEC)
	$(MOVE) STAGES.BIN ../data/stages/
	$(MOVE) stages.h ../src/

clean:
	rm -f $(EXEC)

# all clean and run are not file names
.PHONY: all clean run

EXEC = stages$(EXE)
EXE =
ifeq ($(OS),Windows_NT)
    EXE = .exe
    MOVE = move
else
    MOVE = mv
endif

CC = gcc
# install libatomic.i686 due to using -m32 (sudo dnf install libatomic.i686)
CFLAGS = -m32 -Wall -Wextra -g -Iinclude -I../src
SRC = main.c include/cJSON.c

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

build: $(EXEC)
	./$(EXEC)
	$(MOVE) STAGES.BIN ../data/stages/
	$(MOVE) stages.h ../src/

clean:
	rm -f $(EXEC)

# all, clean and build are not file names
.PHONY: all clean build

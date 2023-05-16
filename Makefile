# Paths
SRC = ./src
INCLUDE = ../include
INPUT = ./input

# Compiler
CC = gcc

# Compile Options
CFLAGS = -pthread -Werror -Wall -I$(INCLUDE)

# Arguments
ARG = $(INPUT)/many_strings.txt 100 10 10

# Executable
EXEC = main

# Object files
OBJ =  $(SRC)/main.o  $(SRC)/child.o 

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) -lm 

run: $(EXEC)
	./$(EXEC) $(ARG)

clean:
	rm -f $(OBJ) $(EXEC) file*

CC = gcc

CFLAGS = -std=c23 \
	-D_POSIX_C_SOURCE=200809L \
	-D_XOPEN_SOURCE=700 \
	-Wall -Wextra -Werror \
	-Wno-unused-parameter \
	-fno-asm

TARGET = shell.out

SRC = src/main.c \
	  src/prompt.c \
	  src/input.c \
	  src/lexer.c \
	  src/parser.c \
	  src/hop.c \
	  src/reveal.c \
	  src/peek.c \
	  src/locate.c \
	  src/execute.c

OBJ = $(SRC:.c=.o)

CPPFLAGS = -Iinclude

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
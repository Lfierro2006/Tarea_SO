CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -g
TARGET = mishell
SRCS = $(wildcard *.c) # busca los archivos .c
OBJS = $(SRCS:.c=.o) # cambia .c a .o para lista de objetos
.PHONY: limpio
all: $(TARGET)

$(TARGET): $(OBJS) # enlaza los .o
		$(CC) $(CFLAGS) -o $@ $^
%.o: %.c # compila cada .c en su .o each
		$(CC) $(CFLAGS) -c $< -o $@

clean:
		rm -f $(OBJS) $(TARGET)
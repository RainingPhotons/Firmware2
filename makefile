CC=gcc
CFLAGS=-Wall -g
OBJ=fx.o server.o 
LIBS=-lpthread

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

lights: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o lights 

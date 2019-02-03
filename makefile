CC=gcc
CFLAGS=-Wall -g
OBJ=fx.o server.o 
SETUP_OBJ=setup_strands.o
LIBS=-lpthread

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

lights: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

setup_strands: $(SETUP_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o lights 

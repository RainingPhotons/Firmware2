CC=gcc
CFLAGS=-Wall -g
OBJDIR=obj
OBJ=$(addprefix $(OBJDIR)/, fx.o server.o)
SETUP_OBJ=$(addprefix $(OBJDIR)/, setup_strands.o)
SOUND_OBJ=$(addprefix $(OBJDIR)/, sndfile-play.o)
ORDERING_OBJ=$(addprefix $(OBJDIR)/, ordering.o)
LIBS=-lpthread -lsndfile -lasound

all: lights setup_strands play_audio ordering

$(OBJDIR)/%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

lights: $(OBJ) fx.h
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

setup_strands: $(SETUP_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

play_audio: $(SOUND_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

ordering: $(ORDERING_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o lights setup_strands play_audio ordering

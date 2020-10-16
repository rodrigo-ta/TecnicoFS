# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=-lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs

tecnicofs: fs/state.o fs/operations.o locks/sync.o locks/mutex.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs fs/state.o fs/operations.o locks/sync.o locks/mutex.o main.o -lpthread

fs/state.o: fs/state.c fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/state.o -c fs/state.c

fs/operations.o: fs/operations.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/operations.o -c fs/operations.c

sync.o: locks/sync.c locks/sync.h
	$(CC) $(CFLAGS) -o sync.o -c sync.c

mutex.o: locks/mutex.c locks/mutex.h 
	$(CC) $(CFLAGS) -o locks/mutex.o -c locks/mutex.c

main.o: main.c locks/sync.h locks/mutex.h fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o main.o -c main.c

clean:
	@echo Cleaning...
	rm -f fs/*.o locks/*.o *.o tecnicofs

run: tecnicofs
	./tecnicofs
CC   = gcc
OBJS = main.o board.o search.o ini.o io.o data.o

OPT = -DNDEBUG -DCSA_LAN

hyperDiceShogi: $(OBJS)
	$(CC) -o $@ $(OBJS) -lm

.c.o:
	$(CC) -c $<  $(OPT) -Wall -O3


$(OBJS): header.h

clean :
	rm *.o
	rm hyperDiceShogi.exe
	rm hyperDiceShogi


CFLAGS:= $(CFLAGS) -I../include -L../lib -lnipp

BIN= ascii2nipp nipp2ascii nipp_terminal

all : $(BIN)

ascii2nipp : nipp_ll.o ascii2nipp.c
	$(CC) $(CFLAGS) ascii2nipp.c nipp_ll.o -o ascii2nipp

nipp2ascii : nipp_ll.o nipp2ascii.c
	$(CC) $(CFLAGS) nipp2ascii.c nipp_ll.o -o nipp2ascii

nipp_terminal : nipp_ll.o nipp_terminal.c
	$(CC) $(CFLAGS) -lreadline nipp_terminal.c nipp_ll.o -o nipp_terminal

test : $(BIN) test.txt
	./ascii2nipp <test.txt | ./nipp2ascii | diff - test.txt

clean :
	rm -f $(BIN) *.o

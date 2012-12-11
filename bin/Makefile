CFLAGS:= $(CFLAGS) -I../include -L../lib -Wall

BIN= ascii2nipp nipp2ascii nipp_terminal

all : $(BIN)

ascii2nipp : nipp_ll.o ascii2nipp.c
	$(CC) $(CFLAGS) ascii2nipp.c nipp_ll.o -lnipp -o ascii2nipp

nipp2ascii : nipp_ll.o nipp2ascii.c
	$(CC) $(CFLAGS) nipp2ascii.c nipp_ll.o -lnipp -o nipp2ascii

nipp_terminal : nipp_ll.o nipp_terminal.c
	$(CC) $(CFLAGS) -lreadline nipp_terminal.c nipp_ll.o\
		-lreadline -lnipp -o nipp_terminal

test.nipp: ascii2nipp test.txt
	./ascii2nipp <test.txt >test.nipp

test : $(BIN) test.nipp
	# First check with a clean input
	./nipp2ascii <test.nipp | diff - test.txt
	# Now force resync
	echo garble | cat - test.nipp | ./nipp2ascii | diff - test.txt

clean :
	rm -f $(BIN) *.o test.nipp

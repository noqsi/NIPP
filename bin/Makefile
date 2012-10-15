CFLAGS:= $(CFLAGS) -I../include -L../lib -lnipp

BIN= ascii2nipp

all : $(BIN)

ascii2nipp : nipp_ll.o
	$(CC) $(CFLAGS) ascii2nipp.c nipp_ll.o

clean :
	rm -f $(BIN) *.o

# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -g -Wall
LIBS = 


all:   cclient server

cclient: myClient.c networks.o gethostbyname.o
	$(CC) $(CFLAGS) -o cclient myClient.c networks.o gethostbyname.o pdu.c pollLib.c safeUtil.c  handleTable.c  $(LIBS)

server: myServer.c networks.o gethostbyname.o
	$(CC) $(CFLAGS) -o server myServer.c networks.o gethostbyname.o pdu.c pollLib.c safeUtil.c  handleTable.c  $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f server cclient *.o





healthcenterserver: healthcenterserver.o
	gcc -o healthcenterserver -g -lm healthcenterserver.o -lnsl -lresolv -lsocket -lpthread

main.o: healthcenterserver.c
	gcc -g -c -Wall healthcenterserver.c 

clean:
	rm -f *.o healthcenterserver

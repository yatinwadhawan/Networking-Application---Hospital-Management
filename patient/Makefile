patient: patient.o
	gcc -o patient -g -lm patient.o -lnsl -lresolv -lsocket -lpthread

main.o: patient.c
	gcc -g -c -Wall patient.c 

clean:
	rm -f *.o patient

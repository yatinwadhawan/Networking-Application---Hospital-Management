doctor: doctor.o
	gcc -o doctor -g -lm doctor.o -lnsl -lresolv -lsocket -lpthread

main.o: doctor.c
	gcc -g -c -Wall doctor.c 

clean:
	rm -f *.o doctor

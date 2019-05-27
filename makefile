output: autograder.o tls.o
	gcc -g -pthread autograder.o tls.o -o app

autograder.o: autograder.c
	gcc -c -g autograder.c

tls.o: tls.c
	gcc -c -g tls.c

clean:
	rm *.o

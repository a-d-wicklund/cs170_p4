output: autograder_main.o tls.o
	gcc -g -pthread autograder_main.o tls.o -o app

autograder_main.o: autograder_main.c
	gcc -c -g autograder_main.c

tls.o: tls.c
	gcc -c -g tls.c

clean:
	rm *.o

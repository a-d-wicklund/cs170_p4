#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_clone(pthread_t tid);

int *arr[25] = {NULL};
int main(){	
	if(tls_create(5000) == -1){
        printf("Error\n");
    }
    if(tls_create(20) == -1){
        printf("Error\n");
    }
	char *c = "this";
	if(tls_write(0,5000,c) == -1){
		printf("error writing too far");
	}
    //printf("integer variable from tls library: %d",first);
}

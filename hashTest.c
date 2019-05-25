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
	if(tls_create(20) == -1){
        printf("Error\n");
    }
    if(tls_create(20) == -1){
        printf("Error\n");
    }
    //printf("integer variable from tls library: %d",first);
}

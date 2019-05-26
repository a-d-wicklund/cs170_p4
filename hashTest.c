#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
	char *c = malloc(sizeof(char)*4501);
	strcpy(c,"Testing one two three\0");
	if(tls_write(4092,30,c) == -1){
		printf("error writing too far\n");
	}
	char *d = malloc(sizeof(char)*4501);
	if(tls_read(4092,22,d) == -1){
		printf("Error reading too far\n");
	}
	printf("String: %s\n", d);
	free(d);
	free(c);
    //printf("integer variable from tls library: %d",first);
}

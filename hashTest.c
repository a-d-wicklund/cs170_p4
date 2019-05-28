#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>

void* tls_get_internal_start_address();
int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_clone(pthread_t tid);

int *arr[25] = {NULL};

void *tfunc(void *input){

	
}
int main(){	
	void *ptr = tls_get_internal_start_address();
	if(ptr ==NULL)
		printf("Does not exist\n");
	else
		printf("Thread local storage exists. first page is at %p\n", ptr);
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
	void *ptr2 = tls_get_internal_start_address();
	if(ptr2 ==NULL)
		printf("Does not exist\n");
	else
		printf("Thread local storage exists. first page is at %p\n", ptr2);
	//if(tls_clone(20) == -1){
	//	printf("Error target does not have tls or self already does\n");
	//}
	d = malloc(sizeof(char)*4501);

	pthread_t tid;
	//pthread_create(&tid,NULL,&tfunc, NULL);
	
    //printf("integer variable from tls library: %d",first);
}

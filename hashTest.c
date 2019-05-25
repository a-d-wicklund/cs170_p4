#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

int *arr[25] = {NULL};
int main(){	
	tls_create(20);
    //printf("integer variable from tls library: %d",first);
}

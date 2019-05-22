#include <stdio.h>
#include <stdlib.h>

#define MAXTHREADS

struct tmem{
    void *mem;
}

//Array to hold references to all threads with TLSs
struct tmem[MAXTHREADS] = NULL;

int tls_create(unsigned int size){
    //Use mmap to create a new block of memory for the thread

}
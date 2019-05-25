#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#define MAXTHREADS 129

int first = 1;
int pagesize;
typedef struct Page{
    void *addr; //Beginning of memory for this page
    int count; //How many threads are pointed to this page
}page;

typedef struct TLS{
    unsigned int size;
    pthread_t tid;
    page **pages; //Each TLS has an array of page**'s
}tls;

tls *tarray[MAXTHREADS] = {NULL}; //References to TLS structs

tls *hashSearch(pthread_t tid){
    int index = ((int) tid) % MAXTHREADS;
    
    //Loop until empty spot found
    while(tarray[index] != NULL){
        if(tarray[index]->tid == tid){
            return tarray[index];
        }
        index++;
        index = index % MAXTHREADS;
    }
    return NULL;
}

void hashInsert(pthread_t tid, tls **mem){
    int index = ((int) tid) % MAXTHREADS;

    while(tarray[index] != NULL){
        index++;
        index = index % MAXTHREADS;
    }
    tarray[index] = *mem;
}

void tls_init(){
    pagesize = getpagesize();
    //Set up the signal handler 
}
int tls_create(unsigned int size){
    if(first){
        tls_init();
    }
    int self = pthread_self();
    //Error handling: less than 1 or thread already has TLS
    if((size < 1) || (hashSearch(self) != NULL)){
        return -1;
    }
    tls *mem = malloc(sizeof(tls));
    hashInsert(self, &mem);
    mem->tid = self;
    mem->size = size;

    //put a new page into the array of pages until finished. 
    unsigned int cur = size;
    int i = 0;
    do{
        page *thisPage = malloc(page); //Create a new page
        thisPage->count = 1;
        thisPage->addr = mmap(NULL, 1, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        mem->pages[i] = thisPage; //Set the pointer to the new page pointer
        cur = cur - pagesize;
        i++;
    }while(cur > 0);
    
}

int tls_write(unsigned int offset, unsigned int length, char *buffer){
    //Error handling: when trying to write more than the storage can hold, or tls dne
    tls *mem = hashSearch(pthread_self());
    if(mem == NULL || offset+length > mem->size){
        return -1; 
    }

    //Unprotect pages until you're at the last page
    while(1){
        int curOffsetPage = offset/pagesize;
        int tlsPageCount = mem->size/pagesize;
        if(curOffsetPage == tlsPageCount || (offset+length - curOffsetPage*pagesize) < pagesize){
            mprotect(mem->pages[tlsPageCount]->addr, 1, PROT_WRITE);
            break;
        }
        else{
            mprotect(mem->pages[curOffsetpage]->addr, )
            curOffsetPage++;
        }
    }
    //if the block is shared, need to use mmap to create a new block of memory. 
    //Maybe just make a call to tls_create?
    //Somehow manage how its connected to previous section of memory
}

int tls_read(unsigned int offset, unsigned int length, char *buffer){
    //Find the block of memory and read from it
}

int tls_clone(pthread_t tid){
    //Insert another thread into hash table
    //
}

int tls_destroy(){

}
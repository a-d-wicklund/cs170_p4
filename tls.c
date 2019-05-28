#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXTHREADS 129
#define MAXPAGES 1025

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
    long int index = ((long int) tid) % MAXTHREADS;   
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
    long int index = ((long int) tid) % MAXTHREADS;

    while(tarray[index] != NULL){
        index++;
        index = index % MAXTHREADS;
    }
	printf("Filled array at index %d\n",index);
    tarray[index] = *mem;
}

void* tls_get_internal_start_address(){
	tls *mem = hashSearch(pthread_self());
	if(mem == NULL)
		return NULL;
	return mem->pages[0]->addr;
}

void tls_init(){
    pagesize = getpagesize();
    //Set up the signal handler 
}
int tls_create(unsigned int size){
    if(first){
        tls_init();
    }
    pthread_t  self = pthread_self();

   //Error handling: less than 1 or thread already has TLS
    if((size < 1) || (hashSearch(self) != NULL)){
        return -1;
    }
    tls *mem = malloc(sizeof(tls));
    hashInsert(self, &mem);
    mem->tid = self;
    mem->size = size;
	mem->pages = malloc(sizeof(page *)*MAXPAGES);
    //put a new page into the array of pages until finished. 
    int cur = (int) size;
    int i = 0;
    do{
		printf("creating new page\n");
        page *thisPage = malloc(sizeof(page)); //Create a new page
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
    int curOffsetPage = offset/pagesize;
    int tlsPageCount = mem->size/pagesize;
	char *bufPos = buffer;
    int once = 0;
	while(1){
		  
        if(mem->pages[curOffsetPage]->count > 1){
            //Create a new page and point to it in array
            printf("About to copy on write\n");
            mem->pages[curOffsetPage]->count--;
            page *p = malloc(sizeof(page));
            p->count = 1;
            p->addr = mmap(NULL, 1, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            mem->pages[curOffsetPage] = p;
	    }
        if(curOffsetPage == tlsPageCount || (offset+length - curOffsetPage*pagesize) < pagesize){
            //At the last page to write to
            mprotect(mem->pages[curOffsetPage]->addr, 1, PROT_WRITE);
            if(once){
                //If we've already wrote past the first page of the array, write at the start of the next page
                int writeLength = (offset+length)%pagesize;
                memcpy(mem->pages[curOffsetPage/pagesize]->addr, bufPos, writeLength);
            }     
            else{
				printf("Writing into the first and only page\n");
                //On the first write, start at the offset
                int writeLength = length;
                memcpy(mem->pages[offset/pagesize]->addr + offset%pagesize, bufPos, writeLength);
				//printf("%d\n",*((int *) mem->pages[offset/pagesize]->addr + offset%pagesize));
            } 
			mprotect(mem->pages[curOffsetPage]->addr, 1, PROT_NONE);
            break;
        }
        else{
            mprotect(mem->pages[curOffsetPage]->addr, 1, PROT_WRITE);
            int writeLength = pagesize;
            memcpy(mem->pages[curOffsetPage/pagesize]->addr, bufPos, pagesize);
            curOffsetPage++;
        }
        once = 1;
    }

	

	
    //if the block is shared, need to use mmap to create a new block of memory. 
    //Maybe just make a call to tls_create?
    //Somehow manage how its connected to previous section of memory
}

int tls_read(unsigned int offset, unsigned int length, char *buffer){
    //Find the block of memory and read from it
    //Error handling: when trying to write more than the storage can hold, or tls dne
    tls *mem = hashSearch(pthread_self());
    if(mem == NULL || offset+length > mem->size){
        return -1; 
    }

    //Unprotect pages until you're at the last page
    int curOffsetPage = offset/pagesize;
    int tlsPageCount = mem->size/pagesize;
	char *bufPos = buffer;
    int once = 0;
	while(1){
		mprotect(mem->pages[curOffsetPage]->addr, 1, PROT_READ);
        if(curOffsetPage == tlsPageCount || (offset+length - curOffsetPage*pagesize) < pagesize){
            if(once){
                //If we've already wrote past the first page of the array, write at the start of the next page
                int writeLength = (offset+length)%pagesize;
                strncpy(bufPos, mem->pages[curOffsetPage/pagesize]->addr, writeLength);
            }     
            else{
				printf("Reading from first and only page\n");
                //On the first write, start at the offset
                int writeLength = length;
                strncpy(bufPos,mem->pages[offset/pagesize]->addr + offset%pagesize, writeLength);
            } 
			mprotect(mem->pages[curOffsetPage]->addr, 1, PROT_NONE);
            break;
        }
        else{
            int writeLength = pagesize;
            strncpy(bufPos, mem->pages[curOffsetPage/pagesize]->addr, pagesize);
            curOffsetPage++;
            mprotect(mem->pages[curOffsetPage]->addr, 1, PROT_NONE);
        }
        once = 1;
    }
}

int tls_clone(pthread_t tid){
    //Insert another thread into hash table
    //Make pages for the new table correspond to pages from cloned one
    tls *memself = hashSearch(pthread_self());
	tls *memtarget = hashSearch(tid);
	if(memself != NULL || memtarget == NULL){
		return -1;
	}
	//printf("Inside clone. No error\n");
	memself = malloc(sizeof(tls));
	memself->tid = pthread_self();
	memself->size = memtarget->size;
	memself->pages = malloc(sizeof(page *)*MAXPAGES);
	memcpy(memself->pages, memtarget->pages, (memtarget->size/pagesize+1)*sizeof(page *));
	hashInsert(pthread_self(), &memself);
	//printf("page pointer at first spot(target): %p\npage pointer at first spot(clone): %p\n",memtarget->pages[0]->addr,memself->pages[0]->addr);
	printf("After memcpy\n");
	//printf("target val: %d\n",(int) *((char *) memtarget->pages[0]->addr));
	//printf("After print\n");
	int i = 0;
	for(i; i < memtarget->size/pagesize; i++){
		memtarget->pages[i]->count++;
	}		
	
}

int tls_destroy(){
    tls *mem = hashSearch(pthread_self());
    //Error Handling: TLS does not exist for this thread
    if(mem == NULL){
        return -1;
    }
    //TODO: go through each page in the "array" of pages and free each one
    //Then free the array
}

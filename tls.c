#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

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
    tarray[index] = *mem;
}

void* tls_get_internal_start_address(){
	tls *mem = hashSearch(pthread_self());
	if(mem == NULL)
		return NULL;
	return mem->pages[0]->addr;
}
void segfault_handler(int sig, siginfo_t *info, void *ucontext){
    pagesize = getpagesize();
    void *address = info->si_addr;
    for (int i = 0; i < MAXTHREADS; i++){
        //Iterate through each tls and check if any of its pages contain this address
        tls *cur_tls = tarray[i];
        if(cur_tls == NULL)
            continue;
        for(int j = 0; j <= cur_tls->size/pagesize; j++){
            //go through each page in the array of pages
            if(cur_tls->pages[j]->addr <= address && address <= cur_tls->pages[j]->addr + pagesize -1){
                printf("Out of bounds\n");
                pthread_exit(0);
            }
        }
    }
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_handler = SIG_DFL;
    if(sigaction(SIGSEGV,&sigact, NULL) == -1)
        perror("Error: cannot handle SIGALRM");	     
    raise(SIGSEGV);

}
void tls_init(){
    pagesize = getpagesize();
    //Set up the signal handler
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = segfault_handler;
    if(sigaction(SIGSEGV,&sigact, NULL) == -1)
        perror("Error: cannot handle SIGALRM");	 
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
    int curOffsetPage = offset/pagesize; //Starting page number that we will write to
    int lastPage = (offset + length)/pagesize; //Last page to write to 
    int tlsPageCount = mem->size/pagesize; //Last page
	char *bufPos = buffer;
    int once = 0;
	while(1){
		page *curPage = mem->pages[curOffsetPage]; 

        if(curPage->count > 1){
            //Create a new page and point to it in array
            //printf("About to copy on write\n");
            //printf("old page address: %p\n", curPage->addr);
            curPage->count--;
            page *p = malloc(sizeof(page));
            p->count = 1;
            p->addr = mmap(NULL, 1, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            curPage = p;
	    }
        char *curPageAddress = (char *) curPage->addr; 
        mem->pages[curOffsetPage] = curPage;
        //printf("new page address: %p\n", mem->pages[curOffsetPage]->addr);
        mprotect(curPageAddress, 1, PROT_WRITE);
        if(curOffsetPage == lastPage){
            //printf("At the last page to write to\n");
            //At the last page to write to
            if(once){
                //Writing into the last of multiple pages
                printf("writing into the last page\n");
                int writeLength = (offset+length)%pagesize;
                memcpy(curPageAddress, bufPos, writeLength);
                bufPos += writeLength;
            }     
            else{
                //Writing into the first and only page
				printf("Writing into the first and only page\n");
                int writeLength = length;
                memcpy(curPageAddress + offset, bufPos, writeLength);
                bufPos += writeLength;
				//printf("%d\n",*((int *) mem->pages[offset/pagesize]->addr + offset%pagesize));
            } 
            mprotect(curPageAddress, 1, PROT_NONE);
            break;
        }
        else{
            //At the first page or somewhere in the middle
            if(once){
                //Writing a whole page in the middle
                printf("Writing a full page somewhere in the middle\n");
                memcpy(curPageAddress,bufPos,pagesize);
                curOffsetPage++;
                bufPos += pagesize;
            }
            else{
                //Writing into the first of multiple pages
                printf("writing into the first page\n");
                int writeLength = (curOffsetPage + 1)*pagesize - offset; //Need to change to be amount remaining in this page
                memcpy(curPageAddress + offset%pagesize, bufPos, writeLength); //HERE is the error
                curOffsetPage++;
                bufPos += writeLength;
            }
            once = 1;
            mprotect(curPageAddress, 1, PROT_NONE);
        } 
        
        
    }
   
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
    int lastPage = (offset + length)/pagesize; //Last page to write to 
    int tlsPageCount = mem->size/pagesize;
	char *bufPos = buffer;
    int once = 0;
	while(1){
        char *curPageAddress = (char *) mem->pages[curOffsetPage]->addr;  
		mprotect(curPageAddress, 1, PROT_READ);
        if(curOffsetPage == lastPage){
            if(once){
                //Reading from the last page
                int readLength = (offset+length)%pagesize;
                memcpy(bufPos, curPageAddress, readLength);
            }     
            else{
                //Reading from the first and only page
				printf("Reading from first and only page\n");
                int readLength = length;
                memcpy(bufPos,curPageAddress + offset%pagesize, readLength);
            } 
			mprotect(curPageAddress, 1, PROT_NONE);
            break;
        }
        else{
            if(once){
                //Reading a whole page in the middle
                printf("Reading a full page somewhere in the middle\n");
                memcpy(bufPos,curPageAddress,pagesize);
                curOffsetPage++;
                bufPos += pagesize;
            }
            else{
                //Reading from the first page of multiple pages
                printf("Reading from the first of multiple pages\n");
                int readLength = (curOffsetPage + 1)*pagesize - offset;
                memcpy(bufPos, curPageAddress + offset%pagesize, readLength);
                curOffsetPage++;
                bufPos += readLength;   
            }
            mprotect(curPageAddress, 1, PROT_NONE);
            once = 1;
        }
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
	printf("After Clone\n");
	//printf("target val: %d\n",(int) *((char *) memtarget->pages[0]->addr));
	//printf("After print\n");
	int i = 0;
	for(i; i < memtarget->size/pagesize; i++){
		memtarget->pages[i]->count++;
	}		
	
}

int tls_destroy(){
    pagesize = getpagesize();
    tls *mem = hashSearch(pthread_self());
    //Error Handling: TLS does not exist for this thread
    if(mem == NULL){
        return -1;
    }
    for(int i = 0; i <= (mem->size-1)/pagesize; i++){
        mem->pages[i]->count--;
        if(mem->pages[i]->count == 0){
            munmap(mem->pages[i]->addr, pagesize);
            free(mem->pages[i]);
        }
    }
    
    //TODO: go through each page in the "array" of pages and free each one
    //Then free the array
}

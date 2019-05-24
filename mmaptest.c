#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>


int main(){

    void *location;

    location = mmap(NULL, 1, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    mprotect(location, getpagesize()+1, PROT_READ | PROT_WRITE);

    char *a = (void *) location;

    *(a+5) = 'a';
    printf("%d\n",(int) *(a));

    //Takeaway: shown that mprotect protects a page at a time. Also, mmap initializes 
    //all values to zero.

}
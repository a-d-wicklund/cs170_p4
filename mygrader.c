#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_clone(pthread_t tid);


static int test1(void){
    pthread_t tid1 = 0;

    // create a thread, to give anyone using their homegrown thread library a chance to init
    //pthread_create(&tid1, NULL,  &_thread_dummy, NULL);

    char buf[8192];
    tls_create(8192);
    tls_read(0, 4, buf);
    tls_write(0, 4, buf);
    tls_destroy();

    return 1;
}


// basic read / write functionality
//==============================================================================
static int test2(void){
    pthread_t tid1 = 0;

    // create a thread, to give anyone using their homegrown thread library a chance to init
    //pthread_create(&tid1, NULL,  &_thread_dummy, NULL);

    char in_buf[4] = {1,1,1,1};
    char out_buf[4] = {0};

    tls_create(8192);
    tls_write(0, 4, in_buf);
    tls_read(0, 4, out_buf);

    for(int i = 0; i < 4; i++){
        if(out_buf[i] != 1){
            return 0;
        }
    }

    return 1;
}


// clone test 1
//==============================================================================
static void* _thread_create(void* arg){
    char in_buf[4] = {2,2,2,2};
    sem_t* mutex_sem = (sem_t*)arg;

    tls_create(8192);
    tls_write(0, 4, in_buf);
    sem_post(mutex_sem);
	printf("Done writing\n");
    while(1);
    return 0;
}


static int test3(void){
    pthread_t tid1 = 0;
    sem_t mutex_sem;
    char out_buf[4] = {0};

    // init sem, call thread, and wait until thread 2 creates and writes to it's LSA
    sem_init(&mutex_sem, 0, 0);
    pthread_create(&tid1, NULL,  &_thread_create, &mutex_sem);
    sem_wait(&mutex_sem);
	printf("tid1 = %d\n", (int) tid1);
    // clone thread 2's LSA and make sure the values read are what we put there
    if(tls_clone(tid1) == -1){
		printf("Error with clone\n");
	}
    tls_read(0, 4, out_buf);

    for(int i = 0; i < 4; i++){
		printf("Output = %d\n",(int) out_buf[i]);
        if(out_buf[i] != 2){
            return 0;
        }
    }

    return 1;
}



int main(){
	/*if(test1()){
		printf("Passed 1!\n");
	}
	
	if(test2()){
		printf("Passed 2!\n");
	}
	*/	
	if(test3()){
		printf("Passed 3!\n");
	}
	

}

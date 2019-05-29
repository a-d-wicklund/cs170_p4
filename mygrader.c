#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

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

// clone test 2
//==============================================================================
static void* _thread_create2(void* arg){
    char in_buf[4] = {2,2,2,2};
    sem_t* mutex_sems = (sem_t*)arg;

    // create new LSA and signal to thread 1 when done
    tls_create(8192);
    sem_post(&mutex_sems[0]);

    // wait for thread 1 to clone, write our own values, then signal back to thread 1
    sem_wait(&mutex_sems[1]);
    tls_write(0, 4, in_buf);
    sem_post(&mutex_sems[0]);

    while(1); // just wait forever, don't want to exit and release LSA memory
    return 0; // shut the compiler up
}

//This test creates a second thread, creates a tls for that thread, clones it in main, writes to it in main,
//which should create a new page for that entry, then writes to it in second thread, finally reads from it in main.
//When read, it should contain what main wrote to it separately
static int test4(void){
    pthread_t tid1 = 0;
    sem_t mutex_sems[2];
    char in_buf[4] = {1,1,1,1};
    char out_buf[4] = {0,0,0,0};


    // init semaphores and wait for thread 2 to create LSA
    sem_init(&mutex_sems[0], 0, 0);
    sem_init(&mutex_sems[1], 0, 0);
    pthread_create(&tid1, NULL,  &_thread_create2, mutex_sems);
    sem_wait(&mutex_sems[0]);

    // clone and write our own new values, and signal to thread 2 it's time to write values
    if(tls_clone(tid1) == -1){
		printf("Error while cloning\n");
	}
    tls_write(0, 4, in_buf);
    sem_post(&mutex_sems[1]);

    // wait until thread 2 writes, then read out the LSA
    sem_wait(&mutex_sems[0]);
    tls_read(0, 4, out_buf);

    // make sure our data was not overwritten by thread 2
    for(int i = 0; i < 4; i++){
		printf("out_buf should be 1 but is %d\n", out_buf[i]);
        if(out_buf[i] != 1){
            return 0;
        }
    }

    return 1;
}


static int test9(void){
    pthread_t tid1 = 0;

    // create a thread, to give anyone using their homegrown thread library a chance to init
    //pthread_create(&tid1, NULL,  &_thread_dummy, NULL);

    char in_buf[6000] = {0};
    for(int i = 0; i < 6000; i++) {
        in_buf[i] = 1;
    }
    char out_buf[6000] = {0};

    tls_create(12288);
    tls_write(4000, 6000, in_buf);
    tls_read(4000, 6000, out_buf);

    for(int i = 0; i < 6000; i++){
        if(out_buf[i] != 1){
            return 0;
        }
    }

    return 1;
}
static void* addr;
static int s = 20;

static void* test_seg(void* arg){
    char in_buf[4] = {2,2,2,2};

    tls_create(8192);
    tls_write(4094, 4, in_buf);
    while(s == 20);
    strcpy(in_buf,(char*)addr);
    s = 30;
    while(1);
    return 0;
}

static void* test_seg1(void* arg){
    char in_buf[4] = {2,2,2,2};

    tls_create(8192);
    tls_write(4094, 4, in_buf);
    strcpy(in_buf,(char*)tls_get_internal_start_address());
    s = 30;
    while(1);
    return 0;
}


static int test21(void){
    pthread_t tid1 = 0;
    tls_create(8192);
    addr = tls_get_internal_start_address();
    // create a thread, to give anyone using their homegrown thread library a chance to init
    pthread_create(&tid1, NULL,  &test_seg, NULL);

    char in_buf[4] = {1,1,1,1};
    char out_buf[4] = {0};

    s = 10;
    pthread_join(tid1, NULL);

    if(s != 10){
        return 0;
    }
    printf("Passed the first\n");
    s = 20;
    pthread_create(&tid1, NULL,  &test_seg1, NULL);
    pthread_join(tid1, NULL);

    if(s != 20){
        return 0;
    }

    return 1;
}

int main(){
	/*if(test1()){
		printf("Passed 1!\n");
	}
	*/
	if(test21()){
		printf("Passed 21!\n");
	}

	

}

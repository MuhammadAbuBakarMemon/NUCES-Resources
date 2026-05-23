#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* thread_routine(void* arg) {
    long thread_num = (long)arg;
    printf("Thread %ld\n", thread_num);
    
    // Terminate and return the value 0
    pthread_exit((void*)0);
}

void * thread_routine(void * arg)
{

    long tid = (long) arg;
    printf("Thread %ld\n", tid);

    pthread_exit((void *) 0);

}

int main() {
    pthread_t threads[3];
    int rc;

    // a) & b) Thread Creation and Error Handling
    for (long i = 0; i < 3; i++) {
        rc = pthread_create(&threads[i], NULL, thread_routine, (void*)(i + 1));
        if (rc) {
            printf("Error\n");
            exit(-1);
        }
    }

    // Join threads to fetch and print their return values
    for (int i = 0; i < 3; i++) {
        void* retval;
        pthread_join(threads[i], &retval);
        printf("Thread %d returns: %ld\n", i + 1, (long)retval);
    }

    return 0;
}
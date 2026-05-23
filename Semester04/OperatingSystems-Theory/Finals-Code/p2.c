#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NT 5

void* printHello(void *tid)
{

    long ti = (long) tid;
    printf("Hello from thread: %ld", tid);
    pthread_exit(NULL);

}

int main(void)
{

    pthread_t threads[NT];
    int rc;
    long t;

    for (t = 0; t < NT; t++)
    {

        printf("In main: creating thread: %ld", t);
        rc = pthread_create(&threads[t], NULL, printHello, &t);
        if (rc)
        {
            printf("Error, return cpde from pthread_create is: %d", rc);
            exit(-1);

        }

    }
    
    pthread_exit(NULL);

    return 0;
}


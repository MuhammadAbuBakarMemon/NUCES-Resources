#include <stdio.h>
#include <pthread.h>

void *computation();

int main(void)
{

    pthread_t thread1;

    pthread_create(thread1, NULL, computation, NULL);

    return 0;

};

void *computation()
{
    printf("Computation\n");

    return NULL;
};
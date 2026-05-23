#include <pthread.h>
#include <stdio.h>

void * thread1()
{
    while(1)
    {

        printf("Salam\n");
    }

}

void* thread2()
{

    printf("kaise hain app\n");
}

int main(void)
{

    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
};
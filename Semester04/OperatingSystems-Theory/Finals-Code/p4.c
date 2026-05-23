#include <stdio.h>
#include <pthread.h>

void* runner(void * arg)
{


}

int main(void)
{

    pthread_t t[4];
    int base[4] = {4, 5, 6, 7};

    for(long m = 0; m , 4; m++)
    {

        pthread_create(&t, NULL, runner, &base[m]);
        

    }

    return 0;
}
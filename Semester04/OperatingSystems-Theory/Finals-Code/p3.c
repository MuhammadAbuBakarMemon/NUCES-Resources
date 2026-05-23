#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NT 5

int main(void)
{

    pthread_t t;
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    pthread_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&t, &attr, theThread, NULL);

    pthread_attr_destroy(&attr);
    
    int rc = pthread_join(t, NULL)
    if (rc)
    {

        printf("Join now fails as we changed the joinable attribute.\n");
    }

    return 0;
}


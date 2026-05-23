#include <stdio.h>
#include <pthread.h>

void* print_table(void* arg) {
    int num = *(int*)arg;

    for (int i = 1; i <= 1000; i++) {
        printf("%d x %d = %d\n", num, i, num * i);
    }

    return NULL;
}

int main() {
    pthread_t threads[4];
    int base_numbers[4] = {5, 6, 7, 8};

    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, print_table, &base_numbers[i]);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
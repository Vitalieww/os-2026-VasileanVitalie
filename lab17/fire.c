#include <stdio.h>
#include <pthread.h>

#define N 1000000

long counter = 0;

void *work(void *arg) {
    long id = (long)arg;
    printf("thread %ld sees counter = %ld\n", id, counter);
    for (int i = 0; i < N; i++)
        counter++;
    return NULL;
}

int main(void) {
    pthread_t a, b, c, d;
    pthread_create(&a, NULL, work, (void *)1);
    pthread_create(&b, NULL, work, (void *)2);
    pthread_create(&c, NULL, work, (void *)3);
    pthread_create(&d, NULL, work, (void *)4);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    pthread_join(c, NULL);
    pthread_join(d, NULL);
    printf("expected: %d\n", 4 * N);
    printf("counter: %ld (lost updates: the race)\n", counter);
    return 0;
}

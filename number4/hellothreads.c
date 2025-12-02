#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

pthread_mutex mutex;

void *hello_thread(void *arg) {
    printf("Hello from thread %d!\n", (int)(long)arg);
    return arg;
}

int main() {
    pthread_t parent_tid, child_tid;
    int status;
    void *result;

    pthread_mutex_init(&mutex, NULL);

    status = pthread_create(&child_tid, NULL, hello_thread, (void *)(long)1);
    if (status != 0) {
        fprintf(stderr, "create child thread: %d", status);
        exit(1);
    }
}
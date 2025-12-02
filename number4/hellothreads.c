#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = 0; // 부모 차례 : 0, 자식 차례 : 1

void *child_thread(void *arg) {
    while(1) {
        pthread_mutex_lock(&mutex);

        while (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("hello parent\n");
        sleep(1);

        turn = 0;
        pthread_cond_signal(&cond);
        
        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    pthread_t child_tid;
    int status;
    void *result;

    pthread_mutex_init(&mutex, NULL);

    status = pthread_create(&child_tid, NULL, child_thread, (void *)(long)1);
    if (status != 0) {
        fprintf(stderr, "create child thread: %d", status);
        exit(1);
    }

    while (1) {
        pthread_mutex_lock(&mutex);

        while (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("hello child\n");

        turn = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    pthread_join(child_tid, NULL);
    return 0;
}
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 3

pthread_mutex_t mutex;
int sum;

void *mutex_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    sum += (int)(long)arg;
    pthread_mutex_unlock(&mutex);

    return arg;
}

int main(int argc, char *argv[]) {
    pthread_t tid[NUM_THREADS];
    int arg[NUM_THREADS], i;
    void *result;
    int status;

    if (argc < 4) {
        fprintf(stderr, "Usage: mutexthread<num1> <num2> <num3>\n");
        exit(1);
    }

    for (i=0; i<NUM_THREADS; i++) {
        arg[i] = atoi(argv[i+1]);
    }
    // 뮤텍스 초기화
    pthread_mutex_init(&mutex, NULL);

    for (i=0; i<NUM_THREADS; i++) {
        status = pthread_create(&tid[i], NULL, mutex_thread, (void *)(long)arg[i]);
        if (status != 0) {
            fprintf(stderr, "create thread: %d", status);
            exit(1);
        }
    }
    for (i=0; i<NUM_THREADS; i++) {
        status = pthread_join(tid[i], &result);
        if (status != 0) {
            fprintf(stderr, "join thread: %d", status);
            exit(1);
        }
    }
    status = pthread_mutex_destroy(&mutex);
    if (status != 0) {
        perror("destroy mutex");
    }
    printf("Sum: %d\n", sum);
    pthread_exit(NULL);
}
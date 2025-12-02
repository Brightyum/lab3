#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define NUM_THREADS 3

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sigset_t sigset;
int completed;

void *signal_thread(void *arg) {
    int signal;
    int count = 0;

    while (1) {
        sigwait(&sigset, &signal);
        
        if (signal == SIGINT) { //SIGINT = Ctrl+C
            printf("Signal thread: SIGINT %d\n", ++count);
        }

        if (count >= 3) {
            pthread_mutex_lock(&mutex);
            completed = 1;
            pthread_mutex_unlock(&mutex);
            break;
        }
    }
    return arg;
}

int main(int argc, char *argv[]) {
    pthread_t tid;
    int arg;
    void *result;
    int status;

    if (argc < 2) {
        fprintf(stderr, "Usage: signalthread<seconds>\n");
        exit(1);
    }
    arg = atoi(argv[1]);
    // 시그널 목록 초기화
    sigemptyset(&sigset);
    // 시그널 목록 Ctrl+C 시그널 추가
    sigaddset(&sigset, SIGINT);

    // Ctrl+C 차단
    status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    if (status != 0) {
        fprintf(stderr, "set sigmask: %d", status);
        exit(1);
    }

    status = pthread_create(&tid, NULL, signal_thread, NULL);
    if (status != 0) {
        fprintf(stderr, "create thread: %d", status);
        exit(1);
    }
    while(1) {
        sleep(arg);
        pthread_mutex_lock(&mutex);
        
        printf("Main thread: mutex locked\n");
        if (completed) { break; }
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_unlock(&mutex);
    pthread_exit(result);
    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 20
#define NUMITEMS 30

typedef struct {
    int item[BUFFER_SIZE];
    int totalitems;
    int in, out;
    pthread_mutex_t mutex;
    pthread_cond_t full, empty;
} buffer_t;

buffer_t bb = { {0,0,0,0,0,0,0,0,0,0},0,0,0,
                PTHREAD_MUTEX_INITIALIZER,
                PTHREAD_COND_INITIALIZER,
                PTHREAD_COND_INITIALIZER };

int produce_item() {
    int item = (int)(100.0*rand()/RAND_MAX + 1.0);
    sleep((unsigned int)(5.0*rand()/RAND_MAX + 1.0));
    printf("produce item: %d\n", item);
    return item; 
}

void *insert_item(int item) {
    int status;
    status = pthread_mutex_lock(&bb.mutex);
    if (status != 0) {
        return status;
    }

    while (bb.totalitems >= BUFFER_SIZE && status == NULL) {
        status = pthread_cond_wait(&bb.empty, &bb.mutex);
    }
    if (status != 0) {
        pthread_mutex_unlock(&bb.mutex);
        return status;
    }
    bb.item[bb.in] = item;
    bb.in = (bb.in + 1) % BUFFER_SIZE;
    bb.totalitems++;

    if (status = pthread_cond_signal(&bb.full)) {
        pthread_mutex_unlock(&bb.mutex);
        return status;
    }
    return pthread_mutex_unlock(&bb.mutex);
}

void *consume_item(int item) {
    sleep((unsigned int)(5.0*rand()/RAND_MAX + 1.0));
    printf("\t\tconsume item: %d\n", item);
}

void *remove_item(int *temp) {
    int status;
    status = pthread_mutex_lock(&bb.mutex);
    if (status != 0) {
        return status;
    }

    while (bb.totalitems <= 0 && status == NULL) {
        status = pthread_cond_wait(&bb.full, &bb.mutex);
    }
    if (status != 0) {
        pthread_mutex_unlock(&bb.mutex);
        return status;
    }
    
    *temp = bb.item[bb.out];
    bb.out = (bb.out + 1) % BUFFER_SIZE;
    bb.totalitems--;

    if (status = pthread_cond_signal(&bb.empty)) {
        pthread_mutex_unlock(&bb.mutex);
        return status;
    }
    return pthread_mutex_unlock(&bb.mutex);
}

void *producer(void *arg) {
    int item;

    while(1) {
        item = produce_item();
        insert_item(item);
    }
}

void *consumer(void *arg) {
    int item;

    while(1) {
        remove_item(&item);
        consume_item(item);
    }
}

int main() {
    int status;
    void *result;
    pthread_t prod_tid, cons_tid;

    status = pthread_create(&prod_tid, NULL, producer, NULL);
    if (status != 0) {
        fprintf(stderr, "create producer thread: %d", status);
        exit(1);
    }
    status = pthread_create(&cons_tid, NULL, consumer, NULL);
    if (status != 0) {
        fprintf(stderr, "create consumer thread: %d", status);
        exit(1);
    }

    status = pthread_join(prod_tid, NULL);
    if (status != 0) {
        fprintf(stderr, "join producer thread: %d", status);
        exit(1);
    }
    status = pthread_join(cons_tid, NULL);
    if (status != 0) {
        fprintf(stderr, "join consumer thread: %d", status);
        exit(1);
    }
    return 0;
}
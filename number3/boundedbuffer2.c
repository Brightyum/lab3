#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 20
#define NUMITEMS 20
#define NUM_THREADS 2

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

int produce_item(int thread_id) {
    int item = (int)(100.0*rand()/RAND_MAX + 1.0);
    sleep((unsigned int)(5.0*rand()/RAND_MAX + 1.0));
    printf("produce item%d: %d\n", thread_id, item);
    return item; 
}

int insert_item(int item) {
    int status;
    status = pthread_mutex_lock(&bb.mutex);
    if (status != 0) {
        return status;
    }

    while (bb.totalitems >= BUFFER_SIZE && status == 0) {
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

void *producer(void *arg) {
    int item;
    int thread_id = (int)(long)arg;
    while(1) {
        item = produce_item(thread_id);
        insert_item(item);
    }
}

int remove_item(int *temp) {
    int status;
    status = pthread_mutex_lock(&bb.mutex);
    if (status != 0) {
        return status;
    }

    while (bb.totalitems <= 0 && status == 0) {
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

int consume_item(int item, int thread_id) {
    sleep((unsigned int)(5.0*rand()/RAND_MAX + 1.0));
    printf("\t consume item%d: %d\n",thread_id, item);
    return item;
}

void *consumer(void *arg) {
    int item;
    int thread_id = (int)(long)arg;
    while(1) {
        remove_item(&item);
        consume_item(item, thread_id);
    }
}
/*  
쓰레드를 사용하여 생산자 소비자 문제를 해결하는 제한 버퍼(Bounded Buffer)를 생성하고 활용하는 프로그램을 구현하시오. 
단, 생산자와 소비자 쓰레드는 각각 둘 이상 가능해야 한다
*/
int main() {
    pthread_t prod_tid[NUM_THREADS], cons_tid[NUM_THREADS];
    int status;
    void *result;

    for (int i = 0; i < NUM_THREADS; i++) {
        status = pthread_create(&prod_tid[i], NULL, producer, (void*)(long)i);
        if (status != 0) {
            fprintf(stderr, "create producer thread: %d", status);
            exit(1);
        }
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        status = pthread_create(&cons_tid[i], NULL, consumer, (void*)(long)i);
        if (status != 0) {
            fprintf(stderr, "create consumer thread: %d", status);
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        status = pthread_join(prod_tid[i], &result);
        if (status != 0) {
            fprintf(stderr, "join producer thread: %d", status);
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        status = pthread_join(cons_tid[i], &result);
        if (status != 0) {
            fprintf(stderr, "join consumer thread: %d", status);
            exit(1);
        }
    }

    return 0;
}
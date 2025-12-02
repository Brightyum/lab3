#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 3

void *hello_thread(void *arg) {
	printf("Thread %d: hello, world!\n", (int)(long)arg);
	return arg;
}

int main() {
	pthread_t tid[NUM_THREADS];
	int i, status;

	for (i=0; i<NUM_THREADS; i++) {
		status = pthread_create(&tid[i], NULL, hello_thread, (void *)(long)i);
		if (status != 0) {
			fprintf(stderr, "Create thread %d: %d", i, status);
			exit(0);
		}
	}
	pthread_exit(NULL);
	return 0;
}

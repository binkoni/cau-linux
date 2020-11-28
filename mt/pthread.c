#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool produced = false;
int data = 0;

void* consumer(void* arg)
{
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < 10; ++i) {
		while (!produced)
			pthread_cond_wait(&cond, &mutex);
		printf("consumed %d\n", data);
		produced = false;
	}
	pthread_mutex_unlock(&mutex);
}

void* producer(void* arg)
{
	for (int i = 0; i < 10; ++i) {
		pthread_mutex_lock(&mutex);
		produced = true;
		data = rand();
		printf("produced %d\n", data);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
}

int main() {
	srand(time(NULL));
	pthread_t threads[2];
	pthread_create(&threads[0], NULL, producer, NULL);
	pthread_create(&threads[1], NULL, consumer, NULL);

	for(int i = 0; i < 2; ++i) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}


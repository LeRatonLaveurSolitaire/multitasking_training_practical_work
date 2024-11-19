#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "acquisitionManager.h"
#include "msg.h"
#include "iSensor.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"

// producer count storage
_atomic volatile unsigned int produceCount = 0;

pthread_t producers[4];

static void *produce(void *params);

/**
 * Semaphores and Mutex
 */
sem_t *sem_empty;
sem_t *sem_full;

#define SEM_FULL_NAME "/full"
#define SEM_EMPTY_NAME "/empty"

#define CHECK_SEMAPHORE(sem)  \
	if (sem != SEM_FAILED)    \
		return ERROR_SUCCESS; \
                              \
	perror("[sem_open");      \
	return ERROR_INIT;

pthread_mutex_t mutex_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_buffer_index = PTHREAD_MUTEX_INITIALIZER;

MSG_BLOCK buffer_data[256];
_atomic volatile unsigned int index_data_read = 0;
_atomic volatile unsigned int index_data_write = 0;

int buffer_index[256];
_atomic volatile unsigned int index_buffer_read = 0;
_atomic volatile unsigned int index_buffer_write = 0;

/*
 * Creates the synchronization elements.
 * @return ERROR_SUCCESS if the init is ok, ERROR_INIT otherwise
 */
static unsigned int createSynchronizationObjects(void);

/*
 * Increments the produce count.
 */
static void incrementProducedCount(void);

static unsigned int createSynchronizationObjects(void)
{

	// Initialize semaphores
	sem_unlink(SEM_EMPTY_NAME);
	sem_unlink(SEM_FULL_NAME);

	// Open semaphores
	sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0644, 255);
	sem_full = sem_open(SEM_FULL_NAME, O_CREAT, 0644, 0);

	// Check semaphores
	CHECK_SEMAPHORE(sem_empty);
	CHECK_SEMAPHORE(sem_full);
	printf("[acquisitionManager]Semaphore created\n");
	return ERROR_SUCCESS;
}

static void incrementProducedCount(void)
{
	produceCount++;
}

unsigned int getProducedCount(void)
{
	unsigned int p = 0;
	p = produceCount;
	return p;
}

MSG_BLOCK getMessage(void)
{
	// prendre le sémpahore
	sem_wait(sem_full);
	// récupérer l'index
	int index_local = buffer_index[index_buffer_read];
	// incrémenter l'index avec remise à zéro
	index_buffer_read = (index_buffer_read + 1) % 256;
	// lire le message
	MSG_BLOCK message = buffer_data[index_local];
	// libérer le sémpahore
	sem_post(sem_empty);
	return message;
}

void writeMessage(MSG_BLOCK message)
{
	// prendre le sémpahore
	sem_wait(sem_empty);
	// prendre le mutex
	pthread_mutex_lock(&mutex_write);
	// récupérer l'index
	int index_local = index_buffer_write;
	// incrémenter l'index avec remise à zéro
	index_buffer_write = (index_buffer_write + 1) % 256;
	// rendre le mutex
	pthread_mutex_unlock(&mutex_write);

	// écrire le message
	buffer_data[index_local] = message;

	// lock mutex
	pthread_mutex_lock(&mutex_buffer_index);
	// mettre l'index dans le buffer de lecture
	buffer_index[index_buffer_write] = index_local;
	// incrémente l'index d'écriture
	index_data_write = (index_data_write + 1) % 256;
	// unlock mutex
	pthread_mutex_unlock(&mutex_buffer_index);
	// libérer le sémpahore
	sem_post(sem_full);
}

unsigned int acquisitionManagerInit(void)
{
	unsigned int i;
	printf("[acquisitionManager]Synchronization initialization in progress...\n");
	fflush(stdout);
	if (createSynchronizationObjects() == ERROR_INIT)
		return ERROR_INIT;

	printf("[acquisitionManager]Synchronization initialization done.\n");

	for (i = 0; i < PRODUCER_COUNT; i++)
	{
		pthread_create(&producers[i], NULL, produce, (void *)i);
	}

	return ERROR_SUCCESS;
}

void acquisitionManagerJoin(void)
{
	unsigned int i;
	for (i = 0; i < PRODUCER_COUNT; i++)
	{
		pthread_join(producers[i], NULL);
	}

	sem_destroy(sem_empty);
	sem_destroy(sem_full);
	printf("[acquisitionManager]Semaphore cleaned\n");
}

void *produce(void *params)
{
	D(printf("[acquisitionManager]Producer created with id %d\n", gettid()));
	unsigned int i = 0;
	while (i < PRODUCER_LOOP_LIMIT)
	{
		i++;
		sleep(PRODUCER_SLEEP_TIME + (rand() % 5));
		MSG_BLOCK mBlock;
		getInput(indexProducer, &mBlock);
		// check message
		if (messageCheck(&mBlock) == 0)
		{
			printf("[acquisitionManager]Message corrupted\n");
		}
		// write message
		writeMessage(mBlock);
		// increment count
		incrementProducedCount();
	}
	printf("[acquisitionManager] %d termination\n", gettid());
	pthread_exit(NULL);
}
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h>
#include <pthread.h>
#include "messageAdder.h"
#include "msg.h"
#include "iMessageAdder.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"

//consumer thread
pthread_t consumer;
//Message computed
volatile MSG_BLOCK out;
//Consumer count storage
volatile unsigned int consumeCount = 0;
// Mutex Sum
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

/**
 * Increments the consume count.
 */
static void incrementConsumeCount(void);

/**
 * Consumer entry point.
 */
static void *sum( void *parameters );


MSG_BLOCK_with_ConsumedCount getCurrentSum(){
	// lock mutex
	pthread_mutex_lock(&mutex);
	MSG_BLOCK currentSum = out;
	unsigned int valconsumedCount = getConsumedCount();
	MSG_BLOCK_with_ConsumedCount currentSumWithCount={currentSum, valconsumedCount};
	// unlock mutex
	pthread_mutex_unlock(&mutex);
	return currentSumWithCount;
}

static void incrementConsumeCount(void){
	consumeCount++;
}

unsigned int getConsumedCount(){
	return (unsigned int)consumeCount;
}


void messageAdderInit(void){
	out.checksum = 0;
	for (size_t i = 0; i < DATA_SIZE; i++)
	{
		out.mData[i] = 0;
	}
	pthread_create(&consumer, NULL, sum, NULL);
}

void messageAdderJoin(void){
	pthread_join(consumer, NULL);
}

static void *sum( void *parameters )
{
	D(printf("[messageAdder]Thread created for sum with id %d\n", gettid()));//gettid()));
	unsigned int i = 0;
	while(i<ADDER_LOOP_LIMIT){
		i++;
		sleep(ADDER_SLEEP_TIME);
		// lire message
		MSG_BLOCK newMessage = getMessage();
		// check message
		if (messageCheck(&newMessage)==0){
			printf("[messageAdder]Message corrupted\n");
		}
		// lock mutex
		pthread_mutex_lock(&mutex);
		// add message
		messageAdd(&out, &newMessage);
		// unlock mutex
		pthread_mutex_unlock(&mutex);
		// increment count
		incrementConsumeCount();
	}
	printf("[messageAdder] %d termination\n", gettid());//gettid());
	pthread_exit(NULL);
}



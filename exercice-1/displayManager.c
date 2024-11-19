#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "displayManager.h"
#include "iDisplay.h"
#include "iAcquisitionManager.h"
#include "iMessageAdder.h"
#include "msg.h"
#include "multitaskingAccumulator.h"
#include "debug.h"

// DisplayManager thread.
pthread_t displayThread;

/**
 * Display manager entry point.
 * */
static void *display( void *parameters );


void displayManagerInit(void){
	pthread_create(&displayThread, NULL, display, NULL);
}

void displayManagerJoin(void){
	pthread_join(displayThread, NULL);
} 

static void *display( void *parameters )
{
	D(printf("[displayManager]Thread created for display with id %d\n", gettid()));//gettid()));
	unsigned int diffCount = 0;
	while(diffCount < DISPLAY_LOOP_LIMIT){
		sleep(DISPLAY_SLEEP_TIME);
		MSG_BLOCK newSum = getCurrentSum();
		// check message
		if (messageCheck(&newSum)==0){
			printf("[displayManager]Message corrupted\n");
		}
		// display sum
		messageDisplay(&newSum);
		// display count
		print(getProducedCount(), getConsumedCount());
	}
	printf("[displayManager] %d termination\n", gettid());//gettid()));
    pthread_exit(NULL);
}
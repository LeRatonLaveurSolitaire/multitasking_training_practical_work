#ifndef I_MESSAGE_ADDER_H
#define I_MESSAGE_ADDER_H

#include "msg.h"

/**
* Gets a message that represents the current value of the sum.
*/
MSG_BLOCK_with_ConsumedCount getCurrentSum();

/**
* Get the number of consumed messages.
*/
unsigned int getConsumedCount();

#endif
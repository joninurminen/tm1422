#ifndef _RINGBUFFER_
#define _RINGBUFFER_

#include <stdbool.h>

#define RINGBUFFER_SIZE 128

typedef struct {
	int position;
	int addPosition;
	//int length;
	char bufferArray[RINGBUFFER_SIZE];
} RingBuffer;

bool addToBuffer(RingBuffer *rb,char data);

bool getFromBuffer(RingBuffer *rb, char *data);

#endif
#include "ringbuffer.h"

bool addToBuffer(RingBuffer *rb,char data)
{
	int newPosition = 0;
	if(rb->addPosition < (RINGBUFFER_SIZE-1)) newPosition = rb->addPosition + 1;
	if(newPosition==rb->position) return false; // overflow
	rb->bufferArray[newPosition]= data;
	rb->addPosition = newPosition;
	return true;
}

bool getFromBuffer(RingBuffer *rb, char *data)
{
	if(rb->position == rb->addPosition) return false;
	int oldPosition = rb->position+1;
	rb->position = 0;
	if(oldPosition < (RINGBUFFER_SIZE-1)) rb->position = oldPosition;
	*data = rb->bufferArray[oldPosition];
	return true;
}
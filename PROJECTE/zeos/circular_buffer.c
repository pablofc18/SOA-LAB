// circular buffer 

#include <circular_buffer.h>


////////////////////////////// 
// FUNCIONS CIRCULAR BUFFER //
void initialize_circularbuffer(CircularBuffer *buff)
{
	buff->head = 0;
	buff->tail = 0;
	buff->count = 0;
}

int circularBufferIsFull(CircularBuffer *buff)
{
	if (buff->count == BUFFER_SIZE) return 1;
	else return 0;
}

int circularBufferIsEmpty(CircularBuffer *buff)
{
	if (buff->count == 0) return 1;
	else return 0;
}

int circularBufferEnqueue(CircularBuffer *buff, char value)
{
	if (circularBufferIsFull(buff)) return 0;
	buff->data[buff->tail] = value;
	buff->tail = (buff->tail + 1) % BUFFER_SIZE;
	buff->count++;
	return 1;
}

int circularBufferDequeue(CircularBuffer *buff, char *value)
{
	if (circularBufferIsEmpty(buff)) return 0;
	*value = buff->data[buff->head];
	buff->head = (buff->head + 1) % BUFFER_SIZE;
	buff->count--;
	return 1;
}
////////////////////////////// 


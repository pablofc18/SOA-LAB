// circular buffer 

#ifndef __CIRCULAR_BUFFER_H__ 
#define __CIRCULAR_BUFFER_H__

#define BUFFER_SIZE 64

//// Definicio del buffer circular //// 
typedef struct {
  char data[BUFFER_SIZE];
  int head;
  int tail;
  int count;
} CircularBuffer; 
//// //// //// //// //// //// //// ////


////////////////////////////////
// FUNCIONS CIRCULAR BUFFER   //
void initialize_circularbuffer(CircularBuffer *buff);

int circularBufferIsFull(CircularBuffer *buff);

int circularBufferIsEmpty(CircularBuffer *buff);

int circularBufferEnqueue(CircularBuffer *buff, char value);

int circularBufferDequeue(CircularBuffer *buff, char *value);
////////////////////////////////

#endif // __CIRCULAR_BUFFER_H__

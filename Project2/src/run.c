/*
run file
*/

#ifndef __USERPROG__
#define __USERPROG__

typedef struct ring_buffer {
  int numprocs;
  char * buffer;
  int head;
  int tail;
  int buffer_size;
  int full;
} ring_buffer;

#define PRODUCER_TO_RUN "producer.dlx.obj"
#define CONSUMER_TO_RUN "consumer.dlx.obj"

#endif
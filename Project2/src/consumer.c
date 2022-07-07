/*
consumer file
*/

#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"


typedef struct ring_buffer {
  int numprocs;
  char * buffer;
  int head;
  int tail;
  int buffer_size;
  int full;
} ring_buffer;


void main (int argc, char *argv[]) {
	ring_buffer *rb;        				// buffer for the shared memory
	sem_t sem_complete; 					// Semaphore to signal when process is done
	lock_t lock_process;					// lock
	uint32 handle_rb;            			// handle
	uint32 handle_buffer;					// buffer for handle
  
	int i = 0;                   			// int for counting the characters
	char string[] = "Hello World";
	int string_length = dstrlen(string);

	if (argc != 5) { 						// if the input size is not 5, throw an error and exit
		Printf("Usage: ");
		Printf(argv[0]); 
		Printf(" number\n");
		Exit();
	} 

    // Converting the cmd line strings into integers to use as references (handles)
	handle_rb = dstrtol(argv[1], NULL, 10); 		
	handle_buffer = dstrtol(argv[2], NULL, 10);
	sem_complete = dstrtol(argv[3], NULL, 10);
	lock_process = dstrtol(argv[4], NULL, 10);
  
	if ((rb->buffer = (char *)shmat(handle_buffer)) == NULL) {
		Printf("Failed to map. Exiting...");
		Exit();
	}  
  
	if ((rb = (ring_buffer *)shmat(handle_rb)) == NULL) {		
		Printf("Failed to map. Exiting...");
		Exit();
	}
	
	//While loop for consuming characters from the buffer
	while(i < string_length){
		if(lock_acquire(lock_process) == -1){			// if lock fails then exit
			Exit();
		}
	  
		else {											// else consume characters 
			Printf("Consumer %d Devoured: %c\n", Getpid(), rb->buffer[rb->head]);
			rb->head = (rb->head + 1) % rb->buffer_size;
			rb->full = 0;
			i++;
		}
    }

    if(lock_release(lock_process) == -1){				
		Exit();
    }
	
	// Wake up the calling process
	if(sem_signal(sem_complete) != 1) {
		printf("Semaphore failed. Exiting...\n");
		Exit();
	}	
}
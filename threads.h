//#include "functions.h"

struct Message;

struct thread_args{

	struct Message *message;
	int semid_read_out; //this semaphore tells when to read from shared memory a message than was sent from a process P
	int semid_write_out;
	int semid_nextSend; //these are semaphores(one for each process) which tells when a process has read a message from C and eventually continue sending new messages to C
	int processCounter; //this is the number from 0 to K of the process which reads the message from C
	char str[10];
};


void *pthread_function(void* arg);

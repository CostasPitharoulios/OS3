//#include "functions.h"

struct Message;

struct thread_outds_args{

	struct Message *message;
	int semid_read_out; //this semaphore tells when to read from shared memory a message than was sent from a process P
	int semid_write_out;
	int semid_readMatch; // this semaphore says when parent process of P can read its pid_match value
	int semid_nextSend; //these are semaphores(one for each process) which tells when a process has read a message from C and eventually continue sending new messages to C
	int processCounter; //this is the number from 0 to K of the process which reads the message from C
	int pid_match; // the number of matches each process has

	int *shared_matches; //pointer to shared memory where pid_matches are stored
};


struct thread_inds_args{
	struct Message *message;
	int semid_read_in;
	int semid_write_in;
	int semid_nextSend;
        int processCounter; //this is the number from 0 to K of the process which reads the message from C
};


struct thread_matches_args{
	int semid_readMatch;
	int *shared_matches; //pointer to shared memory of matches
	int pid_match;
	int processCounter;
};



void *pthread_outds_function(void* arg);
void *pthread_inds_function(void* arg);
void *pthread_matches_function(void* arg);

#include "threads.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "semaphores.h"
#include "functions.h"

// P processes receiving messages from process C
void *pthread_outds_function(void* argp){
    struct thread_outds_args *args = argp;
    for(;;){ // keeps trying to receive new messages from C via shared memory till an ending signal comes

        semDown(args->semid_read_out,0);
        semUp(args->semid_nextSend, args->processCounter);
        if (args->message[1].endOfMessages == 0){
	    printf("Process pid: %d, Sender pid: %s Message Received: %s\n", getpid(),args-> message[1].sender_pid,args->message[1].lineSent);
            int pidSender = atoi(args->message[1].sender_pid);
            if (getpid() ==  pidSender){
                args->pid_match++;
            }
            semUp(args->semid_write_out,0); 
        }
        else{ //ending signal has just came
            semUp(args->semid_read_out,0); //we want all the processes to read the ending signal so we unlock the semaphore for reading again and again
            break;
        }
    }
    args->shared_matches[args->processCounter] = args->pid_match; //storing pid_match value to shared memory for parent process to read it
    semUp(args->semid_readMatch, args->processCounter); // with this, parent knows when pid_match value is available to be read from shared memory
    pthread_exit("Thank you for the CPU time");

   
}

//P writing messages for process C
void *pthread_inds_function(void* argp){
    struct thread_inds_args *args = argp;
    char *randomLine; // this keeps a pointer to the random line chosen from .txt file      
    for(;;){ //keeps trying to send random lines all the time \\ ATTENTION: The loop is going to end when the whole thread gets canceld from main.c
        randomLine = pickRandomLine(); // function which returns a random line from .txt file
//	printf("RANDOM LINE: %s\n", randomLine);
	semDown(args->semid_nextSend, args->processCounter);
	semDown(args->semid_write_in,0);
	sprintf(args->message[0].lineSent,"%s",randomLine);	
	sprintf(args->message[0].sender_pid, "%d", getpid());
	semUp(args->semid_read_in,0);
    }

}

// reads pid_match value from shared memory
void *pthread_matches_function(void* argp){
    struct thread_matches_args *args = argp;
    semDown(args->semid_readMatch,args->processCounter); //this will only get locked when the process P with number processCounter has finished
    args->pid_match =args->shared_matches[args->processCounter];
    pthread_exit("Thank you for the CPU time");

}

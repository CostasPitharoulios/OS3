#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include <pthread.h>

#include <string.h>
#include <stdlib.h>

#include "functions.h"
#include "semaphores.h"
#include "threads.h"

#include <sys/ipc.h> // for shared memory
#include <sys/shm.h> // for shared memory

#define SHM_SIZE 1024 // make a 1K shared memory segment




int main(int argc, char *argv[]){

    // checks if the paremeters given are ok
    if (argc != 3){
       printf("ERROR: You gave wrong parameters!\nTip: Type something like \"./myprog 5 10\" where 5 is the number of processes and 10 the number of loops.\n");
       return -1;
    }

    int numberOfP; //this is the number of processes P that user wishes to create
    numberOfP = atoi(argv[1]);

    int numberOfMessagesK = atoi(argv[2]);  //this is the number of loops/messages that users wishes to be sent
    printf("I am creating %d processes of type P.\n", numberOfP);
    printf("The number of messagges sent from P processes to C will be K=%d\n", numberOfMessagesK);

    pid_t pid;
    int status = 0;



    //===============================================================
    // 		*** Some work for SHARED MEMORY ***
    //===============================================================
    // SOME INFO: I use two structures. One represents in-ds and the other 
    // represents out-ds. This is done using a big segment of shared memory
    // which keeps both of them.
    // Each of these two structures, accomondates a type char* wchich keeps
    // the random line we are sending from .txt file.
    //===============================================================

    int shmid_message;
    messageSent *message;  //messageSent is a struct which contains pid of sender, the line to be sent, and a flag which says when the communication is over
    key_t key;


    // creatig the key of shared memory

    if ((key = ftok("main.c", 'R')) == -1){
	perror("ftok");
	exit(1);
    }

    // creating the segment which keeps the two messageSent structures
    //shared memory is distributed in two parts ->in-ds and out-ds. //So its size should be twice as much as the size of struct Message.
    if ((shmid_message = shmget(key, sizeof(messageSent)*2, 0644 | IPC_CREAT)) == -1) {  
	perror("shmget");
	exit(1);
    }

    // attaching to the segment to get a pointer to it
    message =  shmat(shmid_message, 0,0);
    if (message == NULL){
	perror("shmat");
	exit(1);
    }

    message[1].endOfMessages = 0; //this is a flag. 0: more messages yet to come 1:end of messages

    //===============================================================
    // Now i am going to create a shared memory segment which will be able
    // to keep all the pid_match values of every process P that ends.
    // I do this, because at the end, we want parent process to calculate
    // the sum of all the pid_match values.
    //===============================================================
    int shmid_matches;
    int *shared_matches;
    //creating the segment which keeps pid_match
    if ((shmid_matches = shmget((key_t)1234, sizeof(shared_matches)*numberOfP, 0666|IPC_CREAT)) == -1){
        perror("shmget");
        exit(1);
    }

    // attaching to sthe segment to get a pointer to it
    shared_matches = shmat(shmid_matches,0,0);
        if (shared_matches == NULL){
        perror("shmat");
        exit(1);
    }

    //--------------------------------------------------------------------


    //===============================================================
    //		*** Some work for semaphores ***
    //===============================================================

    // semaphores to manage in-ds read and write of shared memory
    int sem_id_read_in = semCreate((key_t)0000,1,0); // read from shared memory in-ds
    if (sem_id_read_in < 0 ){
	printf("Error on semaphores! Abort. \n");
	exit(0);
    }
    int sem_id_write_in = semCreate((key_t)0001,1,1); // write on shared memory in-ds
    if (sem_id_write_in < 0 ){
        printf("Error on semaphores! Abort. \n");
        exit(0);
    }

    // semaphores to manage out-ds read and write of shared memory
    int sem_id_read_out = semCreate((key_t)0002,1,0); // read from shared memory out-ds
    if (sem_id_read_out < 0 ){
        printf("Error on semaphores! Abort. \n");
        exit(0);
    }
    int sem_id_write_out = semCreate((key_t)0003,1,1); // write on shared memory out-ds
    if (sem_id_write_out < 0 ){
        printf("Error on semaphores! Abort. \n");
        exit(0);
    }

    // these are numberOfP semaphores. We use them in order to prevent a process to send a new message if
    // this process has not read any message from C yet
    int sem_id_nextSend = semCreate((key_t)0004,numberOfP, 1);
    if (sem_id_nextSend < 0 ){
        printf("Error on semaphores! Abort. \n");
        exit(0);
    }

    // these are numberOfP semaphores. We use them in order to make parent process of P processes to know
    // when they are finished, in order to sum their pid_match values.
    int sem_id_readMatch = semCreate((key_t)0005, numberOfP, 0); // at first semaphore is locked
    if (sem_id_readMatch < 0 ){
        printf("Error on semaphores! Abort. \n");
        exit(0);
    }


    //===============================================================
    //		*** PROCESS C WORKS HERE ***
    //===============================================================


    // just creating the only C process
    pid = fork();
    if (pid == 0){
        printf("I just created C process with pid: %d.\n", getpid());
	// just going to use semaphores for admission to shared memory
        int j;
        for (j=0; j<numberOfMessagesK; j++){
            semDown(sem_id_read_in,0);
 	    char Sender[7];
	    sprintf(Sender, "%s", message[0].sender_pid);
	    char Line[1024];
	    sprintf(Line, "%s", message[0].lineSent);
	    semUp(sem_id_write_in,0);
	    
	    semDown(sem_id_write_out,0);
	    char* hashedMessage = str2md5(Line, strlen(Line));
	    sprintf(message[1].sender_pid, "%s", Sender);
            sprintf(message[1].lineSent, "%s", hashedMessage);
            semUp(sem_id_read_out,0);
        }
        semDown(sem_id_write_out,0);
	message[1].endOfMessages = 1; //loop has end. So an ending message must reach every process P
	semUp(sem_id_read_out,0);

        exit(0);
    }

    //===================================================================================================================================
    // 		*** PROCESSES P WORK HERE ***
    //===================================================================================================================================
    // Information: The concept here it to divide each process in two different threads.
    // The first thread is responsible for receiving messages from out-ds of shared memory.
    // The second thread of process is responsible for sending messages to in-ds of shared memory.
    // We are doing this, because we want every message that comes from C, to be able to be read from any of the P processes no matter 
    // if they (P) are stacked on a semaphore for sending messages.
    // IN OTHER WORDS: Sending and Receiving to/from shared memory are two almost independent jobs, so we have to do this independently.
    //===================================================================================================================================

    int i;
    for (i=0; i<numberOfP; i++){
	pid = fork();
	if (pid==0){ // if this is the child
 	    int pid_match = 0; //this counts how many matches is process have. Match means pid of sender == pid of process.

            //-------------------------------------------------------------------
            //		 *** STARTING THE TRHEADS WORK ***		      
            // Information: Creating the thread which is responsible for
            // 		    reading messages from out-ds of shared memory.
            //-------------------------------------------------------------------


	    pthread_t outds_thread; // we are creating a thread to manage reading messages from C process
	    struct thread_outds_args *args_out = malloc(sizeof(struct thread_outds_args)); //this is a struct which keeps the arguments we are going to send to thread function

	    // just giving values to the struct which keeps arguments for thread
	    args_out->semid_read_out = sem_id_read_out; // we are sending the semaphore which says when the thread can read a message from C
	    args_out->message = message; // we send a pointer to shared memory where messages of inds and outds are kept
	    args_out->processCounter = i; // this shows the number of the process which reads the message from out-ds in order to be able to send a new line later
	    args_out->semid_nextSend = sem_id_nextSend; // these are semaphores which say if a process has received a message and as a result if it can send a new line
	    args_out->semid_write_out = sem_id_write_out; // semaphore to manage writing to out-ds shared memory segment
	    args_out->pid_match = pid_match; // this is the nuber of matches (sender-receiver is the same)
	    args_out->shared_matches = shared_matches; // ssending a pointer to shared memory which keeps all pid_match values of processes P
	    args_out->semid_readMatch = sem_id_readMatch; // semaphore which says when parent process can read pid_match in order to sum up them all together

            int res;
	    void *thread_result;
	    res = pthread_create(&outds_thread, NULL, pthread_outds_function,args_out);
	    if (res != 0){
	        perror("Thread creation failed");
	        exit(EXIT_FAILURE);
	    }
            //------------------- End of thread for reading from out-ds creation. --------------------

            //------------------- Start of thread for writing to in-ds creation. ---------------------
            // 		Information: Creating the thread which is responsible for 
            //          writing messages to in-ds of shared memory.
            //----------------------------------------------------------------------------------------


	    pthread_t inds_thread;
	    struct thread_inds_args *args_in = malloc(sizeof(struct thread_inds_args));

	    // just giving values to the struct which keeps arguments for thread
	    args_in->semid_nextSend = sem_id_nextSend;
	    args_in->semid_write_in = sem_id_write_in;
	    args_in->message = message;
	    args_in->semid_read_in = sem_id_read_in;
	    args_in->processCounter = i;
	    
	    int res_inds;
	    void *thread_inds_result;
	    res_inds = pthread_create(&inds_thread, NULL, pthread_inds_function, args_in);
            if (res_inds != 0){
                perror("Thread creation failed");
                exit(EXIT_FAILURE);
            }
            //------------------- End of thread for writing to in-ds creation. -------------------

            //------------------------------------------------------------------------------------
	    //	   Waiting for thread out-ds of reading from out-ds to finish.
            //------------------------------------------------------------------------------------
	    // Information: when thread  of reading from shared memory ends, we know 
	    // that no other messages should be sent to shared memory.
	    // So, we cancel the thread which sends lines to C via shared memory.
	    //------------------------------------------------------------------------------------

//	    printf("Waiting for thread to finish...\n");
	    res = pthread_join(outds_thread, &thread_result); // waiting for thread to  finish
	    if (res != 0) {	    
 	        perror("Thread join failed");
	        exit(EXIT_FAILURE);
  	    }
	    printf("\nPthread of process with pid: %d haw finsished. PID_MATCH = %d\n", getpid(), args_out->pid_match);
            pthread_cancel(inds_thread);
	    free(args_out);
	    free(args_in);
//	    printf("Thread joined, it returned %s\n", (char *)thread_result);

            //------------------------------------------------------------------------------------

	    exit(0); // child exits. No other work to be done by child P.
        }
    }
    int sum_pid_match=0; //this is the sum of pid_match values of each process P
    //-----------------------------------------------------------------------------------
    //           *** STARTING THE TRHEADS WORK ***
    // Information: Creating as many threads as the number of P processes.
    //              Each thread is responsible to read the pid_match value 
    //              of one process P in order for parent to sum up values.
    //-----------------------------------------------------------------------------------

    // *** WE ARE IN THE PARENT NOW ***//


    int z;
    for (z=0; z<numberOfP; z++){
        pthread_t matches_thread;
        struct thread_matches_args *args_matches = malloc(sizeof(struct thread_matches_args));          


        args_matches->semid_readMatch = sem_id_readMatch; //semaphore for reading matches
        args_matches->shared_matches = shared_matches; //pointer to shared memory of matches
        args_matches->processCounter = z; //number of process Po

        int res_match;
        void *thread_result_match;
        res_match = pthread_create(&matches_thread,NULL, pthread_matches_function, args_matches);
        if (res_match != 0){
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }

        //waiting for thread to finish
        res_match = pthread_join(matches_thread, &thread_result_match);
        if (res_match != 0) {         
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
        sum_pid_match += args_matches->pid_match;
        free(args_matches);
    }

    //===============================================================
    pid_t wpid;
    while ((wpid = wait(&status))>0);
    printf("\n\n\nI am the parent and i am exiting. Information for program:\n-------------------------------------------------------------\nNumber of processes P: %d \nNumber of loops K: %d \nSum of pid_match values: %d\n",numberOfP,numberOfMessagesK,sum_pid_match);
    //===============================================================
    //       **** DETACHING AND DELETING SHARED MEMORY ***
    //===============================================================

    // detaching from the segment

    if (shmdt(message) == -1){
        perror("shmdt");
	exit(1);
    }

    // removing shared memory
    if (shmctl(shmid_message, IPC_RMID, NULL) == -1){
	perror("shmctl failed");
	exit(1);
    }


    //detaching shared memory for pid_matches
    if (shmdt(shared_matches) == -1){
        perror("shmdt");
        exit(1);
    }

    // removing from shared memory
    if (shmctl(shmid_matches, IPC_RMID, NULL) == -1){
        perror("shmctl failed");
        exit(1);
    }


    //===============================================================
    //       **** DELETING ALL SEMAPHORES ***
    //===============================================================
    semDelete(sem_id_read_in);
    semDelete(sem_id_write_in);

    semDelete(sem_id_read_out);
    semDelete(sem_id_write_out);

    semDelete(sem_id_nextSend);
    semDelete(sem_id_readMatch);

}

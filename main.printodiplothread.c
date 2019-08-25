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
    // SOME INFO: I use to struxtures. One represents in-ds and the other 
    // represents out-ds. This is done using a big segment of shared memory
    // which keeps both of them.
    // Each of these two structures, accomondates a type char* wchich keeps
    // the random line we are sending from .txt file.
    // But...for both char* (of the two structs) we allocate two different
    // shared memories and inside the structures instead of keeping char* we keep
    // the shared memory IDs as returned by shmget().
    //===============================================================

    int shmid;
    messageSent *message;  //messageSent is a struct which contains pid of sender, the line to be sent, and a flag which says when the communication is over
    //--------------------------------------------------------------------
    key_t key;


    // creatig the key of shared memory

    if ((key = ftok("main.c", 'R')) == -1){
	perror("ftok");
	exit(1);
    }

    // creating the segment which keeps the two messageSent structures
    //shared memory is distributed in two parts ->in-ds and out-ds. //So its size should be twice as much as the size of struct Message.
    if ((shmid = shmget(key, sizeof(messageSent)*2, 0644 | IPC_CREAT)) == -1) {  
	perror("shmget");
	exit(1);
    }

    // attaching to the segment to get a pointer to it
    message =  shmat(shmid, 0,0);
    if (message == NULL){
	perror("shmat");
	exit(1);
    }

    message[1].endOfMessages = 0; //this is a flag. 0: more messages yet to come 1:end of messages

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



    //===============================================================
    //		*** PROCESS C WORKS HERE ***
    //===============================================================


    // just creating the only C process
    pid = fork();
    if (pid == 0){
        printf("I just created C process with pid: %d.\n", getpid());
	//sleep(3);
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
        printf("HI! I am C now i am writing the END MESSAGE to shared memory\n\n\n");
	message[1].endOfMessages = 1;
	semUp(sem_id_read_out,0);
	
        printf("EXITING C PROCESS!!!\n\n\n");
        exit(0);
    }

    //===================================================================================================================================
    // 		*** PROCESSES P WORK HERE ***
    //===================================================================================================================================
    // Information: The concept here it to divide each process in two different threads.
    // One thread created by us with <pthread.h> and the default thread that the each process has.
    // The thread created by us is responsible for receiving messages from out-ds of shared memory.
    // The default thread of process is responsible for sending messages to in-ds of shares memory.
    // We are doing this, because we want every message that comes from C, to be able to be read from all the P processes no matter 
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
	    struct thread_args *args = malloc(sizeof(struct thread_args)); //this is a struct which keeps the arguments we are going to send to thread function

	    args->semid_read_out = sem_id_read_out; // we are sending the semaphore which says when the thread can read a message from C
	    args->message = message; // we send a pointer to shared memory where messages of inds and outds are kept
	    args->processCounter = i; // this shows the number of the process which reads the message from out-ds in order to be able to send a new line later
	    args->semid_nextSend = sem_id_nextSend; // these are semaphores which say if a process has received a message and as a result if it can send a new line
	    args->semid_write_out = sem_id_write_out; // semaphore to manage writing to out-ds shared memory segment
	    args->pid_match = pid_match;

            int res;
	    void *thread_result;
	    res = pthread_create(&outds_thread, NULL, pthread_function,args);
	    if (res != 0){
	        perror("Thread creation failed");
	        exit(EXIT_FAILURE);
	    }
            //------------------- End of thread creation. -------------------


	    char *randomLine; // this keeps a pointer to the random line chosen from .txt file	    

	    for(;;){
printf("ARXHHHHMetra na deis an einai 10 fores ARXHHH\n\n\n\n\n");
 	        randomLine = pickRandomLine(); // function which returns a random line from .txt file
// if (message[1].endOfMessages == 1) printf("BEFORE nextSend, THELW NA EMFANISTEI X10-11\n\n\n\n");
                // *** going to use semaphores for admission to shared memory
		semDown(sem_id_nextSend,i); // this process can not send a new message till it reads a message from C
  /*              if (message[1].endOfMessages == 1){
		    printf("BREAKING: END OF LOOPS FOR PROCESS WITH PID: %d\n\n", getpid());
	            break;
	        }*/ //AXRHSTO EDW POU EINAI
	        semDown(sem_id_write_in,0);
  	        sprintf(message[0].lineSent,"%s",randomLine);
	        sprintf(message[0].sender_pid, "%d", getpid());
  	        semUp(sem_id_read_in,0);
		printf("Metra na deis an einai 10 fores\n\n\n\n\n");
//	    printf("wooooow:I AM PROCESS WITH PID: %d I JUST RECEIVED THIS: %s SENDER's PID WAS: %s\n\n\n\n", getpid(),message[1].lineSent, message[1].sender_pid);
            }//end of for
	    printf("END FOR\n\n\n\n\n\n\n");


            //-------------------------------------------------------------------
	    //		Waiting for thread to finish.
            //-------------------------------------------------------------------

	    printf("Waiting for thread to finish...\n");
	    res = pthread_join(outds_thread, &thread_result);
	    if (res != 0) {	    
 	        perror("Thread join failed");
	        exit(EXIT_FAILURE);
  	    }
	    printf("Thread joined, it returned %s\n", (char *)thread_result);
	    printf("CHILD EXITSSSS!!!\n\n\n");
	    exit(0); // child exits. No other work to be done by child P.
        }
    }

    //===============================================================
//    for (i=0; i<= numberOfP; i++) //this is for the parent in order to wait for all children end
//        wait(NULL);
    pid_t wpid;
    while ((wpid = wait(&status))>0){
        printf("HMMMM %d\n\n\n", wpid);
    }
    printf("I am the parent and i am exiting.\n");

    //===============================================================
    //       **** DETACHING AND DELETING SHARED MEMORY ***
    //===============================================================

    // detaching from the segment

    if (shmdt(message) == -1){
        perror("shmdt");
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

}

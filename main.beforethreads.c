#include <sys/types.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


#include <string.h>
#include <stdlib.h>

#include "functions.h"
#include "semaphores.h"

#include <sys/ipc.h> // for shared memory
#include <sys/shm.h> // for shared memory

#define SHM_SIZE 1024 // make a 1K shared memory segment


int main(int argc, char *argv[]){
printf("HELLOOO\n\n\n\n");    
    int numberOfP;
    numberOfP = atoi(argv[1]);
    printf("I am creating %d processes of type P.\n", numberOfP);

    pid_t pid;

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

    int shmid;/*,lineData0_shmid, lineData1_shmid*/
    messageSent *message;
//    char* lineData0; // in - ds
//    char* lineData1; // out - ds
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
#if 0
/*
    //--------------------------------------------------------------------
    // creating segments to keep char* lineSent

    if ((lineData0_shmid = shmget((key_t)1111, SHM_SIZE, 0666|IPC_CREAT)) == -1){
        perror("shmget");
        exit(1);
    }

    lineData0 = shmat(lineData0_shmid,0,0);
    if (lineData0 == NULL){
	perror("shmget");
	exit(1);
    }
    //--------- and ---------
    if ((lineData1_shmid = shmget((key_t)2222, SHM_SIZE, 0666|IPC_CREAT)) == -1){
        perror("shmget");
        exit(1);
    }

    lineData1 = shmat(lineData1_shmid,0,0);
    if (lineData1 == NULL){
        perror("shmget");
        exit(1);
    }

    // connecting with structs

    message[0].lineSent_id = lineData0_shmid;
    message[1].lineSent_id = lineData1_shmid;*/
#endif

    //--------------------------------------------------------------------

//    char* check = "SKAT";
//    char* check1 = "KAL";
//    sprintf(message[0].sender_pid, "%s", check);
//    sprintf(message[1].sender_pid, "%s", check1);
//    strncpy(data[0].sender_pid, check, sizeof(d));
//    printf("SEE: %s\n\n\n\n", message[0].sender_pid);
//    printf("SEE: %s\n\n\n\n", message[1].sender_pid);

    
//    sprintf(message[0].lineSent, "%s", check1);
//    printf("PRINTREEE: %s\n\n\n\n", message[0].lineSent);

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
	sleep(3);

	// just going to use semaphores for admission to shared memory
int j;
for (j=0; j<5; j++){
	semDown(sem_id_read_in,0);
 	    printf("I AM C AND READ IS LOCKED.I AM C AND A I HAVE READ SEGMENT: %s\n", message[0].lineSent);
	    	semDown(sem_id_write_out,0);
	    char Sender[7];
	    sprintf(Sender, "%s", message[0].sender_pid);
	    char Line[1024];
	    sprintf(Line, "%s", message[0].lineSent);
//	    char* hashedMessage = str2md5(message[0].lineSent, strlen(message[0].lineSent));
//	    printf("Hashed: %s\n", hashedMessage);
	    //semUp(sem_id_read_out,0);
	semUp(sem_id_write_in,0);
	char* hashedMessage = str2md5(Line, strlen(Line));
	sprintf(message[1].sender_pid, "%s", Sender);
        sprintf(message[1].lineSent, "%s", hashedMessage);
            	semUp(sem_id_read_out,0);
	printf("Semaphore WRITE is UNLOCKED from C\n");
}
        exit(0);
    }

    //===============================================================
    // 		*** PROCESSES P WORK HERE ***
    //===============================================================

    int i;
    for (i=0; i<numberOfP; i++){
	pid = fork();
	if (pid==0){ // if this is the child
            printf("Just created son with pid: %d from parent with pid %d.\n", getpid(), getppid());

	    char *randomLine; // this keeps a pointer to the random line chosen from .txt file
	    
	    randomLine = pickRandomLine(); // functio which returns a random line from .txt file

            // *** going to use semaphores for admission to shared memory
	    printf("Son with pid: %d just picked the line: %s.\n\n", getpid(), randomLine);
//		    semDown(sem_id_nextSend,i); // this process can not send a new message till it reads a message from C
	 semDown(sem_id_write_in,0);

	    printf("Semaphore WRITE is locked from process with pid: %d\n", getpid());
	    //strncpy(data->lineSent, randomLine, sizeof(data->lineSent));
	    sprintf(message[0].lineSent,"%s",randomLine);
///	    int int_pid = getpid();
///	    char Sender[7];
///	    itoa(int_pid, Sender,10);
	    sprintf(message[0].sender_pid, "%d", getpid());
//	    message[0].sender_pid = getpid();

	 semUp(sem_id_read_in,0);
                semDown(sem_id_read_out,0);
	    printf("wooooow:I AM PROCESS WITH PID: %d I JUST RECEIVED THIS: %s SENDER's PID WAS: %s\n\n\n\n", getpid(),message[1].lineSent, message[1].sender_pid);
//		    semUp(sem_id_nextSend,i); //this process has read a message from C so it can send a new one
	        semUp(sem_id_write_out,0);
//	 semUp(sem_id_read_in,0);
	    printf("Semaphore READ id unlocked from process with pid: %d\n", getpid());

	    

	    exit(0); // child exits. No other work to be done by child P.
        }
    }


    //===============================================================


    for (i=0; i<= numberOfP; i++) //this is for the parent in order to wait for all children end
        wait(NULL);
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

}

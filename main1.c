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

//#define SHM_SIZE 1024 // make a 1K shared memory segment


int main(int argc, char *argv[]){
printf("HELLOOO\n\n\n\n");    
    int numberOfP;
    numberOfP = atoi(argv[1]);
    printf("I am creating %d processes of type P.\n", numberOfP);

    pid_t pid;

    //===============================================================
    // 		*** Some work for SHARED MEMORY ***
    //===============================================================

    key_t key;
    int shmid;
    Message *data;
    int mode;

    // creatig the key of shared memory

    if ((key = ftok("main.c", 'R')) == -1){
	perror("ftok");
	exit(1);
    }

    // creating the segment

    //shared memory is distributed in two parts ->in-ds and out-ds. //So its size should be twice as much as the size of struct Message.
    if ((shmid = shmget(key, sizeof(Message), 0644 | IPC_CREAT)) == -1) {  
	perror("shmget");
	exit(1);
    }

    // attaching to the segment to get a pointer to it
    data = (Message*) shmat(shmid, NULL,0);
    if (data == NULL){
	perror("shmat");
	exit(1);
    }


    //===============================================================
    //		*** Some work for semaphores ***
    //===============================================================

    int sem_id_read = semCreate((key_t)0000,1,0); // read from shared memory
    if (sem_id_read < 0 ){
	printf("Error on semaphores! Abort. \n");
	exit(0);
    }
    int sem_id_write = semCreate((key_t)0001,1,1); // write on shared memory
    if (sem_id_write < 0 ){
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
	semDown(sem_id_read,0);
 	    printf("I AM C AND READ IS LOCKED.I AM C AND A I HAVE READ SEGMENT: %s\n", data->lineSent);
/////	    char* hashedMessage = str2md5(data[0].lineSent, strlen(data[0].lineSent));
/////	    printf("Hashed: %s\n", hashedMessage);
	semUp(sem_id_write,0);
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
	    semDown(sem_id_write,0);
	    printf("Semaphore WRITE is locked from process with pid: %d\n", getpid());
	    strncpy(data->lineSent, randomLine, sizeof(data->lineSent));
	    semUp(sem_id_read,0);
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

    if (shmdt(data) == -1){
        perror("shmdt");
	exit(1);
    }

    //===============================================================
    //       **** DELETING ALL SEMAPHORES ***
    //===============================================================
    semDelete(sem_id_read);
    semDelete(sem_id_write);


}

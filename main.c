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

    key_t key;
    int shmid;
    char *data;
    int mode;

    // creatig the key of shared memory

    if ((key = ftok("main.c", 'R')) == -1){
	perror("ftok");
	exit(1);
    }

    // creating the segment

    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
	perror("shmget");
	exit(1);
    }

    // attaching to the segment to get a pointer to it
    data = shmat(shmid, (void*)0,0);
    if (data == (char*)(-1)){
	perror("shmat");
	exit(1);
    }

    //just writing something for testing
//    printf("writing to segment: \"%s\"\n", "Hello my name is Costas"); 
//    strncpy(data, "Hello my name is Costas\n", SHM_SIZE);

    //===============================================================
    //		*** Some work for semaphores ***
    //===============================================================

    int sem_id_read; // read from shared memory
    int sem_id_write; // write on shared memory

    sem_id_read = semget((key_t)1234, 1, 0666|IPC_CREAT);
    sem_id_write = semget((key_t)1234, 1, 0666|IPC_CREAT);

    if (!set_semvalue(sem_id_read)){
	fprintf(stderr, "Failed to initialize semaphore for read\n"); 
	exit(EXIT_FAILURE);
    }

    if (!set_semvalue(sem_id_write)){
	fprintf(stderr, "Failed to initialize semaphore for write\n"); 
	exit(EXIT_FAILURE);
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
	if (!semaphore_v(sem_id_read)) exit(EXIT_FAILURE);
 	    printf("I AM C AND A I HAVE READ SEGMENT: %s\n", data);
//	    sleep(5);
	if (!semaphore_v(sem_id_write)) exit(EXIT_FAILURE);

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
//	    printf("segment contains: \"%s\n", data);

	    char *randomLine; // this keeps a pointer to the random line chosen from .txt file
	    randomLine = pickRandomLine(); // functio which returns a random line from .txt file

            // *** going to use semaphores for admission to shared memory
	    printf("Son with pid: %d just picked the line: %s.\n\n", getpid(), randomLine);
	    if (!semaphore_p(sem_id_write)) exit(EXIT_FAILURE);
//	    printf("writing to segment: %s", randomLine); 
	    printf("Semaphore is down from process with pid: %d\n", getpid());
	    strncpy(data, randomLine, SHM_SIZE);
	    sleep(3);
	    if (!semaphore_p(sem_id_read)) exit(EXIT_FAILURE);

	    exit(0); // child exits. No other work to be done by child P.
        }
    }


    //===============================================================


    for (i=0; i< numberOfP; i++) //this is for the parent in order to wait for all children end
        wait(NULL);
    printf("I am the parent and i am exiting.\n");

    //===============================================================
    // last work for shared memory
    //===============================================================

    // detaching from the segment

    if (shmdt(data) == -1){
        perror("shmdt");
	exit(1);
    }

    //===============================================================


}

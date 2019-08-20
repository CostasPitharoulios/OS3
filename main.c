#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>


#include <sys/ipc.h> // for shared memory
#include <sys/shm.h> // for shared memory

#define SHM_SIZE 1024 // make a 1K shared memory segment

int main(int argc, char *argv[]){
    
    int numberOfP;
    numberOfP = atoi(argv[1]);
    printf("I am creating %d processes of type P.\n", numberOfP);

    pid_t pid;

    //===============================================================
    // just some work for shared memory
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
    printf("writing to segment: \"%s\"\n", "Hello my name is Costas"); 
    strncpy(data, "Hello my name is Costas\n", SHM_SIZE);


    //===============================================================
    
    int i;
  
    // just creating the only C process
    pid = fork();
    if (pid == 0){
        printf("I just created C process with pid: %d.\n", getpid());
        exit(0);
    }


    for (i=0; i<numberOfP; i++){
	pid = fork();
	if (pid==0){ // if this is the child
            printf("Just created son with pid: %d from parent with pid %d.\n", getpid(), getppid());
	    printf("segment contains: \"%s\n", data);
	    exit(0);
        }
    }




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

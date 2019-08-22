// ATTENTION: The following functions are right from my solution of homeword 1

#include "semaphores.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>

int semCreate(key_t key, int nsems, int val){
    int semid,i;
    union semun arg;
    int error;

    if ((key < 0) || (nsems <= 0))
        return -1;

    semid = semget(key, nsems, 0666|IPC_CREAT);

    if (semid < 0)
        return -1;

	arg.val = val;
    for(i = 0; i < nsems; i++){

        error = semctl(semid,i,SETVAL,arg);
        if (error < 0)
            return -1;
    }

    return semid;
}

int semDown(int semid,int sem_num){
    struct sembuf sb;

    if((semid < 0) || (sem_num < 0))
        return -1;

    sb.sem_num = sem_num;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    return semop(semid,&sb,1);
}

int semUp(int semid, int sem_num){
    struct sembuf sb;

    if((semid < 0) || (sem_num < 0))
        return -1;
    sb.sem_num = sem_num;
    sb.sem_op = 1;
    sb.sem_flg = 0;

    return semop(semid,&sb,1);
}

int semDelete(int semid){

    if(semid < 0)
        return -1;

    return semctl(semid,0,IPC_RMID);
}




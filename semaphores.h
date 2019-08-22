

union semun{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

#include <sys/types.h>

// Get and initialize semaphores set
int semCreate(key_t,int,int);

//Wait Sem
int semDown(int,int);
//Signal Sem
int semUp(int,int);
// Delete semaphore
int semDelete(int);

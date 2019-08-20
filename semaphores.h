union semun{
    int val; 
    struct semid_ds *buf;
    unsigned short *array;
};

int set_semvalue(int);
void del_semvalue(int);
int semaphore_p(int);
int semaphore_v(int);

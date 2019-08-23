#include "threads.h"
#include <stdio.h>
#include "semaphores.h"
#include "functions.h"

void *pthread_function(void* argp){
   struct thread_args *args = argp;
for(;;){
   semDown(args->semid_read_out,0);
   semUp(args->semid_nextSend, args->processCounter);
   if (args->message[1].endOfMessages == 0){
       printf("wooooow:I AM PROCESS NOD: %d I JUST RECEIVED THIS: %s SENDER's PID WAS: %s\n\n\n\n", /*args->processCounter*/getpid(),args->message[1].lineSent,args-> message[1].sender_pid);
       semUp(args->semid_write_out,0); 
   }
   else{
       semUp(args->semid_read_out,0);
       printf("WARNINGGGGGGGGGI AM PROCESS with PID: %d I JUST RECEIVED END MESSAGE: %s\n\n\n\n", /*args->processCounter*/getpid());
       break;
   }
}


//   printf("thread function is running. Argument is %d and %s.\n", args->semid_read_out, args->str);
//   sleep(2);
//   printf("BYEEEE!! thread function is running. Argument is %d and %s.\n", args->semid_read_out, args->str);
   
}

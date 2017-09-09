/*
 * shm-client - client program to demonstrate shared memory.
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
 #include <string.h>

#define MAXPIDDIGITS 11

int main(int argc, char* argv[])
{
    int shmid;
    key_t key;
    char *shm, *s;
    char * current;
    int pid;
    sem_t * sem;
    char semKey[MAXPIDDIGITS];
    
    if(argc<=1)
    {
        printf("arguments missing\n");
        exit(1);
    }else{
        key=atoi(argv[1]);
    }
 
    if ((shmid = shmget(key, 4000, 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    semKey[0]= '/';
    semKey[1]=0;

    strcat(semKey,argv[1]);

    printf("la clave del semaforo es%s\n",semKey );

    sem = sem_open (semKey, O_CREAT, 0644, 1); 

    if(sem==SEM_FAILED){
        printf("error\n");
    }
    
    sem_unlink (semKey); 

    current=shm + 1;
    while(shm[0]!=0 || *current!=0){

        //if mutex unlocked and *current != 0 then lock mutex and finish printing then unlock mutex
        sem_wait(sem);
        if(*current!= 0 )
        {   
            putchar(*current);
            current++;
        }
        sem_post(sem);
    }

    if (shmctl(shmid, IPC_RMID, NULL) < 0){
        perror("shmctl");
        exit(1);
    }

    exit(0);


}

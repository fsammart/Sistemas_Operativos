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
#define PROCESSING 1
#define END_PROCESSING 2
#define TRUE 1
#define FALSE 0


char * openSharedMemory(key_t key)
{
    int shmid;
    char * shm;

    if((shmid = shmget(key, 4000, 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) - 1)
    {
        perror("shmat");
        exit(1);
    }

    return shm;

}

sem_t * openSemaphore(char * s)
{
    sem_t * sem;
    char * semKey= malloc(MAXPIDDIGITS * sizeof(char));

    semKey[0]= '/';
    semKey[1]=0;

    strcat(semKey,s);


    sem = sem_open (semKey, O_CREAT, 0644, 1); 

    if(sem==SEM_FAILED){
        printf("error\n");
    }
    
    sem_unlink (semKey); 

    return sem;
}

void print( char * current)
{
    while(*current!=0)
        {             
            putchar(*current);
            current++;
        }
}

int main(int argc, char* argv[])
{
    int shmid;
    key_t key;
    char *shm, *s;
    char * current;
    int pid;
    sem_t * sem;
    char semKey[MAXPIDDIGITS];
    int appProcessEnded= TRUE;
    
    if(argc<=1)
    {
        printf("arguments missing\n");
        exit(1);
    }else{
        key=atoi(argv[1]);
    }


    shm=openSharedMemory(key);

    sem = openSemaphore(argv[1]);

    current=shm + 1;

    while(shm[0]==PROCESSING)
      {  

        appProcessEnded=FALSE;

        sem_wait(sem);

        print(current);

        shm[1]=0;
        current=shm +1;
        
        sem_post(sem);
    }

    if(appProcessEnded)
    {
        print(current);
    }

    if (shmctl(shmid, IPC_RMID, NULL) < 0){
        perror("shmctl");
        exit(1);
    }

    exit(0);


}

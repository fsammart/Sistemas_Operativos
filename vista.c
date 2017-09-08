/*
 * shm-client - client program to demonstrate shared memory.
 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
 #include <stdlib.h>

int main(int argc, char* argv[])
{
    int shmid;
    key_t key;
    char *shm, *s;
    char * current;
    int pid;
    if(argc<=1)
    {
        printf("Por favor ingrese PID del proceso\n");
        scanf("%d", &pid);
        printf("Su pid es %d\n", pid);
        key=pid;
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

    current=shm + 1;
    while(shm[0]!=0 || *current!=0){
        if(*current!= 0 )
        {   
            putchar(*current);
            current++;
        }
    }

    exit(0);
}

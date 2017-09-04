#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define SONS 3

void startListening(int fdread, int fdwrite, int son)
{
	char buff[152];
	char num[154];
	char command[100];
	int readbytes;
	char aux[1000];

	strcpy(command,"md5sum");
	command[6] = ' ';
	command[7] = 0;


	while((readbytes = read(fdread,buff,152))>0)
	{	
		command[7]=0;
		sprintf(num,"%d",son);	
		printf("I am your son number %d and I received the file: %s\n", son,buff);
		strcat(command,buff);
		FILE *file = popen(command,"r");	
		fgets(aux,1000,file);	
		strcat(num,aux);
		write(fdwrite,num,strlen(num)+1);		
				
	}

}


char * createSharedMemory(key_t key)
{

    int shmid;
    char * shm;

    printf("%d\n",key );

    if ((shmid = shmget(key, 4000, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }
    return shm;

}

void initializePipes(int pipearr[][2], int q)
{
	for(int i = 0; i < q; i++)
	{
		if(pipe(pipearr[i]) < 0)
		{
			perror("pipe");
			exit(1);
		}
	}

}

void closeUnnecessaryEndsChild(int pipearr[2], int tofather[2])
{
		close(pipearr[1]);
		close(tofather[0]);
}

void closeUnnecesaryEndsFather(int tofather[2], int son[2])
{
	close(tofather[1]);
	close(son[0]);
}

void allocatingNewFile(int pipearr[2], char* file, int length, int son)
{
	printf("Allocating new file %s to son number %d \n", file, son);
	write(pipearr[1], file, length);
}

int main(int argc, char* argv[])
{
	pid_t pids[SONS];
	int pid;
	int pipearr[SONS + 1][2];
	char buff[152];
	int workDoneByEach[SONS] = {1, 1, 1};
	double minForEach = (double)(argc - 1) / SONS;
	int overload = argc - 1 - (int)minForEach * SONS;
	char *shm, *s;


	initializePipes(pipearr, SONS + 1);

	for(int k = 0; k < SONS; k++)
	{

		pid = fork();
		if(pid == 0)
		{
			closeUnnecessaryEndsChild(pipearr[k], pipearr[SONS]);
			startListening(pipearr[k][0], pipearr[SONS][1], k);
			
		}
		else
		{
			if (pid < 0)
			{
				perror("fork");
				exit (1);
			}

			closeUnnecesaryEndsFather(pipearr[SONS], pipearr[k]);
			pids[k] = pid;
		
		}
		
	}


	shm=createSharedMemory(getpid());
	shm[0]=1;
	s=shm +1;

	int m=1;

	for(; m < 4 && m < argc; m++)
	{	
		write(pipearr[m-1][1], argv[m], strlen(argv[m])+1);
		
	}

	for(;m<argc; m++)
	{
		read(pipearr[SONS][0],buff,100);
		strcat(s, buff);

		if(workDoneByEach[buff[0] - '0'] < (int)minForEach)
		{
			allocatingNewFile(pipearr[buff[0] - '0'], argv[m], strlen(argv[m]) +1, buff[0] - '0');
			workDoneByEach[buff[0] - '0']++;

		}
		else
		{
			if((overload > 0 )&& (workDoneByEach[buff[0] - '0'] == (int)minForEach))
			{
				allocatingNewFile(pipearr[buff[0] - '0'], argv[m], strlen(argv[m]) +1, buff[0] - '0');
				workDoneByEach[buff[0] - '0']++;
				overload--;
			}
		}
	}

	for(int h = 1; h<argc && h<SONS +1; h++)
	{	
		read(pipearr[SONS][0],buff,100);
		strcat(s, buff);
		//printf("Hash: %s\n", buff+1);
		
	}
	shm[0]=0;
    
}
	



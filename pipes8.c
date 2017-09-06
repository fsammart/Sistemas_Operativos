#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define SONS 3
#define READ_END 0
#define WRITE_END 1
#define BUFF_MAX 152
#define AUX_MAX 1000
#define COMMAND_MAX 100
#define CHILD_NUMBER buff[0]-'0'

void startListening(int fdread, int fdwrite, int son)
{
	char buff[BUFF_MAX];
	char num[BUFF_MAX];
	char command[COMMAND_MAX];
	int readbytes;
	char aux[AUX_MAX];

	strcpy(command, "md5sum");
	command[6] = ' ';
	command[7] = 0;

	while((readbytes = read(fdread,buff,BUFF_MAX)) > 0)
	{	
		command[7] = 0;
		sprintf(num, "%d", son);	
		printf("I am your son number %d and I received the file: %s\n", son, buff);
		strcat(command, buff);
		FILE * file = popen(command, "r");	
		fgets(aux, AUX_MAX, file);	
		strcat(num, aux);
		write(fdwrite, num, strlen(num) + 1);					
	}

}


char * createSharedMemory(key_t key)
{

    int shmid;
    char * shm;

    printf("%d\n", key);

    if ((shmid = shmget(key, 4000, IPC_CREAT | 0666)) < 0)
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

void initializePipes(int pipearr[][2], int q)
{
	for(int i = 0; i < q; i++)
	{
		if(pipe(pipearr[i]) < 0)
		{
			fprintf(stderr, "Could not create pipe.\n");
			exit(1);
		}
	}

}


void allocatingNewFile(int pipearr[2], char * file, int length, int son)
{
	printf("Allocating new file %s to son number %d \n", file, son);
	write(pipearr[WRITE_END], file, length);
}


void send(int fd, char * file, int length) {

	int written = 0;

	while(written < length)
		written += write(
			fd, file + written, length - written
		);
}


int main(int argc, char * argv[])
{
	pid_t pids[SONS];
	int pid;
	int pipearr[SONS + 1][2];
	char buff[BUFF_MAX];
	int workDoneByEach[SONS] = {1, 1, 1};
	int minForEach = (argc - 1) / SONS;
	int overload = argc - 1 - minForEach * SONS;
	char * shm, * s;
	int jobNumber;


	initializePipes(pipearr, SONS + 1);
	
	for(int k = 0; k < SONS; k++)
	{

		pid = fork();
		if(pid == 0)
		{
			close(pipearr[SONS][READ_END]);
			close(pipearr[k][WRITE_END]);	
			startListening(pipearr[k][READ_END], pipearr[SONS][WRITE_END], k);	
			
			
		}
		else
		{
			if (pid < 0)
			{
				fprintf(stderr, "Could not fork.\n");
				exit(1);
			}
			
			close(pipearr[k][READ_END]);
			pids[k] = pid;
		
		}
		
	}

	close(pipearr[SONS][WRITE_END]);
	shm = createSharedMemory(getpid());
	shm[0] = 1;
	s = shm + 1;
	for(jobNumber = 1; jobNumber < SONS + 1 && jobNumber < argc; jobNumber++)
	{	
		send(pipearr[jobNumber - 1][WRITE_END], argv[jobNumber], strlen(argv[jobNumber]) + 1);
		
	}

	for(;jobNumber < argc; jobNumber++)
	{
		read(pipearr[SONS][READ_END], buff, BUFF_MAX);
		strcat(s, buff);

		if(workDoneByEach[CHILD_NUMBER] < minForEach)
		{
			allocatingNewFile(pipearr[CHILD_NUMBER], argv[jobNumber], strlen(argv[jobNumber]) + 1,CHILD_NUMBER);
			workDoneByEach[CHILD_NUMBER]++;

		}
		else
		{
			if((overload > 0 )&& (workDoneByEach[CHILD_NUMBER] == minForEach))
			{
				allocatingNewFile(pipearr[CHILD_NUMBER], argv[jobNumber], strlen(argv[jobNumber]) + 1, CHILD_NUMBER);
				workDoneByEach[CHILD_NUMBER]++;
				overload--;
			}
		}
	}

	for(int h = 1; h < argc && h < SONS + 1; h++)
	{	
		read(pipearr[SONS][READ_END], buff, BUFF_MAX);
		strcat(s, buff);
		//printf("Hash: %s\n", buff+1);
		
	}
	shm[0] = 0;
    
}
	



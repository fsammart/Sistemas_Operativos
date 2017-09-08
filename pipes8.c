#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#define SONS 3
#define READ_END 0
#define WRITE_END 1
#define BUFF_MAX 152
#define AUX_MAX 1000
#define COMMAND_MAX 100
#define TRUE 1
#define FALSE 0

void startListening(int fdread, int fdwrite, int son);
char * createSharedMemory(key_t key);
void initializePipes(int pipearr[][2], int q);
void allocatingNewFile(int pipearr[2], char * file, int length, int son);
void send(int fd, char * file, int length);
void terminateSons(int pipearr[][2]);
void ditributeJobs(int sons, int  pipearr[][2], char * s, int argc, char * argv[]);



void startListening(int fdread, int fdwrite, int son)
{
	char buff[BUFF_MAX];
	char num[BUFF_MAX];
	char command[COMMAND_MAX];
	int readbytes;
	char aux[AUX_MAX];
	char flag=TRUE;

	strcpy(command, "md5sum");
	command[6] = ' ';
	command[7] = 0;

	while(flag && (readbytes = read(fdread,buff,BUFF_MAX)) > 0)
	{	
		if(buff[0]!=0)
		{
			printf("he leido %d\n", readbytes);
			command[7] = 0;
			sprintf(num, "%d", son);	
			printf("I am your son number %d and I received the file: %s\n", son, buff);
			strcat(command, buff);
			FILE * file = popen(command, "r");	
			fgets(aux, AUX_MAX, file);	
			strcat(num, aux);
			write(fdwrite, num, strlen(num) + 1);
			printf("mande %d digitos \n", strlen(num));
		}else{
			flag=FALSE;
		}					
	}

}


char * createSharedMemory(key_t key)
{

    int shmid;
    char * shm;

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


void send(int fd, char * file, int length) 
{

	int written = 0;

	while(written < length)
		written += write(
			fd, file + written, length - written
		);
}

void terminateSons(int pipearr[][2])
{
	int status;
	int i;
	char  end[1];
	
	end[0] = 0;

	for(i = 0 ; i < SONS; i++){

		send(pipearr[i][WRITE_END], end, 1);
		printf("hijo %d\n", i);

		if (wait(&status) >= 0)
		{
			if (WIFEXITED(status))
			{
				/* Child process exited normally, through `return` or `exit` */
				printf("Child process exited with %d status\n", WEXITSTATUS(status));

			}
		}
	}
}

int * initializeArray(int sons)
{
	int i;
	int * array;

	array= malloc(sons*sizeof(int));

	for (i = 1; i <= sons; ++i)
	{
		array[i]=1;
	}

	return array;
}

void createSonProcesses(int sons, int  pipearr[][2], pid_t pids[])
{	

	int pid;	

	for(int k = 0; k < sons; k++)
	{

		pid = fork();
		if(pid == 0)
		{
			close(pipearr[sons][READ_END]);
			close(pipearr[k][WRITE_END]);	
			startListening(pipearr[k][READ_END], pipearr[SONS][WRITE_END], k);
			exit(0);
			
			
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
}

void ditributeJobs(int sons, int  pipearr[][2], char * s, int argc, char * argv[])
{
	int jobNumber;
	char buff[BUFF_MAX];
	int * workDoneByEach;
	int minForEach;
	int overload ;
	int child_number;


	minForEach = (argc - 1) / SONS;
	overload = argc - 1 - minForEach * SONS;

	workDoneByEach = initializeArray(SONS);

	for(jobNumber = 1; jobNumber < SONS + 1 && jobNumber < argc; jobNumber++)
	{	
		send(pipearr[jobNumber - 1][WRITE_END], argv[jobNumber], strlen(argv[jobNumber]) + 1);
		
	}

	while(jobNumber < argc)
	{
		int size=read(pipearr[SONS][READ_END], buff, 41);

		//lock mutex

		strcat(s, buff+1);

		//unlock mutex

		child_number=buff[0]-'0';

		printf("lei de hijo %d , %d digitos\n", child_number, size);

		if(workDoneByEach[child_number] < minForEach)
		{
			allocatingNewFile(pipearr[child_number], argv[jobNumber], strlen(argv[jobNumber]) + 1,child_number);
			workDoneByEach[child_number]++;

		}
		else
		{
			if((overload > 0 )&& (workDoneByEach[child_number] == minForEach))
			{
				allocatingNewFile(pipearr[child_number], argv[jobNumber], strlen(argv[jobNumber]) + 1, child_number);
				workDoneByEach[child_number]++;
				overload--;
			}else
			{
				jobNumber--;
			}
		}
		jobNumber++;
	}

	for(int h = 1; h < argc && h < SONS + 1; h++)
	{	
		read(pipearr[SONS][READ_END], buff, BUFF_MAX);

		//lock mutex

		strcat(s, buff+1);

		//unlock mutex
	}

}

int main(int argc, char * argv[])
{
	pid_t pids[SONS];
	int pipearr[SONS + 1][2];
	char * shm, * s;

	initializePipes(pipearr, SONS + 1);

	createSonProcesses(SONS, pipearr , pids);
	
	/*close end from parent's pipe*/
	close(pipearr[SONS][WRITE_END]); 
	
	/*create shared memory using pid as key*/
	shm = createSharedMemory(getpid());

	//init mutex

	printf("%d\n", getpid());

	shm[0] = 1;

	s = shm + 1;

	ditributeJobs(SONS, pipearr, s, argc, argv);

	shm[0] = 0;

    terminateSons(pipearr);
}
	



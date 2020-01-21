#include "shell_util.h"
#include "linkedList.h"
#include "helpers.h"

// Library Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

volatile sig_atomic_t chldflag;
volatile sig_atomic_t sigusr1flag;

int intComparator(void* p, void* q) {
    return ((int)p - (int)q);
}

int timeComparator(void* time1, void* time2){
	int time = intComparator((void*)((ProcessEntry_t*)time1)->seconds,(void*)((ProcessEntry_t*)time2)->seconds);
	if(time == 0)
	{
		return 0;
	}
	else if(time < 0)
	{
		return -1;
	}
	else if(time > 0){
 		return 1;}
}

void sigchld_handler(int sig)
{
	if(sig)
	chldflag = 1;		
}

void sigusr1_handler(int sig)
{
	if(sig)
	sigusr1flag = 1;		
}


int main(int argc, char *argv[])
{
	int j=0,k=0,a=0,b=0,c=0,d=0,p[2],pi=0,pipenum=0,ppi[10],pa=-1,pb=-1,pc=-1,pd=-1,counter=0,fg1,fg2,fg3,fg4;
	int i; //loop counter
	char *args[MAX_TOKENS + 1];
	int exec_result;
	int exit_status;
	pid_t pid, ppid;
	pid_t wait_result;
    List_t bg_list;

    //Initialize the linked list
    bg_list.head = NULL;
    bg_list.length = 0;
    bg_list.comparator = NULL;  // Don't forget to initialize this to your comparator!!!

	// Setup segmentation fault handler
	if(signal(SIGSEGV, sigsegv_handler) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}	
	if(signal(SIGCHLD, sigchld_handler) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}
	if(signal(SIGUSR1, sigusr1_handler) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}

	while(1) {
		// DO NOT MODIFY buffer
		// The buffer is dynamically allocated, we need to free it at the end of the loop
		char * const buffer = NULL;
		size_t buf_size = 0;
		a=0;b=0;c=0;d=0;pi=0;pa=-1;pb=-1;pc=-1;pd=-1;pipenum=0;
		// Print the shell prompt
		display_shell_prompt();
		
		// Read line from STDIN
		ssize_t nbytes = getline((char **)&buffer, &buf_size, stdin);

		if (chldflag == 1) 
		{
  			pid = waitpid(-1, &exit_status, WNOHANG);
  			while (pid > 0) 
			{
  				 while (bg_list.head!= NULL)
				 {
    					if (pid == (((ProcessEntry_t*)bg_list.head->value)->pid)) 
					{
     						printf(BG_TERM,(((ProcessEntry_t*)bg_list.head->value)->pid),(((ProcessEntry_t*)bg_list.head->value)->cmd));
     						removeByPid(&bg_list, pid);
          					break;
    					}
    						bg_list.head = bg_list.head->next;
   				}
   				pid = waitpid(-1, &exit_status, WNOHANG);
  			}
  			chldflag = 0;
 		}

		if (sigusr1flag == 1)	
		{
			while (bg_list.head!= NULL)
			{
				printBGPEntry(((ProcessEntry_t*)bg_list.head->value));
				bg_list.head = bg_list.head->next;
			}

		}
	
		// No more input from STDIN, free buffer and terminate
		if(nbytes == -1) {
			free(buffer);
			break;
		}

		// Remove newline character from buffer, if it's there
		if(buffer[nbytes - 1] == '\n')
			buffer[nbytes- 1] = '\0';

		// Handling empty strings
		if(strcmp(buffer, "") == 0) {
			free(buffer);
			continue;
		}
		
		// Parsing input string into a sequence of tokens
		char* command = malloc(strlen(buffer));
		strcpy(command,buffer);
		size_t numTokens;
		*args = NULL;
		counter = 0;
		numTokens = tokenizer(buffer, args);

		if(strcmp(args[0],"exit") == 0) {
			// Terminating the shell
			while (bg_list.head!= NULL)
			{
				printf(BG_TERM,(((ProcessEntry_t*)bg_list.head->value)->pid),(((ProcessEntry_t*)bg_list.head->value)->cmd));
				kill((((ProcessEntry_t*)bg_list.head->value)->pid),SIGKILL);
				bg_list.head = bg_list.head->next;
			}
			kill(-1,SIGKILL);
			free(buffer);
			return 0;
		}

		else if(strcmp(args[0],"cd") == 0) {
			char s[1000];
			if(args[1] == NULL || (strcmp(args[1],"~") == 0))
			{ 
				chdir(getenv("HOME"));//GOES TO VAGRANT
				printf("%s\n", getcwd(s, 100)); 
			}
			else
			{
				const char* cd = args[1];
				char* prevdir = getcwd(s, 1000);
				if(strcmp(cd,"-") == 0)
				{
					chdir(prevdir);
					printf("%s\n", getcwd(s, 1000)); 
				}
    				else if(chdir(cd) == 0)
				{
					printf("%s\n", getcwd(s, 1000)); 
				}
				else if(chdir(cd) == -1)
				{
					fprintf(stderr, DIR_ERR);
				}	
			}
			free(buffer);
			continue;
		}
		else if(strcmp(args[0],"estatus") == 0) {
			if(WIFEXITED(exit_status)) 
   			{ 
				printf("%d\n",WEXITSTATUS(exit_status));
			}
			free(buffer);
			continue;
		}
		for(k=0;k<numTokens;k++)
		{
			if(args[j] != NULL){
			if(strcmp(args[k],"|") == 0)
			{
				if(args[k+1] == NULL||k==0|| k==numTokens-1)
				{
					fprintf(stderr, PIPE_ERR);
					exit(EXIT_FAILURE);
				}
				pi=1;
				//ppi[pipenum]=k;
				pipenum=k;
				args[k] = NULL;
			}}
		}

		for(j=0;j<numTokens;j++)
		{
			if(args[j] != NULL){
			if(strcmp(args[j],"<") == 0){
				if(a == 1)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				a=1;
				pa=j;
				counter++;
				args[j]=NULL;
				if(args[j+1] == NULL || (pa-1) == pb || (pa-1) == pc || (pa-1) == pd)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				fg1 = open(args[pa+1], O_RDONLY);
				if(fg1 == -1)
					{
						fprintf(stderr, RD_ERR);
						exit(EXIT_FAILURE);
					}
			}
			else if(strcmp(args[j],">") == 0){
				if(b == 1)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				b=1;
				pb=j;
				counter++;
				args[j]=NULL;
				if(args[j+1] == NULL || (pb-1) == pa || (pb-1) == pc || (pb-1) == pd)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				fg2 = creat(args[pb+1], O_WRONLY);
				if(fg2 == -1)
					{
						fprintf(stderr, RD_ERR);
						exit(EXIT_FAILURE);
					}

			}			
			else if(strcmp(args[j],"2>") == 0){
				if(c == 1)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				c=1;
				pc=j;
				counter++;
				args[j]=NULL;
				if(args[j+1] == NULL || (pc-1) == pa || (pc-1) == pb || (pc-1) == pd)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				fg3 = creat(args[pc+1], O_WRONLY);
				if(fg3 == -1)
					{
						fprintf(stderr, RD_ERR);
						exit(EXIT_FAILURE);
					}
			}
			else if(strcmp(args[j],">>") == 0){
				if(d == 1)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);	
				}
				d=1;
				pd=j;
				counter++;
				args[j]=NULL;
				if(args[j+1] == NULL || (pd-1) == pb || (pd-1) == pc || (pd-1) == pa)
				{
					fprintf(stderr, RD_ERR);
					exit(EXIT_FAILURE);
				}
				fg4  = open(args[pd+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
				if(fg4 == -1)
					{
						fprintf(stderr, RD_ERR);
						exit(EXIT_FAILURE);
					}
			}
			
			}
		}
		if(pipe(p) == -1)
		{
			exit(EXIT_FAILURE);
		}
			
		pid = fork();   //In need of error handling......	

		if (pid == 0){ //If zero, then it's the child process

			if(counter < 4)
			{
				if(a=1)
				{	
					//int fd = open(args[pa+1], O_RDONLY);
					dup2(fg1, STDIN_FILENO);
					close(fg1);
				}
				if(b=1)
				{
					//int fd = creat(args[pb+1], O_WRONLY);
					dup2(fg2, STDOUT_FILENO);
        				close(fg2);
				}	
				if(c=1)
				{
					//int fd = creat(args[pc+1], O_WRONLY);
					dup2(fg3, STDERR_FILENO);
        				close(fg3);
				}
				if(d=1)
				{
					//int fd = open(args[pd+1], O_WRONLY | O_CREAT | O_APPEND, 0666);
					dup2(fg4, STDOUT_FILENO);
        				close(fg4);
				}
			}
			else
			{
				fprintf(stderr, RD_ERR);
				exit(EXIT_FAILURE);	
			}
			
			if(pi == 1)
			{
				ppid = fork();	
				if (ppid == 0)
				{
					close(p[0]);
					dup2(p[1], STDOUT_FILENO);
					close(p[1]);
					wait(NULL);
					exec_result = execvp(args[0], &args[0]);
					if(exec_result == -1){ //Error checking
							printf(EXEC_ERR, args[0]);
							exit(EXIT_FAILURE);
						}
					exit(EXIT_SUCCESS);
				}
				else if (ppi > 0)
				{
					close(p[1]);
					dup2(p[0], STDIN_FILENO);
					close(p[0]);
					if(exec_result == -1){ //Error checking
						printf(EXEC_ERR, args[0]);
						exit(EXIT_FAILURE);
					}
					exit(EXIT_SUCCESS);
				}
				else
				{
					fprintf(stderr, PIPE_ERR);
					exit(EXIT_FAILURE);
				}
			}

			exec_result = execvp(args[0], &args[0]);
			//perror(RD_ERR);
			if(exec_result == -1){ //Error checking
				printf(EXEC_ERR, args[0]);
				exit(EXIT_FAILURE);
			}
		    exit(EXIT_SUCCESS);
		}
		 else{ // Parent Process
			if(strcmp(args[numTokens-1],"&") == 0)//PART 2
			{
				bg_list.comparator = timeComparator;
				ProcessEntry_t* index = malloc(sizeof(ProcessEntry_t));	
				index->cmd= malloc(strlen(command));
				index->seconds=time(NULL);
				index->pid = pid;
				index->cmd = command;
				insertInOrder(&bg_list,index);
			}
			else
			{
				wait_result = waitpid(pid, &exit_status, 0);
				if(wait_result == -1){
					printf(WAIT_ERR);
					exit(EXIT_FAILURE);
			}
		}
		}
		
		// Free the buffer allocated from getline
		free(buffer);
	}
	return 0;
}


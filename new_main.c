#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include "parse.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>

char *builtInCommands[] = {"cd",  "echo", "logout", "nice", "pwd", "setenv", "unsetenv", "where"};
extern char **environ;
char *directory_pathnames;
int old_stderr, old_stdin, old_stdout, ushrc_fd, oldrc_stdin;
pid_t groupId = 0;
int ushrc_file_present = 0;
Pipe mainPipe;
void ush_shell_execute(Cmd command);
void handling_signal_stp(int signum);



int isBuiltInCommand(char *command)
{
	int i;
	for (i =0; i < sizeof(builtInCommands)/sizeof(builtInCommands[0]); i++)
	{
		if (strcmp(command, builtInCommands[i]) == 0)
			return 1;
	}
	return 0;
}

int searchExecutableFile(Cmd command)
{
	fflush(stdout);
	if(strstr(command->args[0], "/") == NULL)
	{
		const char *path_env = getenv("PATH");
		directory_pathnames = strtok(path_env, ":");
		while (directory_pathnames!= NULL)
		{
			char *tempPath = (char *)malloc(256);
			strcpy(tempPath, directory_pathnames);
			strcat(tempPath, "/");
			strcat(tempPath, command->args[0]);
			if (access(tempPath, F_OK)== 0 && (access(tempPath, R_OK | X_OK) == 0 || !checkIfDirectory(tempPath)))
				return 1;
			directory_pathnames = strtok(NULL, ":");
		}

	}
	else
	{
		if(access(command->args[0], F_OK) != 0)
		{
			printf("command not found\n");
			return 0;
		}
		else if(checkIfDirectory(command->args[0]) || access(command->args[0], R_OK | X_OK) !=0)
		{
			printf("permission denied\n");
			return 0;
		}
	}
	return 1;
}

int checkIfDirectory(const char *path) 
{
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void ush_shell_execute(Cmd command)
{
	if(isBuiltInCommand(command->args[0]))
		goToCommand(command);
	else if (searchExecutableFile(command))
	{
		int pid = fork();
		if(pid != 0)
		{
			int status;
			waitpid(pid, &status, 0);
		}
		else
		{
			execvp(command->args[0], command->args);
			exit(EXIT_SUCCESS);
		}
		
	}
}

void CD_command_execute(Cmd command)
{
	char *home_env = getenv("HOME");
	char *tempPath = (char *)malloc(256);
	char *split_path;

	if(command->args[1] != NULL)
	{
		if(command->args[1][0] == '~')
		{
			tempPath = strtok(command->args[1], "~");
			if(tempPath != NULL)
			{
				command->args[1] = malloc(strlen(home_env) + strlen(tempPath));
				strcat(home_env, tempPath);
				strcpy(command->args[1], home_env);
			}
			else
			{
				command->args[1] = malloc(strlen(home_env));
				strcpy(command->args[1], home_env);
			}
			
				
		}
		if(checkIfDirectory(command->args[1]))
		{
			if(command->args[1][0]=='/')
			{
				chdir("/");
			}
			split_path =(strtok(command->args[1],"/"));
			while(split_path != NULL)
			{
				if(split_path[0] != '~')
				{
					chdir(split_path);
				}
				split_path = strtok(NULL, "/");
			}
		}
		else
			fprintf(stderr, "Invalid directory\n");
	}

	else
	{
		command->args[1] = home_env;
		chdir(command->args[1]);
	}

}

void echo_command_execute(Cmd command)
{
	if (command->args[1] != NULL )
	{
		int i;
		for(i=1; command->args[i] != NULL; i++)
			printf("%s ", command->args[i]);
	}
	printf("\n");
}

void logout_command_execute(Cmd command)
{
	exit(0);
}

void nice_command_execute(Cmd command)
{
	int which, prio;
	id_t who;
	which = PRIO_PROCESS;
	if(command->args[1] == NULL)
	{
		prio = 4;
		setpriority(which, 0, prio);
	}

	else
	{	
		int addConstant = 2;
		if(command->args[1][0] == '+' || command->args[1][0] == '-' || (command->args[1][0] >= '0' && command->args[1][0]<= '9'))
			prio = atoi(command->args[1]);
		else
		{
			prio = 4;
			addConstant = 1;
		}
		setpriority(which, 0, prio);
		Cmd tempNiceCommand;
		tempNiceCommand = malloc(sizeof(struct cmd_t));
		tempNiceCommand->args = malloc(sizeof(char *));
		tempNiceCommand->in = Tnil;
		tempNiceCommand->out = Tnil;
		int total_args = command->nargs;
		int j;
		for(j=0; j<total_args-addConstant; j++)
		{	
			tempNiceCommand->args[j] = malloc(strlen(command->args[j+addConstant]));
			strcpy(tempNiceCommand->args[j], command->args[j+addConstant]);
		}	
		tempNiceCommand->args[j] = NULL;
		ush_shell_execute(tempNiceCommand);
		for(j=0; j<tempNiceCommand->nargs; j++)
		{
			free(tempNiceCommand->args[j]);
		}
		free(tempNiceCommand);
	}
}

void pwd_command_execute(Cmd command)
{
	char* cwd;
    char buff[PATH_MAX + 1];

    cwd = getcwd( buff, PATH_MAX + 1 );
    if( cwd != NULL ) {
        printf("%s\n", cwd );
    }

}

void setenv_command_execute(Cmd command)
{	
	if(command->args[1] == NULL)
	{
		int i = 1;
		char *s = *environ;

		for (; s; i++) 
		{
			printf("%s\n", s);
			s = *(environ+i);
		}
	}
	else 
	{
		if(command->args[2] != NULL)
			setenv(command->args[1], command->args[2], 1);
		else
			setenv(command->args[1], "", 1);
	}
}

void unsetenv_command_execute(Cmd command)
{	if(command->args[1]!= NULL)
		unsetenv(command->args[1]);


}

void where_command_execute(Cmd command)
{	if(command->args[0] != NULL)
	{
		if(isBuiltInCommand(command->args[1]))
			printf("%s is builtin\n", command->args[1]);

		const char *path_env = getenv("PATH");

		directory_pathnames = strtok(path_env, ":");
		while (directory_pathnames!= NULL)
		{
			char *tempPath = (char*)malloc(256);
			strcpy(tempPath, directory_pathnames);
			strcat(tempPath, "/");
			strcat(tempPath, command->args[1]);
			if (access(tempPath, F_OK)== 0 && access(tempPath, X_OK)== 0)
				printf("%s\n", tempPath);
			directory_pathnames = strtok(NULL, ":");
		}
	}
}

void goToCommand(Cmd command)
{
	if(strcmp(command->args[0], "cd") == 0)
	{
		CD_command_execute(command);
	}
	else if(strcmp(command->args[0], "echo") == 0)
	{
		echo_command_execute(command);
	}
	else if(strcmp(command->args[0], "logout") == 0)
	{
		logout_command_execute(command);
	}
	else if(strcmp(command->args[0], "nice") == 0)
	{
		nice_command_execute(command);
	}
	else if(strcmp(command->args[0], "pwd") == 0)
	{
		pwd_command_execute(command);
	}
	else if(strcmp(command->args[0], "setenv") == 0)
	{
		setenv_command_execute(command);
	}
	else if(strcmp(command->args[0], "unsetenv") == 0)
	{
		unsetenv_command_execute(command);
	}
	else if(strcmp(command->args[0], "where") == 0)
	{
		where_command_execute(command);
	}
	else
	{
		printf("Invalid command\n");
	}
}

void processInOut(Cmd command)
{
	
	if (command->out == Tout) 
	{
		int out_file = open(command->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		dup2(out_file, STDOUT_FILENO);
		close(out_file);
	} 
	else if (command->out == ToutErr) {
		int out_file = open(command->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		dup2(out_file, STDERR_FILENO);
		dup2(out_file, STDOUT_FILENO);
		close(out_file);
	} 
	else if (command->out == Tapp) {
		int app_file = open(command->outfile, O_CREAT | O_APPEND | O_RDWR, S_IREAD | S_IWRITE);
		dup2(app_file, STDOUT_FILENO);
		close(app_file);
	} 
	else if (command->out == TappErr) {
		int app_file = open(command->outfile, O_CREAT | O_APPEND | O_RDWR, S_IREAD | S_IWRITE);
		dup2(app_file, STDERR_FILENO);
		dup2(app_file, STDOUT_FILENO);
		close(app_file);
	}
}

void main_ush_shell()
{
	char ushrc_filepath[PATH_MAX];
	char *home_env=(char *)malloc(256);
	strcpy(home_env, getenv("HOME"));
	strcat(home_env, "/.ushrc");
	strcpy(ushrc_filepath, home_env);
	free(home_env);
	ushrc_fd = open(ushrc_filepath, O_RDONLY);
	if(ushrc_fd != -1)
	{
		oldrc_stdin = dup(fileno(stdin));
		if(dup(fileno(stdin)) == -1)
			exit(-1);
		if(dup2(ushrc_fd, fileno(stdin)) == -1)
			exit(-1);
		ushrc_file_present = 1;
		close(ushrc_fd);
	}
	else
		ushrc_file_present = 0;

	while(1)
	{
		if(ushrc_file_present == 0)
		{
			char hostname[PATH_MAX];
			hostname[PATH_MAX-1] = '\0';
			gethostname(hostname, PATH_MAX-1);
			printf("%s%%", hostname);
			fflush(stdout);
			if(dup2(oldrc_stdin, fileno(stdin))== -1)
				exit(-1);
		}

		mainPipe = parse();
		if(mainPipe == NULL)
		{
			ushrc_file_present = 0;
			continue;
		}

		if(strcmp(mainPipe->head->args[0], "end") == 0)
		{
			if(ushrc_file_present == 0)
				break;
			else
			{
				ushrc_file_present = 0;
				continue;
			}
		}
		Pipe local_pipe;
		local_pipe = mainPipe;
		while (local_pipe != NULL)
		{
			Cmd command = local_pipe->head;
			int total_commands = 0, total_number_pipes = 0;
			int cmdPtr = 0, pipePtr = 0;
			int child = 0, i = 0, j = 0;
			int lastInBuiltCommand = 0;
			pid_t childList[10];
			Cmd local_command = command;
			while(local_command != NULL)
			{
				total_commands++;
				if(local_command->out == Tpipe || local_command->out == TpipeErr)
					i++;
				local_command=local_command->next;
			}

			total_number_pipes = i;
			int pipe_fd[i*2];
			for(j=0; j<(2*total_number_pipes); j++)
				pipe(pipe_fd + j*2);

			if((old_stdin = dup(fileno(stdin))) == -1)
				exit(EXIT_FAILURE);
			if((old_stdout = dup(fileno(stdout))) == -1)
				exit(EXIT_FAILURE);
			if((old_stderr = dup(fileno(stderr))) == -1)
				exit(EXIT_FAILURE);

			while(command != NULL)
			{
				if(strcmp(command->args[0], "end") == 0)
					break;
				else if(cmdPtr == total_commands -1 && isBuiltInCommand(command->args[0]))
				{
					lastInBuiltCommand = 1;
					if(total_commands == 1 && command->in == Tin) 
					{
						int in_file = open(command->infile, O_RDONLY);
						dup2(in_file, STDIN_FILENO);
						close(in_file);
					}
					processInOut(command);
					if(total_commands >1)
					{
						if(dup2(pipe_fd[(pipePtr -1)*2], 0) <0)
						{
							perror("failed");
							exit(EXIT_FAILURE);
						}
						i =0;
						while(i < 2*total_number_pipes)
						{
							close(pipe_fd[i]);
							i++;
						}
					}
					ush_shell_execute(command);
				}
				
				if(isBuiltInCommand(command->args[0]) == 0)
				{
					if(searchExecutableFile(command) == 0)
					{
						if(child >0)
							killpg(groupId, 9);
						break;
					}
				}

				if(lastInBuiltCommand == 0)
				{
					pid_t pid = fork();
					if(pid ==0)
					{
						if(child == 0)
							setpgid(0,0);
						else
							setpgid(0, groupId);

						if(total_commands == 1)
						{
							if (command->in == Tin) {
								int in_file = open(command->infile, O_RDONLY);
								dup2(in_file, STDIN_FILENO);
								close(in_file);
							}
							processInOut(command);
						}
						if((cmdPtr != 0)&&(cmdPtr==total_commands-1))
						{
							processInOut(command);
						
							if(dup2(pipe_fd[(pipePtr -1)*2],0) <0)
							{
								perror("failed");
								exit(EXIT_FAILURE);
							}
						}
						if(command->next != NULL)
						{
							if(cmdPtr == 0 && command->in == Tin)
							{
								int in_file = open(command->infile, O_RDONLY);
								dup2(in_file, STDIN_FILENO);
								close(in_file);
							}
							if(command->out != TpipeErr)
							{
								if (dup2(pipe_fd[pipePtr * 2 + 1], 1) < 0) 
								{
									perror("failed");
									exit(EXIT_FAILURE);
								}
							}
							else
							{
								if (dup2(pipe_fd[pipePtr * 2 + 1], 1) < 0) {
									perror("failed");
									exit(EXIT_FAILURE);
								}
								if (dup2(pipe_fd[pipePtr * 2 + 1], 2) < 0) {
									perror("failed");
									exit(EXIT_FAILURE);
								}		
							}
						}
						i =0;
						while(i < 2 * total_number_pipes) 
						{
							close(pipe_fd[i]);
							i++;
						}
						ush_shell_execute(command);
						exit(EXIT_SUCCESS);
					}
					else 
					{
						if (child==0) 
							groupId = pid;
						
						childList[child] = pid;
						child++;
					}
				}
			
				if (command->out == Tpipe || command->out == TpipeErr)
					pipePtr++;
				cmdPtr++;
				command = command->next;
			}
			int status = 0;
			i =0;
			while(i < 2 * total_number_pipes) 
			{
				close(pipe_fd[i]);
				i++;
			}
			i=0;
			while(i < child)
			{
				while(waitpid(childList[i], &status, 0) == -1);
				i++;
			}
			groupId = 0;
			local_pipe = local_pipe->next;
			fflush(stderr);
			fflush(stdout);
			if (dup2(old_stdin, fileno(stdin)) == -1)
			{
				exit(EXIT_FAILURE);
			}
			close(old_stdin);
			if (dup2(old_stdout, fileno(stdout)) == -1)
			{
				exit(EXIT_FAILURE);
			}
			close(old_stdout);
			if (dup2(old_stderr, fileno(stderr)) == -1)
			{
				exit(EXIT_FAILURE);
			}
			close(old_stderr);
		}
	}
}


void signal_handling(int signum) {
	if (signum == SIGINT && groupId != 0) {
		killpg(groupId, 9);
	}
	signal(SIGINT, signal_handling);
	signal(SIGTSTP, handling_signal_stp);
	signal(SIGQUIT, SIG_IGN);
	ushrc_file_present = 0;
}

void handling_signal_stp(int signum){
	if (signum == SIGTSTP && groupId != 0) {
		killpg(groupId, SIGTSTP);
	}
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, signal_handling);
	ushrc_file_present = 0;
	printf("\r");
}


int main(int argc, char const *argv[])
{
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, signal_handling);
	main_ush_shell();
	return 1;
}

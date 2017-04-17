#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_TOKEN_LENGTH 50
#define MAX_TOKEN_COUNT 100
#define MAX_LINE_LENGTH 512
#define MAX_SUBCOMMANDS_COUNT 4

// Simple implementation for Shell command
// Assume all arguments are seperated by space
// Erros are ignored when executing fgets(), fork(), and waitpid(). 

/**
 * Sample session
 *  shell: echo hello
 *   hello
 *   shell: ls
 *   Makefile  simple  simple_shell.c
 *   shell: exit
**/

void cnt(int sig) {
  static int count = 0;
  ++count;
  if(count == 2){
    exit(0);
  }
}

int runsubcommand(char* command, char** args, int in_fd, int out_fd, int* fds, int fds_len) 
{
  pid_t pid = fork();
  if(pid) { // parent
  	return pid;
  } else { // child
    int i;
    if(in_fd != 0) dup2(in_fd, 0);
    if(out_fd != 1) dup2(out_fd, 1);
    for(i = 0; i < fds_len; ++i){
      if(fds[i] != 0 && fds[i] != 1)
        close(fds[i]);
    }
    execvp(command, args);
    return pid; //Avoid Compiler Error
  }
}

void print_fds(int* fds, int fds_len){
  int i;
  for(i = 0; i < fds_len; ++i){
    printf("%d ", fds[i]);
  }
  printf("\n");
}

void runcommand(char** commands, char*** args_group, int num_commands, int* fds, int fds_len)
{
  int i; char* command; char** args; int in_fd; int out_fd;
  int child_process_ids[num_commands];
  for(i = 0; i < num_commands; ++i){
    command = commands[i];
    args = args_group[i];
    in_fd = fds[i*2];
    out_fd = fds[i*2 + 1];
    child_process_ids[i] = runsubcommand(command, args, in_fd, out_fd, fds, fds_len);
  }
  for(i = 0; i < fds_len; ++i){
      if(fds[i] != 0 && fds[i] != 1)
        close(fds[i]);
  }
  for(i = 0; i < num_commands; ++i){
    waitpid(child_process_ids[i], NULL, 0);
  }
}

int openFile(char* file_handle, int type_of_file){
  //type_of_file = 0 for input and 1 for output
  //return file descriptor
  int fd;
  if(type_of_file == 0){
    fd = open(file_handle, O_RDONLY);
      if(fd < 0){ //File does not exist
        printf("shell: %s: No such file or directory\n", file_handle);
        exit(1);
      }
  }
  else if(type_of_file == 1){
    fd = open(file_handle, O_WRONLY| O_CREAT| O_TRUNC, 0666);
  }
  else{
    printf("Type_of_file: %d is not recognized.", type_of_file);
    exit(1);
  }
  return fd;
}

int main(){
  signal(SIGTSTP, cnt); // Ctrl-Z handler
  char line[MAX_LINE_LENGTH];
  while(1) { //Taking input line by line
    fgets(line, MAX_LINE_LENGTH, stdin);
  	// Build the command and arguments, using execv conventions.
  	line[strlen(line)-1] = '\0'; // get rid of the new line
  	char* commands[MAX_SUBCOMMANDS_COUNT]; char* command = NULL;
    char* file_handle; int pipe_fd[2];
    int fds[MAX_SUBCOMMANDS_COUNT*2]; fds[0] = 0; fds[1] = 1; int command_count = 0;
  	char** arguments_group[MAX_SUBCOMMANDS_COUNT];
    char* arguments[MAX_TOKEN_COUNT];
  	int argument_count = 0;
    char* token = strtok(line, " ");

    while(token) { //Break a line down to tokens
  		if(!command){
        command = token;
        commands[command_count] = command;
        ++command_count;
      } 
      if(*token == '>'){ //OUTPUT REDIRECTION
        file_handle = strtok(NULL, " ");
        argument_count--;
        token = arguments[argument_count];
        fds[(command_count - 1)*2 + 1] = openFile(file_handle, 1);
      }
      if(*token == '<'){ //INPUT REDIRECTION
        file_handle = strtok(NULL, " ");
        argument_count--;
        token = arguments[argument_count];
        fds[(command_count - 1)*2] = openFile(file_handle, 0);
      }
      if(*token == '|'){ //Piping
        pipe(pipe_fd);
        char** args;
        args = (char **) malloc(MAX_TOKEN_COUNT * sizeof(char*));
        fds[(command_count - 1)*2 + 1] = pipe_fd[1];
        fds[(command_count - 1)*2 + 2] = pipe_fd[0];
        fds[(command_count - 1)*2 + 3] = 1;
        arguments[argument_count] = NULL;
        memcpy(args, arguments, sizeof arguments);
        arguments_group[command_count - 1] = args;
        command = NULL;
        token = strtok(NULL, " ");
        argument_count = 0;
        continue;
      }
      arguments[argument_count] = token;
      argument_count++;
  		token = strtok(NULL, " ");
  	}
  	arguments[argument_count] = NULL;
    arguments_group[command_count - 1] = arguments;
    if(argument_count>0){
      if (strcmp(arguments[0], "exit") == 0)
        exit(0);
      runcommand(commands, arguments_group, command_count, fds, command_count * 2);
    }    
  }
  return 0;
}

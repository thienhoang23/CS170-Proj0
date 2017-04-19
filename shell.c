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
// Errors are ignored when executing fgets(), fork(), and waitpid(). 
// A command is considered to cosist of one or multiple subcommands.

/**
 * Sample session
 *  shell: echo hello
 *   hello
 *   shell: ls
 *   Makefile  simple  simple_shell.c
 *   shell: exit
**/

void cnt(int sig)
{
  //Ctrl-Z signal handler
  static int count = 0;
  ++count;
  if(count == 2){
    exit(0);
  }
}

int openFile(char* file_handle, int type_of_file)
{
  //type_of_file = 0 for input and 1 for output
  //return file descriptor
  int fd;
  if(type_of_file == 0){
    fd = open(file_handle, O_RDONLY);
      if(fd < 0){ //File does not exist
        printf("shell: %s: No such file or directory\n", file_handle);
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

void print_fds(int* fds, int fds_len)
{
  int i;
  for(i = 0; i < fds_len; ++i){
    printf("%d ", fds[i]);
  }
  printf("\n");
}

void runcommand(char** commands, char* arr_of_args[MAX_SUBCOMMANDS_COUNT][MAX_TOKEN_COUNT], int num_commands, int* fds, int fds_len)
{
  int i; char* command; char** args; int in_fd; int out_fd;
  int child_process_ids[num_commands];
  for(i = 0; i < num_commands; ++i){
    //Run child processes in the pipe concurrently
    command = commands[i];
    args = arr_of_args[i];
    in_fd = fds[i*2];
    out_fd = fds[i*2 + 1];
    child_process_ids[i] = runsubcommand(command, args, in_fd, out_fd, fds, fds_len);
  }
  for(i = 0; i < fds_len; ++i){
    //Close any fd that is not stdin or stdout
    //Call after fork child processes to make sure each child process has access to the right in_fd and out_fd
    if(fds[i] != 0 && fds[i] != 1)
      close(fds[i]);
  }
  for(i = 0; i < num_commands; ++i){
    //Wait for each child process to finish and collect exit code
    int child_status;
    waitpid(child_process_ids[i], &child_status, 0);
  }
}

int runsubcommand(char* command, char** args, int in_fd, int out_fd, int* fds, int fds_len) 
{
  pid_t pid = fork();
  if(pid) { // parent
    return pid; //return pid so parent can wait for child process to finish
  } else { // child
    int i;
    if(in_fd != 0) dup2(in_fd, 0); //Get correct in_fd to sub stdin
    if(out_fd != 1) dup2(out_fd, 1); //Get correct out_fd to sub stdout
    for(i = 0; i < fds_len; ++i){
      //Close all unnecessary fds
      if(fds[i] != 0 && fds[i] != 1)
        close(fds[i]);
    }
    execvp(command, args);
    perror(command);
    exit(1); //Pass error code back to parents.
  }
}

int main(int argc, char *argv[])
{
  signal(SIGTSTP, cnt); // Ctrl-Z handler
  char line[MAX_LINE_LENGTH];
  while(fgets(line, MAX_LINE_LENGTH, stdin)) { //Taking input line by line
  	// Build the command and arguments, using execv conventions.
  	line[strlen(line)-1] = '\0'; // get rid of the new line
  	char* subcommands[MAX_SUBCOMMANDS_COUNT]; //Keep track of each subcommand
    char* command = NULL; char* file_handle; 
    int pipe_fd[2];
    int fds[MAX_SUBCOMMANDS_COUNT*2]; //Keep track of input and ouput redirection of each subcommand 
    fds[0] = 0; fds[1] = 1; 
    int command_count = 0;
  	char* arguments[MAX_SUBCOMMANDS_COUNT][MAX_TOKEN_COUNT]; //Keep track of the arguments of each subcommand
  	int argument_count = 0;
    char* token = strtok(line, " ");

    while(token) { //Break a line down to tokens
  		if(!command){
        command = token;
        subcommands[command_count] = command;
        ++command_count;
      } 
      if(*token == '>'){ //OUTPUT REDIRECTION
        file_handle = strtok(NULL, " ");
        argument_count--;
        token = arguments[command_count - 1][argument_count];
        fds[(command_count - 1)*2 + 1] = openFile(file_handle, 1);
      }
      if(*token == '<'){ //INPUT REDIRECTION
        file_handle = strtok(NULL, " ");
        argument_count--;
        token = arguments[command_count - 1][argument_count];
        fds[(command_count - 1)*2] = openFile(file_handle, 0);
      }
      if(*token == '|'){ //Piping
        pipe(pipe_fd);
        fds[(command_count - 1)*2 + 1] = pipe_fd[1];
        fds[(command_count - 1)*2 + 2] = pipe_fd[0];
        fds[(command_count - 1)*2 + 3] = 1;
        arguments[command_count - 1][argument_count] = NULL;
        command = NULL;
        token = strtok(NULL, " ");
        argument_count = 0;
        continue;
      }
      arguments[command_count - 1][argument_count] = token;
      argument_count++;
  		token = strtok(NULL, " ");
  	}
  	arguments[command_count - 1][argument_count] = NULL;
    if(argument_count>0){
      if (strcmp(arguments[command_count - 1][0], "exit") == 0)
        exit(0);
      runcommand(subcommands, arguments, command_count, fds, command_count * 2);
    }    
  }
  return 0;
}

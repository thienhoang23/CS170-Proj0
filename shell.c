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

void runcommand(char* command, char** args, char* infile, char* outfile) {
  int fd;
  pid_t pid = fork();
  if(pid) { // parent
  	waitpid(pid, NULL, 0);
  } else { // child
    if(infile != NULL){
      fd = open(infile, O_RDONLY);
      if(fd < 0){ //File does not exist
        printf("shell: %s: No such file or directory\n", infile);
        exit(1);
      }
      dup2(fd, 0);
    }
    if(outfile != NULL){
      fd = open(outfile, O_WRONLY| O_CREAT| O_TRUNC, 0666);
      dup2(fd, 1);
    }
  	execvp(command, args);
  }
}

int main(){
  signal(SIGTSTP, cnt); // Ctrl-Z handler
  char line[MAX_LINE_LENGTH];
  while(1) {
    fgets(line, MAX_LINE_LENGTH, stdin);
  	// Build the command and arguments, using execv conventions.
  	line[strlen(line)-1] = '\0'; // get rid of the new line
  	char* command = NULL;
    char* infile = NULL; char* outfile = NULL;
  	char* arguments[MAX_TOKEN_COUNT];
  	int argument_count = 0;
    char* token = strtok(line, " ");
  	
    while(token) {
  		if(!command) command = token;
      if(*token == '>'){ //OUTPUT REDIRECTION
        outfile = strtok(NULL, " ");
        break;
      }
      if(*token == '<'){ //INPUT REDIRECTIOn
        infile = strtok(NULL, " ");
        break; 
      }
      // if(*token == '|'){ //Piping

      // }
      arguments[argument_count] = token;
      argument_count++;
  		token = strtok(NULL, " ");
  	}
  	arguments[argument_count] = NULL;
  	if(argument_count>0){
  		if (strcmp(arguments[0], "exit") == 0)
        exit(0);
      runcommand(command, arguments, infile, outfile);
  	}
  }
  return 0;
}

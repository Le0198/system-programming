/**
*  Lab
* CS 241 - Fall 2018
*/

#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
int main(int argc, char **argv) {
    
    if (argc != 6) { print_usage(); exit(0);} 

    char* inputfile = argv[1];
    char* outputfile = argv[2];
    char* mapper = argv[3];
    char* reducer = argv[4];
    int count = atoi(argv[5]);

    // Create an input pipe for each mapper.
    int fd_m[count * 2];
    for (int i = 0; i < count; i++) {
      int idx = i * 2;
      pipe(fd_m + idx);
      descriptors_add(fd_m[idx]);
      descriptors_add(fd_m[idx + 1]);
    }
     
    // Create one input pipe for the reducer.
    int fd_r[2];
    pipe(fd_r);
    descriptors_add(fd_r[0]);
    descriptors_add(fd_r[1]);

    // Open the output file.
    int output_fd = open(outputfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    descriptors_add(output_fd);

    // Start a splitter process for each mapper.
    pid_t processes[count];
    pid_t children[count];
    for (int i = 0; i < count; i++) {
       pid_t child = fork();
       if (child < 0) { exit(1); } 
       if (child == 0){  
	     int idx = i * 2;
	     close(fd_r[0]);
	     close(fd_r[1]);
	     close(fd_m[idx]); //child does not read
	     dup2(fd_m[idx + 1], 1); //redirect stdout
	     char str[2];
	     sprintf(str, "%d", i);
	     if (execlp("./splitter", "./splitter", inputfile, argv[5], str, (char*)NULL) == -1) {
		exit(1);
             }
       }
       else { children[i] = child; }
    
    close(fd_m[i * 2 + 1]); // close write
    // Start all the mapper processes.
	pid_t mapper_process = fork();

	if (mapper_process < 0) {exit(1);}

	if (mapper_process == 0) {
	  int idx = i * 2;
	  //close(fd_m[idx + 1]); // close write
	  dup2(fd_m[idx], 0); // redirect stdin
	  close(fd_r[0]); // close read
	  dup2(fd_r[1], 1); // redirect stdout
	  char mapper_arg[strlen(mapper) + 3];
	  sprintf(mapper_arg, "./%s", mapper);
	  if (execlp(mapper_arg, mapper_arg, (char*)NULL) == -1) {  exit(1); }
	}
	else{
	    processes[i] = mapper_process;
	}
    }
    close(fd_r[1]); // close write
    // Start the reducer process.
    pid_t reducer_process = fork();
    if (reducer_process < 0) {
	exit(1);
    }
    if (reducer_process == 0) {
	dup2(fd_r[0], 0); // redirect stdin
	dup2(output_fd, 1); // redirect stdout
	char reducer_arg[strlen(reducer) + 3];
        sprintf(reducer_arg, "./%s", reducer);
	if (execlp(reducer_arg, reducer_arg, (char*)NULL) == -1) { exit(1); }
    }
    // Wait for the reducer to finish.

    for (int i = 0; i < count; i++) {
      int status2;
      if(waitpid(children[i], &status2, 0) == -1 ) { exit(1); }
      int status1;
      if(waitpid(processes[i], &status1, 0) == -1 ) { exit(1); }
      if( WIFEXITED(status2) && WEXITSTATUS(status2)){ exit(1); }
      if( WIFEXITED(status1) && WEXITSTATUS(status1)){
	    print_nonzero_exit_status(mapper, status1); 
      }
    }
    int status;
    if(waitpid(reducer_process, &status, 0) == -1 ) { exit(1); };
    if(WIFEXITED(status) && WEXITSTATUS(status)){ 
	print_nonzero_exit_status( reducer, status); 
    } 
    

    descriptors_closeall();
    descriptors_destroy();
    // Print nonzero subprocess exit codes.
    
    // Count the number of lines in the output file.
    //close(output_fd);
    print_num_lines(outputfile);
    return 0;
}

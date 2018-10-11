/**
* Utilities Unleashed Lab
* CS 241 - Fall 2018
*/

#include "format.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BILLION 1E9


int main(int argc, char *argv[]) {

    if(argc <= 1){
	print_time_usage();
    }

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int status;
    pid_t child = fork();
    
    if(child == -1) { 
	print_fork_failed();
    }
    
    if(child == 0){

//??????????????????????????????????????????????????
	int flag = execvp(argv[1], argv + 1);
	if(flag == -1){
	    print_exec_failed();
	}
	
    }

    else{
	
	waitpid(child, &status, 0);
	if( WIFEXITED(status) && WEXITSTATUS(status)){
	    exit(1);
	}    

}
	struct timespec monotime;
	clock_gettime(CLOCK_MONOTONIC, &monotime);
	double time_elapse = monotime.tv_sec - start.tv_sec + (monotime.tv_nsec - start.tv_nsec)/BILLION;
	char ** a = argv;
	display_results(a, time_elapse);
	return 0;


}

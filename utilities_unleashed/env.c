/**
* Utilities Unleashed Lab
* CS 241 - Fall 2018
*/

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include "format.h"
#include <stdlib.h>
#include <ctype.h>



int main(int argc, char *argv[]) {
    
    if(argc < 3){
	print_env_usage();
    }
    int split = 0;
    for(int i = 0; i < argc; i++){
	if(strcmp(argv[i], "--") == 0){
	    split = i;
	    break;
	}
	
    }    
    if(split == 0 || argv[split + 1] == NULL){
	print_env_usage();
    }

    int num = 1;
    int f = 1;    
    if(strcmp(argv[1], "-n") == 0){
	f = 3;
	num = atoi(argv[2]);
    }

//printf("here is ok\n");
//?????????????????????????????????????    
// variables with reference
    int varnum = 0;
    if(f == 3){
	varnum = split - 3;
    }
    else if(f == 1){
	varnum = split - 1;
    }
    char* names[varnum];
    char* vars[varnum];
    for(int i = f; i < split; i++){
	names[i-f] = strtok(argv[i], "=");
	if(names[i-f] != NULL){
	    
	    char* track = names[i-f];
	    while(track != NULL && *track != 0){
	    if(isalpha(*track) == 0){
		if(isdigit(*track) == 0){
		    if(*track != '_'){
		       // if(*track != '%'){
			    print_env_usage();
	    }}}//}
	    track++;
	    }

	    vars[i-f] = strtok(NULL, "=");
	    if(!vars[i-f]){print_env_usage();}
	}
	else{print_env_usage();}
    }
        
//split with ","
    char* var[varnum][num];

    for(int i = 0; i < varnum; i++){
	int j = 0;
	char* token = strtok(vars[i], ",");
//printf("%s\n", token);
//printf("%d\n", i);
   	while(token != NULL && j < num){


	    char* track = token;
	    if(*track == '%'){track++;}
	    while(track && *track != 0){
	    if(isalnum(*track) == 0){
		if(*track != '_'){
		   // if(*track != '%'){
		print_env_usage();
	    }}//}
	    track++;
	    }

	    var[i][j] = token;
	    token = strtok(NULL, ",");
	    j++;
   	}
	while(token == NULL && j < num){
	    var[i][j] = vars[i];
	    j++;
	}
    }



//printf("%d\n%s\n%s\n%s\n", num, names[0], vars[0], command[0]);
//printf("%s\n%s\n", names[1], vars[1]);
//printf("\n%s\n%s\n%s\n%s\n", var[1][0], var[1][1], var[1][2], var[1][3]);


    pid_t children[num];    

    for(int i = 0; i < num; i++){
	pid_t child = fork();
	if(child == -1){
	    print_fork_failed();
	}
	if(child == 0){
	    for(int j = 0; j < varnum; j++){
	
	        char* target = strstr(var[j][i], "%");
		if(target){
		    var[j][i] = "";
		    for(int x = 0; x < j; x++){
			if(strcmp(target + 1, names[x]) == 0){
			    var[j][i] = var[x][i];
			}
		    }
		}
		
		int res = setenv(names[j], var[j][i], 1);
		if(res == -1){
		    print_environment_change_failed();
		}
	    }	
	    int flag = execvp(argv[split+1], argv+split+1);
	    if(flag == -1){
		print_exec_failed();
	    }
	}
	else{
	    children[i] = child;
	    int status;

	    waitpid(child, &status, 0);
	    if( WIFEXITED(status) && WEXITSTATUS(status)){
	        exit(1);
	    }    
	}
    }


    return 0;
}

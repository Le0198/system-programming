/**
* Shell Lab
* CS 241 - Fall 2018
*/

#include "format.h"
#include "shell.h"
#include "vector.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <signal.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/stat.h>

typedef struct process {
    char *command;
    char *status;
    pid_t pid;
} process;


static vector* bkgd;
//bkgd = vector_create(NULL, NULL, NULL);

void read_commands_from_history(const char *path_to_file, vector* vec) {
    if(path_to_file == NULL){
        print_history_file_error();
        exit(0);
    }

    if(strcmp(path_to_file, "/") == 0 || path_to_file == NULL) { print_usage(); exit(0); }
    
    FILE* fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen(path_to_file, "a+");
    if(fp == NULL){
        print_usage();
	exit(0);
    }
    while((read = getline(&line, &len, fp)) != -1){
        if(line[read - 1] == '\n'){line[read - 1] = '\0';}
        vector_push_back(vec, line);
    }
    free(line);
    fclose(fp);
    return;
}


void update_history(const char *path_to_file, vector *commands) {    
    FILE* fp;
    fp = fopen(path_to_file, "w");
    if(fp == NULL){
        print_history_file_error();
        exit(0);
    }
    for (size_t i = 0; i < vector_size(commands); i++) {
	fprintf(fp, "%s\n", vector_get(commands, i));
    }
    fclose(fp);
}


vector* split(char* cmd) {
    char* ptr = cmd;
    vector* v = string_vector_create();
    while ( ptr ) {
    char *s = strsep(&ptr, " ");
        vector_push_back(v, s);
    }
    for (size_t i = 0; i < vector_size(v); i++) {
        if (strcmp(*vector_at(v, i), "") == 0) {
            vector_erase(v, i);
        }
    }
    return v;
}


vector* process_logical_operation(char* input) {
    char* temp = strdup(input);
    vector *v = NULL;
    char* ptr = temp;
    if (strstr(temp, "||")) {
	v = string_vector_create();
	vector_push_back(v, "||");
        while ( ptr ) {
            char *s = strsep(&ptr, "||");
	    vector_push_back(v, s);
	}
	
    }
    else if (strstr(temp, "&&")) {
        v = string_vector_create();
	vector_push_back(v, "&&");
        while ( ptr ) {
            char *s = strsep(&ptr, "&&");
            vector_push_back(v, s);
        }
    }
    else if (strstr(temp, ";")) {
        v = string_vector_create();
	vector_push_back(v, ";");
        while ( ptr ) {
            char *s = strsep(&ptr, ";");
            vector_push_back(v, s);
        }
    }
    if (v != NULL) {
	for (size_t i = 0; i < vector_size(v); i++) {
            if (strcmp(*vector_at(v, i), "") == 0) {
                vector_erase(v, i);
            }
        }
    }
    free(temp);
    return v;
}


void prompt(pid_t pid) {
    char cwd[PATH_MAX];    
    //pid_t pid = getpid();
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
	print_prompt(cwd, pid);
	fflush( stdout );
    }
}


char* read_command_from_stdin() {
    char str[1000];
    fgets(str, 999, stdin);
    size_t len = strlen(str);
    if (str[len - 1] == '\n') { str[len -1] = '\0'; }
    char* s = malloc(len + 1);
    strcpy(s, str);
    
    return s;
}


void sigintHandler() {
    return;
}


void rm_process (pid_t pid) {
    for(size_t i = 0; i < vector_size(bkgd); i++) {
        process* p = (process*)vector_get(bkgd, i);
        if (p->pid == pid) { 
	    free(vector_get(bkgd, i));
	    vector_erase(bkgd, i);
	}
    }
}


char* get_process (pid_t pid) {
    for(size_t i = 0; i < vector_size(bkgd); i++) {
        process* p = (process*)vector_get(bkgd, i);
        if (p->pid == pid) {
            return strdup(p->command);
        }
    }
    return NULL;
}


void ch_process (pid_t pid, char* status) {
    for(size_t i = 0; i < vector_size(bkgd); i++) {
        process* p = vector_get(bkgd, i);
	if (p->pid == pid) {
            p->status = status; 
	}
    }
}


void sigchld_handler() {
    pid_t pid;
    if ((pid = waitpid(-1, NULL, WNOHANG)) != -1) {
	rm_process(pid);
    }
}


void kill_zombies() {
    for (size_t i = 1; i < vector_size(bkgd); i++) {
	process *p = vector_get(bkgd, i); 
	kill(p->pid, SIGTERM);
    }
}



int exec_cmd(char* cmd, vector* hist, int hflag, char* hfile, int logic);


void logic_exec(char *command, vector* hist, int hflag, char* hfile) {
    vector* logicv = process_logical_operation(command);
        if (logicv) {
            int islogic = 1;
            vector_push_back(hist, command);
            char* log = *vector_front(logicv);
            char* cmd1 = vector_get(logicv, 1);
            char* cmd2 = vector_get(logicv, 2);
            int success = exec_cmd(cmd1, hist, hflag, hfile, islogic);
            if (strcmp(log, "&&") == 0) {
                if (success  == 0) {
                    exec_cmd(cmd2, hist, hflag, hfile, islogic);
                }
            }
            else if (strcmp(log, "||") == 0) {
                if (success  != 0) {
                    exec_cmd(cmd2, hist, hflag, hfile, islogic);
                }
            }
            else {
                 exec_cmd(cmd2, hist, hflag, hfile, islogic);
            }
        }

        else { exec_cmd(command, hist, hflag, hfile, 0); }
}



//Build-in functions
int exec_buildin(char* cmd, vector* hist, int hflag, char* hfile) {
    assert(cmd);
    assert(hist);
    char* prefix = strdup(cmd);
    vector* vec = split(cmd);
    size_t sz = vector_size(vec);
    size_t szh = vector_size(hist);

    if ( strcmp(*vector_front(vec), "cd" ) == 0 ) {
	if (sz != 2) {print_no_directory(""); return 1;}
	else {
	    int f = chdir(vector_get(vec, 1)); 
	    if (f == -1) {print_no_directory(vector_get(vec, 1)); return 1;}
	}
	vector_destroy(vec);
	return 0;
    }

    if ( strcmp(*vector_front(vec), "!history") == 0 ) {
	vector_pop_back(hist);
	if (sz != 1) {print_invalid_command(cmd); }
	else {
	    for (size_t i = 0; i < szh-1; i++) {
		print_history_line(i, vector_get(hist, i));
	    }
	}
	vector_destroy(vec);
	return 0;
    }

    if ( sz == 1 && ((char*)vector_get(vec, 0))[0] == '#' ) {
	vector_pop_back(hist);
	int num = atoi( *vector_front(vec) + 1 );
	assert(num >= 0);
	if ((size_t)num >= szh - 1) { print_invalid_index(); }
	else { 
	    char* mystr = strdup(vector_get(hist, (size_t)num));
	    logic_exec(mystr, hist, hflag, hfile);
	    free(mystr);
 	}
	vector_destroy(vec);
	return 0;
    }

    if ( prefix[0] == '!' ) {
	vector_pop_back(hist);
	size_t len = strlen(prefix) - 1;
	
	int idx = -1;
	for (int i = szh - 2; i >= 0; i--) {
	    if( strncmp(prefix+1, *vector_at(hist, i), len) == 0 ) { idx = i; break; }
	}
	free(prefix);
	if(len == 0) {idx = szh - 2;}
	if(idx == -1) {print_no_history_match();}
	if(idx > -1) {
	    char* mystr = strdup(vector_get(hist, idx));
            logic_exec(mystr, hist, hflag, hfile); 
            free(mystr);
	}
	vector_destroy(vec);
	return 0;
    }

    if (strcmp(*vector_front(vec), "ps") == 0) {
	for (size_t i = 0; i < vector_size(bkgd); i++) {
	    process* t = *vector_at(bkgd, i);
	    print_process_info(t->status, (int)(t->pid), t->command); 
	}
	vector_destroy(vec);
	return 0;
    }

    if (strcmp(*vector_front(vec), "kill") == 0) {
	if (sz != 2) {print_invalid_command(cmd); return 1; }
	else {
	    int pid = atoi(*vector_back(vec));
            int f = kill((pid_t)pid, SIGTERM);
	    if (f == -1) { print_no_process_found(pid); return 1; }
	    else {
		char* proc = get_process(pid);
		rm_process(pid);
		print_killed_process(pid, proc);
		free(proc); 
	    }
	}
	vector_destroy(vec);
	return 0;
    }

    if (strcmp(*vector_front(vec), "stop") == 0) {
        if (sz != 2) {print_invalid_command(cmd); return 1; }
        else {
            int pid = atoi(*vector_back(vec));
            int f = kill((pid_t)pid, SIGTSTP);
            if (f == -1) { print_no_process_found(pid); return 1; }
            else {
		char* proc = get_process(pid);
		ch_process(pid, STATUS_STOPPED);
		print_stopped_process(pid, proc);
		free(proc);
	    }
        }
	vector_destroy(vec);
	return 0;
    }

    if (strcmp(*vector_front(vec), "cont") == 0) {
        if (sz != 2) {print_invalid_command(cmd); return 1; }
        else {
            int pid = atoi(*vector_back(vec));
            int f = kill((pid_t)pid, SIGCONT);
            if (f == -1) { print_no_process_found(pid); return 1; }
	    else {
		ch_process(pid, STATUS_RUNNING);
	    }
        }
	vector_destroy(vec);
	return 0;
    }
    free(prefix);
    vector_destroy(vec);
    return 2;
}


bool is_backgrounding(vector* v) {
    char* last = *vector_back(v);
    if ( *last == '&' ) {
	vector_pop_back(v);
	return true;
    }
    else if (last[strlen(last) - 1] == '&' ) { 
	((char*)vector_get(v, vector_size(v) - 1))[strlen(last) - 1] = '\0';
	return true; 
    }
    return false;
}


int exec_cmd(char* cmd, vector* hist, int hflag, char* hfile, int logic) {
    if (strlen(cmd) == 0) {return 1;}
//exit
    char* sss = strdup(cmd);
    vector* aaa = split(sss);
    if ( strcmp(*vector_front(aaa), "exit") == 0) {
	if (hflag) { update_history(hfile, hist); }
	free(sss);
	vector_destroy(aaa);
	kill_zombies();
	exit(0);
    }
    free(sss); vector_destroy(aaa);
//catch CTRL-D
    if( feof(stdin) ) { 
	if ( hflag ) { update_history(hfile, hist); }
	kill_zombies();
	exit(0); 
    }
//execution
    char* tmp = strdup(cmd);
    vector_push_back(hist, cmd);
    if (logic) { vector_pop_back(hist); }
//execute build in
    int flag = exec_buildin(tmp, hist, hflag, hfile);
    free(tmp);
    if (flag == 0) {return 0;}
    else if (flag == 1) {return 1;}
//execute external
    else {
	char* ptr = strdup(cmd);
	vector* vec = split(ptr);
	free(ptr);
	bool bck = is_backgrounding(vec);
	size_t sz = vector_size(vec);
	char* args[sz + 1];
	for (size_t i = 0; i < sz; i++) {
	    args[i] = *vector_at(vec, i);
	}
	args[sz] = NULL;

	fflush(stdout);
	signal(SIGCHLD, sigchld_handler);

	pid_t child = fork();
	if (child == -1) { print_fork_failed(); exit(1); }
	else if (child == 0) {
	    pid_t curr = getpid();
	    print_command_executed(curr);
	    if(execvp(args[0], args) == -1) { print_exec_failed(cmd); exit(1);}
	}
	else {
	    vector_destroy(vec);
	    int status;

	    if(bck) {
		if(setpgid(child, child) == -1) { print_setpgid_failed(); exit(1); }
		else {
		    process *temp = malloc(sizeof(process));
		    temp->command = strdup(cmd);
		    temp->pid = child;
		    temp->status = STATUS_RUNNING;
		    vector_push_back(bkgd, temp);
		}
            }
	    else {
	        if(waitpid(child, &status, 0) == -1 ) { print_wait_failed(); exit(1); };
	        if( WIFEXITED(status) && WEXITSTATUS(status)){  return 1; }
	    }
        }
    }
    return 0;
}
int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    if(argc != 5 && argc != 3 && argc != 1) { print_usage(); exit(0); }
    signal(SIGINT, sigintHandler);
    if( feof(stdin) ) { exit(0); }
    int opt, ffnd, hfnd;
    char* script_file = NULL;
    char* history_file = NULL;
    char* history = NULL;
    ffnd = 0;
    hfnd = 0;
    opterr = 0;
    while ((opt = getopt(argc, argv, "f:h:")) != -1) {	
	switch (opt) {
	case 'f':
	    script_file = optarg;
	    ffnd++;
	    break;
	case 'h':
	    history = optarg;
	    hfnd++;
	    break;
	default:
	    print_usage();
	    exit(0);
	}
    }
    if (optind > argc || ffnd > 1 || hfnd > 1) {
        print_usage();
        exit(0);
    }
    bkgd = vector_create(NULL, NULL, NULL);
    
    vector* mycmds = string_vector_create();
    pid_t pid = getpid();
    process main_proc;
    main_proc.pid = pid;
    main_proc.command = "./shell";
    main_proc.status = STATUS_RUNNING;
    vector_push_back(bkgd, &main_proc);
    if ( hfnd ) {
	history_file = get_full_path(history);
	read_commands_from_history(history_file, mycmds); }
// Read command from stdin
    if (ffnd == 0) {
	while(1) {
	prompt(pid);
	char* str = read_command_from_stdin();
	char command[strlen(str)];
	strcpy(command, str);
	free(str);
	logic_exec(command, mycmds, hfnd, history_file);
	}
    }

// Read commands from files
    if ( ffnd ) {
        if(script_file == NULL){
            print_script_file_error();
            return 0;
        }
        FILE* fp;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        fp = fopen(script_file, "r");
        if(fp == NULL){
            print_script_file_error();
            return 0;
        }
        while((read = getline(&line, &len, fp)) != -1){
            if(line[read - 1] == '\n'){line[read - 1] = '\0';}
	    else { line[read - 1] = '\0'; }
            prompt(pid); 
	    print_command(line);
	    fflush(stdout);
	    logic_exec(line, mycmds, hfnd, history_file);	
        }
	
        if ( hfnd ) { update_history(history_file, mycmds); }
	
    	free(line);
    	fclose(fp);
	kill_zombies();
	return 0;
    }

    vector_destroy(mycmds);
    return 0;
}

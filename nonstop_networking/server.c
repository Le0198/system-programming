/**
* Networking Lab
* CS 241 - Fall 2018
*/

#include "common.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector.h>
#include <signal.h>
#include <dictionary.h>
#include <dirent.h>
#include <sys/epoll.h>

#define MAX_EVENTS 16


static volatile int serverSocket;
static volatile int endSession = 0;
static vector* file_list;
static char tempdir[] = "./XXXXXX";
static vector* jobs;
static int epfd;

typedef enum{
    READ_HEADER,
    READ_SIZE,
    READ_DATA,
    WRITE_RESPONSE,
    WRITE_SIZE,
    WRITE_DATA,
    SERVING,
    DONE,
    ERROR
}state_t;

typedef struct _job{
    int fd;
    verb request;
    char* filename;
    int filefd;
    state_t state;
    char buf[1024];
    size_t writesz;
    size_t readsz;
    size_t datasz;
    size_t filesize;
    const char* errmsg;
}job;

/********************************************************************
                           HELPER FUNCTION                          *
*********************************************************************/
int write_err(job* j) {
    free(j->filename);
    char* tmp;
    asprintf(&tmp, "ERROR\n%s", j->errmsg);
    size_t offset = 0;
    int ret = writer(j->fd, tmp + j->writesz, strlen(tmp) - j->writesz, &offset);
    j->writesz += offset;
    if (ret == 1) {
        return 1;
    }
    if (ret == -1) { return -1; }
    j->writesz = 0;
    shutdown(j->fd, SHUT_RDWR);
    free(tmp);
    close(j->fd);
    j->state = DONE;
    return 0;
}

int write_response(job* j) {
    char* str = "OK\n";
    size_t offset = 0;
    int ret = writer(j->fd, str + j->writesz, 3 - j->writesz, &offset);
    j->writesz += offset;
    if (ret == 1) {
	return 1;
    }
    if (ret == -1) { return -1; }
    j->writesz = 0;
    if (j->request == LIST || j->request == GET) { 
	j->state = WRITE_SIZE;
	return 0; 
    }
    shutdown(j->fd, SHUT_RDWR);
    close(j->fd);
    j->state = DONE;
    return 0;
}

int write_size(job* j) {
    size_t offset = 0;
    size_t size = j->filesize;
    int ret = writer(j->fd, (char*)&size + j->writesz, sizeof(size_t) - j->writesz, &offset);
    j->writesz += offset;
    if (ret == 1) { return 1; }
    if (ret == -1) { return -1; }
    j->writesz = 0;
    if (j->filesize == 0) { j->state = DONE;}
    else { j->state = WRITE_DATA; }
    return 0;
}

int openfile(job* j) {
    int fd = open(j->filename, O_RDONLY);
    if (fd == -1) {
	j->state = ERROR;
	j->errmsg = err_no_such_file;
    }
    return fd;
}

/********************************************************************
                           HANDLE HEADER                            *
*********************************************************************/
char* parse_header(char* header, verb *req) {
    char* tmp = strchr(header, ' ');
    if (tmp == NULL) {
        if (strcmp(header, "LIST") != 0) {
            *req = V_UNKNOWN;
        }
        else {
            *req = LIST;
        }
        return NULL;
    } else {
    char* ptr = header;
    int len = tmp - ptr;
    char vb[len+1];
    strncpy(vb, ptr, len);
    vb[len] = '\0';
    if (strcmp(vb, "PUT") == 0) { *req = PUT; }
    else if (strcmp(vb, "GET") == 0) {*req = GET;}
    else if (strcmp(vb, "DELETE") == 0) {*req = DELETE;}
    tmp++;
    char* file = strdup(tmp);
    return file;
  }
}

int get_header(int socket, char* header, size_t *size) {
  size_t sz = 0;
  char a[2];
  int i = 0;
  do {
      int ret = reader(socket, a, 1, &sz);
      if (ret == 1) {
          return 1;
      }
      if (ret == -1) {
          return -1;
      }
      *size = *size + 1;
      header[i] = a[0];
      i++;
      if (*size > 262) { return -2;}
  } while(a[0] != '\n');
  header[i - 1] = '\0';
//printf("header is %s\n", header);
  return 0;
}

int read_header(job* myjob) {
    if (myjob->state == ERROR) {return 0;}
    int fd = myjob->fd;
    int result = get_header(fd, myjob->buf + myjob->readsz, &(myjob->readsz));
    if (result == -1 || result == -2) {
	myjob->state = ERROR;
        myjob->errmsg = err_bad_request;
        return 0;
    }
    if (result == 1) {return 1;}

    char* ret = parse_header(myjob->buf, &(myjob->request));
    if (myjob->request == V_UNKNOWN) {
        myjob->state = ERROR;
        myjob->errmsg = err_bad_request;
        return -1;
    }
    
    if (myjob->request != LIST){
      if (strlen(ret) > 255) {
        myjob->request = V_UNKNOWN;
	myjob->state = ERROR;
        myjob->errmsg = err_bad_request;
        return 0;
      }
      myjob->filename = ret;
      if (myjob->request == PUT) {
	myjob->state = READ_SIZE;
      } else {
	myjob->state = SERVING;
      }
    }
    else{
        myjob->state = SERVING;
    }
    memset(myjob->buf, 0, 1024);
    myjob->readsz = 0;
//printf("file is %s\n", myjob->filename);
    return 0;
}

/********************************************************************
                     HANDLE DIFFERENT REQUEST                       *
*********************************************************************/
//PUT
int put(job* myjob) {
//printf("using put\n");
    if (myjob->state == ERROR) {return 0;}
    size_t offset = 0;
    //read size
    if (myjob->state == READ_SIZE) {
	int ret = reader(myjob->fd, (char*)(&(myjob->filesize)) + myjob->readsz, sizeof(size_t) - myjob->readsz, &offset);
	myjob->readsz += offset;
	if (ret == 1) {
	    return 1;
	}
        if (ret == -1) {
	    return -1;
	}
	//myjob->filesize = (size_t)ntohl(sz);
	myjob->state = SERVING;
	memset(myjob->buf, 0, 1024);
	myjob->readsz = 0;
    }
    //start serving
    if (myjob->state == SERVING) {
	int result = reader(myjob->fd, (char*)myjob->buf + myjob->readsz, 1024 - myjob->readsz, &offset);
	myjob->readsz += offset;
	if (result == 1) {return 1;}
        if (result == -1) {return -1;}
        if (result == 0) {
	    int fd = open(myjob->filename, O_CREAT | O_RDWR | O_TRUNC, S_IRWXG | S_IRWXO | S_IRWXU);
	    if (fd == -1) {perror("open"); exit(1);}
	    myjob->datasz += myjob->readsz;
            write_all(fd, myjob->buf, myjob->readsz);
	    close(fd);
            if (myjob->datasz > myjob->filesize) {
		myjob->state = ERROR;
		myjob->errmsg = err_bad_file_size;
		remove(myjob->filename);
		return 0;
	    }
	    if (myjob->datasz == myjob->filesize) {myjob->state = DONE;}
	    if (myjob->datasz < myjob->filesize && myjob->readsz < 1024) {
		myjob->state = ERROR;
		myjob->errmsg = err_bad_file_size;
		remove(myjob->filename);
                return 0; 
	    }
	    if (myjob->datasz < myjob->filesize && myjob->readsz == 1024) {
		myjob->state = READ_DATA;
	    }
	    myjob->readsz = 0;
	    memset(myjob->buf, 0, 1024);
	}
    }
    //continue serving
    while (myjob->state == READ_DATA) {
	int result = reader(myjob->fd, myjob->buf + myjob->readsz, 1024 - myjob->readsz, &offset);
	myjob->readsz += offset;
	if (result == 1) {return 1;}
	if (result == -1) { return -1;}
	if (result == 0) {
	    int fda = open(myjob->filename, O_CREAT | O_RDWR | O_APPEND, S_IRWXG | S_IRWXO | S_IRWXU);
	    myjob->datasz += myjob->readsz;
	    write_all(fda, myjob->buf, myjob->readsz);
	    close(fda);
	    if (myjob->datasz > myjob->filesize) {
		myjob->state = ERROR;
                myjob->errmsg = err_bad_file_size;
		remove(myjob->filename);
                return 0;
	    }
	    else if (myjob->datasz == myjob->filesize) {myjob->state = DONE;}
            else {
		if (myjob->readsz < 1024) {
                  myjob->state = ERROR;
                  myjob->errmsg = err_bad_file_size;
		  remove(myjob->filename);
		  return 0;
                } 
                 
            }
            myjob->readsz = 0;
            memset(myjob->buf, 0, 1024);
	}
    }
    //job done
    int flag = 1;
    for (int i = 0; i < (int)vector_size(file_list); i++) {
	if (strcmp(vector_get(file_list, i), myjob->filename) == 0) {
	    flag = 0;
	}
    }
    if (flag) {
	vector_push_back(file_list, myjob->filename);
    }
    myjob->state = WRITE_RESPONSE;
    write_response(myjob);
    return 0;
}

//LIST
int list(job* j) {
    if (j->state == ERROR) {return 0;}
    int len = 0;
    char* flist = malloc(1);
    if (j->state == SERVING) {
      if (vector_size(file_list) > 0){
	for (int i = 0; i < (int)vector_size(file_list); i++) {
	    len += (strlen(vector_get(file_list, i)) + 1);
	}
	flist = realloc(flist, len);
	strcpy(flist, vector_get(file_list, 0));
	for (int i = 1; i < (int)vector_size(file_list); i++) {
	    char* str = vector_get(file_list, i);
	    strcat(flist, "\n");
	    strcat(flist, str);
	}
	j->filesize = len - 1;
	j->state = WRITE_RESPONSE;
      } else {
	j->state = WRITE_RESPONSE;
	j->filesize = 0;
      }
    }
    if (j->state == WRITE_RESPONSE) {
	int ret = write_response(j);
	if (ret == 1) {return 1;}
	if (ret == -1) {return -1;}
    }
    if (j->state == WRITE_SIZE) {
	int rett = write_size(j);
	if (rett == 1) {return 1;}
	if (rett == -1) {return -1;}
    }
    while (j->state == WRITE_DATA) {
       size_t offset;
       int result = writer(j->fd, flist + j->writesz, j->filesize - j->writesz, &offset);
       j->writesz += offset;
       if (result == 1) {return 1;}
       if (result == -1) {return -1;}
       j->datasz += j->writesz;
       j->writesz = 0;
       memset(j->buf, 0, 1024);
       if (j->datasz >= j->filesize) {j->state = DONE;}
    }
    shutdown(j->fd, SHUT_RDWR);
    close(j->fd);
    free(flist);
    return 0;
}

//DELETE
int handle_delete(job* j) {
    if (j->state == ERROR) {return 0;}
    int fd = openfile(j);
    if (fd == -1) { return 0;}
    if (remove(j->filename) == 0) {
	for (int i = 0; i < (int)vector_size(file_list); i++) {
	    char* filename = vector_get(file_list, i);
	    if (strcmp(filename, j->filename) == 0) {
		free(j->filename);
		free(filename);
		vector_erase(file_list, i);
	    }
	}
	j->state = WRITE_RESPONSE;
    }
    else {
	j->state = ERROR; j->errmsg = err_no_such_file; return 0;
    }
    if (j->state == WRITE_RESPONSE) {
	int ret = write_response(j);
	if (ret == 1) {return 1;}
	if (ret == -1) {return -1;}
    }
    return 0;
}

//GET
int handle_get(job* j) {
    if (j->state == ERROR) {return 0;}
    if (j->state == SERVING) {
      int fd = openfile(j);
      if (fd == -1) { return 0; }
      j->filefd = fd;
      struct stat sb;
      if (fstat(fd, &sb) == -1) {perror("stat"); exit(1);}
      size_t size = sb.st_size;
      j->filesize = size;
      j->state = WRITE_RESPONSE;
    }
    if (j->state == WRITE_RESPONSE) {
        int ret = write_response(j);
        if (ret == 1) {return 1;}
        if (ret == -1) {return -1;}
    }
    if (j->state == WRITE_SIZE) {
        int rett = write_size(j);
        if (rett == 1) {return 1;}
        if (rett == -1) {return -1;}
    }
    while (j->state == WRITE_DATA) {
	if (j->writesz == 0) {
	    j->readsz = read_all(j->filefd, j->buf, 1024);
	    if ((int)(j->readsz) == -1) {perror("read"); exit(1);}
	}
	size_t offset;
	int result = writer(j->fd, j->buf + j->writesz, j->readsz - j->writesz, &offset);
	j->writesz += offset;
	if (result == 1) {return 1;}
	if (result == -1) {return -1;}
	j->datasz += j->writesz;
	j->writesz = 0;
	memset(j->buf, 0, 1024);
	if (j->datasz >= j->filesize) {j->state = DONE;}
    }
    shutdown(j->fd, SHUT_RDWR);
    close(j->fd);
    close(j->filefd);
    return 0;
}

/********************************************************************
                             UTILITIES                              *
*********************************************************************/
void init_job(int fd, job* myjob){
    myjob->fd = fd;
    myjob->request = V_UNKNOWN;
    myjob->filename = NULL;
    myjob->state = READ_HEADER;
    myjob->readsz = 0;
    myjob->datasz = 0;
    myjob->filesize = 0;
    myjob->writesz = 0;
    myjob->filefd = -1;
    memset(myjob->buf, 0, 1024);
    myjob->errmsg = NULL;
    vector_push_back(jobs, myjob);
   }

void setnonblocking(int fd) {
    int flags;
    if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {flags = 0;}
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
       perror("fcntl"); exit(1);
    }
}


void cleanup() {
    if (shutdown(serverSocket, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(serverSocket);

    //--------------clear directory-----------------
    struct dirent *ent;
    DIR *mydir = opendir(".");
    if (!mydir) { perror("DIR"); exit(1); }
    while(1) {
	errno = 0;
        ent = readdir(mydir);
        if (!ent) {break;}
        if (!ent && errno != 0) {perror("readdir"); exit(1);}
        if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
            if (unlink(ent->d_name) == -1) {perror("unlink"); exit(1);}
        }
    }
    if (closedir(mydir) == -1) {perror("close"); exit(1);}
    if (chdir("..") == -1) {perror("chdir"); exit(1);}
    if (rmdir(tempdir) == -1) {perror("rmdir"); exit(1);}
    //------------clear data structure-----------------
    for (int i = 0; i < (int)vector_size(file_list); i++) {
        free(vector_get(file_list, i));
    }
    vector_destroy(file_list);
    for (int i = 0; i < (int)vector_size(jobs); i++) {
	free(vector_get(jobs, i));
    }
    vector_destroy(jobs);
}

void close_server() {
    endSession = 1;
}

void sigpipe() {
    return;
}

void epoll_add(int fd, void *data) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.ptr = data;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1){
        perror("epoll_ctl");
        exit(1);
    }
}

/********************************************************************
                                Main                                *
*********************************************************************/
int main(int argc, char **argv) {
    // good luck!
    if (argc != 2) {
        fprintf(stderr, "%s <port>\n", argv[0]);
        return -1;
    }
    char* port = argv[1];
    //-----------------signal handle---------------------

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    signal(SIGPIPE, sigpipe);
    //-----------------create directory------------------
    char* dirret = mkdtemp(tempdir);
    if (dirret == NULL) { perror("mkdtemp"); exit(1); }
    print_temp_directory(tempdir);
    if (chdir(tempdir) == -1) { perror("chdir"); exit(1); }

    //create file_list
    file_list = shallow_vector_create();
    jobs = shallow_vector_create();
    //create epoll
    epfd = epoll_create(1);
    if (epfd == -1) { perror("epoll_create"); exit(1); }

    //--------------------runserver-----------------------
    int s;
    serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (serverSocket == -1) {perror("socket()");}
    struct addrinfo hints, *result;

    int optval = 1;
    int setret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (setret == -1) { perror("setsocketopt()"); }
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    if (bind(serverSocket, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(serverSocket, 32) != 0) {
        perror("listen()");
        exit(1);
    }

    struct sockaddr_in *result_addr = (struct sockaddr_in *) result->ai_addr;
    printf("Initializing server");
    printf("Listening on port %d\n", ntohs(result_addr->sin_port));
    printf("Ready to accept incoming connections\n");

    freeaddrinfo(result);

    endSession = 0;

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = serverSocket;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, serverSocket, &ev) == -1) {
        perror("epoll_ctl: serverSocket");
        exit(1);
    }

    while (endSession == 0) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) { continue; }
        for (int n = 0; n < nfds; n++) {
            if (events[n].data.fd == serverSocket) {
		int client = accept(serverSocket, NULL, NULL);
                if (client == -1 && errno != EAGAIN) {
                    perror("accept: "); exit(1);
                }
                setnonblocking(client);
                job* new_job = malloc(sizeof(job));
                init_job(client, new_job);
                epoll_add(client, new_job);
            } else {
	      job* myjob = events[n].data.ptr;
              //read header
              if (myjob->state == READ_HEADER) {
                int ret = read_header(myjob);
		if (ret == 1) {continue;}
	      }
	      //handle different request
	      if (myjob->request == PUT) {
		  int run = put(myjob);
		  if (run == 1) { continue; }
	      }
	      if (myjob->request == LIST) {
		  int run = list(myjob);
		  if (run == 1) { continue; }
	      }
	      if (myjob->request == DELETE) {
                  int run = handle_delete(myjob);
		  if (run == 1) { continue; }
              }
	      if (myjob->request == GET) {
		  int run = handle_get(myjob);
		  if (run == 1) { continue; }
	      }
	      if (myjob->state == ERROR) {
		  if(write_err(myjob) == 1) { continue; }
	      }
            }
        }
        if (endSession == 1) {
            break;
        }
    }
    cleanup();
//    free(server);
}


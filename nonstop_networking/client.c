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


//static volatile int serverSocket;

char **parse_args(int argc, char **argv);
verb check_args(char **args);
int connect_to_server(const char*, const char*);
void close_server_connection();
char* get_response_header(int socket);
void process_response(char* str, int socket);

int main(int argc, char **argv) {
    // Good luck!
//-------------grab arguments--------------
    char** args = parse_args(argc, argv);
    verb request = check_args(args);
    char* remote = NULL;
    char* local = NULL;
    if (request == GET || request == PUT) {
	     remote = args[3];
	     local = args[4];
    }
    if (request == DELETE) {
	     remote = args[3];
    }

//------------connect to server------------
    int serverSocket = connect_to_server(args[0], args[1]);

    int local_fd;
    char buffer[1024];

//------------------GET--------------------
    if (request == GET) {
	local_fd = open(local, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	size_t len = strlen(remote);
	char str[len + 6];
	sprintf(str, "GET %s\n", remote);
	str[len+5] = '\0';
	ssize_t count = write_all(serverSocket, str, len + 5);
	if (count == -1) {
	    print_connection_closed();
	    exit(1);
	}
	if (shutdown(serverSocket, SHUT_WR)) {
	    perror("shut down()");
	    exit(3);
	}
	// get header
	char* header = get_response_header(serverSocket);
	process_response(header, serverSocket);
	free(header);

	size_t readsz = read_message_size(serverSocket);

	size_t read_count = 0;

	while (read_count < readsz) {
	    size_t read_bytes = read_all(serverSocket, buffer, 1024);

	    if (read_bytes == 0) {
		print_too_little_data();
		exit(1);
	    }

	    write_all(local_fd, buffer, read_bytes);
	    read_count += read_bytes;

	}

	if (read_count > readsz || read_all(serverSocket, buffer, 1024) > 0) {
	    print_received_too_much_data();
	    exit(1);
	}
	close(local_fd);
    }


//-----------------PUT----------------------
  if (request == PUT) {
      size_t len = strlen(remote);
      char str[len + 6];
      sprintf(str, "PUT %s\n", remote);
      ssize_t count = write_all(serverSocket, str, len + 5);
      if (count == -1) {
	print_connection_closed();
	exit(1);
      }
      struct stat sb;
      if (stat(local, &sb) == -1) { perror("stat"); exit(1); }
      size_t writesz = (size_t)sb.st_size;
//      ssize_t sz_count = write_message_size(serverSocket, writesz);
      size_t sz_count = write_all(serverSocket, (char *)&writesz, sizeof(size_t));
      if ((int)sz_count == -1) {
	print_connection_closed();
	exit(1);
      }

      local_fd = open(local, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
      if (local_fd == -1) {
        print_connection_closed();
        exit(1);
      }
      size_t write_count = 0;
      size_t available = read(local_fd, buffer, 1024);
      while (write_count < writesz) {
	if ((int)available == -1) {
	  print_connection_closed();
	  exit(1);
	}
	size_t written = write_all(serverSocket, buffer, available);
        write_count += written;
	available = read(local_fd, buffer, 1024);
      }

      close(local_fd);
      if (shutdown(serverSocket, SHUT_WR)) {
            perror("shut down()");
            exit(3);
      }

      // get header
      char* header = get_response_header(serverSocket);
      process_response(header, serverSocket);
      free(header);
      print_success();
  }

//----------------DELETE---------------------
  if (request == DELETE) {
	  size_t len = strlen(remote);
	  char str[len + 9];
	  sprintf(str, "DELETE %s\n", remote);
	  str[len + 8] = '\0';
	  ssize_t count = write_all(serverSocket, str, len + 8);
	  if (count == -1) {
	    print_connection_closed();
	    exit(1);
	  }
	  if (shutdown(serverSocket, SHUT_WR)) {
	    perror("shutdown()");
	    exit(3);
	  }
    // get header
  	char* header = get_response_header(serverSocket);
  	process_response(header, serverSocket);
  	free(header);
	print_success();
  }


//------------------LIST---------------------
  if (request == LIST) {
	  ssize_t count = write_all(serverSocket, "LIST\n", 6);
    if (count == -1) {
      print_connection_closed();
      exit(1);
    }
    if (shutdown(serverSocket, SHUT_WR)) {
      perror("shut down()");
      exit(3);
    }
    // get header
    char* header = get_response_header(serverSocket);
    process_response(header, serverSocket);
    free(header);
    ssize_t readsz = read_message_size(serverSocket);
    ssize_t read_count = 0;
    while (read_count < readsz) {
      size_t read_bytes = read_all(serverSocket, buffer, 1024);
      if (read_bytes == 0) {
  	print_too_little_data();
  	exit(1);
      }
      write_all(1, buffer, read_bytes);
      read_count += read_bytes;
    }
    write(1, "\n", 1);
    if (read_count > readsz || read_all(serverSocket, buffer, 1024) > 0) {
      print_received_too_much_data();
      exit(1);
    }
  }
  free(args);

}

//-------------------------------------------


void process_response(char* str, int socket) {
    if (strcmp(str, "ERROR\n") == 0) {
	char* err = get_response_header(socket);
	print_error_message(err);
	free(str);
	exit(10);
    }
    if (strcmp(str, "OK\n") != 0) {
	print_invalid_response();
	free(str);
	exit(11);
    }
}

char* get_response_header(int socket) {
    char* header = malloc(1);
    char a[2];
    size_t size = 1;
    int i = 0;
    do {
        ssize_t sz = read_all(socket, a, 1);
	if (sz == -1) {exit(1);}
	header[i] = a[0];
	i++;
	size++;
	header = realloc(header, size);
    } while(a[0] != '\n');
    header[size - 1] = '\0';
    return header;
}

void close_server_connection(int serverSocket) {
    int ret = shutdown(serverSocket, SHUT_RDWR);
    if (ret) {
	perror("shut down()");
	exit(3);
    }
    close(serverSocket);
    serverSocket = -1;
    print_connection_closed();
}

int connect_to_server(const char* host, const char* port) {
    int s;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	exit(1);
    }

    if (connect(socket_fd, result->ai_addr, result->ai_addrlen) == -1) {
	perror("connect");
	exit(1);
    }
    freeaddrinfo(result);
    return socket_fd;

}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}


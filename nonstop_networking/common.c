/**
* Networking Lab
* CS 241 - Fall 2018
*/

#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


ssize_t read_message_size(int socket) {
    ssize_t size;
    ssize_t read_bytes = read_all(socket, (void*)&size, sizeof(size_t));
    if (read_bytes == 0 || read_bytes == -1) {
	return read_bytes;
    }
    return (ssize_t)size;
}


ssize_t write_message_size(int socket, size_t size) {
    //int32_t sz = htonl(size);
    ssize_t write_bytes = write_all(socket, (char*)&size, sizeof(size_t));
    return write_bytes;
}


ssize_t read_all(int socket, char *buffer, size_t count) {
    ssize_t offset = 0;
    ssize_t result = 0;
    
    while ((size_t)offset < count) {
    	result = read(socket, buffer + offset, count - offset);
	if (result == -1) {
	    if (errno == EINTR) {
		continue; 
	    } else {
		return -1;
	    }
	}
	if (result == 0) {return offset;}
	
	offset += result;
    
    }
    return count;
}


ssize_t write_all(int socket, const char *buffer, size_t count) {

    ssize_t offset = 0;
    ssize_t result = 0;

    while ((size_t)offset < count) {
        result = write(socket, (void*)(buffer + offset), count - offset);
        if (result == -1) {
            if (errno == EINTR)
                continue;
	    if (errno == EPIPE)
		return -1;
        }
	if (result == 0) {
	    return offset;
	}
	offset += result;
    }
    return count;
}


int reader(int fd, void* buf, size_t count, size_t *offset) {
    *offset = 0;
    ssize_t nread;
    while(count > 0) {
	nread = read(fd, buf, count);
	if (nread == -1 && errno == EAGAIN) {
	    return 1;
	}
	if (nread == -1 && errno == EINTR) {
	    continue;
	}
	if (nread == 0) {
	    return 0;
	}
	if (nread == -1) {
	    return -1;
	}
	*offset += nread;
	count -= nread;
	buf += nread;
    }
    return 0;
}


int writer(int fd, void* buf, size_t count, size_t *offset) {
    *offset = 0;
    ssize_t writtern;
    while(count > 0) {
	writtern = write(fd, buf, count);
	if (writtern == -1 && errno == EAGAIN) {
	    return 1;
	}
	if (writtern == -1 && errno == EINTR) {
	    continue;
	}
	if (writtern == 0) {
	    return 0;
	}
	if (writtern == -1) {
	    return -1;
	}
	*offset += writtern;
	count -= writtern;
	buf += writtern;
    }
    return 0;
}

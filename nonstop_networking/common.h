/**
* Networking Lab
* CS 241 - Fall 2018
*/

#pragma once
#include <stddef.h>
#include <sys/types.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

ssize_t read_message_size(int socket);

ssize_t write_message_size(int socket, size_t size);

ssize_t read_all(int socket, char* buffer, size_t count);

ssize_t write_all(int socket, const char *buffer, size_t count);

int reader(int fd, void* buf, size_t count, size_t* offset);

int writer(int fd, void* buf, size_t count, size_t* offset);

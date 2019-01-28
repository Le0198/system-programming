/**
* Mad Mad Access Patterns Lab
* CS 241 - Fall 2018
*/

#include "tree.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

void search(void* data, int off, char* word) {
    if (off == 0) {
	printNotFound(word);
	return;
    }
    BinaryTreeNode *node = data + off;
    int cmp = strcmp(word, node->word);
    if (cmp == 0) {
	printFound(word, node->count, node->price);
	return;
    } else if (cmp < 0) {
	search(data, node->left_child, word);
    } else {
	search(data, node->right_child, word);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printArgumentUsage();
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {openFail(argv[1]); return 2;}
    struct stat s;
    if (fstat(fd, &s) == -1) {
	openFail(argv[1]); return 2;
    }
    if (!S_ISREG(s.st_mode)) {
	openFail(argv[1]); return 2;
    }
    void* data = mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {mmapFail(argv[1]); return 3;}
    if (close(fd) == -1) {openFail(argv[1]); return 2;}
    if (strncmp(data, "BTRE", 4) != 0) {
	formatFail(argv[1]);
	return 2;
    }

    for (int i = 2; i < argc; i++) {
	search(data, 4, argv[i]);
    }

    return 0;
}

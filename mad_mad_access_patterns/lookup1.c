/**
* Mad Mad Access Patterns Lab
* CS 241 - Fall 2018
*/

#include "tree.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

int main(int argc, char **argv) {
    if (argc < 3) {
	printArgumentUsage();
	return 1;
    }
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) { openFail(argv[1]); return 2; }
    char ptr[5];
    size_t ret = fread(ptr, 1, 4, file);
    ptr[4] = '\0';
    if (ret != 4 || strcmp(ptr, "BTRE") != 0) { 
	formatFail(argv[1]); return 2;
    }

    for (int i = 2; i < argc; i++) {
      if (fseek(file, 4, SEEK_SET) != 0) { 
	formatFail(argv[1]);
	return 2; 
      }
      BinaryTreeNode node;
      fread(&node, sizeof(BinaryTreeNode), 1, file);

      char* word = argv[i];
      while(1) {
  //      fseek(file, sizeof(BinaryTreeNode), SEEK_CUR);
        char nword[100];
        if (fgets(nword, 100, file) == NULL) {
          formatFail(argv[1]);
          exit(3);
        }
//printf("nword is : %s\n", nword);
        int cmp = strcmp(word, nword);
        if (cmp == 0){
	  printFound(word, node.count, node.price);
	  break;
        }
        else if (cmp > 0) {
	  if (node.right_child == 0) { printNotFound(word); break; }
	  if (fseek(file, node.right_child, SEEK_SET) != 0) {return 1;}
	  fread(&node, sizeof(BinaryTreeNode), 1, file);
        }
        else {
	  if (node.left_child == 0) { printNotFound(word); break; }
	  if (fseek(file, node.left_child, SEEK_SET) != 0) {return 1;}
	  fread(&node, sizeof(BinaryTreeNode), 1, file);
	}
      }
    }
    fclose(file);
    return 0;
}

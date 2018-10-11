/**
* Mini Valgrind Lab
* CS 241 - Fall 2018
*/

#include <stdio.h>
#include <stdlib.h>

int main() {
    // Your tests here using malloc and free
    //void *p1 = "abcd";
    //p1 = realloc(p1, 80);
    int* p2 = calloc(20, 4);
    int *p3 = (int*)malloc(40);
    //char* p4 = "abcd";
    p3 = realloc(p3, 100);

    p3 = realloc(p3, 20);
    //free(p1);
    free(p2);
    free(p3);
    //free(p4);
    //free(p3);
    free(NULL);
    return 0;
}

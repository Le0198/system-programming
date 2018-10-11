/**
* Pointers Gone Wild Lab
* CS 241 - Spring 2018
*/

#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here

//1
    first_step(81);

//2    
    int ss = 132;
    int* second = &ss;
    second_step(second);

//3
    int* third = malloc(sizeof(int)*2);
    third[0] = 8942;
    third[1] = 0;
    int** d = &third;
    double_step(d);
    free(third);

//4 
    
    char forth[10];
    forth[0] = 1;
    forth[1] = 2;
    forth[2] = 3;
    forth[3] = 4;
    forth[5] = 15;
    forth[6] = '\0';
    forth[7] = '\0';
    forth[8] = '\0';
    forth[9] = '\0';
    //int strange = 15;
    //int* what = &strange;
    //what = (char*) what;
    //strcat(forth, (char*)what);
    //forth[6] = '\0';
    strange_step(forth);


//5
    char fifth[4] = "www";
    fifth[3] = '\0';
    //printf("%s\n%d\n", (char*)fifth, ((char*)fifth)[3]==0);
    empty_step(fifth);
    

//6
    char* s = strdup("uuuuu");
    //void* t = &s;
    //printf("%d\n", t==s);
    two_step(s, s);
    free(s);

//7
    char* s7 = strdup("asscdeesd");
    char* t1 = s7 + 2;
    char* t2 = t1 + 2;
    three_step(s7, t1, t2);
    free(s7);

//8
    char* s8_2 = strdup("sssss");
    char* s8_3 = strdup("kkk{");
    char* s8_1 = strdup("kkkk");
    //printf("%c  %c  %c", s8_1[1]+8, s8_2[2], s8_3[3]+8);
    step_step_step(s8_1, s8_2, s8_3);
    free(s8_2);
    free(s8_3);
    free(s8_1); 
    
//9
    char* s9 = strdup("c");
    int sb = 99;
    it_may_be_odd(s9, sb);
    free(s9);   

//10
    char* s10 = strdup("qeh,CS241,sse");
    tok_step(s10);
    free(s10);

//11
/*
    int filedes = 3; 
    FILE * stream = fdopen (filedes, "w");
    fprintf (stream, "c");
    void* orange = stream;
    the_end(orange, orange);
    fclose(stream);
*/
    char blue[4];
    //char* blue = &eleven;
    //int* orange = (int*)&eleven;
    blue[3] = '\0';
    blue[0] = 1;
    blue[1] = 4;
    blue[2] = 1;
    char* orange = blue;
    //printf("%d\n%d\n", ((char*)blue)[0], *((int*)blue));
    the_end(orange, blue);


    return 0;
}

/**
* Extreme Edge Cases Lab
* CS 241 - Fall 2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

/*
 * Testing function for various implementations of camelCaser.
 *
 * @param  camelCaser   A pointer to the target camelCaser function.
 * @param  destroy      A pointer to the function that destroys camelCaser
 * output.
 * @return              Correctness of the program (0 for wrong, 1 for correct).
 */
int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Return 1 if the passed in function works properly; 0 if it doesn't.

    int array[14];


//NULL case
    char* input1 = NULL;
    char** output1 = camelCaser(input1);
    if(!output1){
        array[0] = 1;
    }	
    else{
	array[0] = 0;
    }
    destroy(output1);
    	
    
//Normal case
    char* input2 = "Welcome! There is much fun.";
    char** output2 = camelCaser(input2);
    char** tmp = output2;
  
    if(strcmp(*tmp, "welcome") == 0 && strcmp(*(tmp+1), "thereIsMuchFun") == 0 && *(tmp+2) == NULL){
	array[1] = 1;
    }
    else{
	array[1] = 0;
    }
    destroy(output2);


//edge case 
    char* input3 = ".Welcome! There is much fun.";
    char** output3 = camelCaser(input3);
    tmp = output3;

    if(strcmp(*tmp, "") == 0 && strcmp(tmp[1], "welcome") == 0 && strcmp(tmp[2], "thereIsMuchFun") == 0 && tmp[3] == NULL){
	array[2] = 1;
    }
    else{
	array[2] = 0;
    }
    destroy(output3);


//only contain punctuation
    char* input4 = "..     .";
    char** output4 = camelCaser(input4);
    tmp = output4;

    if(strcmp(tmp[0], "") == 0 && strcmp(tmp[1], "") == 0 && strcmp(tmp[2], "")  == 0 && tmp[3] == NULL) {
	array[3] = 1;
    }
    else{
	array[3] = 0;
    }
    destroy(output4);


//no punctuation at the end
    char* input5 = "Hello, World";
    char** output5 = camelCaser(input5);
    tmp = output5;
   
    if(strcmp(tmp[0], "hello") == 0 && tmp[1] == NULL){
	array[4] = 1;

    }
    else{
	array[4] = 0;
    }
    destroy(output5);
   

//All uppercase
    char* input6 = "HELLO, WELCOME TO THE NEW WORLD!";
    char** output6 = camelCaser(input6);
    tmp = output6;
   
    if(strcmp(tmp[0], "hello") == 0 && strcmp(tmp[1], "welcomeToTheNewWorld") == 0  && tmp[2] == NULL){
	array[5] = 1;
    }
    else{
	array[5] = 0;
    }
    destroy(output6);


//All lowercase

    char* input9 = "hello, world little cat.";
    char** output9 = camelCaser(input9);
    tmp = output9;
   
    if(strcmp(tmp[0], "hello") == 0 && strcmp(tmp[1], "worldLittleCat") == 0 && tmp[2] == NULL){
	array[8] = 1;

    }
    else{
	array[8] = 0;
    }
    destroy(output9);



//Escape sequence 
    char* input7 = "Hello\n, \tWorld    \vnancy.";
    char** output7 = camelCaser(input7);
    tmp = output7;

    if(strcmp(tmp[0], "hello") == 0 && strcmp(tmp[1], "worldNancy") == 0  && tmp[2] == NULL){
	array[6] = 1;

    }
    else{
	array[6] = 0;
    }
    destroy(output7);

//Empty input 
    char* input8 = "";
    char** output8 = camelCaser(input8);
    tmp = output8;
   
    if(tmp[0] == NULL){
	array[7] = 1;

    }
    else{
	array[7] = 0;
    }
    destroy(output8);


//case10
    char* input10 = "This is a b c d.";
    char** output10 = camelCaser(input10);
    tmp = output10;

    if(strcmp(tmp[0], "thisIsABCD") == 0 && tmp[1] == NULL){
	array[9] = 1;
    }
    else{
	array[9] = 0;
    }
    destroy(output10);


// No punctuation

    char* input11 = "This is a b c d";
    char** output11 = camelCaser(input11);
    tmp = output11;

    if(tmp[0] == NULL){
	array[10] = 1;
    }
    else{
	array[10] = 0;
    }
    destroy(output11);


// Only one punctuation


    char* input12 = "!";
    char** output12 = camelCaser(input12);
    tmp = output12;

    if(strcmp(tmp[0], "") == 0 && tmp[1] == NULL){
	array[11] = 1;
    }
    else{
	array[11] = 0;
    }
    destroy(output12);


// Only have space

    char* input13 = " ";
    char** output13 = camelCaser(input13);
    tmp = output13;

    if(tmp[0] == NULL){
	array[12] = 1;
    }
    else{
	array[12] = 0;
    }
    destroy(output13);

// Number


    char* input14 = "hello 007ssss 112dddd. ";
    char** output14 = camelCaser(input14);
    tmp = output14;

    if(strcmp(tmp[0], "hello007Ssss112Dddd") ==0 && tmp[1] == NULL){
	array[13] = 1;
    }
    else{
	array[13] = 0;
    }
    destroy(output14);



tmp = NULL;


    for(int i = 0; i < 14; i++){
	if(array[i] == 0){
//printf("%d\n", array[i]);
	    return 0;
	}
//printf("%d\n", array[i]);
    }

    return 1;
}




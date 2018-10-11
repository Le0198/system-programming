/**
* Extreme Edge Cases Lab
* CS 241 - Fall 2018
*/

#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

char **camel_caser(const char *input_str) {
    if(!input_str){
    	return NULL;
    }
    char** result = (char**)malloc(sizeof(char*));
    const char* temp1 = input_str;
    const char* temp2 = input_str;
    int count = 0;
    int strcount = 0;
//printf("input %d\n", (int)strlen(input_str));
    while(strcount < (int)strlen(input_str)){
	if(ispunct(*temp2)){

    	    result = (char**)realloc(result, sizeof(char*) * (2 + count));
	    if(temp1 == temp2){
		result[count] = (char*)malloc(1);
		//strcpy(result[count],"");
		result[count][0] = '\0';
	    }
	    else{
		result[count] = (char*)malloc(temp2 - temp1 + 1);
		strncpy(result[count], temp1, (int)(temp2 - temp1));
		result[count][temp2-temp1] = '\0';
	    }
	     
	    count ++;
//printf("%s   %d\n", result[count-1], count);    
            temp1 = temp2 + 1;
	    
	}

	temp2 ++;
	strcount ++;
	//if(strcount == 13){break;}
//printf("strcount: %d\n", strcount);
//printf("%c\n", *temp2);
    }

//printf("................................\n");



    result[count] = NULL;
//printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    for(int i = 0; i < count; i++){
	
	for(int j = 0; j < (int)strlen(result[i]); j++){
	    if(isalpha(result[i][j])){
		result[i][j] = tolower(result[i][j]);
		if(j > 0){
	    	if(isdigit(result[i][j-1])){
		result[i][j] = toupper(result[i][j]);
	       } 
	}
            }

	    while(isspace(result[i][j])){	
		memmove(&result[i][j], &result[i][j+1], strlen(result[i]) - j);			
		if(isalpha(result[i][j])){
		    result[i][j] = toupper(result[i][j]);
		}
	    }
	}

	if(isalpha(result[i][0])){
	   result[i][0] = tolower(result[i][0]);
	}
    }
    return result;
}

void destroy(char **result) {
    if(result == NULL){return;}
    char** tmp = result;
    while(*tmp){
	free(*tmp);
        tmp++;
    }
    free(result);
    result = NULL;
    return;
}

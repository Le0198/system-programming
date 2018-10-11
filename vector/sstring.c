/**
* Vector Lab
* CS 241 - Fall 2018
*/

#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {

  //  vector* source = char_vector_create();
  //  size_t len;
    char* source;
    size_t len; 
};

sstring *cstr_to_sstring(char *input) {
    // your code goes here
    sstring * result = malloc(sizeof(sstring));
    result->source = strdup(input);
    result->len = strlen(input);
    return result;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    if(!input){return NULL;}
    char* result = strdup(input->source);
    return result;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    // NULL case?????
    this->source = realloc(this->source, this->len + addition->len + 1);
    strcat(this->source, addition->source);
    this->len += addition->len;
    return this->len;
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    char d[2];
    d[0] = delimiter;
    d[1] = '\0';
    char* token;
    vector* v = string_vector_create();
    char* sstr = strdup(this->source);
    token = strtok(sstr, d);
    while(token != NULL){
	vector_push_back(v, token);
	token = strtok(NULL, d);
    }
    free(sstr);
    return v;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here

    //printf("1: %s\n", this->source);

    char* a = strstr(this->source + offset, target);
    if(a == NULL){
        return -1;
    }
    int part1 = (int)(a - this->source);
    int part2 = (int)(this->source + strlen(this->source) - a - strlen(target));
    //printf("part1: %d\npart2: %d\n" , part1, part2);
    //printf("2: %s\n", a);
    char b[part1 + 1];
    char c[part2 + 1];
    strncpy(b, this->source, part1);
    b[part1] = '\0';
    c[part2] = '\0';
    strcpy(c, a + strlen(target));
   
    //printf("3: %s\n4: %s\n", b, c);
    this->len = this->len - strlen(target) + strlen(substitution);
    this->source = realloc(this->source, this->len + 1);
    strcpy(this->source, b);
    strcat(this->source, substitution);
    strcat(this->source, c);
   
    return 0;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char* a = this->source + start;
    char* b = malloc(end-start+1);
    b[end-start] = '\0';
    strncpy(b, a, end-start);
    return b;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    //vector_destroy(this->source);
    free(this->source);
    free(this);
}

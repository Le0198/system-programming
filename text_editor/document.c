/**
* Text Editor Lab
* CS 241 - Fall 2018
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "document.h"
#include "vector.h"
#include <fcntl.h>
#include <unistd.h>

struct document {
    vector *vector;
};

document *document_create() {
    document *this = (document *)malloc(sizeof(document));
    assert(this);
    this->vector = vector_create(string_copy_constructor, string_destructor,
                                 string_default_constructor);
    return this;
}

void document_write_to_file(document *this, const char *path_to_file) {
    assert(this);
    assert(path_to_file);
    // see the comment in the header file for a description of how to do this!
    // TODO: your code here!
    mode_t mode = S_IROTH | S_IWOTH | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    int fildes = open(path_to_file, O_CREAT | O_TRUNC | O_RDWR, mode);
    for(int i = 0; i < (int)vector_size(this->vector); i++){
	dprintf(fildes, "%s\n", vector_get(this->vector, i));
    }
    close(fildes);
    return;
}

document *document_create_from_file(const char *path_to_file) {
    assert(path_to_file);
    // this function will read a file which is created by document_write_to_file
    // TODO: your code here!
    FILE* fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    document* dd = document_create();
    fp = fopen(path_to_file, "r");
    if(fp == NULL){
	return dd;
    }
    while((read = getline(&line, &len, fp)) != -1){
	if(line[read - 1] == '\n'){line[read - 1] = '\0';}
	vector_push_back(dd->vector, line);
    }
    free(line);
    fclose(fp);
    
    return dd;
}

void document_destroy(document *this) {
    assert(this);
    vector_destroy(this->vector);
    free(this);
}

size_t document_size(document *this) {
    assert(this);
    return vector_size(this->vector);
}

void document_set_line(document *this, size_t line_number, const char *str) {
    assert(this);
    assert(str);
    size_t index = line_number - 1;
    vector_set(this->vector, index, (void *)str);
}

const char *document_get_line(document *this, size_t line_number) {
    assert(this);
    assert(line_number > 0);
    size_t index = line_number - 1;
    return (const char *)vector_get(this->vector, index);
}

void document_insert_line(document *this, size_t line_number, const char *str) {
    assert(this);
    assert(str);
    // TODO: your code here!
    // How are you going to handle the case when the user wants to
    // insert a line past the end of the document?
    
    if(line_number > vector_size(this->vector)){
	for(size_t i = vector_size(this->vector); i < line_number - 1; i++){
	  vector_insert(this->vector, i, "");  
	}
    }
    //vector_resize(this->vector, vector_size(this->vector) + 1);
    vector_insert(this->vector, line_number - 1, (char*)str);

}


void document_delete_line(document *this, size_t line_number) {
    assert(this);
    assert(line_number > 0);
    size_t index = line_number - 1;
    vector_erase(this->vector, index);
}

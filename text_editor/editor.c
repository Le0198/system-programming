/**
* Text Editor Lab
* CS 241 - Fall 2018
*/

#include "document.h"
#include "editor.h"
#include "format.h"
#include "sstring.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_filename(int argc, char *argv[]) {
    // TODO implement get_filename
    // take a look at editor_main.c to see what this is used for
    char* name = argv[argc-1];   
    return name;
}

sstring *handle_create_string() {
    // TODO create empty string
    sstring* s = cstr_to_sstring(""); 
    return s;
}

document *handle_create_document(const char *path_to_file) {
    // TODO create the document
    
    return document_create_from_file(path_to_file);
}

void handle_cleanup(editor *editor) {
    // TODO destroy the document
    document_destroy(editor->document);
}

void handle_display_command(editor *editor, size_t start_line,
                            ssize_t max_lines, size_t start_col_index,
                            ssize_t max_cols) {
    // TODO implement handle_display_command
    size_t dsize = document_size(editor->document);
    if(editor->document == NULL || dsize == 0){
	print_document_empty_error();
    }
    else{
	if(max_lines == -1){
	    for(size_t i = start_line; i <= dsize; i++){
	        print_line(editor->document, i, start_col_index, max_cols);
	    }
        }
        else{
	    for(size_t i = start_line; i < start_line + max_lines && i <= dsize; i++){
		print_line(editor->document, i, start_col_index, max_cols);
            }
        }
    }

}

void handle_insert_command(editor *editor, location loc, const char *line) {
    // TODO implement handle_insert_command
    assert(loc.line_no >= 1);
    assert(loc.idx >= 0);
    assert(line != NULL);
    if(document_size(editor->document) < loc.line_no){
	document_insert_line(editor->document, loc.line_no, line);
    }
    else{
	char* tmp = (char*)document_get_line(editor->document, loc.line_no);
	char str[strlen(tmp) + strlen(line) + 1];
	strncpy(str, tmp, loc.idx);
	str[loc.idx] = '\0';
	char* second = tmp + loc.idx;
	    
	strcat(str, line);
	strcat(str, second);
	
	document_set_line(editor->document, loc.line_no, str);
	
	return;
	
    }
}

void handle_append_command(editor *editor, size_t line_no, const char *line) {
    // TODO implement handle_append_command
    assert(line);
    assert(editor->document);
    assert(line_no > 0);
    if(line_no > document_size(editor->document)){
	document_insert_line(editor->document, line_no, "");
    }
    size_t len = strlen(line);
    char* str = malloc(len + 1);
    char* ptr = str;
    while(*line != '\0'){
	if(*line == '\\'){
            if(*(line + 1) == 'n'){
                *ptr++ = '\n';
                line++;
            }
	    else if(*(line + 1) != '\0') {
                *ptr++ = *(line + 1);
                line++;
            }
        } 
	else {
            *ptr++ = *line;
        }
	line++;
    }
    *ptr = '\0';
    vector *v = string_vector_create();
    ptr = str;
    while ( ptr ) {
        char *s = strsep(&ptr, "\n");
        vector_push_back(v, s);
    }
    free(str);
    char *first = *vector_front(v);
    const char *origin = document_get_line(editor->document, line_no);
    char *mystr = malloc(strlen(origin) + strlen(first) + 1);
    strcpy(mystr, origin);
    strcat(mystr, first);
    document_set_line(editor->document, line_no, mystr);
    free(mystr);
    for (size_t i = 1; i < vector_size(v); i++) {
        document_insert_line(editor->document, ++line_no, vector_get(v, i));
    }
    vector_destroy(v);
}


void handle_write_command(editor *editor, size_t line_no, const char *line) {
    // TODO implement handle_write_command
    assert(line_no > 0);
    if(line_no > document_size(editor->document)){
	document_insert_line(editor->document, line_no, "");	
    }
    char* temp = strdup(line);
    //if(temp[strlen(temp) - 1] == '\\'){temp[strlen(temp) - 1] = '\0';}
    size_t len = strlen(temp);
    for(size_t j = 0; j < len; j++){
        if(temp[j] == '\\'){
            if(temp[j + 1] != 'n'){
                memmove(temp + j, temp + j + 1, strlen(temp) - j);
                //temp[strlen(temp)-1] = '\0';
            }
            else{
                temp[j + 1] = '\n';
                memmove(temp + j, temp + j + 1, strlen(temp) - j);
            } 
         }
    }
    vector *v = string_vector_create();
    char* ptr = temp;
    while ( ptr ) {
        char *s = strsep(&ptr, "\n");
        vector_push_back(v, s);
    }
    free(temp);
    char *first = *vector_front(v);
    document_set_line(editor->document, line_no, first);
    
    for (size_t i = 1; i < vector_size(v); i++) {
        document_insert_line(editor->document, ++line_no, vector_get(v, i));
    }
    vector_destroy(v);
}

void handle_delete_command(editor *editor, location loc, size_t num_chars) {
    // TODO implement handle_delete_command
    assert(num_chars >= 0);
    if(loc.line_no > document_size(editor->document)){
	return;
    }
    char* line = strdup(document_get_line(editor->document, loc.line_no));
    sstring* mystr = cstr_to_sstring(line);
    size_t len = strlen(line);
    if(loc.idx + num_chars >= len){
	line = sstring_slice(mystr, 0, loc.idx);
	document_set_line(editor->document, loc.line_no, line);
    }
    else{
	char* temp = strdup(sstring_slice(mystr, 0, loc.idx));
	temp = realloc(temp, strlen(line) - num_chars + 1);
	strcat(temp, line + num_chars + loc.idx);
	document_set_line(editor->document, loc.line_no, temp);
	free(temp);
    }
    sstring_destroy(mystr);
    free(line);
}

void handle_delete_line(editor *editor, size_t line_no) {
    // TODO implement handle_delete_line
    if(line_no > document_size(editor->document)){return;}
    document_delete_line(editor->document, line_no);
}

location handle_search_command(editor *editor, location loc,
                               const char *search_str) {
    // TODO implement handle_search_command
    if(strcmp(search_str, "") == 0){
	return (location){0, 0};
    }
    int index = (int)loc.idx;
    for(size_t i = loc.line_no; i < loc.line_no + document_size(editor->document); i++){
	size_t j = i; 
	if(i > document_size(editor->document)){
	    j = i - document_size(editor->document);
	}
	char* curr = (char*)document_get_line(editor->document , j);
	if(index >= (int)strlen(curr)){
	    curr = (char*)document_get_line(editor->document , j);
	    index = -1;
	}
	if(i > loc.line_no){index = -1;}
	char* search = strstr(curr + index + 1, search_str);
	if(search){
	    location target;
	    target.line_no = j;
	    target.idx = search - curr;
	    return target;
	}
    }

    return (location){0, 0};
}

void handle_merge_line(editor *editor, size_t line_no) {
    // TODO implement handle_merge_line
    assert(line_no < document_size(editor->document));
    char* temp = strdup(document_get_line(editor->document, line_no));
    char* next = (char*)document_get_line(editor->document, line_no + 1);
    temp = realloc(temp, strlen(temp) + strlen(next) + 1);
    strcat(temp, next);
    document_set_line(editor->document, line_no, temp);
    document_delete_line(editor->document, line_no + 1);
    free(temp);
}

void handle_split_line(editor *editor, location loc) {
    // TODO implement handle_split_line
    
    char* temp = (char*)document_get_line(editor->document, loc.line_no);
    if(loc.idx >= strlen(temp)){
	return;
    }
    char* nextline = temp + loc.idx;
    document_insert_line(editor->document, loc.line_no + 1, nextline);
    sstring* mystr = cstr_to_sstring(temp);
    temp = sstring_slice(mystr, 0, loc.idx);
    document_set_line(editor->document, loc.line_no, temp);
    sstring_destroy(mystr);
}

void handle_save_command(editor *editor) {
    // TODO implement handle_save_command
    document_write_to_file(editor->document, editor->filename);

}

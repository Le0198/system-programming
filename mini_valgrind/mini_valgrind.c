/**
* Mini Valgrind Lab
* CS 241 - Fall 2018
*/

#include "mini_valgrind.h"
#include <stdio.h>
#include <string.h>



meta_data* head = NULL;
size_t total_memory_requested = 0;
size_t total_memory_freed = 0;
size_t invalid_addresses = 0;


void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here

    if(request_size == 0){return NULL;}
    meta_data* ptr = NULL;
    
    void* ptr2 = NULL;
    
    ptr = (meta_data*)malloc(request_size + sizeof(meta_data));
    if(ptr == NULL){
	return NULL;
    }
    if(head){
	meta_data* tmp = head->next;
    	head->next = ptr;
	ptr->next = tmp;
    }
    if(!head){
	head = ptr;
    }
    ptr2 = ptr + 1;
    
    ptr->request_size = request_size;
    ptr->filename = filename;
    ptr->instruction = instruction;
    
    total_memory_requested += request_size;    

    return ptr2;
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    if(num_elements == 0 || element_size == 0){
	return NULL;
    }
    meta_data* ptr = (meta_data*)malloc(num_elements * element_size + sizeof(meta_data));
    if(ptr == NULL){return NULL;}
    if(head){
        meta_data* tmp = head->next;
        head->next = ptr;
        ptr->next = tmp;
    }
    if(!head){
        head = ptr;
    }
    void* ptr2 = ptr + 1;
    memset(ptr2, 0, num_elements * element_size);
    
    ptr->request_size = num_elements * element_size;
    ptr->filename = filename;
    ptr->instruction = instruction;
    
    total_memory_requested += num_elements * element_size;
    
    return (void*)ptr2;

}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    if(request_size == 0 && payload == NULL){
	return NULL;
    }
    else if(request_size == 0){
	mini_free(payload);
	return NULL;
    }
    else if(payload == NULL){
	return mini_malloc(request_size, filename, instruction);
    }
    else{
	meta_data* ptr = (meta_data*)payload - 1;
        meta_data* temp = head;
        int flag = 0;
        while(temp){
            if(ptr == temp){
		ptr = realloc(ptr, request_size + sizeof(meta_data));
                flag = 1;
	        int sub = (int)request_size - (int)ptr->request_size; 	
		if(sub < 0){
		    total_memory_freed -= sub;
		}
		else{
		    total_memory_requested += sub;
		}
		ptr->request_size = request_size;
		ptr->filename = filename;
    		ptr->instruction = instruction;
		meta_data* ptr2 = ptr + 1;
		return (void*)ptr2;
            }
            temp = temp->next;
        }
        if(flag == 0){
            invalid_addresses++;
        }
	return NULL;

    }
    
    


    return NULL;
}

void mini_free(void *payload) {
    // your code here
    if(payload == NULL){return;}
    meta_data* ptr = (meta_data*)payload - 1;
    int flag = 0;
    if(head == ptr){
	head = head->next;
	total_memory_freed += ptr->request_size;
	free(ptr);
	flag = 1;
    }
    else{
	meta_data* temp = head;
	while(temp){
	    if(ptr == temp->next){
	        total_memory_freed += ptr->request_size;
	        temp->next = ptr->next;
	        free(ptr);
	        flag = 1;
		break;
	    }
	    temp = temp->next;
        }
    }
    if(flag == 0){
	invalid_addresses++;
    }
}

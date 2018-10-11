/**
* Vector Lab
* CS 241 - Fall 2018
*/

#include "vector.h"
#include "stdio.h"
#include "unistd.h"
int main(/*int argc, char *argv[]*/) {
    // Write your test cases here

    vector* v = string_vector_create();
    vector_insert(v, 0, "Alsa");

    printf("%s\n",(char*)vector_get(v, 0));
    vector_set(v, 0, "Lucy");
    vector_push_back(v, "Grey");
    vector_push_back(v, "Nastu");
    vector_push_back(v, "Wendy");
    vector_push_back(v, "Labby");
    vector_push_back(v, "May");
    vector_push_back(v, "Luffy");
    vector_insert(v, 1, "Nami");
    vector_resize(v, 10);

    for(size_t i = 0; i < vector_size(v); i++){
	printf("%d : %s\n", (int)i, (char*)(*vector_at(v, i)));
    }
    printf("Capacity: %d\n", (int)vector_capacity(v));
    printf("%s\n", (char*)(*vector_front(v)));
    printf("%s\n", (char*)(*vector_back(v)));

    vector_erase(v, 3);
    for(size_t i = 0; i < vector_size(v); i++){
	printf("%d : %s\n", (int)i, (char*)vector_get(v, i));
    }
    vector_reserve(v, 20);
    printf("Size: %d, Capacity: %d\n", (int)vector_size(v), (int)vector_capacity(v));
    vector_pop_back(v);
    printf("Current size: %d\n", (int)vector_size(v));
    printf("%s\n", (char*)(*vector_back(v)));
    
    vector_resize(v, 4);
    for(size_t i = 0; i < vector_size(v); i++){
	printf("%d : %s\n", (int)i, (char*)vector_get(v, i));
    }
    printf("Size: %d, Capacity: %d\n", (int)vector_size(v), (int)vector_capacity(v));
    
    vector_resize(v, 1);
    
    for(size_t i = 0; i < vector_size(v); i++){
	printf("%d : %s\n", (int)i, (char*)vector_get(v, i));
    }
    printf("Size: %d, Capacity: %d\n", (int)vector_size(v), (int)vector_capacity(v));
    vector_resize(v, 0); 
    vector_destroy(v);
    

    vector* vec = int_vector_create();
    int a = 1;
    int b = 2;
    int* a1 = &a;
    int* a2 = &b;
    vector_push_back(vec, a1);
    vector_push_back(vec, a2);
    vector_clear(vec);
    printf("\n\n\n%d\n", *((int*)vector_get(vec, 0)));
    vector_destroy(vec);


    

    return 0;
}

/**
* Vector Lab
* CS 241 - Fall 2018
*/

#include "sstring.h"

int main() {
    // TODO create some tests
    sstring* test;
    char * a = "hello";
    test = cstr_to_sstring(a);
    char * b = "world";
    sstring* test2 = cstr_to_sstring(b);
    int n = sstring_append(test, test2);
    char* ssssh = sstring_to_cstr(test);
    printf("%d\n%s\n", n, ssssh);

    char* s = "I love you!";
    sstring* test3 = cstr_to_sstring(s);
    vector* v = sstring_split(test3, ' ');
    for(int i = 0; i < (int)vector_size(v); i++){
	printf("%s\n", vector_get(v, i));
    }
    char* s0 = sstring_slice(test3, 2, 5);
    printf("%s\n", s0);
    
    
    sstring *replace_me = cstr_to_sstring("This is a {} day, {}!");
    sstring_substitute(replace_me, 18, "{}", "friend");
    char* s1 = sstring_to_cstr(replace_me); // == "This is a {} day, friend!"
    printf("%s\n", s1);    

    sstring_substitute(replace_me, 0, "{}", "good");
    char* s2 = sstring_to_cstr(replace_me); // == "This is a good day, friend!"
    printf("%s\n", s2);
    
    sstring *slice_me = cstr_to_sstring("1234567890");
    char* result = sstring_slice(slice_me, 2, 5);
    printf("%s\n", result);
        
    sstring_destroy(slice_me);
    free(result);
    free(ssssh);
    free(s0);
    free(s1);
    free(s2);
    sstring_destroy(test);
    sstring_destroy(replace_me);
    sstring_destroy(test2);
    sstring_destroy(test3);
    vector_destroy(v);
    
    return 0;
}

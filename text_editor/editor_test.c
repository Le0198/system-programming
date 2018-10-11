/**
* Text Editor Lab
* CS 241 - Fall 2018
*/

#include "document.h"
#include "editor.h"
#include "format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * You can programatically test your text editor.
*/
int main() {
    // Setting up a docment based on the file named 'filename'.
    char *filename = "test.txt";
    document *doc = document_create();//document_create_from_file(filename);
    document_insert_line(doc, 1, "Good morning!!!!!!!!!!!");
    document_insert_line(doc, 2, "The mousetrap!!!!!!!!!!!");
    document_insert_line(doc, 3, "Then there were no one.!!!!!!!1");
    document_insert_line(doc, 10, "Thanks giving day!!!!!!!!!!!!!!!!!!");
    document_write_to_file(doc, filename);
   

    return 0;
}

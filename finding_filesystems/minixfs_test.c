/**
* Finding Filesystems Lab
* CS 241 - Fall 2018
*/

#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Write tests here!
    file_system* fs = open_fs("/virtual");

    inode* n = get_inode(fs, "/to_create"); // should return null
    inode* created = minixfs_create_inode_for_path(fs, "/to_create");

    printf("%d\n", n == NULL);

    struct stat s; 
    minixfs_stat(fs, "/to_create", &s);
    printf("n is %ld\n", s.st_ctime);


    inode* n1 = get_inode(fs, "/to_create"); // should be the same as created
    printf("%d\n", created == n1);
    inode* n2 = minixfs_create_inode_for_path(fs, "/to_create"); // should return null, but the timestamps should be updated.
    printf("%d\n", n2 == NULL);

    minixfs_stat(fs, "/to_create", &s);
    printf("n is %ld\n", s.st_ctime);

    close_fs(&fs);
}

/**
* Finding Filesystems Lab
* CS 241 - Fall 2018
*/

#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", "myvir"};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string, "Free blocks: %zd\n"
                            "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows! 
    const char* mypath = is_virtual_path(path);
    if (mypath) {path = strdup(mypath);}
    
    inode* node = get_inode(fs, path);
    if (node == NULL) {
	errno = ENOENT;
	return -1;
    }

    uint16_t file_type = node->mode >> RWX_BITS_NUMBER;
    uint16_t perm = (uint16_t)new_permissions;
    perm &= 0x01ff;
    node->mode = (file_type << RWX_BITS_NUMBER) | perm;

    clock_gettime(CLOCK_REALTIME, &node->ctim);
    if (mypath) { free(path); }
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    const char* mypath = is_virtual_path(path);
    if (mypath) {path = strdup(mypath);}
    
    inode* node = get_inode(fs, path);
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }

    if (owner != ((uid_t)-1)) {
	node->uid = owner;
    }
    if (group != ((gid_t)-1)) {
	node->gid = group;
    }
    clock_gettime(CLOCK_REALTIME, &node->ctim);
    if (mypath) { free(path); }
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!

    //virtual path???

    if (path == NULL) {return NULL;} 
    const char* filenm = (const char*)malloc(256);
    inode* parent = parent_directory(fs, path, &filenm);
    if (parent == NULL || valid_filename(filenm) == 0 || is_file(parent)) {  
	return NULL; 
    }
    inode* node = get_inode(fs, path);
    if (node) { 
	clock_gettime(CLOCK_REALTIME, &node->ctim);
	return NULL; 
    }

    inode_number inum = first_unused_inode(fs);
    if (inum == -1) {return NULL;}
    

    char* ptr = NULL;
    data_block_number block = 0;
    
    // add to direct
    //parent size = 0
    if (parent->size == 0) {
	block = add_data_block_to_inode(fs, parent);
	if (block == -1) {return NULL;}
	ptr = (fs->data_root + parent->direct[0])->data;
    }
    //parent size != 0
    if (ptr == NULL) {
        int bnum = parent->size/sizeof(data_block);
        size_t occupied  = parent->size - bnum * sizeof(data_block);
        size_t room = sizeof(data_block) - occupied;
        if ( room >= 256 ) {
	    ptr = (fs->data_root + parent->direct[bnum])->data + occupied;
        } else {
	    block = add_data_block_to_inode(fs, parent);
	    if (block == -1) { return NULL; }
	    else if (block != 0) {	
	        ptr = (fs->data_root + parent->direct[bnum])->data;
	    }
	}
    }

    //add to indirect
    if (ptr == NULL) {
	inode_number ind = add_single_indirect_block(fs, parent);
	if (ind == -1) { return NULL; }
	data_block_number* barray = (data_block_number*)(fs->data_root + parent->indirect);
	size_t indsz = parent->size - NUM_DIRECT_INODES * sizeof(data_block);
	// indsz == 0
	if (indsz == 0) {
	    block = add_data_block_to_indirect_block(fs, barray);
	    if (block == -1) {return NULL;}
	    ptr = (fs->data_root + barray[0])->data;
	}
	// indsz != 0
	if (indsz != 0) {
	    int indnum = indsz/sizeof(data_block);
	    size_t used = indsz - indnum * sizeof(data_block);
	    size_t remain = sizeof(data_block) - used;
	    if ( remain >= 256 ) {
	        ptr = (fs->data_root + barray[indnum])->data + used;
	    } else {
		block = add_data_block_to_indirect_block(fs, barray);
		if (block == -1) {return NULL;}
		else if (block != 0) {
		    ptr = (fs->data_root + barray[indnum])->data;
		}
	    }
	}
    }
    if (ptr == NULL) {return NULL;}

    //Initialize inode
    node = fs->inode_root + inum;
    init_inode(parent, node);

    minixfs_dirent *entry = malloc(sizeof(minixfs_dirent));
    entry->inode_num = inum;
    entry->name = (char*)filenm;
    make_string_from_dirent(ptr, *entry);
    parent->size += 256;
    free(entry); 

    //free((void*)filenm);
    return node;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info") || !strcmp(path, "myvir")) {
        // TODO implement the "info" virtual file here
	size_t used = 0;
	size_t unused = 0;
	superblock* tmp = fs->meta;
	for (size_t i = 0; i < tmp->dblock_count; i++) {
	    if (GET_DATA_MAP(tmp)[i] == 1) {
		used++;
	    }
	    if (GET_DATA_MAP(tmp)[i] == 0) {
		unused++;
	    }
	}
	char str[100];
	sprintf(str, "Free blocks: %lu\nUsed blocks: %lu\n", unused, used);
	char* p = str;
	size_t sz = 0;
	while (*p != '\n') { p++; sz++; }
	p++; sz++;
	while (*p != '\n') { p++; sz++; }
	sz++;	
	if ((size_t)(*off) >= sz) {
	    return 0;
	}
	if (*off + count > sz) { 
	    count = sz - *off;
	}
	char* ptr = str + *off;

	memcpy(buf, ptr, count);
	*off += count;
	return (ssize_t)count;
    }
    // TODO implement your own virtual file here
    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    size_t maxsz = sizeof(data_block) * (NUM_DIRECT_INODES + NUM_INDIRECT_INODES);
    if (*off + count > maxsz) {errno = ENOSPC; return -1;}
    
    int needBlock = (*off + count)/sizeof(data_block);
    size_t newoff = (*off + count) % sizeof(data_block);
    if (newoff > 0) {needBlock++;}

    size_t num = *off/sizeof(data_block);
    size_t offset = *off % sizeof(data_block);

    if (minixfs_min_blockcount(fs, path, needBlock) == -1) {
	errno = ENOSPC;
	return -1;
    }

    inode* node = get_inode(fs, path);
    clock_gettime(CLOCK_REALTIME, &node->mtim);
    clock_gettime(CLOCK_REALTIME, &node->atim);

    // find where to start
    int flag = 0;
    char* ptr = NULL;
    if (num < NUM_DIRECT_INODES) {
        ptr = (fs->data_root + node->direct[num])->data + offset;
    }
    else if (num < (NUM_INDIRECT_INODES + NUM_DIRECT_INODES)) {
        flag = 1;
        data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
        ptr = (fs->data_root + barray[num - NUM_DIRECT_INODES])->data + offset;
    }
    else { errno = ENOSPC; return -1; }
    
    //----------------write to data block----------------------
    size_t remain = sizeof(data_block) - offset;
    if (count <= remain) { 
        memcpy(ptr, buf, count);
	node->size += (*off + count - node->size);
	*off += count;
	return count; 
    }
    memcpy(ptr, buf, remain);
    const void *p = buf + remain;
    size_t new_count = count - remain;
    int block = new_count/sizeof(data_block);
    // start from indirect
    if (flag) {
	data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
	for (int i = 0; i < block; i++) {
	    ptr = (fs->data_root + barray[num - NUM_DIRECT_INODES + i + 1])->data;
	    memcpy(ptr, p, sizeof(data_block));
	    p += sizeof(data_block);
        }
        if (newoff > 0) {
           ptr = (fs->data_root + barray[num - NUM_DIRECT_INODES + block + 1])->data;
           memcpy(ptr, p, newoff);
	   p += newoff;
        }
    }
    // start from direct
    if (flag == 0) {
       for (int i = 1; i < block + 1; i++) {
	  if (num + i < NUM_DIRECT_INODES) {
              ptr = (fs->data_root + node->direct[num + i])->data;
	      memcpy(ptr, p, sizeof(data_block));
              p += sizeof(data_block);
	  }
	  else{
              data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
              ptr = (fs->data_root + barray[num + i - NUM_DIRECT_INODES])->data;
              memcpy(ptr, p, sizeof(data_block));
              p += sizeof(data_block);
          }
       }
       if (newoff > 0) {
	  if (num + block + 1 < NUM_DIRECT_INODES) {
            ptr = (fs->data_root + node->direct[num + block + 1])->data;
            memcpy(ptr, p, newoff);
            p += newoff;
          }
	  else if (num + block + 1 >= NUM_DIRECT_INODES) {
	    data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
            ptr = (fs->data_root + barray[num + block + 1 - NUM_DIRECT_INODES])->data;
            memcpy(ptr, p, sizeof(data_block));
            p += sizeof(newoff); 
	  }
        }
    }
    node->size = *off + count;
    *off += count;
    return count;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
//printf("using read\n\n\n");
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!

    if (path == NULL) {errno = ENOENT; return -1;}
    inode* node = get_inode(fs, path);
    if (node == NULL) {errno = ENOENT; return -1;}

    // updata atim
    clock_gettime(CLOCK_REALTIME, &node->atim);
    uint64_t myoff = (uint64_t)(*off);
    if (myoff >= node->size) { return 0; }
    int flag = 0;
    size_t num = myoff/sizeof(data_block);
    size_t offset = myoff - num * sizeof(data_block);
    size_t remain = sizeof(data_block) - offset;
    
    char* tmp = buf;
    
    char* ptr = NULL;
    // find where to start
    if (num < NUM_DIRECT_INODES) {
    	ptr = (fs->data_root + node->direct[num])->data + offset;
    } 
    else if (num < (NUM_INDIRECT_INODES +  NUM_DIRECT_INODES)) {
	flag = 1;
	data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
	ptr = (fs->data_root + barray[num - NUM_DIRECT_INODES])->data + offset;
    }

    //should not reach here
    else { return 0; }  

    // process count
    if (myoff + count >= node->size) {
	count = node->size - myoff;
    } 

    //---------copy into buf-----------
    if (count <= remain) {
	//strncpy((char*)buf, ptr, count);
	memcpy((char*)buf, ptr, count);
	//((char*)buf)[count] = '\0';
	tmp += count;
    }

    else {
//	strncpy((char*)buf, ptr, remain);
	memcpy((char*)buf, ptr, remain);
	tmp += remain;
	//((char*)buf)[remain] = '\0';
	char* p = NULL;
	int blocknum = (count - remain)/sizeof(data_block);
	size_t newoff = (count - remain) - blocknum * sizeof(data_block);
	// start from indirect
	if (flag) {
	    data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
            for (int i = 0; i < blocknum; i++) {
		p = (fs->data_root + barray[num - NUM_DIRECT_INODES + 1 + i])->data;
            	//strncat((char*)buf, p, sizeof(data_block));
	    	memcpy(tmp, p, sizeof(data_block));
		tmp += sizeof(data_block);
	    }
	    if (num - NUM_DIRECT_INODES + 1 + blocknum < NUM_INDIRECT_INODES) {
		p = (fs->data_root + barray[num - NUM_DIRECT_INODES + 1 + blocknum])->data;
	        //strncat((char*)buf, p, newoff);
		memcpy(tmp, p, newoff);
                tmp += newoff;
	    }
	}
	// start from direct
	if (flag == 0) {
	    if (num + blocknum + 1 < NUM_DIRECT_INODES) {
		for (int i = 0; i < blocknum; i++) {
                    p = (fs->data_root + node->direct[num + 1 + i])->data;
                    //strncat((char*)buf, p, sizeof(data_block));
                    memcpy(tmp, p, sizeof(data_block));
                    tmp += sizeof(data_block);
		}
		if (num + 1 + blocknum < NUM_DIRECT_INODES) {
		    p = (fs->data_root + node->direct[num + 1 + blocknum])->data;
		    //strncat((char*)buf, p, newoff);
		    memcpy(tmp, p, newoff);
                    tmp += newoff;
		}
	    }
	    if (num + blocknum + 1 >= NUM_DIRECT_INODES) {
		for (size_t i = 0; i < NUM_DIRECT_INODES - num - 1; i++) {
                    p = (fs->data_root + node->direct[num + 1 + i])->data;
                    //strncat((char*)buf, p, sizeof(data_block));
                    memcpy(tmp, p, sizeof(data_block));
                    tmp += sizeof(data_block);
		}
		size_t remain_block = num + blocknum + 1 - NUM_DIRECT_INODES;
		data_block_number* barray = (data_block_number*)(fs->data_root + node->indirect);
		for (size_t i = 0; i < remain_block; i++) {
		    p = (fs->data_root + barray[i])->data;
		    //strncat((char*)buf, p, sizeof(data_block));
		    memcpy(tmp, p, sizeof(data_block));
                    tmp += sizeof(data_block);
		}
		if (remain_block < NUM_INDIRECT_INODES) {
		    p = (fs->data_root + barray[remain_block])->data;
		    //strncat((char*)buf, p, newoff);
		    memcpy(tmp, p, newoff);
                    tmp += newoff;
		}
	    }
	}
    }
    *off += count;
//printf("%lu\n", count);
    return count;
}

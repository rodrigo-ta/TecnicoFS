#ifndef FS_H
#define FS_H
#include "state.h"
#include "../locks/rwlock.h"


#define WRITE 1
#define READ 0

void init_fs();
void destroy_fs();
int get_parent(char*, char*, Locks*, char*, type*, union Data*);
int get_child(char*, char*, char*, Locks*, char *, union Data);
int is_dir_empty(DirEntry*);
int exit_create_with_error(char*, char*, char*, Locks*, char*);
int create(char*, type);
int move(char*, char*);
int delete(char*);
int startlookup(char*);
int lookup(char*, Locks*, int);
void print_tecnicofs_tree(FILE*);

Locks * create_locks();

#endif /* FS_H */

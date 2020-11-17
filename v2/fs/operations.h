#ifndef FS_H
#define FS_H
#include "state.h"
#include "../locks/rwlock.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>


/* Constants that describe type of operation to realize */
#define DONOTHING 0
#define WRITE 1
#define READ 2

#define MAXSLEEPTIME 1

/* Maximum i-node numbers in move command (1 for parent, 1 for source) */
#define MAXINUMBERS 2

void init_fs();
void destroy_fs();
int exit_and_unlock(Locks*);
int is_dir_empty(DirEntry*);
int exit_create_with_message(char*, char*, char*, Locks*, char*);
int create(char*, type);
int verify_source(Locks*, char*, char*, char*, int*);
int verify_destination(Locks*, char*, char*, char*, char*, int, int*);
int move(char*, char*);
int delete(char*);
int lookup(char*);
int lookup_node(char*, Locks*, int);
void print_tecnicofs_tree(FILE*);

#endif /* FS_H */

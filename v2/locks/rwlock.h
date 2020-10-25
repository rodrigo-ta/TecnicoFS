/* 
 * HEADER FILE FOR SYNC
 */

#ifndef _RWLOCK_
#define _RWLOCK_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct{
    pthread_rwlock_t ** rwlocks;
    int num;
} Locks;

Locks * create_locks_list();
void list_add_lock(Locks*, pthread_rwlock_t*);
void unlock_all(Locks*);
void free_locks_list(Locks*);
void rwlock_init(pthread_rwlock_t*);
void rwlock_read_lock(pthread_rwlock_t*);
bool rwlock_try_lock(pthread_rwlock_t*);
void rwlock_write_lock(pthread_rwlock_t*);
void rwlock_unlock(pthread_rwlock_t*);
void rwlock_destroy(pthread_rwlock_t*);

#endif
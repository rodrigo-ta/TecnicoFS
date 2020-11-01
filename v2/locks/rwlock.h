/* 
 * HEADER FILE FOR SYNC
 */

#ifndef _RWLOCK_
#define _RWLOCK_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct{
    pthread_rwlock_t ** rwlocks;
    int num;
} Locks;

Locks * list_create();
void list_add_lock(Locks*, pthread_rwlock_t*);
void list_unlock_all(Locks*);
void list_free(Locks*);
void list_write_lock(Locks*);
int list_try_write_lock(Locks*);
void list_read_lock(Locks*);
void rwlock_init(pthread_rwlock_t*);
void rwlock_read_lock(pthread_rwlock_t*);
int rwlock_try_write_lock(pthread_rwlock_t*);
void rwlock_write_lock(pthread_rwlock_t*);
void rwlock_unlock(pthread_rwlock_t*);
void rwlock_destroy(pthread_rwlock_t*);

#endif
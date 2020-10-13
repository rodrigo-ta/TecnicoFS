#ifndef _SYNC_
#define _SYNC_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MUTEX 1
#define RWLOCK 2
#define NOSYNC 3


/* mutex lock */
pthread_mutex_t mutexlock;

/* read-write lock */
pthread_rwlock_t rwlock;


void sync_init(int);
void sync_lock(int);
void sync_read_lock(int);
void sync_write_lock(int);
void sync_unlock(int);
void sync_destroy(int);
void mutex_init();
void mutex_lock();
void mutex_unlock();
void mutex_destroy();

#endif
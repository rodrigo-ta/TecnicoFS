#ifndef _SYNC_
#define _SYNC_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define MUTEX 1
#define RWLOCK 2
#define NOSYNC 3

int synchStrategy;

/* mutex lock */
pthread_mutex_t mutexlock;

/* read-write lock */
pthread_rwlock_t rwlock;


void sync_init();
void sync_read_lock();
bool sync_try_lock();
void sync_write_lock();
void sync_unlock();
void sync_destroy();

#endif
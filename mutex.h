#ifndef _MUTEX_
#define _MUTEX_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void mutex_init(pthread_mutex_t*);
void mutex_lock(pthread_mutex_t*);
void mutex_unlock(pthread_mutex_t*);
void mutex_destroy(pthread_mutex_t*);

#endif
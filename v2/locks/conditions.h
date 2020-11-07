/* 
 * HEADER FILE FOR CONDITIONS
 */

#ifndef _CONDITIONS_
#define _CONDITIONS_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void cond_init_all(pthread_cond_t*, pthread_cond_t*);
void cond_wait(pthread_cond_t*, pthread_mutex_t*);
void cond_signal(pthread_cond_t*);
void cond_broadcast(pthread_cond_t*);
void cond_destroy_all(pthread_cond_t*, pthread_cond_t*);

#endif
/* 
 * SOURCE FILE OF CONDITIONS LOCKS
 */

#include "conditions.h"

/* Initializes condition variable */
void cond_init(pthread_cond_t * cond){
    if(pthread_cond_init(cond, NULL) != 0){
        fprintf(stderr, "Error: Couldn't initialize condition variable\n");
        exit(EXIT_FAILURE);
    }
}

/* Block a condition variable */
void cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex){
    if(pthread_cond_wait(cond, mutex) != 0){
        fprintf(stderr, "Error: Couldn't block a condition variable\n");
        exit(EXIT_FAILURE);
    }
}

/* Unlock a condition variable */
void cond_signal(pthread_cond_t * cond){
    if(pthread_cond_signal(cond) != 0){
        fprintf(stderr, "Error: Couldn't unlock a condition variable\n");
        exit(EXIT_FAILURE);
    }
}

/* Unlocks all threads locked by a condition */
void cond_broadcast(pthread_cond_t * cond){
    if(pthread_cond_broadcast(cond) != 0){
        fprintf(stderr, "Error: Couldn't unlock all threads locked by a condition\n");
        exit(EXIT_FAILURE);
    }
}

/* Destroy condition variable */
void cond_destroy(pthread_cond_t * cond){
    if(pthread_cond_destroy(cond) != 0){
        fprintf(stderr, "Error: Couldn't destroy condition variable\n");
        exit(EXIT_FAILURE);
    }
}
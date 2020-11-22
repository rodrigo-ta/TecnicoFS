/* 
 * SOURCE FILE OF CONDITIONS LOCKS
 */

#include "conditions.h"

/* Initializes all condition variables */
void cond_init_all(pthread_cond_t * cond1, pthread_cond_t * cond2){
    if(pthread_cond_init(cond1, NULL) != 0){
        fprintf(stderr, "Error: Couldn't initialize condition variable\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_cond_init(cond2, NULL) != 0){
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

/* Destroy all condition variables*/
void cond_destroy_all(pthread_cond_t * cond1, pthread_cond_t * cond2){
    if(pthread_cond_destroy(cond1) != 0){
        fprintf(stderr, "Error: Couldn't destroy condition variable\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_cond_destroy(cond2) != 0){
        fprintf(stderr, "Error: Couldn't destroy condition variable\n");
        exit(EXIT_FAILURE);
    }
}
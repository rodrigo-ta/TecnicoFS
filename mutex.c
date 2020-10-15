#include "mutex.h"

/* Initializes mutex lock */
void mutex_init(pthread_mutex_t * mutex){
    if(pthread_mutex_init(mutex, NULL)){ // returns != 0 if not successful
        fprintf(stderr, "Error: couldn't initiate mutex.\n");
        exit(EXIT_FAILURE);
    }
}

/* Locks mutex */
void mutex_lock(pthread_mutex_t * mutex){
    if(pthread_mutex_lock(mutex)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't lock read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Unlocks mutex */
void mutex_unlock(pthread_mutex_t * mutex){
    if(pthread_mutex_unlock(mutex)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't unlock mutex.(MUTEX.c)\n");
        exit(EXIT_FAILURE);
            }
}

/* Destroy mutex */
void mutex_destroy(pthread_mutex_t * mutex){
    if(pthread_mutex_destroy(mutex)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't destroy mutex.\n");
        exit(EXIT_FAILURE);
            }
}
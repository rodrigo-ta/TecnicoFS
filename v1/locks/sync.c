/* 
 * SOURCE FILE OF GLOBAL LOCKS (MUTEX OR RWLOCK) TO FS
 */

#include "sync.h"

/* Initializes mutex lock or read-write lock */
/* Does nothing if sync strategy is nosync. */
void sync_init(){
    switch (synchStrategy){

        case MUTEX:
            if(pthread_mutex_init(&mutexlock, NULL)){ // returns != 0 if not successful
                fprintf(stderr, "Error: couldn't initiate mutex.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case RWLOCK:
            if(pthread_rwlock_init(&rwlock, NULL)){ // returns != 0 if not successful
                fprintf(stderr, "Error: couldn't initiate rwlock.\n");
                exit(EXIT_FAILURE);
            }
            break;

        /* Just presenting sync strategy. */
        case NOSYNC:
            break;

        default:
            exit(EXIT_FAILURE);
    }
}

/* Locks rwlock or mutex in order to read critical areas of access */
/* Does nothing if sync strategy is nosync. */
void sync_read_lock(){
    switch (synchStrategy){

        case MUTEX:   
            if(pthread_mutex_lock(&mutexlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't lock mutex.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case RWLOCK:
            if(pthread_rwlock_rdlock(&rwlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't apply read lock to read-write lock.\n");
                exit(EXIT_FAILURE);
            }
            
            break;

        /* Just presenting sync strategy. */
        case NOSYNC:
            break;

        default:
            exit(EXIT_FAILURE);
    }
}

/* Checks if sync lock is currently in use. If not, locks it */
/* Does nothing if sync strategy is nosync */
bool sync_try_lock(){
    switch (synchStrategy)
    {
        case MUTEX:
            if((pthread_mutex_trylock(&mutexlock)) == 0){
                return true;
            };
            break;
        
        case RWLOCK:
            if((pthread_rwlock_tryrdlock(&rwlock)) == 0){
                return true;
            };
            break;

        /* Just presenting sync strategy. */
        case NOSYNC:
            break;

        default:
            exit(EXIT_FAILURE);
    }
    return false;
}

/* Locks rwlock or mutex in order to write in critical areas of acess. */
/* Does nothing if sync strategy is nosync. */
void sync_write_lock(){
    switch (synchStrategy){

        case MUTEX:
            if(pthread_mutex_lock(&mutexlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't lock mutex.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case RWLOCK:
            if(pthread_rwlock_wrlock(&rwlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't apply write lock to read-write lock.\n");
                exit(EXIT_FAILURE);
            }
            break;

        /* Just presenting sync strategy. */
        case NOSYNC:
            break;

        default:
            exit(EXIT_FAILURE);
    }
}

/* Unlocks rwlock or mutex */
/* Does nothing if sync strategy is nosync. */
void sync_unlock(){
    switch (synchStrategy){

        case MUTEX:
            if(pthread_mutex_unlock(&mutexlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't unlock mutex.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case RWLOCK:
            if(pthread_rwlock_unlock(&rwlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't unlock read-write lock.\n");
                exit(EXIT_FAILURE);
            }
            break;

        /* Just presenting sync strategy. */
        case NOSYNC:
            break;
    }
}

/* Destroys rwlock, mutex. */
/* Does nothing if sync strategy is nosync. */
void sync_destroy(){
    switch (synchStrategy){
        
        case MUTEX:
            if(pthread_mutex_destroy(&mutexlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't destroy mutex.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case RWLOCK:
            if(pthread_rwlock_destroy(&rwlock)){ // returns != 0 if not successful
                fprintf(stderr, "Error: Couldn't destroy read-write lock.\n");
                exit(EXIT_FAILURE);
            }

        /* Just presenting sync strategy. */
        case NOSYNC:
            break;

        default:
            exit(EXIT_FAILURE);
    }
}
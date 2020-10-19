/* 
 * SOURCE FILE OF GLOBAL LOCKS (MUTEX OR RWLOCK) TO FS
 */

#include "rwlock.h"

/* Initializes mutex lock or read-write lock */
void rwlock_init(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_init(rwlock, NULL)){ // returns != 0 if not successful
        fprintf(stderr, "Error: couldn't initiate rwlock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Locks rwlock or mutex in order to read critical areas of access */
void rwlock_read_lock(pthread_rwlock_t* rwlock){  
    if(pthread_rwlock_rdlock(rwlock)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't apply read lock to read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Checks if rwlock lock is currently in use. If not, locks it */
bool rwlock_try_lock(pthread_rwlock_t* rwlock){
    if((pthread_rwlock_tryrdlock(rwlock)) == 0)
        return true;
    return false;
}

/* Locks rwlock or mutex in order to write in critical areas of acess. */
void rwlock_write_lock(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_wrlock(rwlock)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't apply write lock to read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Unlocks rwlock or mutex */
void rwlock_unlock(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_unlock(rwlock)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't unlock read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Destroys rwlock, mutex. */
void rwlock_destroy(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_destroy(rwlock)){ // returns != 0 if not successful
        fprintf(stderr, "Error: Couldn't destroy read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}
/* 
 * SOURCE FILE OF GLOBAL LOCKS (MUTEX OR RWLOCK) TO FS
 */

#include "rwlock.h"

/* Creates a list with an array of rwlocks and number of rwlocks */
Locks * list_create(int size){
	Locks * locks = (Locks*) malloc(sizeof(Locks));
    if(!locks){
        fprintf(stderr, "Error: couldn't allocate memory for locks list.\n");
        exit(EXIT_FAILURE);
    }
	locks->rwlocks = (pthread_rwlock_t**) malloc(size * sizeof(pthread_rwlock_t*));
    if(!(locks->rwlocks)){
        fprintf(stderr, "Error: couldn't allocate memory for locks array of pointers.\n");
        exit(EXIT_FAILURE);
    }
	locks->num = 0;
	return locks;
}

/* Add last rwlock address and increments number of locks */
void list_add_lock(Locks * locks, pthread_rwlock_t * p_rwlock){
    locks->rwlocks[locks->num] = p_rwlock;
    locks->num++;
}

/* Remove last rwlock address and decrements number of locks */
void list_remove_lock(Locks * locks){
    locks->rwlocks[locks->num-1] = NULL;
    locks->num--;
}

/* Unlock every rwlock in locks list */
void list_unlock_all(Locks * locks){
    for(int i = 0; i < locks->num; i++)
		rwlock_unlock(locks->rwlocks[i]);
}

/* Free memory associated to rwlocks array and list */
void list_free(Locks * locks){
    free(locks->rwlocks);
    free(locks);
}

/* Locks (write) last rwlock of array */
void list_write_lock(Locks * locks){
    rwlock_write_lock(locks->rwlocks[locks->num - 1]);
}

/* Checks if last lock is currently in use. If not, locks it. Return value of trying to lock it*/
int list_try_write_lock(Locks * locks){
    printf("trying to lock %p\n", locks->rwlocks[locks->num - 1]);
	pthread_rwlock_unlock(locks->rwlocks[locks->num - 1]);
    return rwlock_try_write_lock(locks->rwlocks[locks->num - 1]);
}

/* Locks (read) last rwlock of array */
void list_read_lock(Locks * locks){
    rwlock_read_lock(locks->rwlocks[locks->num - 1]);
}


/* Initializes mutex lock or read-write lock */
void rwlock_init(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_init(rwlock, NULL) != 0){
        fprintf(stderr, "Error: couldn't initiate rwlock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Locks rwlock or mutex in order to read critical areas of access */
/* Ignores EDEADLK error */
void rwlock_read_lock(pthread_rwlock_t* rwlock){
    int value = pthread_rwlock_rdlock(rwlock);
    if(value != 0 && value != EDEADLK){ /* ignores error EDEADLK in case of move command */  
        fprintf(stderr, "Error: Couldn't apply read lock to read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* 
** Try lock rwlock in order to write.
** Return value if locks was successful (0) or can't be acquired (EBUSY).
** If error condition is other than EBUSY, prints error in stderr and exit program.
*/
int rwlock_try_write_lock(pthread_rwlock_t* rwlock){
    int value = pthread_rwlock_trywrlock(rwlock);
    if(value != 0 && value != EBUSY){
        fprintf(stderr, "Error while trying to lock rwlock in order to write.\n");
        exit(EXIT_FAILURE);
    }
    return value;  
}

/* 
** Try lock rwlock in order to read.
** Return value if locks was successful (0) or can't be acquired (EBUSY).
** If error condition is EINVAL, prints error in stderr and exit program.
*/
int rwlock_try_read_lock(pthread_rwlock_t* rwlock){
    int value = pthread_rwlock_tryrdlock(rwlock);
    if(value != 0 && value != EBUSY){
        fprintf(stderr, "Error while trying to lock rwlock in order to write.\n");
        exit(EXIT_FAILURE);
    }
    return value; 
        
}

/* Locks rwlock or mutex in order to write in critical areas of acess. */
void rwlock_write_lock(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_wrlock(rwlock) != 0){
        fprintf(stderr, "Error: Couldn't apply write lock to read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Unlocks rwlock or mutex */
void rwlock_unlock(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_unlock(rwlock) != 0){
        fprintf(stderr, "Error: Couldn't unlock read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Destroys rwlock, mutex. */
void rwlock_destroy(pthread_rwlock_t* rwlock){
    if(pthread_rwlock_destroy(rwlock) != 0){
        fprintf(stderr, "Error: Couldn't destroy read-write lock.\n");
        exit(EXIT_FAILURE);
    }
}
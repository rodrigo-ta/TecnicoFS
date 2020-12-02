/* 
 *
 * TecnicoFS Server
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fs/operations.h"
#include "locks/mutex.h"
#include "locks/conditions.h"
#include "../tecnicofs-api-constants.h"

int numberThreads = 0;

/* server socket variables */
char * socketName;
char socket_path[MAX_INPUT_SIZE];
int sockfd;
struct sockaddr_un server_addr;
socklen_t server_addrlen;

/* variables and conditional variables */
int threads_waiting_client = 0; // represents if a thread is waiting for a client message
int printing = 0; // represents if one of the threads is currently printing a tree
pthread_mutex_t commands_mutex;
pthread_mutex_t counting_mutex;
pthread_cond_t process_commands;



/* write error message into stdin and exit program */
void exit_with_error(const char* err_msg){
    fprintf(stderr, "%s", err_msg);
    exit(EXIT_FAILURE);
}

void error_parse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void display_usage(char* appName){
    fprintf(stderr, "Usage: %s numthreads socketname\n", appName);
    exit(EXIT_FAILURE);
}

int apply_commands(char * command){
    int result = FAIL;
    char token, type;
    char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
    sscanf(command, "%c %s", &token, arg1);
    switch (token) {
        case 'c':
            sscanf(command, "%c %s %c", &token, arg1, &type);
            switch (type) {
                case 'f':
                    result  = create(arg1, T_FILE);
                    break;
                case 'd':
                    result  = create(arg1, T_DIRECTORY);
                    break;
            }
            break;
        case 'l':
            result  = lookup(arg1);
            break;

        case 'd':
            result = delete(arg1);
            break;

        case 'm':
            sscanf(command, "%c %s %s", &token, arg1, arg2);
            result = move(arg1, arg2);
            break;

        case 'p':
            sscanf(command, "%c %s", &token, arg1);
            printing = 1;
            
            /* wait until all threads complete their operation */
            while(threads_waiting_client < numberThreads - 1);

            result = print_tecnicofs_tree(arg1);

            printing = 0;
            cond_broadcast(&process_commands);
            break;
    }
    return result;
}

/* Command line and argument passing */
void parse_args(int argc, char* argv[]){
    if(argc == 3){
        numberThreads = atoi(argv[1]);
        socketName = argv[2];

        if(numberThreads <= 0)
            exit_with_error("Error: invalid number of threads\n");

    }
    else
        display_usage(argv[0]);
}

/* Set socket address */
int set_socket_address(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;

    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

/* Create socket associated with a socket path */
void * create_socket(){
    if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        exit_with_error("tecnicofs-server: can't open socket\n");

    server_addrlen = set_socket_address(socket_path, &server_addr);

    if(bind(sockfd, (struct sockaddr *) &server_addr, server_addrlen) < 0)
        exit_with_error("tecnicofs-server: bind error\n");

    printf("======= RUNNING SERVER =======\n");
    printf("Socket Path: %s\n", socket_path);

    return NULL;
}

void * process_client(){
    while(1){
        struct sockaddr_un client_addr;
        socklen_t client_addrlen;
        char rbuffer[MAX_INPUT_SIZE], sbuffer[sizeof(int) + 1];

        client_addrlen = sizeof(struct sockaddr_un);

        /* increments number of threads waiting for client */
        mutex_lock(&counting_mutex);
        threads_waiting_client++;
        mutex_unlock(&counting_mutex);

        int nread = recvfrom(sockfd, rbuffer, MAX_INPUT_SIZE - 1, 0, (struct sockaddr *)&client_addr, &client_addrlen);
        
        /* if no message was received */
        if(nread <= 0)
            continue;

        rbuffer[nread] = '\0';

        printf("%s\n", rbuffer);

        /* puts thread on wait if another thread is currently printing a tree */
        mutex_lock(&commands_mutex);
        while(printing == 1)
            cond_wait(&process_commands, &commands_mutex);

        mutex_unlock(&commands_mutex);
        
        /* decrements number of threads waiting for client */
        mutex_lock(&counting_mutex);
        threads_waiting_client--;
        mutex_unlock(&counting_mutex);

        int result = apply_commands(rbuffer);
        sprintf(sbuffer, "%d", result);

        int nsent = sendto(sockfd, sbuffer, strlen(sbuffer), 0, (struct sockaddr *)&client_addr, client_addrlen);
    
        if(nsent < 0)
            exit_with_error("tecnicofs-server: error sending message to the server\n");
    }
    return NULL;
}

/* Runs threads and prints threads execution time */
void run_threads(){
    pthread_t main_thread, *slave_threads;
    struct timeval begin, end;
    double duration;

    /* start counting time */
    gettimeofday(&begin, 0);

    /* allocate memory to slave threads pointer */
    if((slave_threads = (pthread_t*) malloc(sizeof(pthread_t) * numberThreads)) == NULL)
        exit_with_error("Error alocating memory to slave threads\n");
    
    /* create main thread */
    if(pthread_create(&main_thread, NULL, &create_socket, NULL) != 0)
        exit_with_error("Error creating main thread.\n");

    /* waiting for main thread to terminate */
    if(pthread_join(main_thread, NULL) != 0)
        exit_with_error("Error joining main thread.\n");

    /* create slave threads */
    for (int i = 0; i < numberThreads; i++){
        if(pthread_create(&slave_threads[i], NULL, &process_client, NULL) != 0)
            exit_with_error("Error creating thread.\n");
    }

    /* waiting for slave threads to terminate */
    for(int i = 0; i < numberThreads; i++){
        if(pthread_join(slave_threads[i], NULL) != 0)
            exit_with_error("Error joining thread.\n");
        printf("completed %d\n", i);
    }

    /* stop counting time */
    gettimeofday(&end, 0);
    duration = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) * 1e-6;

    free(slave_threads);

    /* print threads execution time */
    fprintf(stdout, "TecnicoFS completed in %0.4f seconds.\n", duration);
}

void create_socket_path(){
    strcpy(socket_path, tmp_dir);
    strcat(socket_path, socketName);
}

int main(int argc, char* argv[]) {
    parse_args(argc, argv);
    create_socket_path();

    /* just to prevent cases where last application exit with error, without unlinking socket */
    unlink(socket_path);

    /* init all */
    init_fs();
    mutex_init(&commands_mutex);
    mutex_init(&counting_mutex);
    cond_init(&process_commands);

    run_threads();

    /* destroy all */
    destroy_fs();
    mutex_destroy(&commands_mutex);
    mutex_destroy(&counting_mutex);
    cond_destroy(&process_commands);

    if(close(sockfd) != 0)
        exit_with_error("tecnicofs-server: error closing socket\n");

    if(unlink(socket_path) != 0)
        exit_with_error("tecnicofs-client: error unlinking client socket path\n");

    exit(EXIT_SUCCESS);
}

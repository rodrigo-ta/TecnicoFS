#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include "sync.h"
#include "mutex.h"
#include <pthread.h>
#include <sys/time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MAX_SYNC_CHAR 7

#define INPUT 4
#define OUTPUT 5

#define TRUE 1
#define FALSE 0

int numberThreads = 0;

int synchStrategy;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t mutex;

/* pointer to string of input file */
char *p_inputFile = NULL;

/* pointer to string of output file */
char *p_outputFile = NULL;


int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void displayUsage(char* appName){
    fprintf(stderr, "Usage: %s inputfile outputfile numthreads synchstrategy\n", appName);
    exit(EXIT_FAILURE);
}

FILE * openFile(char type){
    FILE * fp;
    
    /* input file */
    if(type == INPUT)
        fp = fopen(p_inputFile, "r");

    /* output file */    
    if(type == OUTPUT)
        fp = fopen(p_outputFile, "w");

    /* if unable to open the file */

    if(!fp){
        if(type == INPUT)
            fprintf(stderr, "Error: invalid input file\n");
        if(type == OUTPUT)
            fprintf(stderr, "Error: invalid output file\n");
        exit(EXIT_FAILURE);
    }

    return fp;
}

void processInput(){
    FILE *inputFile;

    inputFile = openFile(INPUT);

    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }

    fclose(inputFile);
}

void * applyCommands(void * arg){

    while (TRUE){

        /* lock mutex*/
        mutex_lock(&mutex);

        if (numberCommands != 0)
        {

            const char* command = removeCommand();
            if (command == NULL){
                
                mutex_unlock(&mutex);

                continue;
            }

            char token, type;
            char name[MAX_INPUT_SIZE];
            int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
            if (numTokens < 2) {
                fprintf(stderr, "Error: invalid command in Queue\n");

                mutex_unlock(&mutex);

                exit(EXIT_FAILURE);
            }

            int searchResult;
            switch (token) {
                case 'c':
                    switch (type) {
                        case 'f':
                            printf("Create file: %s\n", name);

                            mutex_unlock(&mutex);

                            create(name, T_FILE);/* condition */
                            break;
                        case 'd':
                            printf("Create directory: %s\n", name);

                            mutex_unlock(&mutex);

                            create(name, T_DIRECTORY);
                            break;
                        default:
                            fprintf(stderr, "Error: invalid node type\n");

                            mutex_unlock(&mutex);

                            exit(EXIT_FAILURE);
                    }
                    break;
                case 'l':

                    mutex_unlock(&mutex);

                    searchResult = lookup(name);
                    if (searchResult >= 0)
                    {
                        printf("Search: %s found\n", name);
                    }
                    else
                    {
                        printf("Search: %s not found\n", name);
                    }
                    break;
                case 'd':
                    printf("Delete: %s\n", name);
                    
                    mutex_unlock(&mutex);

                    delete(name);
                    break;
                default: { /* error */
                    fprintf(stderr, "Error: command to apply\n");

                    mutex_unlock(&mutex);
                    
                    exit(EXIT_FAILURE);
                }
            }
        }
        else
        {
            mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        
    }
    pthread_exit(NULL);
}

/* Command line and argument passing */
void parseArgs(int argc, char* argv[]){
    char buffer[MAX_SYNC_CHAR];
    if(argc == 5){
        p_inputFile = argv[1];
        p_outputFile = argv[2];
        numberThreads = atoi(argv[3]);

        if(numberThreads == 0){
            fprintf(stderr, "Error: invalid number of threads\n");
            exit(EXIT_FAILURE);
        }
        
        strcpy(buffer, argv[4]);
        
        /* checks if synchstrategy is valid */
        if(!strcmp(buffer, "mutex"))
            synchStrategy = MUTEX;

        else if(!strcmp(buffer, "rwlock"))
            synchStrategy = RWLOCK;

        else if(!strcmp(buffer, "nosync"))
            synchStrategy = NOSYNC;

        else{
            fprintf(stderr, "Error: invalid sync strategy\n");
            exit(EXIT_FAILURE);
        }

        /* Nosync strategy requires only 1 thread */
        if(synchStrategy == NOSYNC && numberThreads != 1){
            fprintf(stderr, "Error: Nosync strategy requires only 1 thread\n");
            exit(EXIT_FAILURE);
        }
    }
    else
        displayUsage(argv[0]);
}

/* Runs threads and gets execution time */
void run_threads(){
    pthread_t * pthreads_id;
    struct timeval begin, end;
    double duration;
    int i;

    pthreads_id = (pthread_t*) malloc(sizeof(pthread_t) * numberThreads);

    /* start counting time */
    gettimeofday(&begin, 0);

    /* create slave threads*/
    for (i = 0; i < numberThreads; i++){
        if(pthread_create(&pthreads_id[i], NULL, &applyCommands, NULL)){ // returns zero if successful
            fprintf(stderr, "Error creating thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* waiting for created threads to to terminate*/
    for (i = 0; i < numberThreads; i++){
        if(pthread_join(pthreads_id[i], NULL)){ // returns zero if successful
            fprintf(stderr, "Error joining thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* stop counting time */
    gettimeofday(&end, 0);
    duration = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) * 1e-6;

    /* Free memory associated to pthreads_id */
    free(pthreads_id);

    fprintf(stdout, "TecnicoFS completed in %0.4f seconds.\n", duration);
}

int main(int argc, char* argv[]) {
    FILE *outputFile;

    /* verify app arguments */
    parseArgs(argc, argv);

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput();

    mutex_init(&mutex);

    run_threads();

    outputFile = openFile(OUTPUT);

    print_tecnicofs_tree(outputFile);
    
    mutex_destroy(&mutex);


    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}

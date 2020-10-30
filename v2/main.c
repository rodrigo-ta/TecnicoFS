/* 
 * SISTEMAS OPERATIVOS - PROJETO (1/3) - TECNICOFS
 * 
 *      >> GRUPO 23 <<
 * 92513 - Mafalda Ferreira
 * 92553 - Rodrigo Antunes
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include "locks/mutex.h"
#include <pthread.h>
#include <sys/time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MAX_SYNC_CHAR 7

#define INPUT 4
#define OUTPUT 5
#define TRUE 1

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

pthread_mutex_t mutex;

/* pointer to string of input file given in app arguments*/
char *p_inputFile = NULL;

/* pointer to string of output file given in app arguments */
char *p_outputFile = NULL;

/* write error message into stdin and exit program */
void exit_with_error(const char* err_msg){
    fprintf(stderr, "%s", err_msg);
    exit(EXIT_FAILURE);
}

int insert_command(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* remove_command() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void error_parse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void display_usage(char* appName){
    fprintf(stderr, "Usage: %s inputfile outputfile numthreads\n", appName);
    exit(EXIT_FAILURE);
}

FILE * open_file(char type){
    FILE * fp;
    /* open input file read */
    if(type == INPUT){
        fp = fopen(p_inputFile, "r");
        if(!fp)
            exit_with_error("Error: invalid input file\n");
    }

    /* open output file to write */
    else if(type == OUTPUT){
        fp = fopen(p_outputFile, "w");
        if(!fp)
            exit_with_error("Error: invalid output file\n");
    }

    /* error */
    else
        exit(EXIT_FAILURE);

    return fp;
}

void process_input(){
    FILE *inputFile;

    inputFile = open_file(INPUT);

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
                    error_parse();
                if(insert_command(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    error_parse();
                if(insert_command(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    error_parse();
                if(insert_command(line))
                    break;
                return;
            
            case 'm':
                if(numTokens != 3)
                    error_parse();
                if(insert_command(line))
                    break;
                return;

            case '#':
                break;
            
            default: { /* error */
                error_parse();
            }
        }
    }

    fclose(inputFile);
}

void * apply_commands(){

    while (TRUE){

        /* lock mutex*/
        mutex_lock(&mutex);

        if (numberCommands != 0)
        {

            const char* command = remove_command();
            if (command == NULL){
                
                mutex_unlock(&mutex);

                continue;
            }

            char token, type;
            char name[MAX_INPUT_SIZE], name_src[MAX_INPUT_SIZE], name_destn[MAX_INPUT_SIZE];
            int numTokens = sscanf(command, "%c %s", &token, name);
            if (numTokens < 2) {
                fprintf(stderr, "Error: invalid command in Queue\n");

                mutex_unlock(&mutex);

                exit(EXIT_FAILURE);
            }

            int searchResult;
            switch (token) {
                case 'c':
                    sscanf(command, "%c %s %c", &token, name, &type);
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
                case 'm':

                    sscanf(command, "%c %s %s", &token, name_src, name_destn);
                    printf("Move %s to %s\n", name_src, name_destn);
                    mutex_unlock(&mutex);

                    move(name_src, name_destn);

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
            return NULL;
        }
        
    }
    return NULL;
}

/* Command line and argument passing */
void parse_args(int argc, char* argv[]){
    if(argc == 4){
        p_inputFile = argv[1];
        p_outputFile = argv[2];
        numberThreads = atoi(argv[3]);

        if(numberThreads <= 0)
            exit_with_error("Error: invalid number of threads\n");

    }
    else
        display_usage(argv[0]);
}

/* create slave threads*/
void create_threads(pthread_t * pthreads_id){
    int i;
    for (i = 0; i < numberThreads; i++){
        if(pthread_create(&pthreads_id[i], NULL, &apply_commands, NULL)) // returns zero if successful
            exit_with_error("Error creating thread.\n");
    }
}

/* waiting for created threads to to terminate*/
void join_threads(pthread_t * pthreads_id){
    int i;
    for (i = 0; i < numberThreads; i++){
        if(pthread_join(pthreads_id[i], NULL)) // returns zero if successful
            exit_with_error("Error joining thread.\n");
    }
}

/* Free memory associated to pthreads_id */
void free_threads(pthread_t * pthreads_id){
    free(pthreads_id);
}

/* Runs threads and prints threads execution time */
void run_threads(){
    pthread_t * pthreads_id;
    struct timeval begin, end;
    double duration;

    pthreads_id = (pthread_t*) malloc(sizeof(pthread_t) * numberThreads);

    /* start counting time */
    gettimeofday(&begin, 0);

    create_threads(pthreads_id);
    join_threads(pthreads_id);

    /* stop counting time */
    gettimeofday(&end, 0);
    duration = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) * 1e-6;

    free_threads(pthreads_id);

    /* print threads execution time */
    fprintf(stdout, "TecnicoFS completed in %0.4f seconds.\n", duration);
}

int main(int argc, char* argv[]) {

    /* verify app arguments */
    parse_args(argc, argv);

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    process_input();

    mutex_init(&mutex);

    run_threads();

    print_tecnicofs_tree(open_file(OUTPUT));
    
    /* release mutex locks */
    mutex_destroy(&mutex);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}

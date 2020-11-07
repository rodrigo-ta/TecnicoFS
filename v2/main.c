/* 
 * SISTEMAS OPERATIVOS - PROJETO (2/3) - TECNICOFS
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
#include <pthread.h>
#include <sys/time.h>

#include "fs/operations.h"
#include "locks/mutex.h"
#include "locks/conditions.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

#define INPUT 4
#define OUTPUT 5

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;

/* represents next position in inputCommands for each insert and remove operations */
int insert_ptr= 0;
int remove_ptr = 0;

/* represents if process input has reached end of input file */
int eof = 0;

pthread_mutex_t mutex;
pthread_cond_t can_insert;
pthread_cond_t can_remove;

/* pointer to string of input file given in app arguments*/
char *p_inputFile = NULL;

/* pointer to string of output file given in app arguments */
char *p_outputFile = NULL;

/* write error message into stdin and exit program */
void exit_with_error(const char* err_msg){
    fprintf(stderr, "%s", err_msg);
    exit(EXIT_FAILURE);
}

void insert_command(char* data) {
    while(numberCommands == MAX_COMMANDS)
        cond_wait(&can_insert, &mutex);
    strcpy(inputCommands[insert_ptr], data);
    insert_ptr = (insert_ptr + 1) % MAX_COMMANDS;
    numberCommands++;
    cond_signal(&can_remove);
}

int remove_command(char * command){
    while(numberCommands == 0 && eof == 0)
        cond_wait(&can_remove, &mutex);

    if(eof == 1)
        return 0;
        
    strcpy(command, inputCommands[remove_ptr]);
    remove_ptr = (remove_ptr + 1) % MAX_COMMANDS;
    numberCommands--;
    cond_signal(&can_insert);
    return 1;
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

void * process_input(){
    FILE *inputFile;

    inputFile = open_file(INPUT);

    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token, type;
        char name[MAX_INPUT_SIZE], name2[MAX_INPUT_SIZE];

        mutex_lock(&mutex);

        int numTokens = sscanf(line, "%c", &token);

        /* perform minimal validation */
        if (numTokens < 1) {
            mutex_unlock(&mutex);
            continue;
        }
        switch (token) {
            case 'c':
                numTokens = sscanf(line, "%c %s %c", &token, name, &type);
                if(numTokens != 3){
                    mutex_unlock(&mutex);
                    error_parse();
                }
                insert_command(line);
                break;
            
            case 'l':
                numTokens = sscanf(line, "%c %s", &token, name);
                if(numTokens != 2){
                    mutex_unlock(&mutex);
                    error_parse();
                }
                insert_command(line);
                break;
            
            case 'd':
                numTokens = sscanf(line, "%c %s %c", &token, name, &type);
                if(numTokens != 2){
                    mutex_unlock(&mutex);
                    error_parse();
                }
                insert_command(line);
                break;
            
            case 'm':
                numTokens = sscanf(line, "%c %s %s", &token, name, name2);
                if(numTokens != 3){
                    mutex_unlock(&mutex);
                    error_parse();
                }
                insert_command(line);
                break;

            case '#':
                break;
            
            default: { /* error */
                mutex_unlock(&mutex);
                error_parse();
            }
        }
        mutex_unlock(&mutex);
    }
    while(numberCommands != 0);
    eof = 1;
    cond_broadcast(&can_remove);
    fclose(inputFile);
    return NULL;
}

void * apply_commands(){
    while (1){
        mutex_lock(&mutex);
        char command[MAX_INPUT_SIZE];
        if(remove_command(command) == 0){
            mutex_unlock(&mutex);
            return NULL;
        }
        char token, type;
        char name[MAX_INPUT_SIZE], src_name[MAX_INPUT_SIZE], destn_name[MAX_INPUT_SIZE];
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
                sscanf(command, "%c %s %s", &token, src_name, destn_name);
                printf("Move %s to %s\n", src_name, destn_name);
                mutex_unlock(&mutex);

                move(src_name, destn_name);

                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");

                mutex_unlock(&mutex);
                    
                exit(EXIT_FAILURE);
            }
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
    if(pthread_create(&main_thread, NULL, &process_input, NULL) != 0)
        exit_with_error("Error creating main thread.\n");
    
    /* create slave threads */
    for (int i = 0; i < numberThreads; i++){
        if(pthread_create(&slave_threads[i], NULL, &apply_commands, NULL) != 0)
            exit_with_error("Error creating thread.\n");
    }

    /* waiting for main thread to terminate */
    if(pthread_join(main_thread, NULL) != 0)
        exit_with_error("Error joining main thread.\n");

    /* waiting for slave threads to terminate */
    for(int i = 0; i < numberThreads; i++){
        if(pthread_join(slave_threads[i], NULL) != 0)
            exit_with_error("Error joining thread.\n");
    }

    /* stop counting time */
    gettimeofday(&end, 0);
    duration = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) * 1e-6;

    free(slave_threads);

    /* print threads execution time */
    fprintf(stdout, "TecnicoFS completed in %0.4f seconds.\n", duration);
}

int main(int argc, char* argv[]) {

    /* verify app arguments */
    parse_args(argc, argv);

    /* init filesystem, command mutex and conditional variables*/
    init_fs();
    mutex_init(&mutex);
    cond_init_all(&can_insert, &can_remove);

    run_threads();
    print_tecnicofs_tree(open_file(OUTPUT));
    
    /* release conditional variables, mutex locks and fs memory */
    cond_destroy_all(&can_insert, &can_remove);
    mutex_destroy(&mutex);
    destroy_fs();

    exit(EXIT_SUCCESS);
}

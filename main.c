#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <time.h>
#include <pthread.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define MUTEX 1
#define RWLOCK 2
#define NOSYNC 3

#define INPUT 4
#define OUTPUT 5

int numberThreads = 0;
int synchStrategy;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

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
        fp = fopen(p_outputFile, "a");

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
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);/* condition */
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    pthread_exit(NULL);
}


void parseArgs(int argc, char* argv[]){
    char buffer[5];
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
            fprintf(stderr, "Error: invalid synchstrategy\n");
            exit(EXIT_FAILURE);
        }

        if(synchStrategy == NOSYNC && numberThreads != 1){
            fprintf(stderr, "Error: synchstrategy and numberThreads are out of sync\n");
            exit(EXIT_FAILURE);
        }

    }
    else
        displayUsage(argv[0]);
}

void debug_print() {
    int i;

    printf("\t___DEBUG___\t\n");
    printf("|\tCOMMANDS\t|\n");
    for (i = 0; i < numberCommands; i++)
    {
        printf("|%s", inputCommands[i]);
    };
}

int main(int argc, char* argv[]) {
    FILE *outputFile;
    clock_t begin_timer, end_timer;
    double run_time;
    int i,j;

    pthread_t * pthreads_id = malloc(sizeof(* pthreads_id) * numberThreads);

    begin_timer = clock();

    /* verify app arguments */
    parseArgs(argc, argv);

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput();



    /* create slave threads*/
    for (i = 0; i < numberThreads; i++)
    {
        pthread_create(&pthreads_id[headQueue], NULL, &applyCommands, NULL);
    }

    /* waiting for created threads to to terminate*/
    for (j = numberThreads; i > 0; j--)
    {
        pthread_join(pthreads_id[headQueue - j], NULL);
        pthread_exit(NULL);
    }
    


    /*DEBUG*/
    debug_print();

    outputFile = openFile(OUTPUT);

    print_tecnicofs_tree(outputFile);

    /* release allocated memory */
    destroy_fs();

    end_timer = clock();
    run_time = (double)(end_timer - begin_timer) / CLOCKS_PER_SEC;
    printf("RUN TIME: %f SECONDS\n", run_time);

    exit(EXIT_SUCCESS);
}

//---------------------------------------------------------------------------
// shell.c
// Provides basic shell functionality for one and two command entries
// Author: Arbour
//---------------------------------------------------------------------------
// Assumptions:
// Values provided to the shell are either single commands with flags,
// or are two commands with operators between (i.e. <, >, |, &).
//---------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAX_LINE 80 /* The maximum length command */

void tokenizeString(char *string, char **args);
void execute(char **args, int concurr);
void redirectRight(char **args, int concurr);
void redirectLeft(char **input, int concurr);
void execPipe(char **args, int concurr);
char concurrSearch(char** args);
char operatorSearch(char **args);
void trimArgEnd(char **args);

char *command = NULL;
char *args[MAX_LINE/2 + 1]; /* command line arguments */
char *prevArgs[MAX_LINE/2 + 1]; /* Stored args for history command */

int main(void)
{
    int should_run = 1; /* flag to determine when to exit program */
    size_t size = 80;

    while (should_run) {
        fflush(stdout);
        memcpy(prevArgs, args, sizeof(args));
        memset(args, 0, sizeof(args));     // Reset values of args[]
        int concurr = 0;
        char operator = '\0';

        char *line = NULL;
        size_t bytes_read;
        printf("osh>");

        bytes_read = getline (&line, &size, stdin);

        if(bytes_read == 1){   // Do nothing
        } 
        else {
            tokenizeString(line, args);

            if(strcmp(args[0], "!!") == 0 && args[1] == NULL){
                memcpy(args, prevArgs, sizeof(args));
                int count = 0;
                while(args[count] != NULL){
                    printf("%s ", args[count]);
                    count++;
                }
                printf("\n");
            }

            if(strcmp(args[0], "exit") == 0 && args[1] == NULL) {
                should_run = 0;
                exit(0);
            }
            // operatorSearch only returns '&' when 
            // it occurs at the end of arg[]
            if(concurrSearch(args) == '&'){
                trimArgEnd(args);
                concurr = 1;
            }
            if(operatorSearch(args) == '>'){
                redirectRight(args, concurr);
                continue;
            }
            if(operatorSearch(args) == '<'){
                redirectLeft(args, concurr);
                continue;
            }
            if(operatorSearch(args) == '|'){
                execPipe(args, concurr);
                continue;
            }
            else {
                execute(args, concurr);
                continue;
            }
        }
    }
    return 0;
}

void tokenizeString(char *string, char **args){
    string[strcspn(string, "\n")] = 0;
    char *token = strtok(string, " ");
    command = token;
    args[0] = token;
    
    int count = 1;
    while(token != NULL){
        token = strtok(NULL, " ");
        if(token != NULL){
            args[count] = token;
            count++;
        }
    }
}

void execute(char **args, int concurr){
    pid_t pid = fork();
    if(pid < 0){    /* error occured */
        fprintf(stderr, "Fork Failed");
        exit(1);
    }
    else if(pid == 0){  /* child process */
        int status_code = execvp(args[0], args);
        if(status_code < 0){
            fprintf(stderr, "Execute Failed\n");
            exit(1);
        }
    }
    else if(concurr != 0){} /* run in background */
    else{
        int stat;
        wait(&stat);
    }
}

void redirectRight(char **input, int concurr){
    int count = 0;
    while(strcmp(input[count], ">") != 0){
        count++;
    }
    char *args[MAX_LINE/2 + 1];
    memcpy(args, input, sizeof(args));

    char dest[MAX_LINE];
    strcpy(dest, args[count + 1]);

    trimArgEnd(args); // Removes destination
    trimArgEnd(args); // Removes '>'
    
    pid_t pid = fork();
    if(pid < 0){    /* error occured */
        fprintf(stderr, "Fork Failed");
        exit(1);
    }
    if(pid == 0){  /* child process */
        // Opens file and stores newly created
        // file descriptor to fd
        int fd = open(dest, O_RDWR | O_CREAT, 0666);

        // Redirects stdout to point to file
        dup2(fd, STDOUT_FILENO);

        int status_code = execvp(args[0], args);
        if(status_code < 0){
            fprintf(stderr, "Execute Failed\n");
            exit(1);
        }
    }
    else if(concurr != 0){} /* run in background */
    else{
        int stat;
        wait(&stat);
    }
}

void redirectLeft(char **input, int concurr){
    int count = 0;
    while(strcmp(input[count], "<") != 0){
        count++;
    }
    char *args[MAX_LINE/2 + 1];
    memcpy(args, input, sizeof(args));

    char src[MAX_LINE];
    strcpy(src, args[count + 1]);

    trimArgEnd(args); // Removes destination
    trimArgEnd(args); // Removes '>'
    
    pid_t pid = fork();
    if(pid < 0){    /* error occured */
        fprintf(stderr, "Fork Failed");
        exit(1);
    }
    if(pid == 0){  /* child process */
        // Opens file and stores newly created
        // file descriptor to fd
        int fd = open(src, O_RDWR | O_CREAT, 0666);

        // Redirects stdout to point to file
        dup2(fd, STDIN_FILENO);

        int status_code = execvp(args[0], args);
        if(status_code < 0){
            fprintf(stderr, "Execute Failed\n");
            exit(1);
        }
    }
    else if(concurr != 0){} /* run in background */
    else{
        int stat;
        wait(&stat);
    }
}

void execPipe(char **input, int concurr){
    int total = 0;

    while(input[total] != NULL){
        total++;
    }

    char *arg1[MAX_LINE/2 + 1] = {0};
    int count = 0;
    while(strcmp(input[count], "|") != 0){
        arg1[count] = malloc(20 * sizeof(char));
        strcpy(arg1[count], input[count]);
        count++;
    }

    // Copies values after '|' into arg2
    char *arg2[MAX_LINE/2 + 1] = {0};
    for(int i = 0; i < total - count -1; i++){
        arg2[i] = malloc(20 * sizeof(char));
        strcpy(arg2[i], input[i + count + 1]);
    }
    
    int fd[2];
    pipe(fd);

    pid_t pid1 = fork();
    if(pid1 < 0){    // error occured
        fprintf(stderr, "Fork Failed");
        exit(1);
    }
    else if(pid1 == 0){  // first child process
        close(fd[0]);
        if(dup2(fd[1], STDOUT_FILENO) < 0){
            printf("Unable to duplicate file descriptor");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);
        int status_code = execvp(arg1[0], arg1);
        if(status_code < 0){
            fprintf(stderr, "Execute Failed\n");
            exit(1);
        }
    }

    pid_t pid2 = fork();
    if(pid2 < 0){    // error occured
        fprintf(stderr, "Fork Failed");
        exit(1);
    }
    else if(pid2 == 0){  // second child process
        close(fd[1]);
        if(dup2(fd[0], STDIN_FILENO) < 0){
            printf("Unable to duplicate file descriptor");
            exit(EXIT_FAILURE);
        }
        close(fd[0]);
        int status_code = execvp(arg2[0], arg2);
        if(status_code < 0){
            fprintf(stderr, "Execute Failed\n");
            exit(1);
        }
    }
    close(fd[0]);
    close(fd[1]);

    count = 0;
    while(arg1[count] != NULL){
        free(arg1[count]);
        count++;
    }
    count = 0;
    while(arg2[count] != NULL){
        free(arg2[count]);
        count++;
    }
    if(concurr != 0){} // run in background
    else{
        int stat1, stat2;
        wait(&stat1);
        wait(&stat2);
    }
}


char concurrSearch(char** args){
    int count = 0;
    while(args[count] != NULL) count++;
    if(strcmp(args[count - 1], "&") == 0) return '&';
    return '\0';
}

char operatorSearch(char **args){
    int count = 0;
    while(args[count] != NULL){
        if(strcmp(args[count], "<") == 0) return '<';
        if(strcmp(args[count], ">") == 0) return '>';
        if(strcmp(args[count], "|") == 0) return '|';
        count++;
    }
}

void trimArgEnd(char **args){
    int count = 0;
    while(args[count] != NULL){
        count++;
    }
    args[count - 1] = NULL;
}
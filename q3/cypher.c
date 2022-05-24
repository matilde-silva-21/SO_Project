#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1

#define READ "r"
#define BUFFERSIZE 264
#define WORDSIZE 64
#define PBUFFSIZE 8

char wordBuffer[WORDSIZE];

typedef struct{
    char target[WORDSIZE];
    char swap[WORDSIZE];
}cypherEntry;

typedef struct{
    cypherEntry* table;
    int size;
}cypher;

/**
 * @brief Loads cypher from cypher.txt into memory
 * 
 * @param cyphr a pointer to the cypher struct
 * @return cypher* the loaded cypher struct
 */
cypher* loadCypher(cypher* cyphr){
    int nbytes, bufferSize = BUFFERSIZE;
    char* buffer = (char*) malloc(sizeof(char) * bufferSize);
    cyphr->size = 0;
    while(1){
        FILE* f = fopen("cypher.txt", READ);
        nbytes = fread(buffer, sizeof(char), bufferSize, f);
        fclose(f);
        if(nbytes == bufferSize) {
            bufferSize *= 2;
            buffer = (char*)realloc(buffer, sizeof(char) * bufferSize);
        }
        else break;
    }
    for(int i = 0; i < bufferSize; i++)
        if(buffer[i] == '\n')cyphr->size++;
    cyphr->table = (cypherEntry*)malloc(sizeof(cypherEntry)* cyphr->size);
    char* swap, *target = strtok(buffer, " ");
    int i=0;
    do{
        swap = strtok(NULL, "\n");
        strcpy(cyphr->table[i].target, target);
        strcpy(cyphr->table[i].swap, swap);
        target=strtok(NULL, " ");
        i++;
    }while(target != NULL);
    free(buffer);
    return cyphr;
}

/**
 * @brief Sees if a string has punctuation in it
 * 
 * @param where the index where the punctuation starts
 * @return int 0 if there is punctuation, 1 otherwise
 */
int hasPunct(int *where){
    for(int i = 0; i < strlen(wordBuffer); i++)
        if(ispunct(wordBuffer[i])){
            *where = i;
            return 0;
        }
    return 1;
}

/**
 * @brief Compares wordBuffer against the items in the cypher table.
 * 
 * @param cyphr the cypher table
 * @param index the index of the cypher table (return value)
 * @param target 0 if the word matches the target entry, 1 otherwise
 * @return int 0 if the wordBuffer is present in the cypher table, 1 otherwise
 */
int compareCypher(cypher* cyphr, int* index, int* target){
    for(int i=0;i<cyphr->size;i++){
        if(!strncmp(wordBuffer, cyphr->table[i].target,
                strlen(cyphr->table[i].target))){
            *index = i;
            *target =0;
            return 0;
        }
        if(!strncmp(wordBuffer, cyphr->table[i].swap,
                strlen(cyphr->table[i].swap))){
            *index = i;
            *target = 1;
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Cyphers words when there's punctuation at the start of the string
 * 
 * @param cyphr the cypher table
 * @param index index of the cypher table to be accessed
 * @param target if 0 swaps target for swap, if 1 swaps swap for target
 */
void handlePrefix(cypher* cyphr, int index, int target){
    char lastChar = wordBuffer[strlen(wordBuffer)-1];
    int i=1;
    char* shift;
    if(target) shift = cyphr->table[index].target;
    else shift = cyphr->table[index].swap;
    for(;i<strlen(shift);i++)
        wordBuffer[i]=shift[i];
    wordBuffer[i] = lastChar;
    wordBuffer[i+1]='\0';
}

/**
 * @brief Cyphers words when there's punctuation at the end of the string
 * 
 * @param cyphr the cypher table
 * @param index the index of the cypher table to be accessed
 * @param where index of the string where punctuation starts
 * @param target if 0 swaps target for swap, if 1 swaps swap for target
 */
void handleSuffix(cypher* cyphr, int index, int where, int target){
    char punct[PBUFFSIZE];
    int i = where;
    for(;wordBuffer[i]!='\0';i++){
        punct[i-where]=wordBuffer[i];
    }
    punct[i-where] = '\0';
    char*shift;
    if(target) shift = cyphr->table[index].target;
    else shift = cyphr->table[index].swap;
    int usedBuffer = strlen(wordBuffer);
    for(i=0;i<strlen(shift);i++)
        wordBuffer[i]=shift[i];
    for(int j=0;punct[j]!='\0';j++){
        wordBuffer[i+j]=punct[j];
    }
    wordBuffer[i+strlen(punct)]='\0';
}

/**
 * @brief Cyphers words when there is no punctuation involved
 * 
 * @param cyphr the cypher table
 * @param index the index of the cypher table
 * @param target if 0 swaps target for swap, if 1 swaps swap for target
 */
void handleNoffix(cypher* cyphr, int index, int target){
    char lastChar = wordBuffer[strlen(wordBuffer)-1];
    int i=0;
    char*shift;
    if(target) shift = cyphr->table[index].target;
    else shift = cyphr->table[index].swap;
    for(;i<strlen(shift);i++)
        wordBuffer[i]=shift[i];
    wordBuffer[i]=lastChar;
    wordBuffer[i+1]='\0';
}

/**
 * @brief Cyphers a piece of text
 * 
 * @param cyphr the cypher table
 * @param text the text to be cyphered
 * @return char* pointer to the cyphered text
 */
char* cypherText(cypher* cyphr, char* text){
    char* cypheredText;
    strcpy(cypheredText, "\0");
    int lastIndex = 0, foundWord = 0;
    for(int i = 0; i <strlen(text); i++){
        if(text[i] == ' ' || text[i] == '\n'){
            int j=0;
            for(; j<=i-lastIndex;j++)
                wordBuffer[j] = text[lastIndex+j];
            wordBuffer[j]='\0';
            lastIndex=i+1;
            foundWord=1;
        }
        if(foundWord){
            int index, target;
            if(!compareCypher(cyphr, &index, &target)){
                int where;
                if(!hasPunct(&where)){
                    if(!where) handlePrefix(cyphr, index, target);
                    else handleSuffix(cyphr, index, where, target);
                }
                else handleNoffix(cyphr, index, target);
            }
            strcat(cypheredText, wordBuffer);
            foundWord=0;
        }
    }
    text = cypheredText;
    return cypheredText;
}

/**
 * @brief Flushes a buffer a buffer 
 * 
 * @param buffer pointer to the buffer to be flushed
 * @param buffersize the size of the buffer
 */
void flush(char* buffer, int buffersize){
    for(int i = 0; i <buffersize;i++)
        buffer[i]='\0';
}

/**
 * @brief Reads text from a file descriptor
 * 
 * @param fd the file descriptor
 * @param text where the read text will be stored
 * @return char* pointer to the read text
 */
char* rfrom(int fd, char* text){
    char buffer[BUFFERSIZE];
    int textsize = BUFFERSIZE, nbytes;
    text = (char*)malloc(sizeof(char)*textsize);
    while(1){
        nbytes = read(fd, buffer, BUFFERSIZE);
        if(nbytes == -1){
            fprintf(stderr, "error reading input\n");
            return NULL;
        }
        strcat(text, buffer);
        if(nbytes == textsize){
            textsize+=BUFFERSIZE;
            text = (char*)realloc(text, textsize);
            flush(buffer, BUFFERSIZE);
        }else break;
    }
    close(fd);
    return text;
}

/**
 * @brief Reads from a pipe
 * 
 * @param fd arrway with the pipe's file descriptors
 * @param text where the read text will be stored
 * @return char* pointer to the read text
 */
char* rfromp(int fd[2], char* text){
    close(fd[WRITE_END]);
    return rfrom(fd[READ_END], text);
}

int main(int argc, char* argv[]){
    int fd1[2], fd2[2], nbytes;
    pid_t pid;
    char buffer[BUFFERSIZE];
    
    if(pipe(fd1) < 0) {
        perror( "fd1 pipe error\n");
        exit(EXIT_FAILURE);
    }

    if(pipe(fd2) < 0) {
        perror("fd2 pipe error\n");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) < 0) {
        perror("fork error\n");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        char* inbuffer = rfrom(STDIN_FILENO, inbuffer);
        close(fd1[READ_END]);
        write(fd1[WRITE_END], inbuffer, strlen(inbuffer));
        close(fd1[WRITE_END]);
        waitpid(0, NULL, 0);
        inbuffer = rfromp(fd2, inbuffer);
        write(STDOUT_FILENO, inbuffer, strlen(inbuffer));
        exit(EXIT_SUCCESS);

    } else {
        int textsize = BUFFERSIZE;
        char* text = rfromp(fd1, text);
        cypher cyphr;
        loadCypher(&cyphr);
        char* cypheredtext = cypherText(&cyphr, text);
        write(fd2[WRITE_END], cypheredtext, strlen(cypheredtext));
        close(fd2[WRITE_END]);
        
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_SUCCESS);
}
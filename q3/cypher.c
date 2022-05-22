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
#define BUFFERSIZE 256
#define PBUFFSIZE 8

char wordBuffer[64];

typedef struct{
    char target[64];
    char swap[64];
}cypherEntry;

typedef struct{
    cypherEntry* table;
    int size;
}cypher;

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

    return cyphr;
}

int hasPunct(int *where){
    for(int i = 0; i < strlen(wordBuffer); i++)
        if(ispunct(wordBuffer[i])){
            *where = i;
            return 0;
        }
    return 1;
}

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

void flush(char* buffer){
    for(int i = 0; i <BUFFERSIZE;i++)
        buffer[i]='\0';
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
        int inbuffersize = BUFFERSIZE;
        char* inbuffer = (char*)malloc(sizeof(char)*inbuffersize);
        while(1){
            nbytes = read(STDIN_FILENO, buffer, BUFFERSIZE);
            if(nbytes == -1){
                fprintf(stderr, "error reading input\n");
                exit(EXIT_FAILURE);
            }
            strcat(inbuffer, buffer);
            if(nbytes==BUFFERSIZE){
                inbuffersize+=BUFFERSIZE;
                inbuffer = (char*)realloc(inbuffer, inbuffersize);
                flush(buffer);
            }else break;
        }
        close(fd1[READ_END]);
        write(fd1[WRITE_END], inbuffer, inbuffersize);
        close(fd1[WRITE_END]);
        waitpid(0, NULL, 0);
        inbuffer = (char*)malloc(sizeof(char)*inbuffersize);
        while(1){
            nbytes = read(fd2[READ_END], buffer, BUFFERSIZE);
            if(nbytes == -1){
                fprintf(stderr, "error reading input\n");
                exit(EXIT_FAILURE);
            }
            strcat(inbuffer, buffer);
            flush(buffer);
            if(nbytes < BUFFERSIZE) break;
        }
        write(STDOUT_FILENO, inbuffer, strlen(inbuffer));
        exit(EXIT_SUCCESS);

    } else {
        int textsize = BUFFERSIZE;
        char* text = (char*)malloc(sizeof(char)*textsize);
        close(fd1[WRITE_END]);
        while(1){
            nbytes = read(fd1[READ_END], buffer, BUFFERSIZE);
            if(nbytes == -1){
                fprintf(stderr, "error reading input\n");
                exit(EXIT_FAILURE);
            }
            strcat(text, buffer);
            if(nbytes == textsize){
                textsize+=BUFFERSIZE;
                text = (char*)realloc(text, textsize);
                flush(buffer);
            }else break;
        }
        close(fd1[READ_END]);
        cypher cyphr;
        loadCypher(&cyphr);
        char* cypheredtext = cypherText(&cyphr, text);
        write(fd2[WRITE_END], cypheredtext, strlen(cypheredtext));
        close(fd2[WRITE_END]);
        
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_SUCCESS);
}
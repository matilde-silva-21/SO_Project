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

void flush(char* buffer, int buffersize);

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
 * @brief Compares two words to see if they're the same
 * 
 * @param w1 the first word
 * @param w2 the second word
 * @return int 1 if they're the same, 0 otherwise
 */
int cmpwrd(char* w1, char* w2){
    int where;
    if(strlen(w1) == strlen(w2) || strlen(w1) < strlen(w2)) 
        return (!strcmp(w1, w2));
    else{
        int i = 0, size;
        if(strlen(w1) < strlen(w2)) size = strlen(w1);
        else size = strlen(w2);
        while(i < size){
            if(w1[i] == w2[0]) break;
            else i++;
        }
        if(i == size) return 0;
        else{
            for(int j = 0; j < size; j++)
                if(w1[i+j] != w2[j]) return 0;
        }
        return 1; 
    }
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
    if(*index >= cyphr->size){
        fprintf(stderr, "tried to access invalid memory position\n"
                        "source: compareCypher\n"
                        "position: %d max: %d", *index, cyphr->size);
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<cyphr->size;i++){
            if(!cmpwrd(wordBuffer, cyphr->table[i].target)){
                if(cmpwrd(wordBuffer, cyphr->table[i].swap)){
                    *index = i;
                    *target = 1;
                    return 0;
                }
            }
            else{
                *index = i;
                *target = 0;
                return 0;
            }
        }
    return 1;
}

/**
 * @brief Cyphers the contents of wordBuffer
 * 
 * @param cypher the word to be used as cypher
 */
void cypherWord(char* cypher){
    char* word = (char*)malloc(sizeof(char)*WORDSIZE);
    strcpy(word, wordBuffer);
    flush(wordBuffer, WORDSIZE);
    int j, i=0;
    for(j=0;ispunct(word[j]);j++){
        wordBuffer[i] = word[j];
        i++;
    }
    for(int k = 0; k <strlen(cypher);k++){
        wordBuffer[i] = cypher[k];
        i++;
    }
    for(;j < strlen(word);j++){
        if(ispunct(word[j]) || isspace(word[j])){
            wordBuffer[i] = word[j];
            i++;
        }
    }
    free(word);
}

/**
 * @brief Cyphers a piece of text
 * 
 * @param cyphr the cypher table
 * @param text the text to be cyphered
 * @return char* pointer to the cyphered text
 */
void cypherText(cypher* cyphr, char* text){
    char* cypheredText = (char*)malloc(sizeof(char)*strlen(text));
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
                if(!target) cypherWord(cyphr->table[index].swap);
                else cypherWord(cyphr->table[index].target);
            }
            strcat(cypheredText, wordBuffer);
            foundWord=0;
        }
    }
    strcpy(text, cypheredText);
    free(cypheredText);
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
    char* textbuffer = (char*)malloc(sizeof(char)*textsize);
    while(1){
        nbytes = read(fd, buffer, BUFFERSIZE);
        if(nbytes == -1){
            fprintf(stderr, "error reading input\n");
            exit(EXIT_FAILURE);
        }
        strcat(textbuffer, buffer);
        if(nbytes == textsize){
            textsize+=BUFFERSIZE;
            textbuffer = (char*)realloc(textbuffer, textsize);
            flush(buffer, BUFFERSIZE);
        }else break;
    }
    close(fd);
    text = (char*)malloc(sizeof(char)*strlen(textbuffer));
    strcpy(text, textbuffer);
    free(textbuffer);
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
    text = rfrom(fd[READ_END], text);
    close(fd[READ_END]);
    return text;
}

int main(int argc, char* argv[]){
    int fd1[2], fd2[2], nbytes;
    pid_t pid;
    
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
        free(inbuffer);
        exit(EXIT_SUCCESS);

    } else {
        int textsize = BUFFERSIZE;
        char* text = rfromp(fd1, text);
        cypher cyphr;
        loadCypher(&cyphr);
        cypherText(&cyphr, text);
        write(fd2[WRITE_END], text, strlen(text));
        close(fd2[WRITE_END]);
        free(text);
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_SUCCESS);
}
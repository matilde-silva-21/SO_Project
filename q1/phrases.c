#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *in;

void getargs(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "usage: phrases [-l] file\n");
        exit(0);
    }
    else if (argc == 2)
    {
        in = fopen(argv[1], "r");
    }
    else if (argc == 3 && strcmp(argv[1], "-l")==0)
    {
        in = fopen(argv[2], "r");
    }
    else{
        fprintf(stderr, "usage: phrases [-l] file\n");
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    getargs(argc, argv);
    char c;
    int size_sentence=1024, char_count = 0;
    char *sentence = malloc(size_sentence);
    int line_count=0;
    
    while (!feof(in))
    {
        
        c = getc(in);
        if(c==' ' || c=='\n')
        {
            c = getc(in);
        }
        sentence[char_count] = c;
        while (c != '.' && c != '!' && c != '?' && c != EOF)
        {
            if(char_count >= size_sentence-1){
                size_sentence += 256;
                sentence = realloc(sentence, size_sentence);
                if(sentence == NULL){
                    fprintf(stderr, "out of memory\n");
                    return -1;
                }
            }

            c = getc(in);
            if (c != '\n' && c!=EOF)
            {
                sentence[++char_count] = c;
            }     
        }

        line_count++;
        sentence[++char_count] = '\0';
        

        if(argc==3)
        {
            printf("[%d] %.*s\n",line_count,char_count, sentence);
        }
        
        char_count = 0;
    }
    if(argc==2)
    {printf("%d\n",line_count);}
}

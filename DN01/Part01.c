#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_TOKENS 100
#define MAX_LINE 1024

char line[MAX_LINE];
char* tokens[MAX_TOKENS];
//int token_count;

int tokenize(char* line){
    int token_count = 0;
    char* pointer = line;

    while(*pointer)
    {
        //jumps whitespaces
        while(isspace(*pointer))
        {
            pointer++;
        }

        //end of the line
        if(*pointer == '\0')
        {
            break;
        }

        if(*pointer == '"')
        {
            pointer++;
            tokens[token_count] = pointer;
            token_count++;
            while(*pointer != '"')
            {
                pointer++;
            }
            if(*pointer == '"')
            {
                *pointer = '\0';
                pointer++;
            }
        }
        else
        {
            tokens[token_count] = pointer;
            token_count++;
            while(*pointer && !isspace(*pointer))
            {
                pointer++;
            }
            if(isspace(*pointer) || *pointer == '\0')
            {
                *pointer = '\0';
                pointer++;
            }
        }

    }
    
    return token_count;
    
}

int isEmpty(char* line){
    for(int i = 0; line[i] != 0; i++)
    {
        if(!isspace(line[i]))
        {
            return 0;
        }
    }
    return 1;
}

void removeComment(char* line){
    for(int i = 0; line[i] != 0; i++)
    {
        if((i == 0 && line[i] == '#') || (line[i] == '#' && isspace(line[i - 1])))
        {
            line[i] = '\0';
            return;
        }
    }
}

int main(){
    int interactive = isatty(STDIN_FILENO);
    
    while(1)
    {
        if(interactive)
        {
            printf("mysh> ");
            fflush(stdout);
        }

        if(fgets(line, MAX_LINE, stdin) == NULL)
        {
            break;
        }

        line[strcspn(line, "\n")] = '\0';
        
        //Writing to standard outpur, Input line (+comments!!)
        printf("Input line: '%s'\n", line);

        removeComment(line);
        if (isEmpty(line)) continue;


        int counter = tokenize(line);

        //Writing to standard output, each token
        //printf("Input line: '%s'\n", line);
        for(int i = 0; i < counter; i++)
        {
            printf("Token %d: '%s'\n", i, tokens[i]);
        }

    }

    return 0; 

}
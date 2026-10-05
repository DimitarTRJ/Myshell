//Basically Part01.c + (logic for last 3 tokens)


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_TOKENS 100
#define MAX_LINE 1024

char line[MAX_LINE];
char* tokens[MAX_TOKENS];
int token_count;

int tokenize(char* line){
    token_count = 0;
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

void lastThree(int counter, char** tokens){ 
    for(int i = 3; i >= 1; i--) 
    { 
        if(counter > 2 && (tokens[counter - i][0] == '<' || tokens[counter - i][0] == '>' || strcmp(tokens[counter - i], "&") == 0)) 
        { 
            if(tokens[counter - i][0] == '<') 
            { 
                printf("Input redirect: '%s'\n", tokens[counter - i] + 1);
                token_count--;
            } 
            if(tokens[counter - i][0] == '>') 
            { 
                printf("Output redirect: '%s'\n", tokens[counter - i] + 1);
                token_count--;
            } 
            if(strcmp(tokens[counter - i], "&") == 0) 
            { 
                printf("Background: 1\n"); 
                token_count--;
            } 
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

        //Input redirect, Output redirect, Background
        lastThree(counter, tokens);

    }

    return 0; 

}
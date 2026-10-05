#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_TOKENS 100
#define MAX_LINE 1024

char line[MAX_LINE];
char tempLine[MAX_LINE];
char* tokens[MAX_TOKENS];
int token_count;

int debug_level = 0;
int last_status = 0;
char prompt[10] = "mysh";

int background = 0;

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
                background = 1;
            } 
        } 
    } 
}

//------------------------------------------------------------//

int execute_calculation(char* arg1, char* arg2, char* operation){
    int calcResult = 0;

        if(strcmp(operation, "+") == 0)
        {
            calcResult = atoi(arg1) + atoi(arg2);
        }
        else if(strcmp(operation, "-") == 0)
        {
            calcResult = atoi(arg1) - atoi(arg2);
        }
        else if(strcmp(operation, "*") == 0)
        {
            calcResult = atoi(arg1) * atoi(arg2);
        }
        else if(strcmp(operation, "/") == 0)
        {
            calcResult = atoi(arg1) / atoi(arg2);
        }
        else if(strcmp(operation, "%") == 0)
        {
            calcResult = atoi(arg1) % atoi(arg2);
        }

    return calcResult;
}

//------------------------------------------------------------//

int is_builtin(char *cmd) {
    return strcmp(cmd, "debug") == 0 ||
           strcmp(cmd, "prompt") == 0 ||
           strcmp(cmd, "status") == 0 ||
           strcmp(cmd, "exit") == 0 ||
           strcmp(cmd, "help") == 0 ||
           strcmp(cmd, "print") == 0 ||
           strcmp(cmd, "echo") == 0 ||
           strcmp(cmd, "len") == 0 ||
           strcmp(cmd, "sum") == 0 ||
           strcmp(cmd, "calc") == 0 ||
           strcmp(cmd, "basename") == 0 ||
           strcmp(cmd, "dirname") == 0;
}

int execute_builtin() {
    char *cmd = tokens[0];

    if (strcmp(cmd, "debug") == 0) {
        if (token_count == 1) {
            printf("%d\n", debug_level);
        } else {
            debug_level = atoi(tokens[1]);
        }
        return 0;
    }

    if (strcmp(cmd, "prompt") == 0) {
        if (token_count == 1) {
            printf("%s\n", prompt);
        } else {
            if (strlen(tokens[1]) > 8) return 1;
            strcpy(prompt, tokens[1]);
        }
        return 0;
    }

    if (strcmp(cmd, "status") == 0) {
        printf("%d\n", last_status);
        return last_status; // IMPORTANT: does not change
    }

    if (strcmp(cmd, "exit") == 0) {
        if (token_count > 1)
            exit(atoi(tokens[1]));
        else
            exit(last_status);
    }

    if (strcmp(cmd, "help") == 0) {
        printf("Builtins: debug prompt status exit help\n");
        return 0;
    }
    
    if(strcmp(cmd, "print") == 0){
        for(int i = 1; i < token_count - 1; i++)
        {
            printf("%s ", tokens[i]);
        }
        printf("%s", tokens[token_count - 1]);
    }

    if(strcmp(cmd, "echo") == 0){
        for(int i = 1; i < token_count - 1; i++)
        {
            printf("%s ", tokens[i]);
        }
        printf("%s\n", tokens[token_count - 1]);
    }

    if(strcmp(cmd, "len") == 0){
        int lenCounter = 0;
        for(int i = 1; i < token_count; i++)
        {
            lenCounter += strlen(tokens[i]);
        }
        printf("%d\n", lenCounter);
    }

    if(strcmp(cmd, "sum") == 0){
        int sumSum = 0;
        for(int i = 1; i < token_count; i++)
        {
            sumSum += atoi(tokens[i]);
        }
        printf("%d\n", sumSum);
    }

    if(strcmp(cmd, "calc") == 0){
        printf("%d\n", execute_calculation(tokens[1], tokens[3], tokens[2]));
    }
    
    if(strcmp(cmd, "basename") == 0){
        if(token_count < 2) return 1;
        char *path = tokens[1];
        char *last = strrchr(path, '/');
        if(last == NULL)
            printf("%s\n", path);
        else
            printf("%s\n", last + 1);
            
        return 0;
    }
    
    if(strcmp(cmd, "dirname") == 0){
        if(token_count < 2) return 1;
        char *path = tokens[1];
        char *last = strrchr(path, '/');
        if(last == NULL){
            printf(".\n");
        } else if(last == path){
            printf("/\n");
        } else {
            *last = '\0';
            printf("%s\n", path);
        }
        
        return 0;
    }
    

    return 0;
}

int execute_external() {
    printf("External command '");
    for (int i = 0; i < token_count; i++) {
        printf("%s ", tokens[i]);
    }
    printf("'\n");
    return 0;
}


//==================================================================
//==================================================================

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
        strcpy(tempLine, line);

        removeComment(line);
        if (isEmpty(line)) continue;


        int counter = tokenize(line);
        
        //Writing to standard outpur, Input line (whitout comments!!)
        /*if(is_builtin(tokens[0]) == 0 || (counter > 1 && strcmp(tokens[0], "debug") == 0 && !isdigit(tokens[1][0])))
        {
            printf("Input line: '%s'\n", tempLine);
        }
        //Writing to standard output, each token
        if(is_builtin(tokens[0]) == 0 || (counter > 1 && strcmp(tokens[0], "debug") == 0 && !isdigit(tokens[1][0]))){
            for(int i = 0; i < counter; i++)
            {
                printf("Token %d: '%s'\n", i, tokens[i]);
            }
        }*/
        
        if (debug_level > 0) 
        {
            printf("Input line: '%s'\n", tempLine);

            for(int i = 0; i < counter; i++) 
            {
                printf("Token %d: '%s'\n", i, tokens[i]);
            }
        }
        
        //Input redirect, Output redirect, Background
        lastThree(counter, tokens);

        int status;

        if (is_builtin(tokens[0])) 
        {
            if (debug_level > 0) 
            {
                printf("Executing builtin '%s' in %s\n", tokens[0], background ? "background" : "foreground");
            }
            status = execute_builtin();
        } 
        else 
        {
            printf("External command '");
            for (int i = 0; i < token_count; i++) 
            {
                printf("%s", tokens[i]);
                if(i != token_count - 1)
                {
                    printf(" ");
                }
            }
            printf("'\n");
            status = 0;
        }

        if (!background) 
        {
            last_status = status;
        }

    }
    
    return last_status;

}
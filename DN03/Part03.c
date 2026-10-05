#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#define MAX_TOKENS 100
#define MAX_LINE 1024

char line[MAX_LINE];
char tempLine[MAX_LINE];
char* tokens[MAX_TOKENS];
int token_count;

int debug_level = 0;
int last_status = 0;
char prompt[10] = "mysh";
char cwd[MAX_LINE];

char proc_path[MAX_LINE] = "/proc";

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

/*void lastThree(int counter, char** tokens){ 
    for(int i = counter - 1; i >= 0; i--) 
    { 
        if(tokens[i][0] == '<') 
        { 
            printf("Input redirect: '%s'\n", tokens[i] + 1);
            token_count--;
        } 
        else if(tokens[i][0] == '>') 
        { 
            printf("Output redirect: '%s'\n", tokens[i] + 1);
            token_count--;
        } 
        else if(strcmp(tokens[i], "&") == 0) 
        { 
            printf("Background: 1\n"); 
            token_count--;
            background = 1;
        } 
    } 
}*/

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

int cmp_int(const void *a, const void *b){
    int x = *(int*)a;
    int y = *(int*)b;
    return x - y;
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
           strcmp(cmd, "dirname") == 0 ||
           strcmp(cmd, "dirch") == 0 ||
           strcmp(cmd, "dirwd") == 0 ||
           strcmp(cmd, "dirmk") == 0 ||
           strcmp(cmd, "dirrm") == 0 ||
           strcmp(cmd, "dirls") == 0 ||
           strcmp(cmd, "rename") == 0 ||
           strcmp(cmd, "unlink") == 0 ||
           strcmp(cmd, "remove") == 0 ||
           strcmp(cmd, "linkhard") == 0 ||
           strcmp(cmd, "linksoft") == 0 ||
           strcmp(cmd, "linkread") == 0 ||
           strcmp(cmd, "linklist") == 0 ||
           strcmp(cmd, "cpcat") == 0 ||
           strcmp(cmd, "pid") == 0 ||
           strcmp(cmd, "ppid") == 0 ||
           strcmp(cmd, "uid") == 0 ||
           strcmp(cmd, "euid") == 0 ||
           strcmp(cmd, "gid") == 0 ||
           strcmp(cmd, "egid") == 0 ||
           strcmp(cmd, "sysinfo") == 0 ||
           strcmp(cmd, "proc") == 0 ||
           strcmp(cmd, "pids") == 0 ||
           strcmp(cmd, "pinfo") == 0;
}

int execute_builtin() {
    char *cmd = tokens[0];

    if(strcmp(cmd, "proc") == 0){                   //PROC     
        if (token_count == 1) {
            printf("%s\n", proc_path);
            return 0;
        }
        else if (token_count == 2) {
            if (access(tokens[1], F_OK | R_OK) != 0) {
                return 1;  
            }
            strncpy(proc_path, tokens[1], MAX_LINE - 1);
            proc_path[MAX_LINE - 1] = '\0';

            return 0;
        } 
        else {
            return 1;
        } 
    }
    else if(strcmp(cmd, "pids") == 0){            //PIDS
        DIR *dir = opendir(proc_path);
        if (dir == NULL) 
        {
            perror("pids");
            return errno;
        }

        int pids[10000];
        int count = 0;

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            int is_pid = 1;
            for (int i = 0; entry->d_name[i] != '\0'; i++) {
                if (!isdigit((unsigned char)entry->d_name[i])) {
                    is_pid = 0;
                    break;
                }
            }

            if (is_pid) {
                pids[count++] = atoi(entry->d_name);
            }
        }

        closedir(dir);

        //Sorting
        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (pids[i] > pids[j]) {
                    int tmp = pids[i];
                    pids[i] = pids[j];
                    pids[j] = tmp;
                }
            }
        }
        for (int i = 0; i < count; i++) {
            printf("%d\n", pids[i]);
        }

        return 0;
    }
    else if(strcmp(cmd, "pinfo") == 0){           //PINFO
        DIR *dir = opendir(proc_path);
        if (dir == NULL) {
            perror("pinfo");
            return errno;
        }

        int pids[10000];
        int count = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            int ok = 1;
            for (int i = 0; entry->d_name[i]; i++) {
                if (!isdigit((unsigned char)entry->d_name[i])) {
                    ok = 0;
                    break;
                }
            }

            if (ok) 
            {
                pids[count++] = atoi(entry->d_name);
            }
        }

        closedir(dir);
        qsort(pids, count, sizeof(int), cmp_int);
        printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");

        for (int i = 0; i < count; i++) 
        {
            char path[MAX_LINE];
            snprintf(path, sizeof(path), "%s/%d/stat", proc_path, pids[i]);
            FILE *f = fopen(path, "r");
            if (!f) continue;

            int pid, ppid;
            char state;
            char name[256];
            fscanf(f, "%d (%[^)]) %c %d", &pid, name, &state, &ppid);
            fclose(f);

            printf("%5d %5d %6c %s\n", pid, ppid, state, name);
        }

        return 0;
    }
    else if(strcmp(cmd, "sysinfo") == 0){           //SYSINFO    
        struct utsname u;
        if(uname(&u) != 0)
        {
            int err = errno;
            fflush(stdout);
            perror("sysinfo");
            return err;
        }

        printf("Sysname: %s\n", u.sysname);
        printf("Nodename: %s\n", u.nodename);
        printf("Release: %s\n", u.release);
        printf("Version: %s\n", u.version);
        printf("Machine: %s\n", u.machine);

        return 0;
    }
    else if(strcmp(cmd, "egid") == 0){              //EGID
        if(token_count != 1)
        {
            return 1;
        }
        printf("%i\n", getegid());
        return 0;
    }
    else if(strcmp(cmd, "gid") == 0){               //GID
        if(token_count != 1)
        {
            return 1;
        }
        printf("%i\n", getgid());
        return 0;
    }
    else if(strcmp(cmd, "euid") == 0){              //EUID
        if(token_count != 1)
        {
            return 1;
        }
        printf("%i\n", geteuid());
        return 0;
    }
    else if(strcmp(cmd, "uid") == 0){               //UID
        if(token_count != 1)
        {
            return 1;
        }
        printf("%i\n", getuid());
        return 0;
    }
    else if(strcmp(cmd, "ppid") == 0){              //PPID
        if(token_count != 1)
        {
            return 1;
        }
        printf("%i\n", getppid());
        return 0;
    }
    else if(strcmp(cmd, "pid") == 0){               //PID
        if(token_count != 1)
        {
            return 1;
        }
        printf("%i\n", getpid());
        return 0;
    }
    else if (strcmp(cmd, "debug") == 0) {           //DEBUG
        if (token_count == 1) {
            printf("%d\n", debug_level);
        } else {
            debug_level = atoi(tokens[1]);
        }
        return 0;
    }
    else if (strcmp(cmd, "prompt") == 0) {          //PROMPT
        if (token_count == 1) {
            printf("%s\n", prompt);
        } else {
            if (strlen(tokens[1]) > 8) return 1;
            strcpy(prompt, tokens[1]);
        }
        return 0;
    }
    else if (strcmp(cmd, "status") == 0) {          //STATUS
        printf("%d\n", last_status);
        return last_status; // IMPORTANT: does not change
    }
    else if (strcmp(cmd, "exit") == 0) {            //EXIT
        if (token_count > 1)
            exit(atoi(tokens[1]));
        else
            exit(last_status);
    }
    else if (strcmp(cmd, "help") == 0) {            //HELP
        printf("Builtins: debug prompt status exit help\n");
        return 0;
    }
    else if(strcmp(cmd, "print") == 0){             //PRINT
        for(int i = 1; i < token_count - 1; i++)
        {
            printf("%s ", tokens[i]);
        }
        printf("%s", tokens[token_count - 1]);

        return 0;
    }
    else if(strcmp(cmd, "echo") == 0){              //ECHO
        for(int i = 1; i < token_count; i++)
        {
            printf("%s", tokens[i]);
            if(i != token_count - 1) printf(" ");
        }
        printf("\n");
        return 0;
    }
    else if(strcmp(cmd, "len") == 0){               //LEN
        int lenCounter = 0;
        for(int i = 1; i < token_count; i++)
        {
            lenCounter += strlen(tokens[i]);
        }
        printf("%d\n", lenCounter);

        return 0;
    }
    else if(strcmp(cmd, "sum") == 0){               //SUM
        int sumSum = 0;
        for(int i = 1; i < token_count; i++)
        {
            sumSum += atoi(tokens[i]);
        }
        printf("%d\n", sumSum);

        return 0;
    }
    else if(strcmp(cmd, "calc") == 0){              //CALC
        printf("%d\n", execute_calculation(tokens[1], tokens[3], tokens[2]));

        return 0;
    }
    else if(strcmp(cmd, "basename") == 0){          //BASENAME
        if(token_count < 2) return 1;
        char *path = tokens[1];
        char *last = strrchr(path, '/');
        if(last == NULL)
            printf("%s\n", path);
        else
            printf("%s\n", last + 1);
            
        return 0;
    }
    else if(strcmp(cmd, "dirname") == 0){           //DIRNAME
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
    else if(strcmp(cmd, "dirch") == 0){             //DIRCH
        errno = 0;
        int dirResult;

        if(token_count < 2)
        {
            dirResult = chdir("/");
        }
        else
        {
            dirResult = chdir(tokens[1]);
        }

        if(dirResult != 0)
        {
            int err = errno;
            fflush(stdout);
            perror("dirch");
            return err;
        }

        return 0;
    }
    else if(strcmp(cmd, "dirwd") == 0){             //DIRWD
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            int err = errno;
            fflush(stdout);
            perror("dirwd");
            return err;
        }

        if (token_count == 1 || strcmp(tokens[1], "base") == 0) { 
            if (strcmp(cwd, "/") == 0) {
                printf("/\n");
            } else {
                char *last = strrchr(cwd, '/');
                printf("%s\n", last + 1);
            }
        } 
        else if (strcmp(tokens[1], "full") == 0) {
            printf("%s\n", cwd);
        } 
        else {
            return 1;
        }

        return 0;
    }
    else if (strcmp(cmd, "dirmk") == 0) {           //DIRMK
        if (token_count < 2) return 1;

        if (mkdir(tokens[1], 0777) != 0) {
            int err = errno;
            fflush(stdout);
            perror("dirmk");
            return err;
        }

        return 0;
    }
    else if (strcmp(cmd, "dirrm") == 0) {           //DIRRM
        if (token_count < 2) return 1;

        if (rmdir(tokens[1]) != 0) {
            int err = errno;
            fflush(stdout);
            perror("dirrm");
            return err;
        }

        return 0;
    }
    else if (strcmp(cmd, "dirls") == 0) {           //DIRLS
        const char *path = (token_count < 2) ? 
        "." : tokens[1];

        DIR *dir = opendir(path);
        if (dir == NULL) {
            fflush(stdout);
            perror("dirls");
            return errno;
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) {
            printf("%s  ", entry->d_name);
        }
        printf("\n");

        closedir(dir);
        return 0;
    }
    else if(strcmp(cmd, "rename") == 0){            //RENAME
        if(rename(tokens[1], tokens[2]) != 0)
        {
            fflush(stdout);
            perror("rename");
            return errno;
        }
        return 0;
    }
    else if(strcmp(cmd, "unlink") == 0){            //UNLINK
        if(unlink(tokens[1]) != 0)
        {
            fflush(stdout);
            perror("unlink");
            return errno;
        }
        return 0;
    }
    else if(strcmp(cmd, "remove") == 0){            //REMOVE
        if(remove(tokens[1]) != 0)
        {
            fflush(stdout);
            perror("remove");
            return errno;
        }
        return 0;
    }
    else if(strcmp(cmd, "linkhard") == 0){          //LINKHARD
        if(link(tokens[1], tokens[2]) != 0)
        {
            fflush(stdout);
            perror("linkhard");
            return errno;
        }
        return 0;
    }
    else if(strcmp(cmd, "linksoft") == 0){          //LINKSOFT
        if(symlink(tokens[1], tokens[2]) != 0)
        {
            fflush(stdout);
            perror("linksoft");
            return errno;
        }
        return 0;
    }
    else if(strcmp(cmd, "linkread") == 0){          //LINKREAD
        char bufferLink[MAX_LINE];
        int leng = readlink(tokens[1], bufferLink, sizeof(bufferLink) - 1);
        if(leng == -1)
        {
            fflush(stdout);
            perror("linkread");
            return errno;
        }
        
        bufferLink[leng] = '\0';
        printf("%s\n", bufferLink);                         
        return 0;                                       
    }                                                   
    else if (strcmp(cmd, "linklist") == 0) {         //LINKLIST
        if (token_count < 2) return 1;

        struct stat target_st;

        if (stat(tokens[1], &target_st) != 0) {
            int err = errno;
            fflush(stdout);
            perror("linklist");
            return err;
        }

        DIR *dir = opendir(".");
        if (!dir) {
            int err = errno;
            fflush(stdout);
            perror("linklist");
            return err;
        }

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL) 
        {

            struct stat st;
            if (stat(entry->d_name, &st) == 0) 
            {

                if (st.st_ino == target_st.st_ino) {
                    printf("%s  ", entry->d_name);
                }
            }
        }

        printf("\n");
        closedir(dir);
        return 0;
    }
    else if(strcmp(cmd, "cpcat") == 0){             //CPCAT
        int fd0 = STDIN_FILENO;
        int fd1 = STDOUT_FILENO;

        if (token_count >= 2 && strcmp(tokens[1], "-") != 0) {
            fd0 = open(tokens[1], O_RDONLY);
            if (fd0 < 0) {
                int err = errno;
                fflush(stdout);
                perror("cpcat");
                return err;
            }
        }

        if (token_count >= 3) {
            fd1 = open(tokens[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd1 < 0) {
                int err = errno;
                fflush(stdout);
                perror("cpcat");
                if (fd0 != STDIN_FILENO) close(fd0);
                return err;
            }
        }

        char buffer[1024];
        ssize_t n;

        while ((n = read(fd0, buffer, sizeof(buffer))) > 0) {
            if (write(fd1, buffer, n) != n) {
                fflush(stdout);
                perror("cpcat");
                if (fd0 != STDIN_FILENO) close(fd0);
                if (fd1 != STDOUT_FILENO) close(fd1);
                return errno;
            }
        }

        if (n < 0) {
            perror("cpcat");
        }

        if (fd0 != STDIN_FILENO) close(fd0);
        if (fd1 != STDOUT_FILENO) close(fd1);

        return (n < 0) ? errno : 0;
    }
    else {
        return 1;
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
        //background = 0;
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

        tokens[counter] = NULL; 
        
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
            fflush(stdin);

            pid_t pid = fork();

            if (pid < 0) {
                perror("fork");
                status = errno;
            }
            else if (pid == 0) {
                // CHILD

                execvp(tokens[0], tokens);

                perror("exec");
                exit(127);   
            }
            else {
                // PARENT
                if (!background) {
                    int wstatus;
                    waitpid(pid, &wstatus, 0);

                    if (WIFEXITED(wstatus)) {
                       status = WEXITSTATUS(wstatus);
                    } else {
                      status = 1;
                    }
                } else {
                    status = 0;
                }
            }
        }

        if (!background) 
        {
            last_status = status;
        }
        
        fflush(stdout);

    }
    
    return last_status;

}
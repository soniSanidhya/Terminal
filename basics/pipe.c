#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include<unistd.h>
#include <stdlib.h>

int main(){
    char command[1024] = "";
    while(1){
        printf("terminal:~$ ");
        if(fgets(command , sizeof(command) , stdin) == NULL){
            perror("input");
            break;
        }

        command[strcspn(command , "\n")] = '\0';

        char* args[100];
        int arg_len = 0;

        char* token = strtok(command , " ");
        
        while(token != NULL && arg_len < 99){
            args[arg_len++] = token;
            token = strtok(NULL , " ");
        }
        args[arg_len] = NULL;

        int fd[2];
        if(pipe(fd) == -1){
            perror("pipe");
            return 1;
        }

        pid_t pid = fork();

        if(pid < 0){
            perror("Child");
            close(fd[0]);
            close(fd[1]);
            return 1;
        }

        if(pid == 0){
            close(fd[0]);

            dup2(fd[1] , STDOUT_FILENO);

            close(fd[1]);

            execvp(args[0] , args);

            perror("exec");
            _exit(127);
        }

        close(fd[1]);

        char buffer[1024];
        ssize_t n = 0;

        while((n = read( fd[0] , buffer , sizeof(buffer))) > 0){
            fwrite(buffer , 1 , n , stdout);
        }

        close(fd[0]);

        waitpid(pid , NULL , 0);
    }
}
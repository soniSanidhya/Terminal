#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    char command[1024];

    while (1)
    {
        printf("mini-shell$ ");

        if (fgets(command, sizeof(command), stdin) == NULL)
            break;

        command[strcspn(command, "\n")] = '\0';

        char *args[100];
        int i = 0;

        char *token = strtok(command, " ");

        while (token != NULL && i < 99)
        {
            args[i++] = token;
            token = strtok(NULL, " ");
        }

        args[i] = NULL;

        if (args[0] == NULL)
            continue;

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
        }
        else if (pid == 0)
        {
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
        else
        {
            waitpid(pid, NULL, 0);
        }
    }

    return 0;
}
    #include <stdio.h>
    #include <sys/wait.h>
    #include <string.h>
    #include <stdlib.h>

int main()
{
    char command[1024];

    while (1)
    {
        printf("terminal$: ");
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

        if (strcmp(args[0], "exit") == 0)
            break;

        if (strcmp(args[0], "cd") == 0)
        {
            char *path = args[1];

            if (path == NULL)
            {
                path = getenv("HOME");
            }
            if (path == NULL || chdir(path) != 0)
            {
                perror("cd");
            }
            continue;
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            // error in creating child
            perror("fork");
            return 1;
        }
        else if (pid == 0)
        {
            // child process;
            execvp(args[0], args);
            perror("exec");
            _exit(127);
        }
        else
        {
            // paren process
            waitpid(pid, NULL, 0);
        }
    }

    return 0;
}
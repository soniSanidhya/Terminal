#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void draw_line(Display *display, Window win, GC gc, char *prompt, int prompt_len, char *input, int input_len, int x_start, int y_start, bool fclr)
{
    int char_width = 8;
    if (fclr)
        XClearWindow(display, win);
    XDrawString(display, win, gc, x_start, y_start, prompt, prompt_len);
    XDrawString(display, win, gc, x_start + prompt_len * char_width, y_start, input, input_len);
}
void draw_output(Display *display, Window win, GC gc, char *str, int str_len, int x_start, int y_start)
{
    int y = 0, st = 0;
    for (int i = 0; i < str_len; i++)
    {
        if (str[i] == '\n')
        {
            XDrawString(display, win, gc, x_start, y_start + y * 20, &str[st], i - st);
            st = i + 1;
            y++;
        }
    }
    if (st < str_len)
    {
        XDrawString(display, win, gc, x_start, y_start + y * 20, &str[st], str_len - st);
    }
}

void run_cmd(char *command, char *output, int *output_len)
{
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
        return;

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
        return;
    }

    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("pipe");
        return;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        // erorr in child creation
        close(fd[0]);
        close(fd[1]);
        perror("child");
        return;
    }
    else if (pid == 0)
    {
        // chld process
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(args[0], args);
        perror("exec");
        _exit(127);
    }

    char buffer[2048];
    ssize_t n;
    close(fd[1]);
    while ((n = read(fd[0], buffer, sizeof(buffer))) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            if (*output_len < 10000 - 1)
                output[(*output_len)++] = buffer[i];
        }
        output[*output_len] = '\0';
    }
    close(fd[0]);
    // parent process;
    waitpid(pid, NULL, 0);
}

int main()
{
    Display *display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Can't connect to display\n");
        return 1;
    }
    fprintf(stdout, "connected successfully TT\n");
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    Window win = XCreateSimpleWindow(
        display,
        root,
        100,
        100,
        900,
        500,
        3,
        WhitePixel(display, screen),
        BlackPixel(display, screen));

    XSelectInput(display, win, ExposureMask | KeyPressMask);

    GC gc = XCreateGC(display, win, 0, NULL);
    XSetForeground(display, gc, WhitePixel(display, screen));
    XMapWindow(display, win);

    XEvent event;
    char input[1024] = "";
    char output[10000] = "";
    int output_len = 0;
    int input_len = 0;
    char prompt[] = "user@terminator:~$";
    int prompt_len = sizeof(prompt) - 1;
    int cmd_count = 1;
    while (1)
    {
        XNextEvent(display, &event);

        if (event.type == Expose && event.xexpose.count == 0)
        {
            printf("Display Exposed\n");
            draw_line(display, win, gc, prompt, prompt_len, input, input_len, 10, 20, 0);
            draw_output(display, win, gc, output, output_len, 10, 40);
        }
        if (event.type == KeyPress)
        {
            char buffer[32];
            KeySym keysym;
            int len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, NULL);

            if (keysym == XK_Return)
            {
                char copyinput[1024];
                strcpy(copyinput, input);
                printf("%s\n", copyinput);
                fflush(stdout);
                output[0] = '\0';
                output_len = 0;
                run_cmd(input, output, &output_len);
                input[0] = '\0';
                input_len = 0;
            }
            else if (keysym == XK_BackSpace)
            {
                if (input_len > 0)
                {
                    input_len--;
                    input[input_len] = '\0';
                }
            }
            else
            {
                for (int i = 0; i < len; i++)
                {
                    if (input_len < (int)sizeof(input) - 1)
                    {
                        input[input_len++] = buffer[i];
                    }
                }
                input[input_len] = '\0';
            }
            draw_line(display, win, gc, prompt, prompt_len, input, input_len, 10, 20, 1);
        }
        if (output_len > 0)
        {
            draw_output(display, win, gc, output, output_len, 10, 40);
        }
    }

    XDestroyWindow(display, win);
    XCloseDisplay(display);

    return 0;
}
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define HISTORYLEN 50000
#define MAXCMDHISTORY 20
#define MAXTAB 100
struct Terminal
{
    char history[HISTORYLEN];
    int history_len;
    char input[1024];
    int input_len;
    char output[10000];
    int output_len;
    char *cmd_history[MAXCMDHISTORY];
    int cmd_history_len;
    int scroll_offset;
};

int count_lines(char *str, int str_len)
{
    int total_lines = str_len > 0;
    for (int i = 0; i < str_len; i++)
    {
        if (str[i] == '\n')
        {
            total_lines++;
        }
    }
    return total_lines;
}

void draw_line(Display *display, Window win, GC gc, char *input, int input_len, int x_start, int y_start, bool fclr)
{
    int char_width = 8;
    if (fclr)
        XClearWindow(display, win);
    XDrawString(display, win, gc, x_start, y_start, input, input_len);
}
int draw_output(Display *display, Window win, GC gc, char *str, int str_len, int x_start, int y_start, int height, int scroll_offset)
{

    int total_lines = count_lines(str, str_len);
    int visible_lines = (height - y_start) / 20;
    int max_skip = total_lines - visible_lines;
    if (max_skip < 0)
        max_skip = 0;
    int skip_lines = max_skip - scroll_offset;
    if (skip_lines < 0)
        skip_lines = 0;
    int y = 0, st = 0, line_no = 0;
    for (int i = 0; i < str_len; i++)
    {

        if (str[i] == '\n')
        {

            if (line_no >= skip_lines)
            {
                XDrawString(display, win, gc, x_start, y_start + y * 20, &str[st], i - st);
                y++;
            }
            st = i + 1;
            line_no++;
        }
    }
    if (st < str_len && line_no >= skip_lines)
    {
        XDrawString(display, win, gc, x_start, y_start + y * 20, &str[st], str_len - st);
    }
    return y;
}

int run_cmd(char *command, char *output, int *output_len, struct Terminal *tab)
{
    char *args[100];
    int i = 0;
    int redirection_ind = strcspn(command, ">");
    char *filename = NULL;
    if (redirection_ind < strlen(command))
    {
        printf("%d\n", redirection_ind);

        command[redirection_ind] = '\0';
        redirection_ind++;
        filename = malloc(strlen(command + redirection_ind));
        strcpy(filename, command + redirection_ind);
        filename = command + redirection_ind;
    }
    printf("comd %s \n", command);
    printf("filename%s \n", filename);
    char *token = strtok(command, " ");

    while (token != NULL && i < 99)
    {

        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL)
        return 0;

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
        return 0;
    }
    if (strcmp(args[0], "exit") == 0)
    {
        return 1;
    }
    if (strcmp(args[0], "history") == 0)
    {
        printf("history %d\n", tab->cmd_history_len);
        for (int i = 0; i < tab->cmd_history_len; i++)
        {
            if (*output_len + strlen(tab->cmd_history[i]) + 1 < 10000 - 1)
            {
                printf("%s\n", tab->cmd_history[i]);
                strcpy(output + *output_len, tab->cmd_history[i]);
                *output_len += strlen(tab->cmd_history[i]);
                // strcpy(output+*output_len , "\n");
                output[*output_len] = '\n';
                (*output_len)++;
            }
        }
        printf("%d", *output_len);
        output[*output_len] = '\0';
        printf("%s\n", output);
        printf("done \n");
        return 0;
    }
    int fd[2];

    
    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 0;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        // erorr in child creation
        close(fd[0]);
        close(fd[1]);
        perror("child");
        return 0;
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
        
        {
            for (int i = 0; i < n; i++)
            {
                if (*output_len < 10000 - 1)
                    output[(*output_len)++] = buffer[i];
            }
            output[*output_len] = '\0';
        }
    }
    close(fd[0]);
    // parent process;
    waitpid(pid, NULL, 0);
    return 0;
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

    XSelectInput(display, win, ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask);

    GC gc = XCreateGC(display, win, 0, NULL);
    XSetForeground(display, gc, WhitePixel(display, screen));
    XMapWindow(display, win);
    XWindowAttributes attr;
    XGetWindowAttributes(display, win, &attr);
    int window_height = attr.height;
    XEvent event;
    struct Terminal tabs[MAXTAB];
    int tab_count = 1;
    int current_tab = 0;
    tabs[current_tab] = (struct Terminal){
        .history = "user@terminator:~$ ",
        .history_len = 19,
        .cmd_history_len = 0,
        .input = "",
        .input_len = 0,
        .output = "",
        .output_len = 0,
        .scroll_offset = 0};
    int history_ind = tabs[current_tab].cmd_history_len;
    // tabs[1] =

    // char history[HISTORYLEN] = "user@terminator:~$ ";
    // int history_len = 19;
    // char input[1024] = "";
    // char output[10000] = "";
    // int output_len = 0;
    // int input_len = 0;
    char prompt[] = "user@terminator:~$ ";
    int prompt_len = sizeof(prompt) - 1;

    while (1)
    {
        XNextEvent(display, &event);
        if (event.type == Expose && event.xexpose.count == 0)
        {
            printf("Display Exposed %d\n", attr.height);
        }
        if (event.type == ConfigureNotify)
        {
            printf("Window resized: %d x %d\n",
                   event.xconfigure.width,
                   event.xconfigure.height);
            window_height = event.xconfigure.height;
        }
        if (event.type == KeyPress)
        {
            char buffer[32];
            KeySym keysym;
            int len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, NULL);
            // printf("keysym = %lu, state = %u\n",
            //        (unsigned long)keysym, event.xkey.state);
            if ((ControlMask & event.xkey.state) && (ShiftMask & event.xkey.state))
            {

                if ((keysym == XK_ISO_Left_Tab))
                {
                    current_tab = (current_tab - 1 + tab_count) % tab_count;
                    printf("prev tab %d \n", current_tab);
                    history_ind = tabs[current_tab].cmd_history_len;
                }
            }
            else if ((event.xkey.state & ControlMask))
            {

                if (keysym == XK_Tab)
                {
                    current_tab = (current_tab + 1) % tab_count;
                    history_ind = tabs[current_tab].cmd_history_len;
                    printf("next tab %d \n", current_tab);
                }
                else if (tab_count < MAXTAB && keysym == XK_t)
                {
                    tabs[tab_count] = (struct Terminal){.history = "user@terminator:~$ ",
                                                        .history_len = 19,
                                                        .cmd_history_len = 0,
                                                        .input = "",
                                                        .input_len = 0,
                                                        .output = "",
                                                        .output_len = 0,
                                                        .scroll_offset = 0};
                    current_tab = tab_count++;
                    history_ind = tabs[current_tab].cmd_history_len;
                    printf("new tab created %d \n", current_tab);
                }
                else if (keysym == XK_w)
                {
                    int t = current_tab;
                    tab_count--;
                    for (int i = 0; i < tabs[current_tab].cmd_history_len; i++)
                    {
                        free(tabs[current_tab].cmd_history[i]);
                    }
                    if (tab_count == 0)
                        break;
                    else if (tab_count == current_tab)
                    {
                        current_tab--;
                    }
                    while (t < tab_count)
                    {
                        tabs[t] = tabs[t + 1];
                        t++;
                    }
                    XClearWindow(display, win);
                    int y = draw_output(display, win, gc, tabs[current_tab].history, tabs[current_tab].history_len, 10, 20, window_height, tabs[current_tab].scroll_offset);
                    draw_line(display, win, gc, tabs[current_tab].input, tabs[current_tab].input_len, 10 + prompt_len * 6, 20 + y * 20, 0);
                }
            }
            else if (keysym == XK_Up)
            {
                printf("up key pressed %d for tab %d and history length %d\n", history_ind, current_tab, tabs[current_tab].cmd_history_len);
                if (tabs[current_tab].cmd_history_len > 0)
                {
                    history_ind = (history_ind - 1 + tabs[current_tab].cmd_history_len) % tabs[current_tab].cmd_history_len;
                    strcpy(tabs[current_tab].input, tabs[current_tab].cmd_history[history_ind]);
                    tabs[current_tab].input_len = strlen(tabs[current_tab].cmd_history[history_ind]);
                }
            }
            else if (keysym == XK_Down)
            {

                if (tabs[current_tab].cmd_history_len > 0)
                {
                    if (history_ind == tabs[current_tab].cmd_history_len)
                    {
                        tabs[current_tab].input[0] = '\0';
                        tabs[current_tab].input_len = 0;
                    }
                    else
                    {
                        history_ind = (history_ind + 1) % tabs[current_tab].cmd_history_len;
                        strcpy(tabs[current_tab].input, tabs[current_tab].cmd_history[history_ind]);
                        tabs[current_tab].input_len = strlen(tabs[current_tab].cmd_history[history_ind]);
                    }
                }
            }
            else if (keysym == XK_Return)
            {
                char cpyinp[1024] = "";
                strcpy(cpyinp, tabs[current_tab].input);
                printf("%s\n", tabs[current_tab].input);
                fflush(stdout);
                if (tabs[current_tab].cmd_history_len < MAXCMDHISTORY)
                {
                    printf("histr length is less that max\n");
                    char *temp = malloc(strlen(tabs[current_tab].input) + 1);
                    if (!temp)
                        break;
                    tabs[current_tab].cmd_history[tabs[current_tab].cmd_history_len] = temp;
                    strcpy(tabs[current_tab].cmd_history[tabs[current_tab].cmd_history_len++], tabs[current_tab].input);
                }
                else
                {
                    printf("max reached %d\n", tabs[current_tab].cmd_history_len);
                    int i = 0;
                    free(tabs[current_tab].cmd_history[0]);
                    while (i < tabs[current_tab].cmd_history_len - 1)
                    {
                        tabs[current_tab].cmd_history[i] = tabs[current_tab].cmd_history[i + 1];
                        i++;
                    }
                    printf("compied successfully\n");
                    char *temp = malloc(strlen(tabs[current_tab].input) + 1);
                    if (!temp)
                        break;
                    tabs[current_tab].cmd_history[i] = temp;
                    strcpy(tabs[current_tab].cmd_history[i], tabs[current_tab].input);
                    // printf("unable to add ip\n");
                }
                history_ind = tabs[current_tab].cmd_history_len;

                tabs[current_tab].output[0] = '\0';
                tabs[current_tab].output_len = 0;
                if (tabs[current_tab].history_len + tabs[current_tab].input_len < HISTORYLEN - 1)
                {
                    memcpy(tabs[current_tab].history + tabs[current_tab].history_len, tabs[current_tab].input, tabs[current_tab].input_len);
                    tabs[current_tab].history_len += tabs[current_tab].input_len;
                    tabs[current_tab].history[tabs[current_tab].history_len] = '\0';
                }
                int ex = run_cmd(cpyinp, tabs[current_tab].output, &tabs[current_tab].output_len, &tabs[current_tab]);
                if (ex == 1)
                {
                    break;
                }

                if (tabs[current_tab].history_len + tabs[current_tab].output_len + prompt_len + 2 < HISTORYLEN - 1)
                {
                    memcpy(tabs[current_tab].history + tabs[current_tab].history_len, "\n", 1);
                    tabs[current_tab].history_len++;
                    memcpy(tabs[current_tab].history + tabs[current_tab].history_len, tabs[current_tab].output, tabs[current_tab].output_len);
                    tabs[current_tab].history_len += tabs[current_tab].output_len;
                    if (tabs[current_tab].history_len > 1 && tabs[current_tab].history[tabs[current_tab].history_len - 1] == '\n')
                    {
                        printf("\nend with \n");
                    }
                    else
                    {
                        printf("\ndoes not end with \n");
                        tabs[current_tab].history[tabs[current_tab].history_len++] = '\n';
                        tabs[current_tab].history[tabs[current_tab].history_len] = '\0';
                    }
                    memcpy(tabs[current_tab].history + tabs[current_tab].history_len, prompt, prompt_len);
                    tabs[current_tab].history_len += prompt_len;
                    tabs[current_tab].history[tabs[current_tab].history_len] = '\0';
                }
                tabs[current_tab].input[0] = '\0';
                tabs[current_tab].input_len = 0;
            }
            else if (keysym == XK_BackSpace)
            {
                if (tabs[current_tab].input_len > 0)
                {
                    tabs[current_tab].input_len--;
                    tabs[current_tab].input[tabs[current_tab].input_len] = '\0';
                }
            }

            else
            {
                for (int i = 0; i < len; i++)
                {
                    if (tabs[current_tab].input_len < (int)sizeof(tabs[current_tab].input) - 1)
                    {
                        tabs[current_tab].input[tabs[current_tab].input_len++] = buffer[i];
                        // if (tabs[current_tab].history_len + input_len < HISTORYLEN - 1)
                        //     history[tabs[current_tab].history_len++] = buffer[i];
                    }
                }
                // history[tabs[current_tab].history_len] = '\0';
                tabs[current_tab].input[tabs[current_tab].input_len] = '\0';
            }
        }

        if (event.type == ButtonPress)
        {
            if (event.xbutton.button == Button4)
            {
                printf("scrolled up");
                int total_lines = count_lines(tabs[current_tab].history, tabs[current_tab].history_len);
                int visible_lines = (window_height - 20) / 20;
                int max_skip = total_lines - visible_lines;
                if (max_skip < 0)
                    max_skip = 0;
                if (tabs[current_tab].scroll_offset < max_skip)
                    tabs[current_tab].scroll_offset++;
            }
            else if (event.xbutton.button == Button5)
            {
                printf("scrolled up");

                if (tabs[current_tab].scroll_offset > 0)
                {
                    tabs[current_tab].scroll_offset--;
                }
            }
        }
        XClearWindow(display, win);
        int y = draw_output(display, win, gc, tabs[current_tab].history, tabs[current_tab].history_len, 10, 20, window_height, tabs[current_tab].scroll_offset);
        draw_line(display, win, gc, tabs[current_tab].input, tabs[current_tab].input_len, 10 + prompt_len * 6, 20 + y * 20, 0);
    }
    XDestroyWindow(display, win);
    XCloseDisplay(display);
    printf("done with it");
    return 0;
}
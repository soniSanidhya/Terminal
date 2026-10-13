#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>

int main()
{

    Display *display = XOpenDisplay(NULL);
    int screen = DefaultScreen(display);
    Window root = XDefaultRootWindow(display);

    Window win = XCreateSimpleWindow(display, root, 300, 300, 500, 500, 2, BlackPixel(display, screen), BlackPixel(display, screen));

    XSelectInput(display, win, ExposureMask | KeyPressMask);
    char input[1024] = "";
    int input_len = 0;
    GC gc = XCreateGC(display, win, 0, NULL);
    XSetForeground(display, gc, WhitePixel(display, screen));

    XMapWindow(display, win);
    XEvent event;

    while (1)
    {
        XNextEvent(display, &event);

        if (event.type == KeyPress)
        {
            char buffer[32];
            KeySym keysym;
            int len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, NULL);

            if (keysym == XK_Return)
            {
                printf("%s\n", input);
                fflush(stdout);
                input_len = 0;
                input[0] = '\0';
            }
            else
            {
                for (int i = 0; i < len; i++)
                {
                    if (input_len < (int)sizeof(input) - 1)
                    {
                        // printf("entered %.*s\n", len, buffer);
                        input[input_len++] = buffer[i];
                    }
                }
                input[input_len] = '\0';
            }
            XClearWindow(display, win);
            XDrawString(display, win, gc, 20, 100, input, input_len);
        }
        if (event.type == Expose)
        {
            printf("Exposed\n");
            XDrawString(display, win, gc, 20, 100, input, input_len);
        }
    }
    fflush(stdout);
    XDestroyWindow(display, win);
    XCloseDisplay(display);
    return 0;
}
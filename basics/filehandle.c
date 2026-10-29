
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Hello using printf\n");

    write(1, "Hello using write\n", 18);

    write(2, "This is an error\n", 17);

    return 0;
}

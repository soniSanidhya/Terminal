#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORYSIZE 50000

int main()
{

    char history[HISTORYSIZE] = "";
    int historyLen = 0;
    char input[] = "hello";
    char output[] = "\nworld";
    int input_len = 6, output_len = 7;
    memcpy(&history[historyLen], input, input_len);
    historyLen += input_len - 1;
    memcpy(&history[historyLen], output, output_len);
    historyLen += output_len - 1;
    printf("%s", history);
}
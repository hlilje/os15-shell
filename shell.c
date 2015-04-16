#include <stdio.h>
#include <string.h>
#include "shell.h"

int main(int argc, const char* argv[])
{
    for (;;)
    {
        char str[80]; /* Fixed length */
        int i;

        fgets(str, 80, stdin);

        /* Remove newline, if present */
        i = strlen(str) - 1;
        if(str[ i ] == '\n') str[i] = '\0';

        printf("%s\n", str);
    }

    return 0;
}

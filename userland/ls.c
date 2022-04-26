#include "syscall.h"
#include "lib.h"

#define ARGC_ERROR "Error: in the arguments.\n"

int
main(int argc, char *argv[])
{
    if (argc > 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    char result[200];
    Ls(result);

    Write(result, strlen(result), CONSOLE_OUTPUT);
    return 0;
}
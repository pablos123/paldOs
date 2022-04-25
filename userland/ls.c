#include "syscall.h"
#include "lib.h"

#define ARGC_ERROR "Error: in the arguments.\n"

int
main(int argc, char *argv[])
{
    //if (argc > 2)) {
    //    Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
    //    Exit(1);
    //}

    //Ls(argv[0])
    char result[40];
    Ls(result);

    Write(result, strlen(result), CONSOLE_OUTPUT);
    return 0;
}
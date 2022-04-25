#include "syscall.h"
#include "lib.h"

#define ARGC_ERROR "Error: missing argument.\n"
#define NOTFOUND_ERROR "Error: directory not found.\n"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    if(Cd(argv[0]))
        Write(NOTFOUND_ERROR, strlen(NOTFOUND_ERROR), CONSOLE_OUTPUT);

    return 0;
}
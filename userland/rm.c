#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument."
#define REMOVE_ERROR  "Error: could not remove the file"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    if(Remove(argv[0])) {
        Write(REMOVE_ERROR, sizeof(REMOVE_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    return 0;
}
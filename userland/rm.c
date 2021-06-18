#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define REMOVE_ERROR  "Error: could not remove the file.\n"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    int success = 1;
    for (unsigned i = 0; i < argc; i++) {
        if (Remove(argv[i])) {
            Write(REMOVE_ERROR, sizeof(REMOVE_ERROR) - 1, CONSOLE_OUTPUT);
            success = 0;
        }
    }

    return !success;
}
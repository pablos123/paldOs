#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define CREATE_ERROR  "Error: could not create directory.\n"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    int success = 1;
    for (unsigned i = 0; i < argc; i++) {
        if (Create(argv[i], 1)) {
            Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
            success = 0;
        }
    }
    return !success;
}

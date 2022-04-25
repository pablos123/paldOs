#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define DIRNOTFOUND_ERROR  "Error: directory does not exist.\n"
#define DIRNOTEMPTY_ERROR  "Error: directory not empty.\n"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    int success = 1;
    for (unsigned i = 0; i < argc; i++) {
        int response = Remove(argv[i], 1);
        if (response == -1) {
            Write(DIRNOTEMPTY_ERROR, sizeof(DIRNOTEMPTY_ERROR) - 1, CONSOLE_OUTPUT);
            success = 0;
        } else if (response == -2) {
            Write(DIRNOTFOUND_ERROR, sizeof(DIRNOTFOUND_ERROR) - 1, CONSOLE_OUTPUT);
            success = 0;
        }
    }
    return !success;
}

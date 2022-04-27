#include "syscall.h"
#include "lib.h"

#define ARGC_ERROR   "Error: missing argument.\n"
#define OPEN_ERROR   "Error: could not open file.\n"
#define CREATE_ERROR "Error: could not create file.\n"

int
PrintString(const char *s, OpenFileId fileToWrite)
{
    unsigned len = strlen(s);
    return Write(s, len, fileToWrite);
}

int
PrintChar(char c, OpenFileId fileToWrite)
{
    return Write(&c, 1, fileToWrite);
}

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId fileToWrite = Open(argv[0]);

    if(fileToWrite < 0) {
        int notSuccess = Create(argv[0], 0);
        if(notSuccess) {
            Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
            Exit(1);
        }

        fileToWrite = Open(argv[0]);
        if(fileToWrite < 0) {
            Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
            Exit(1);
        }
    }

    if(argc > 1) {
        for (unsigned i = 1; i < argc; i++) {
            if (i != 1) {
                PrintChar(' ', fileToWrite);
            }
            PrintString(argv[i], fileToWrite);
        }
    }

    Close(fileToWrite);
}
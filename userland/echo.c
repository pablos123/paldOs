/// Outputs arguments entered on the command line.

#include "syscall.h"
#include "lib.h"

int
PrintString(const char *s)
{
    unsigned len = strlen(s);
    return Write(s, len, CONSOLE_OUTPUT);
}

int
PrintChar(char c)
{
    return Write(&c, 1, CONSOLE_OUTPUT);
}

int
main(int argc, char *argv[])
{
    for (unsigned i = 0; i < argc; i++) {
        if (i != 0) {
            PrintChar(' ');
        }
        PrintString(argv[i]);
    }
    PrintChar('\n');
}

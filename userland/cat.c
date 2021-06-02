/// Creates files specified on the command line.

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument."
#define OPEN_ERROR    "Error: could not open the first file"

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId first_file = Open(argv[0]);
    if(first_file) {
        int nb = 1;
        char c[1] = {'\0'};
        while(nb  != 0){
            c[0] = '\0';
            nb = Read(c, 1, first_file);
            Write(c, 1, CONSOLE_OUTPUT);
        }
        c[0] = '\n';
        Write(c, 1, CONSOLE_OUTPUT);
    }
    else{
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        return 1;
    }

    Close(first_file);
    return 0;

}

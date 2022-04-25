/// Creates files specified on the command line.

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument.\n"
#define OPEN_ERROR1   "Error: could not open the first file.\n"
#define OPEN_ERROR2   "Error: could not open the second file.\n"

static void print_file(OpenFileId fid);

int
main(int argc, char *argv[])
{
    if (argc < 1) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    OpenFileId second_file = -1;
    if(argc == 2) {
        second_file = Open(argv[1]);
    }
    const OpenFileId first_file = Open(argv[0]);

    if(first_file > 0) {
        print_file(first_file);
    } else{
        Write(OPEN_ERROR1, sizeof(OPEN_ERROR1) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    if(argc == 2) {
        if(second_file > 0) {
            print_file(second_file);
        } else{
            Write(OPEN_ERROR2, sizeof(OPEN_ERROR2) - 1, CONSOLE_OUTPUT);
            Exit(1);
        }
    }

    return 0;
}


static void print_file(OpenFileId fid) {

    int nb = 1;
    char c[1] = {'\0'};
    while(nb != 0){ //lo hacemos para leer muchos bytes sin necesidad de memoria estatica grande
        c[0] = '\0';
        nb = Read(c, 1, fid);
        Write(c, 1, CONSOLE_OUTPUT);

    }

    c[0] = '\n'; //convencion de cat
    Write(c, 1, CONSOLE_OUTPUT);

    Close(fid);
    return;
}
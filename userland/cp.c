/// Creates files specified on the command line.

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument."
#define OPEN_ERROR    "Error: could not open the file"
#define CREATE_ERROR    "Error: could not create the file"


static void copy_file(OpenFileId fid1, OpenFileId fid2);

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }
    
    int to_copy_file =  Create(argv[1]);
    OpenFileId copied_file;
    if(to_copy_file) {
        Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    } else {
        copied_file = Open(argv[1]);
    }

    const OpenFileId first_file = Open(argv[0]);

    if(first_file > 0 && copied_file > 0) {
        copy_file(first_file, copied_file);
    } else{
        Write(OPEN_ERROR, sizeof(OPEN_ERROR) - 1, CONSOLE_OUTPUT);
        Exit(1);
    }

    return 0;
}


static void copy_file(OpenFileId fid1, OpenFileId fid2) {

    int nb = 1;
    char c[1] = {'\0'};
    while(nb != 0){ //lo hacemos para leer muchos bytes sin necesidad de memoria estatica grande
        c[0] = '\0';
        nb = Read(c, 1, fid1);
        Write(c, 1, fid2);
    }

    Close(fid1);
    Close(fid2);
    return;
}
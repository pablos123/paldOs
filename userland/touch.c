/// Creates files specified on the command line.

#include "syscall.h"

#define ARGC_ERROR    "Error: missing argument."
#define CREATE_ERROR  "Error: could not create file."

int
main(int argc, char *argv[])
{
    if (1) { //file test
        //Write(ARGC_ERROR, sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
        char string[] = {'f','i','l','e','1'};
        Create(string);
        
        int fileid = Open(string);

        int writedbytes = Write("hola como estas", sizeof("hola como estas") - 1, fileid);

        char buffer[sizeof("hola como estas") - 1];

        Close(fileid); // para que el puntero vuelva al principio
        
        fileid = Open(string); // para que el puntero vuelva al principio

        int bytesreaded = Read(buffer, writedbytes, fileid);

        Write(buffer, sizeof("hola como estas") - 1, CONSOLE_OUTPUT);

        Remove(string);

        Exit(1);
    }

    Write(argv[0], sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);
    Write(argv[1], sizeof(ARGC_ERROR) - 1, CONSOLE_OUTPUT);

    int success = 1;
    for (unsigned i = 0; i < argc; i++) {
        if (Create(argv[i]) < 0) {
            Write(CREATE_ERROR, sizeof(CREATE_ERROR) - 1, CONSOLE_OUTPUT);
            success = 0;
        }
    }
    return !success;
}

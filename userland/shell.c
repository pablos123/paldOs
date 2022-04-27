#include "lib.h"

#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '

#define NULL ((void *) 0)

static inline void
WritePrompt(OpenFileId output)
{
    static const char PROMPT[] = "--> ";
    Write(PROMPT, sizeof PROMPT - 1, output);
    return;
}

static inline void
WriteError(const char *description, OpenFileId output)
{
    static const char PREFIX[] = "Error: ";
    static const char SUFFIX[] = "\n";
    unsigned len = strlen(description);

    Write(PREFIX, sizeof PREFIX - 1, output);

    !len ? Write("bad description", sizeof("bad description"), output)
         : Write(description, len, output);

    Write(SUFFIX, sizeof SUFFIX - 1, output);

    return;
}

static unsigned
ReadLine(char *buffer, unsigned size, OpenFileId input)
{
    if(buffer == NULL)
        return 0;

    unsigned i;

    for (i = 0; i < size; i++) {
        Read(&buffer[i], 1, input);
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
    }

    return i;
}

static int
PrepareArguments(char *line, char **argv, unsigned argvSize)
{
    // The user cannot have ARG_SEPARATOR inside an argument!
    // And there will be no check for before-command trailing space.
    // If the user input have before-command trailing space it
    // will be just a forking error

    unsigned argCount = 0;

    if(argvSize > MAX_ARG_COUNT || line == NULL || argv == NULL)
        return 1;

    for (unsigned i = 0; line[i] != '\0'; i++) {
        if (line[i] == ARG_SEPARATOR) {
            if (argCount == argvSize - 1) {
                // The maximum of allowed arguments is exceeded, and
                // therefore the size of `argv` is too.  Note that 1 is
                // decreased in order to leave space for the NULL at the end.
                return 1;
            }
            line[i] = '\0';
            while(line[++i] == ARG_SEPARATOR); //to support more spaces between the arguments
            if(line[i] != '\0') //if im not in the end of the string add the argument, this is for trailing space
                argv[argCount++] = &line[i];
        }
    }

    argv[argCount] = NULL;

    return 0;
}


SpaceId ExecuteAlias(char* line, char** argv, int joinable) {

    SpaceId result = 0;

    if(argv[0] == NULL)
        argv = NULL;

    /// Utils
    if(strcmpp(line, "echo"))
        result = Exec("userland/echo", argv, joinable);

    else if(strcmpp(line, "exit"))
        result = Exec("userland/halt", argv, joinable);

    else if(strcmpp(line, "halt"))
        result = Exec("userland/halt", argv, joinable);

    else if(strcmpp(line, "sort"))
        result = Exec("userland/sort", argv, joinable);

    else if(strcmpp(line, "cat"))
        result = Exec("userland/cat", argv, joinable);

    else if(strcmpp(line, "cat>"))
        result = Exec("userland/write", argv, joinable);

    else if(strcmpp(line, "write"))
        result = Exec("userland/write", argv, joinable);

    else if(strcmpp(line, "matmult"))
        result = Exec("userland/matmult", argv, joinable);


    // Filesystem realated
    else if(strcmpp(line, "touch"))
        result = Exec("userland/touch", argv, joinable);

    else if(strcmpp(line, "create"))
        result = Exec("userland/touch", argv, joinable);

    else if(strcmpp(line, "rm"))
        result = Exec("userland/rm", argv, joinable);

    else if(strcmpp(line, "cp"))
        result = Exec("userland/cp", argv, joinable);

    else if(strcmpp(line, "ls"))
        result = Exec("userland/ls", argv, joinable);

    else if(strcmpp(line, "cd"))
        result = Exec("userland/cd", argv, joinable);

    else if(strcmpp(line, "mkdir"))
        result = Exec("userland/mkdir", argv, joinable);

    else if(strcmpp(line, "rmdir"))
        result = Exec("userland/rmdir", argv, joinable);

    else if(strcmpp(line, "help"))
        result = Exec("userland/help", argv, joinable);


    else
        result = Exec(line, argv, joinable);

    return result;
}

int
main(void)
{
    const OpenFileId      INPUT = CONSOLE_INPUT;
    const OpenFileId      OUTPUT = CONSOLE_OUTPUT;
    char                  line[MAX_LINE_SIZE];
    char                  *argv[MAX_ARG_COUNT];

    for (;;) {
        WritePrompt(OUTPUT);

        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);
         if (lineSize == 0) {
            WriteError("bad buffer.", OUTPUT);
            continue;
        }

        const int args = PrepareArguments(line, argv, MAX_ARG_COUNT);
        if(args) {
            WriteError("bad arguments.", OUTPUT);
            continue;
        }

        //convention: &command args
        //Excecute a given program with the argvs

        if(line[0] == '&') { //Execute in the background

            const SpaceId newProc = ExecuteAlias(&line[1], argv, 0);

            if(newProc < 0) {
                WriteError("error forking child", OUTPUT);
                continue;
            }
        } else { //Execute with join

            const SpaceId newProc = ExecuteAlias(line, argv, 1);

            if(newProc < 0) {
                WriteError("error forking child", OUTPUT);
                continue;
            } else {
                Join(newProc);
            }
        }
    }

    // Never reached.
    return -1;
}

#include "syscall.h"

#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '

#define NULL ((void *) 0)

static inline unsigned
strlen(const char *s)
{
    if(s == NULL)
        return 0;

    unsigned i;
    for (i = 0; s[i] != '\0'; i++) {}
    return i;
}

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
    unsigned argCount;
    argCount = 0;

    if(argvSize > MAX_ARG_COUNT || line == NULL || argv == NULL) {
        return 1;
    }

    // Traverse the whole line and replace spaces between arguments by null
    // characters, so as to be able to treat each argument as a standalone
    // string.
    //
    // TODO: what happens if there are two consecutive spaces?, and what
    //       about spaces at the beginning of the line?, and at the end?
    //
    // TODO: what if the user wants to include a space as part of an
    //       argument?

    
    for (unsigned i = 0; line[i] != '\0'; i++) {
        if (line[i] == ARG_SEPARATOR) {
            if (argCount == argvSize - 1) {
                // The maximum of allowed arguments is exceeded, and
                // therefore the size of `argv` is too.  Note that 1 is
                // decreased in order to leave space for the NULL at the end.
                return 1;
            }
            line[i] = '\0';
            argv[argCount++] = &line[i + 1]; //porque no tiene en cuenta muchos espacios
        }
    }

    argv[argCount] = NULL;
    return 0;
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

        //convencion: &command args
        //Excecute a given program with the argvs
        if(line[0] == '&') { //Execute in the background
            const SpaceId newProc = Exec(&line[1], argv, 0);

            if(newProc < 0) {
                WriteError("error forking child", OUTPUT);
                continue; 
            } 
        } else { //Execute with join
             const SpaceId newProc = Exec(line, argv, 1);

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

#include "syscall.h"
#include "lib.h"

#define HELP_HELP "\n`help`: Print this message and exit.\n\n"
#define LS_HELP "`ls`: Print the content of the current working directory.\n\n"

#define CD_HELP "`cd`: Change the current directory, supports an absolute path or a path in the current working directory.\n\n"
#define CD_USE  "    $ cd home    |    $ cd /home/usr    |    $ cd /\n\n\n"

#define CAT_HELP "`cat`: Concat two files and print the content in the terminal, can take one or two files.\n\n"
#define CAT_USE  "     $ cat file1    |    $ cat file1 file2\n\n\n"

#define WRITE_HELP "`write`: Write a chunk of text into a file.\n\n`cat>`: Alias for write.\n\n"
#define WRITE_USE  "       $ write file1 example text    |    $ cat> file2 example text\n\n\n"

#define TOUCH_HELP "`touch`: Create one or more files, supports an absolute path or a path in the current working directory.\n\n`create`: Alias for touch.\n\n"
#define TOUCH_USE  "       $ touch /home/usr/file1 file2 /home/usr1/file1    |    $ create hello_world \n\n\n"

#define MKDIR_HELP "`mkdir`: Create a directory, supports an absolute path or a path in the current directory.\n\n"
#define MKDIR_USE  "       $ mkdir /home/usr/dir1 dir2 /home/usr1/dir2\n\n\n"

#define RM_HELP "`rm`: Remove a file in the current working directory.\n\n"
#define RMDIR_HELP "`rmdir`: Remove a directory in the current working directory.\n\n"

#define EXIT_HELP "`exit`: Alias for Halt.\n\n"
#define HALT_HELP "`halt`: Halt the machine and exit.\n\n"

#define ECHO_HELP "`echo`: Print the input to stdout.\n\n"
#define SORT_HELP "`sort`: Stress the virtual memory sorting and array.\n\n"
#define MATMULT_HELP "`matmult`: Stress the virtual memory multiplying two matrices.\n\n"

int
main(int argc, char *argv[])
{
    Write(HELP_HELP,    sizeof(HELP_HELP)    - 1, CONSOLE_OUTPUT);

    Write(LS_HELP,      sizeof(LS_HELP)      - 1, CONSOLE_OUTPUT);

    Write(CD_HELP,      sizeof(CD_HELP)      - 1, CONSOLE_OUTPUT);
    Write(CD_USE,      sizeof(CD_USE)      - 1, CONSOLE_OUTPUT);

    Write(TOUCH_HELP,   sizeof(TOUCH_HELP)   - 1, CONSOLE_OUTPUT);
    Write(TOUCH_USE,   sizeof(TOUCH_USE)   - 1, CONSOLE_OUTPUT);

    Write(WRITE_HELP,   sizeof(WRITE_HELP)   - 1, CONSOLE_OUTPUT);
    Write(WRITE_USE,   sizeof(WRITE_USE)   - 1, CONSOLE_OUTPUT);

    Write(CAT_HELP,     sizeof(CAT_HELP)     - 1, CONSOLE_OUTPUT);
    Write(CAT_USE,     sizeof(CAT_USE)     - 1, CONSOLE_OUTPUT);

    Write(RM_HELP,      sizeof(RM_HELP)      - 1, CONSOLE_OUTPUT);

    Write(MKDIR_HELP,   sizeof(MKDIR_HELP)   - 1, CONSOLE_OUTPUT);
    Write(MKDIR_USE,   sizeof(MKDIR_USE)   - 1, CONSOLE_OUTPUT);

    Write(RMDIR_HELP,   sizeof(RMDIR_HELP)   - 1, CONSOLE_OUTPUT);

    Write(ECHO_HELP,    sizeof(ECHO_HELP)    - 1, CONSOLE_OUTPUT);

    Write(SORT_HELP,    sizeof(SORT_HELP)    - 1, CONSOLE_OUTPUT);
    Write(MATMULT_HELP, sizeof(MATMULT_HELP) - 1, CONSOLE_OUTPUT);

    Write(HALT_HELP,    sizeof(HALT_HELP)    - 1, CONSOLE_OUTPUT);
    Write(EXIT_HELP,    sizeof(EXIT_HELP)    - 1, CONSOLE_OUTPUT);

    return 0;
}
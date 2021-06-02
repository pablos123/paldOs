#include "syscall.h"
#include "lib.h"

int
main(void)
{
    // const OpenFileId input  = CONSOLE_INPUT; // pero por que no anda si descomento esto,
    // const OpenFileId output = CONSOLE_OUTPUT; // si dejo descomentado esto, no anda el programa
    char       ch, buffer[20];
    char*      argv[3];
    int        i;
    

    for (;;) {
        static const char PROMPT[] = {'-', '-', '>', ' ', ' ', ' ', '4', '6', '6','6','6','6','6', '$','$','$','$','$','$','$','$','$','$','$','$','$',};
        Write(PROMPT, 6, CONSOLE_OUTPUT); // si aca se pone output no imprime nada
        i = 0;
        do {
            Read(&buffer[i], 1, CONSOLE_INPUT);
        } while (buffer[i++] != '\n');

        buffer[--i] = '\0';

        char param1[6] = {'s', 'h', 'y','1', '\n', '\0'};
        char param2[6] = {'s', 'h', 'y','2', '\n', '\0'};

        argv[0] = param1;
        argv[1] = param2;

        if (i > 0) {
            SpaceId newProc = Exec(buffer, argv);
            Join(newProc);
            Exit(0);
        }
    }

    return -1;
}

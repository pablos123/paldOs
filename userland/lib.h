#include "syscall.h"

inline unsigned strlen(const char *s) {
    unsigned iter = 0;
    for(; s[iter] != '\0'; ++iter);
    return iter;
}

void putss(const char *s) {
    Write(s, sizeof(s), CONSOLE_OUTPUT);
}

static int elevar(int b, int e) {
    if(e == 0) {
        return b;
    }

    return b * elevar(b, e - 1);
}

void itoa(int i, char* c) {

    int counter = 0;
    char dummy[100];
    char digit;
    do
    {
        char digit = (char)(i/ elevar(10,counter));
        dummy[0] = digit;
        counter++;
    } while (digit >= 1);
    int estopper = counter;
    for(int i = 0; i < estopper; i++,counter--){
        c[i] = dummy[counter];
    }
    return;
}

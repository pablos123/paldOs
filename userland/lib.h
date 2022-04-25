#include "syscall.h"

unsigned strlen(const char *s) {
    unsigned iter = 0;
    for(; s[iter] != '\0'; ++iter);
    return iter;
}

char strcmpp(const char *s1, const char *s2) {
    unsigned iter = 0;
    for(;   s1[iter] != '\0'
         && s2[iter] != '\0'
         && s1[iter] == s2[iter]; ++iter);

    return s1[iter] == '\0' && s2[iter] == '\0' ? 1 : 0;
}

void putss(const char *s) {
    Write(s, strlen(s), CONSOLE_OUTPUT);
    Write("\n", 1, CONSOLE_OUTPUT);
    return;
}

void itoa(int n, char* c) {
    if (n == 0) {
        c[0] = 48;
        c[1] = '\0';
        return;
    }
    int i = 0;
    char dumb[9];
    while(n > 0) {
        int mod = n % 10;
        dumb[i] = mod + 48;
        n /= 10;
        i++;
    }

    int counter = i;
    for(int j = 0; j < i; j++)
        c[j] = dumb[--counter];

    c[i] = '\0';
    return;
}

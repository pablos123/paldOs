#include <stdio.h>

typedef struct {
    int i;
    int j;
    int array[10];
} RawHeader;

class FileHeader{
public:
    RawHeader raw;
};

int main(){
    FileHeader* fh = new FileHeader;
    printf("%d \n", sizeof(*fh));

    return 0;
}
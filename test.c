#include<stdio.h>

#define NUM_DIRECT (128 - 2 * 4) / 4

#define MAX_FILE_SIZE = NUM_DIRECT * 128

struct RawFileHeader {
    unsigned numBytes;  ///< Number of bytes in the file.
    unsigned numSectors;  ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                        ///< block in the file.
};

struct NewRawFileHeader {
    unsigned numBytes;  ///< Number of bytes in the file.
    unsigned numSectors;  ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                        ///< block in the file.
    void* indirectFileHeaders[NUM_DIRECT / 2 + 1];
};

int main() {

    printf("sizeof old raw file head: %lu\n", sizeof(struct RawFileHeader));
    printf("sizeof old raw file head: %lu\n", sizeof(struct NewRawFileHeader));


    return 0;
}
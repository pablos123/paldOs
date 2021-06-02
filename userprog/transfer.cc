/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"
#include <stdio.h>


void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount > 0 && byteCount < 50);

    unsigned count = 0;

    int temp;
    do {
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outBuffer = (unsigned char) temp;
        outBuffer++;
    } while (count < byteCount);

    *outBuffer = (unsigned char)'\0';

    return;
}

bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp)); //aca se modifica temp
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount > 0 && byteCount < 50);

    unsigned count = 0;

    do{
        count++;
        ///int temp = *buffer;
        ASSERT(machine->WriteMem(userAddress++, 1, *(int*) buffer));

    } while(*buffer++ != '\0' && count < byteCount);

    return;

}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    unsigned count = 0;
    do {
        count++;
        ASSERT(machine->WriteMem(userAddress++, 1, *(int*) string));

    } while (*string++ != '\0');
    return;
}

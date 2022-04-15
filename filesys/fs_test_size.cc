#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>


static const unsigned TRANSFER_SIZE = 10;  // Make it small, just to be
                                           // difficult.

/// Copy the contents of the UNIX file `from` to the Nachos file `to`.
void
CopyBigSize(const char *from, const char *to)
{
    ASSERT(from != nullptr);
    ASSERT(to != nullptr);

    // Open UNIX file.
    FILE *fp = fopen(from, "r");
    if (fp == nullptr) {
        printf("Copy: could not open input file %s\n", from);
        return;
    }

    // Figure out length of UNIX file.
    fseek(fp, 0, 2);
    int fileLength = ftell(fp);
    fseek(fp, 0, 0);

    DEBUG('f', "Copying file %s, size %u, to file %s\n",
          from, fileLength, to);

    // Create a Nachos file of the same length.
    if (!fileSystem->Create(to, fileLength)) {  // Create Nachos file.
        printf("Copy: could not create output file %s\n", to);
        fclose(fp);
        return;
    }

    OpenFile *openFile = fileSystem->Open(to);
    ASSERT(openFile != nullptr);

    // Copy the data in `TRANSFER_SIZE` chunks.
    char *buffer = new char [TRANSFER_SIZE];
    int amountRead;
    while ((amountRead = fread(buffer, sizeof(char),
                               TRANSFER_SIZE, fp)) > 0)
        openFile->Write(buffer, amountRead);
    delete [] buffer;

    // Close the UNIX and the Nachos files.
    delete openFile;
    fclose(fp);
}

/// Print the contents of the Nachos file `name`.
void
PrintBigSize(const char *name)
{
    ASSERT(name != nullptr);

    OpenFile *openFile = fileSystem->Open(name);
    if (openFile == nullptr) {
        fprintf(stderr, "Print: unable to open file %s\n", name);
        return;
    }

    char *buffer = new char [TRANSFER_SIZE];
    int amountRead;
    while ((amountRead = openFile->Read(buffer, TRANSFER_SIZE)) > 0) {
        for (unsigned i = 0; i < (unsigned) amountRead; i++) {
            printf("%c", buffer[i]);
        }
    }

    delete [] buffer;
    delete openFile;  // close the Nachos file
}


/// Creates a file with a big number of bytes
void
SizeTest()
{
    printf("Creating big file...\n");
    stats->Print();
    if (!fileSystem->Create("Big file", 70000)) {
        printf("Perf test: cannot create %s\n", "Big file");
    } else{
        printf("Perf test: File %s created successfully!\n", "Big file");
    }
    OpenFile *openFile = fileSystem->Open("Big file");
    if (openFile == nullptr) {
        fprintf(stderr, "Print: unable to open file %s\n", "Big file");
    } else {
        printf("File Big File opened successfully\n");
    }

    openFile->Close();

    if(! fileSystem->Remove("Big file")){
       printf("Perf test: cannot remove %s\n", "Big file");
    } else{
        printf("Perf test: File %s removed successfully!\n", "Big file");
    }
    stats->Print();
}

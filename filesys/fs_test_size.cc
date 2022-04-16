#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>

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

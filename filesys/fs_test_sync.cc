/// Simple test routines for the file system.
///
/// We implement:
///
/// Copy
///     Copy a file from UNIX to Nachos.
/// Print
///     Cat the contents of a Nachos file.
/// Perftest
///     A stress test for the Nachos file system read and write a really
///     really large file in tiny chunks (will not work on baseline system!)
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>

/// Performance test
///
/// Stress the Nachos file system by creating a large file, writing it out a
/// bit at a time, reading it back a bit at a time, and then deleting the
/// file.
///
/// Implemented as three separate routines:
/// * `FileWrite` -- write the file.
/// * `FileReadSync` -- read the file.
/// * `PerformanceTest` -- overall control, and print out performance #'s.

static const char FILE_NAME[] = "TestFile";
static const char CONTENTS[] = "1234567890";
static const unsigned CONTENT_SIZE = sizeof CONTENTS - 1;
static const unsigned FILE_SIZE = 200000;

static void
FileWriteSync(void* threadName)
{
    printf("Sequential write of %u byte file, in %u byte chunks\n",
           FILE_SIZE, CONTENT_SIZE);

    OpenFile *openFile = fileSystem->Open(FILE_NAME);
    if (openFile == nullptr) {
        fprintf(stderr, "Perf test: unable to open %s\n", FILE_NAME);
        return;
    }

    for (unsigned i = 0; i < FILE_SIZE; i += CONTENT_SIZE) {
        DEBUG('f', "THREAD Writing %s, with %s thread\n", (char *)threadName, (char *)threadName);
        unsigned numBytes = (unsigned)openFile->Write(CONTENTS, CONTENT_SIZE);
        if (numBytes < CONTENT_SIZE) {
            fprintf(stderr, "Perf test: unable to write %s\n", FILE_NAME);
            break;
        }
    }

    delete openFile;
}

static void
FileReadSync(void* threadName)
{
    DEBUG('t', "Executing %s thread\n", (char *)threadName);
    printf("Sequential read of %u byte file, in %u byte chunks\n",
           FILE_SIZE, CONTENT_SIZE);

    OpenFile *openFile = fileSystem->Open(FILE_NAME);
    if (openFile == nullptr) {
        fprintf(stderr, "Perf test: unable to open file %s\n", FILE_NAME);
        return;
    }

    char *buffer = new char [CONTENT_SIZE];
    for (unsigned i = 0; i < FILE_SIZE; i += CONTENT_SIZE) {
        int numBytes = openFile->Read(buffer, CONTENT_SIZE);
        if (numBytes < 10 || strncmp(buffer, CONTENTS, CONTENT_SIZE)) {
            printf("Perf test: unable to read %s\n", FILE_NAME);
            break;
        }
        printf("Read: %s\n", buffer);
    }

    delete [] buffer;
    delete openFile;
}

static void
FileRemoveSync(void* threadName)
{
    DEBUG('f', "Executing %s thread\n", (char *)threadName);
    DEBUG('f', "Removing file: %s\n", FILE_NAME);
    fileSystem->Remove(FILE_NAME);
}

static void
FileCreateSync(void* threadName)
{
    DEBUG('f', "Executing %s thread\n", (char *)threadName);
    DEBUG('f', "Creating file: %s\n", FILE_NAME);
    fileSystem->Create(FILE_NAME);
}

static void
FileCreateALot(void* threadName) {
    char *fileName = new char [10];
    for(int i = 0; i < 1000; i++) {
        printf("---------------ABOUT TO CREATE FILE NUMBER %u----------------\n", i);
        sprintf(fileName, "test%d", i);
        if (!fileSystem->Create(fileName)) {
            fprintf(stderr, "Perf test: cannot create %s\n", fileName);
            return;
        }
    }
}

void
PerformanceTestSync()
{
    printf("Starting syncronous file system performance test:\n");
    stats->Print();

    Thread** threads = new Thread*[6];

    char *name  = new char [8];
    char *name1 = new char [8];
    char *name2 = new char [8];
    char *name3 = new char [8];
    char *name4 = new char [8];
    char *name5 = new char [8];

    strncpy(name,  "2st", 8);
    strncpy(name1, "3rd", 8);
    strncpy(name2, "4th", 8);
    strncpy(name3, "5st", 8);
    strncpy(name4, "6rd", 8);
    strncpy(name5, "7th", 8);

    if (!fileSystem->Create(FILE_NAME)) {
        fprintf(stderr, "Perf test: cannot create %s\n", FILE_NAME);
        delete [] name;
        delete [] name1;
        delete [] name2;
        delete [] name3;
        delete [] name4;
        delete [] name5;
        return;
    }

    ////////////////////WRITE//////////////////////
    Thread *newThread = new Thread(name, true);
    newThread->Fork(FileWriteSync, (void *) name);
    threads[0] = newThread;

    Thread *newThread1 = new Thread(name1, true);
    newThread1->Fork(FileWriteSync, (void *) name1);
    threads[1] = newThread1;

    Thread *newThread2 = new Thread(name2, true);
    newThread2->Fork(FileWriteSync, (void *) name2);
    threads[2] = newThread2;

    Thread *newThread3 = new Thread(name3, true);
    newThread3->Fork(FileWriteSync, (void *) name3);
    threads[3] = newThread3;

    Thread *newThread4 = new Thread(name4, true);
    newThread4->Fork(FileWriteSync, (void *) name4);
    threads[4] = newThread4;

    Thread *newThread5 = new Thread(name5, true);
    newThread5->Fork(FileWriteSync, (void *) name5);
    threads[5] = newThread5;

    FileWriteSync((void *) "1st");

    for(int i = 0; i < 6; ++i) threads[i]->Join();

    ////////////////////READ//////////////////////

    newThread = new Thread(name, true);
    newThread->Fork(FileReadSync, (void *) name);
    threads[0] = newThread;

    newThread1 = new Thread(name1, true);
    newThread1->Fork(FileReadSync, (void *) name1);
    threads[1] = newThread1;

    newThread2 = new Thread(name2, true);
    newThread2->Fork(FileReadSync, (void *) name2);
    threads[2] = newThread2;

    newThread3 = new Thread(name3, true);
    newThread3->Fork(FileReadSync, (void *) name3);
    threads[3] = newThread3;

    newThread4 = new Thread(name4, true);
    newThread4->Fork(FileReadSync, (void *) name4);
    threads[4] = newThread4;

    newThread5 = new Thread(name5, true);
    newThread5->Fork(FileReadSync, (void *) name5);
    threads[5] = newThread5;

    FileReadSync((void *) "1st");

    fileSystem->Remove(FILE_NAME);  // to test the wait for the other threads

    for(int i = 0; i < 6; ++i) threads[i]->Join();

    //////////// REMOVE ///////////

    if (!fileSystem->Create(FILE_NAME)) {
        fprintf(stderr, "Perf test: cannot create %s\n", FILE_NAME);
    }

    printf("About to try 3 sequetial removes\n");

    newThread = new Thread(name, true);
    newThread->Fork(FileRemoveSync, (void *) name);
    threads[0] = newThread;

    newThread1 = new Thread(name1, true);
    newThread1->Fork(FileRemoveSync, (void *) name1);
    threads[1] = newThread1;

    newThread2 = new Thread(name2, true);
    newThread2->Fork(FileRemoveSync, (void *) name2);
    threads[2] = newThread2;

    for(int i = 0; i < 3; ++i) threads[i]->Join();

    /////////////// CREATE ///////////

    newThread = new Thread(name, true);
    newThread->Fork(FileCreateSync, (void *) name);
    threads[0] = newThread;

    newThread1 = new Thread(name1, true);
    newThread1->Fork(FileCreateSync, (void *) name1);
    threads[1] = newThread1;

    newThread2 = new Thread(name2, true);
    newThread2->Fork(FileCreateSync, (void *) name2);
    threads[2] = newThread2;

    for(int i = 0; i < 3; ++i) threads[i]->Join();

    printf("About to terminate the test with the final remove...\n");
    fileSystem->Remove(FILE_NAME);  // to test the wait for the other threads

    delete [] threads;

    delete [] name;
    delete [] name1;
    delete [] name2;
    delete [] name3;
    delete [] name4;
    delete [] name5;

    stats->Print();
}

void FileTestCreate() {
    Thread** threads = new Thread*[6];

    char *name  = new char [8];
    char *name1 = new char [8];
    char *name2 = new char [8];

    strncpy(name,  "1st", 8);
    strncpy(name1, "2nd", 8);
    strncpy(name2, "3rd", 8);

    Thread* newThread = new Thread(name, true);
    newThread->Fork(FileCreateALot, (void *) name);
    threads[0] = newThread;

    Thread* newThread1 = new Thread(name1, true);
    newThread1->Fork(FileCreateALot, (void *) name1);
    threads[1] = newThread1;

    Thread* newThread2 = new Thread(name2, true);
    newThread2->Fork(FileCreateALot, (void *) name2);
    threads[2] = newThread2;

    for(int i = 0; i < 3; ++i) threads[i]->Join();

    delete [] threads;

    delete [] name;
    delete [] name1;
    delete [] name2;

    FileCreateALot(nullptr);
}

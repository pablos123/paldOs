#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>

void
DirectoryTest() {

    printf("Creating tmp directory..\n");
    fileSystem->CreateDir("tmp"); // Create a directory in the root dir

    printf("Creating bin directory..\n");
    fileSystem->CreateDir("bin"); // Create a directory in the root dir

    printf("Creating bin directory..\n");
    fileSystem->CreateDir("bin"); // Create a directory in the root dir

    printf("Creating simulated binaries..\n");
    fileSystem->Create("/bin/cat"); // Create a directory in the root dir
    fileSystem->Create("/bin/cd"); // Create a directory in the root dir
    fileSystem->Create("/bin/ls"); // Create a directory in the root dir
    fileSystem->Create("/bin/touch"); // Create a directory in the root dir
    fileSystem->Create("/bin/echo"); // Create a directory in the root dir

    printf("Creating lib directory..\n");
    fileSystem->CreateDir("lib"); // Create a directory in the root dir

    printf("Creating proc directory..\n");
    fileSystem->CreateDir("proc"); // Create a directory in the root dir

    printf("Creating simulated procs..\n");
    fileSystem->Create("/proc/proc0"); // Create a directory in the root dir
    fileSystem->Create("/proc/proc1"); // Create a directory in the root dir
    fileSystem->Create("/proc/proc2"); // Create a directory in the root dir
    fileSystem->Create("/proc/proc3"); // Create a directory in the root dir
    fileSystem->Create("/proc/proc4"); // Create a directory in the root dir

    printf("Creating root directory..\n");
    fileSystem->CreateDir("root"); // Create a directory in the root dir

    printf("Creating lost+found directory..\n");
    fileSystem->CreateDir("lost+found"); // Create a directory in the root dir

    printf("Creating usr directory..\n");
    fileSystem->CreateDir("usr"); // Create a directory in the root dir

    printf("Creating etc directory..\n");
    fileSystem->CreateDir("etc"); // Create a directory in the root dir

    printf("Creating home directory..\n");
    fileSystem->CreateDir("home"); // Create a directory in the root dir

    printf("Changing current directory to home directory..\n");
    fileSystem->ChangeDir("/home"); // Changing directories

    printf("Creating a bunch of user's home...\n");
    /// Create a lot of directories in the current directory
    fileSystem->CreateDir("user1"); // Create a directory in the home dir
    fileSystem->CreateDir("user2"); // Create a directory in the home dir
    fileSystem->CreateDir("user3"); // Create a directory in the home dir
    fileSystem->CreateDir("user4"); // Create a directory in the home dir
    fileSystem->CreateDir("user5"); // Create a directory in the home dir
    fileSystem->CreateDir("user6"); // Create a directory in the home dir
    fileSystem->CreateDir("user7"); // Create a directory in the home dir
    fileSystem->CreateDir("user8"); // Create a directory in the home dir
    fileSystem->CreateDir("user9"); // Create a directory in the home dir
    fileSystem->CreateDir("user10"); // Create a directory in the home dir
    fileSystem->CreateDir("user11"); // Create a directory in the home dir
    fileSystem->CreateDir("user12"); // Create a directory in the home dir
}
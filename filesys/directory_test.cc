#include "file_system.hh"
#include "lib/utility.hh"
#include "machine/disk.hh"
#include "machine/statistics.hh"
#include "threads/thread.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>

void DirectoryTest() {
    fileSystem->PrintDir(); // List the dir entries in the current directory
    printf("Creating tmp directory..\n");
    fileSystem->CreateDir("tmp"); // Create a directory in the root dir
    printf("Creating home directory..\n");
    fileSystem->CreateDir("home"); // Create a directory in the root dir
    fileSystem->PrintDir(); // List the dir entries in the current directory
    printf("Changing current directory to tmp directory..\n");
    fileSystem->ChangeDir("home"); // Changing directories
    printf("Creating aldu directory in home folder..\n");
    fileSystem->CreateDir("aldu"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario1"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario2"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario3"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario4"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario5"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario6"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario7"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario8"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario9"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario10"); // Create a directory in the home dir
    fileSystem->CreateDir("usuario11"); // Create a directory in the home dir
    printf("Creating pab directory in home folder..\n");
    fileSystem->CreateDir("pab"); // Create a directory in the home dir
    fileSystem->PrintDir(); // List the dir entries in the current directory
}
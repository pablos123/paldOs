/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// “open” continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "threads/system.hh"
#include "threads/channel.hh"
#include "file_system.hh"
#include "directory.hh"
#include "file_header.hh"
#include "lib/bitmap.hh"

#include <stdio.h>
#include <string.h>


/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        Bitmap     *freeMap = new Bitmap(NUM_SECTORS);
        Directory  *dir     = new Directory(NUM_DIR_ENTRIES);
        FileHeader *mapH    = new FileHeader;
        FileHeader *dirH    = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapH->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        mapH->GetRaw()->nextFileHeader = 0; // FAT convention for indicating the end of the current N blocks of a file
        ASSERT(dirH->Allocate(freeMap, DIRECTORY_FILE_SIZE));
        dirH->GetRaw()->nextFileHeader = 0;

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapH->WriteBack(FREE_MAP_SECTOR);
        dirH->WriteBack(DIRECTORY_SECTOR);

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);

        // Once we have the files “open”, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile);     // flush changes to disk
        dir->WriteBack(directoryFile, true);

        if (debug.IsEnabled('f')) {
            freeMap->Print();
            dir->Print();

            delete freeMap;
            delete dir;
            delete mapH;
            delete dirH;
        }
    } else {
        // If we are not formatting the disk, just open the files
        // representing the bitmap and directory; these are left open while
        // Nachos is running.
        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);
        // Get the current size of the directory table, this is necessry since we
        // support 'unlimited' directory headers in the directory
        Directory  *dir = new Directory(2000); // we dont want to create a new Directory
        unsigned result = dir->FetchFrom(directoryFile, true);
        directorySize = result;
        delete dir;
    }
}

FileSystem::~FileSystem()
{
    delete freeMapFile;
    delete directoryFile;
}

/// Debug function for the raw table
void
PrintTable(RawDirectory raw) {
    DEBUG('w', "DEBUG TABLEE\n\n", raw.tableSize);
    DEBUG('w', "table size: %u\n", raw.tableSize);
    for (unsigned i = 100; i < raw.tableSize; i++)
        DEBUG('w', "Index: %u, InUse? %d Name: %s\n",i,raw.table[i].inUse,raw.table[i].name);
    DEBUG('w', "\n\n");
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// `Create` the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created. <-- Deprecated
/// The created file always has initial size equal to 0
bool
FileSystem::Create(const char *name, unsigned dummyParam, bool isBin) // Dummy param to maintain the same interface with the other 'more simple' create provided
{
    ASSERT(name != nullptr);

    DEBUG('f', "Creating file %s, size 0\n", name);

    if(isBin){
        int fileDescriptor = SystemDep::OpenForWrite(name);
            if (fileDescriptor == -1) {
                return false;
            }
            SystemDep::Close(fileDescriptor);
            return true;
    }

    bool success = true;
    filesysCreateLock->Acquire();

    //abrimos el file del current directory
    // dir->FetchFrom(currentdirectoryfile);
    Directory *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);

    if (dir->Find(name) != -1) {
        DEBUG('f',"File is already in directory\n");
        success = false;  // File is already in directory.
    } else {
        // Creates a copy of the freeMapFile
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);

        int sector = freeMap->Find();
        if (sector == -1) {
            DEBUG('f', "There is not enough disk space for the file's FH\n");
            success = false;
        }

        if(!dir->Add(name, sector)) {
            success = false;
        } else if(success) {

            FileHeader *firstHeader = new FileHeader;
            firstHeader->GetRaw()->nextFileHeader = 0;
            firstHeader->GetRaw()->numBytes = 0;
            firstHeader->GetRaw()->numSectors = 0;

            DEBUG('w', "First FH initialized. FH first sector: %u, Num sectors: %u\n", sector, firstHeader->GetRaw()->numSectors);

            firstHeader->WriteBack(sector);
            freeMap->WriteBack(freeMapFile);
            dir->WriteBack(directoryFile);

            // To make sure that removed is false because in the same execution of nachos
            // we can have this situation:
            // 1: nachos creates a file in sector x
            // 2: we delete this file, so sector x have removed = true
            // 3: nachos creates again a file in sector x
            openFilesTable[sector]->removed = false;
            openFilesTable[sector]->removing = false;
            openFilesTable[sector]->removeLock = new Lock("Remove Lock");
            openFilesTable[sector]->writeLock = nullptr;
            openFilesTable[sector]->closeLock = nullptr;
            openFilesTable[sector]->count = 0;

            DEBUG('f',"File created successfully!\n");
            delete firstHeader;
        }
        delete freeMap;
    }
    filesysCreateLock->Release();
    delete dir;
    return success;
}


bool
FileSystem::CreateDir(const char *name)
{
    ASSERT(name != nullptr);

    DEBUG('f', "Creating directory %s\n", name);

    bool success = true;
    filesysCreateLock->Acquire();

    // Make a temporal copy of the current directory
    Directory *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);

    if (dir->Find(name) != -1) {
        DEBUG('f',"File is already in directory\n");
        success = false;  // File is already in directory.
    } else {
        // Creates a copy of the freeMapFile
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);

        int sector = freeMap->Find();
        if (sector == -1) {
            DEBUG('f', "There is not enough disk space for the file's FH\n");
            success = false;
        }

        if(!dir->Add(name, sector, true)) {
            success = false;
        } else if(success) {

            FileHeader *dirHeader = new FileHeader;
            dirHeader->GetRaw()->nextFileHeader = 0;
            dirHeader->GetRaw()->numBytes = 0;
            dirHeader->GetRaw()->numSectors = 0;

            success = dirHeader->Allocate(freeMap, DIRECTORY_FILE_SIZE);

            if(!success){
                DEBUG('f', "There is not enough space for a new directory\n");
                delete freeMap;
                delete dirHeader;
                delete dir;
                return success;
            }

            DEBUG('w', "First FH initialized. FH first sector: %u, Num sectors: %u\n", sector, dirHeader->GetRaw()->numSectors);

            dirHeader->WriteBack(sector);
            freeMap->WriteBack(freeMapFile);

            // Create the new directory
            Directory  *newDir = new Directory(NUM_DIR_ENTRIES);
            OpenFile* newDirectoryFile = new OpenFile(sector);
            // Flush to the disk the table estructure
            newDir->WriteBack(newDirectoryFile, true);

            //update the current directory
            dir->WriteBack(directoryFile);

            // To make sure that removed is false because in the same execution of nachos
            // we can have this situation:
            // 1: nachos creates a file in sector x
            // 2: we delete this file, so sector x have removed = true
            // 3: nachos creates again a file in sector x
            openFilesTable[sector]->removed = false;
            openFilesTable[sector]->removing = false;
            openFilesTable[sector]->removeLock = new Lock("Remove Lock");
            openFilesTable[sector]->writeLock = nullptr;
            openFilesTable[sector]->closeLock = nullptr;
            openFilesTable[sector]->count = 0;

            DEBUG('f',"File created successfully!\n");
            delete dirHeader;
        }
        delete freeMap;
    }
    filesysCreateLock->Release();
    delete dir;
    return success;
}

bool
FileSystem::ChangeDir(const char* name) {

    ASSERT(name != nullptr);

    DEBUG('f', "Changing current directory to: %s\n", name);

    bool success = true;

    DEBUG('9', "the directory size before changing is: %u\n", directorySize);
    // Make a temporal copy of the current directory
    Directory *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);

    if (dir->FindDir(name)) {
        DEBUG('f',"Dir %s found!\n", name);

        OpenFile* currentDirectoryFile = Open(name);
        OpenFile* openFileToDelete = directoryFile;

        directoryFile = currentDirectoryFile;

        delete openFileToDelete;

        unsigned result = dir->FetchFrom(directoryFile);
        DEBUG('9', "the result of the read after changing is: %u\n", result);
        directorySize = unsigned(result/ sizeof (DirectoryEntry));
        DEBUG('9', "the directory size after changing is: %u\n", directorySize);
        /// cambiar el directorySize
    } else {
        DEBUG('f',"The directory does not exists...\n");
        success = false;  // File is already in directory.
    }

    return success;
}

void
FileSystem::PrintDir() {
    DEBUG('f', "Printing directory...\n");
    Directory  *dir     = new Directory(directorySize);
    dir->FetchFrom(directoryFile);
    dir->Print();
}

unsigned
FileSystem::Ls(char* into) {
    Directory  *dir     = new Directory(directorySize);
    dir->FetchFrom(directoryFile);
    unsigned bytesRead = dir->PrintNames(into);
    return bytesRead;
}

OpenFile*
FileSystem::GetFreeMap() {
    return freeMapFile;
};

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *
FileSystem::Open(const char *name, bool isBin)
{
    ASSERT(name != nullptr);

    if(isBin) {
        int fileDescriptor = SystemDep::OpenForReadWrite(name, false);
        if (fileDescriptor == -1) {
            return nullptr;
        }
        return new OpenFile(fileDescriptor, isBin);
    }

    Directory *dir = new Directory(directorySize);
    OpenFile  *openFile = nullptr;

    DEBUG('0', "Opening file %s\n", name);
    dir->FetchFrom(directoryFile);
    int sector = dir->Find(name);

    DEBUG('w', "the sector found is: %d!!\n", sector);
    if (sector >= 0 && !openFilesTable[sector]->removing) { // `name` was found in directory.
        openFile = new OpenFile(sector);
    }

    delete dir;
    return openFile;  // Return null if not found.
}

unsigned
FileSystem::GetDirectorySize() {
    return directorySize;
}

void
FileSystem::SetDirectorySize(unsigned newDirectorySize) {
    directorySize = newDirectorySize;
}


/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool
FileSystem::Remove(const char *name)
{
    ASSERT(name != nullptr);

    Directory *dir = new Directory(directorySize);
    dir->FetchFrom(directoryFile);
    int sector = dir->Find(name);   // first sector

    if (sector == -1) {
       delete dir;
       DEBUG('f',"File to remove not found\n");
       return false;  // file not found
    }
    // For supporting ad-hoc calls to this function
    if(openFilesTable[sector]->removeLock != nullptr)
        openFilesTable[sector]->removeLock->Acquire();

    if(openFilesTable[sector]->removed) {
       delete dir;
       return false;  // file already removed
    }

    FileHeader *fileH = new FileHeader;
    fileH->FetchFrom(sector);

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);

    // Set the file in "removing" state
    openFilesTable[sector]->removing = true;
    openFilesTable[sector]->removerSpaceId = currentThread->GetSpaceId();
    // Wait for the other threads with the file open to finish their work
    if (openFilesTable[sector]->count > 0) {
        DEBUG('f', "Going to receive a message with remove.\n");
        int* msg = new int;
        currentThread->GetRemoveChannel()->Receive(msg);
        DEBUG('f', "Received %d\n", *msg);
        ASSERT(!*msg);
        delete msg;
    }

    fileH->Deallocate(freeMap);  // Remove data blocks.
    freeMap->Clear(sector);
    unsigned nextSector  = fileH->GetRaw()->nextFileHeader;
    while(nextSector) {
        fileH->FetchFrom(nextSector);
        fileH->Deallocate(freeMap);  // Remove data blocks.
        freeMap->Clear(nextSector);      // Remove header block.
        nextSector  = fileH->GetRaw()->nextFileHeader;
    }

    dir->Remove(name);

    freeMap->WriteBack(freeMapFile);  // Flush to disk.
    dir->WriteBack(directoryFile);    // Flush to disk.

    openFilesTable[sector]->removed = true; // For supporting multi threading in this subrutine
    // For supporting ad-hoc calls to this function
    if(openFilesTable[sector]->removeLock != nullptr)
        openFilesTable[sector]->removeLock->Release();

    delete fileH;
    delete dir;
    delete freeMap;

    DEBUG('f', "Setting removing to false.\n");
    openFilesTable[sector]->removing = false;
    DEBUG('f', "File removed successfully!\n");
    return true;
}

/// List all the files in the file system directory.
void
FileSystem::List()
{
    Directory *dir = new Directory(directorySize);

    dir->FetchFrom(directoryFile);
    dir->List();
    delete dir;
}

static bool
AddToShadowBitmap(unsigned sector, Bitmap *map)
{
    ASSERT(map != nullptr);

    if (map->Test(sector)) {
        DEBUG('f', "Sector %u was already marked.\n", sector);
        return false;
    }
    map->Mark(sector);
    DEBUG('f', "Marked sector %u.\n", sector);
    return true;
}

static bool
CheckForError(bool value, const char *message)
{
    if (!value) {
        DEBUG('f', "Error: %s\n", message);
    }
    return !value;
}

static bool
CheckSector(unsigned sector, Bitmap *shadowMap)
{
    if (CheckForError(sector < NUM_SECTORS,
                      "sector number too big.  Skipping bitmap check.")) {
        return true;
    }
    return CheckForError(AddToShadowBitmap(sector, shadowMap),
                         "sector number already used.");
}

static bool
CheckFileHeader(const RawFileHeader *rh, unsigned num, Bitmap *shadowMap)
{
    ASSERT(rh != nullptr);

    bool error = false;

    DEBUG('f', "Checking file header %u.  File size: %u bytes, number of sectors: %u.\n",
          num, rh->numBytes, rh->numSectors);
    error |= CheckForError(rh->numSectors >= DivRoundUp(rh->numBytes,
                                                        SECTOR_SIZE),
                           "sector count not compatible with file size.");
    error |= CheckForError(rh->numSectors < NUM_DIRECT,
                           "too many blocks.");
    for (unsigned i = 0; i < rh->numSectors; i++) {
        unsigned s = rh->dataSectors[i];
        error |= CheckSector(s, shadowMap);
    }
    return error;
}

static bool
CheckBitmaps(const Bitmap *freeMap, const Bitmap *shadowMap)
{
    bool error = false;
    for (unsigned i = 0; i < NUM_SECTORS; i++) {
        DEBUG('f', "Checking sector %u. Original: %u, shadow: %u.\n",
              i, freeMap->Test(i), shadowMap->Test(i));
        error |= CheckForError(freeMap->Test(i) == shadowMap->Test(i),
                               "inconsistent bitmap.");
    }
    return error;
}

static bool
CheckDirectory(const RawDirectory *rd, Bitmap *shadowMap)
{
    ASSERT(rd != nullptr);
    ASSERT(shadowMap != nullptr);

    bool error = false;
    unsigned nameCount = 0;
    const char *knownNames[NUM_DIR_ENTRIES];

    for (unsigned i = 0; i < NUM_DIR_ENTRIES; i++) {
        DEBUG('f', "Checking direntry: %u.\n", i);
        const DirectoryEntry *e = &rd->table[i];

        if (e->inUse) {
            if (strlen(e->name) > FILE_NAME_MAX_LEN) {
                DEBUG('f', "Filename too long.\n");
                error = true;
            }

            // Check for repeated filenames.
            DEBUG('f', "Checking for repeated names.  Name count: %u.\n",
                  nameCount);
            bool repeated = false;
            for (unsigned j = 0; j < nameCount; j++) {
                DEBUG('f', "Comparing \"%s\" and \"%s\".\n",
                      knownNames[j], e->name);
                if (strcmp(knownNames[j], e->name) == 0) {
                    DEBUG('f', "Repeated filename.\n");
                    repeated = true;
                    error = true;
                }
            }
            if (!repeated) {
                knownNames[nameCount] = e->name;
                DEBUG('f', "Added \"%s\" at %u.\n", e->name, nameCount);
                nameCount++;
            }

            // Check sector.
            error |= CheckSector(e->sector, shadowMap);

            // Check file header.
            FileHeader *h = new FileHeader;
            const RawFileHeader *rh = h->GetRaw();
            h->FetchFrom(e->sector);
            error |= CheckFileHeader(rh, e->sector, shadowMap);
            delete h;
        }
    }
    return error;
}

bool
FileSystem::Check()
{
    DEBUG('f', "Performing filesystem check\n");
    bool error = false;

    Bitmap *shadowMap = new Bitmap(NUM_SECTORS);
    shadowMap->Mark(FREE_MAP_SECTOR);
    shadowMap->Mark(DIRECTORY_SECTOR);

    DEBUG('f', "Checking bitmap's file header.\n");

    FileHeader *bitH = new FileHeader;
    const RawFileHeader *bitRH = bitH->GetRaw();
    bitH->FetchFrom(FREE_MAP_SECTOR);
    DEBUG('f', "  File size: %u bytes, expected %u bytes.\n"
               "  Number of sectors: %u, expected %u.\n",
          bitRH->numBytes, FREE_MAP_FILE_SIZE,
          bitRH->numSectors, FREE_MAP_FILE_SIZE / SECTOR_SIZE);
    error |= CheckForError(bitRH->numBytes == FREE_MAP_FILE_SIZE,
                           "bad bitmap header: wrong file size.");
    error |= CheckForError(bitRH->numSectors == FREE_MAP_FILE_SIZE / SECTOR_SIZE,
                           "bad bitmap header: wrong number of sectors.");
    error |= CheckFileHeader(bitRH, FREE_MAP_SECTOR, shadowMap);
    delete bitH;

    DEBUG('f', "Checking directory.\n");

    FileHeader *dirH = new FileHeader;
    const RawFileHeader *dirRH = dirH->GetRaw();
    dirH->FetchFrom(DIRECTORY_SECTOR);
    error |= CheckFileHeader(dirRH, DIRECTORY_SECTOR, shadowMap);
    delete dirH;

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);
    Directory *dir = new Directory(directorySize);
    const RawDirectory *rdir = dir->GetRaw();
    dir->FetchFrom(directoryFile);
    error |= CheckDirectory(rdir, shadowMap);
    delete dir;

    // The two bitmaps should match.
    DEBUG('f', "Checking bitmap consistency.\n");
    error |= CheckBitmaps(freeMap, shadowMap);
    delete shadowMap;
    delete freeMap;

    DEBUG('f', error ? "Filesystem check failed.\n"
                     : "Filesystem check succeeded.\n");

    return !error;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void
FileSystem::Print()
{
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);

    printf("The total amount of sectors in the disk is: %u\n", NUM_SECTORS);
    unsigned freeSectors = freeMap->CountClear();
    printf("The number of occuppied sectors in the disk are: %u\n", NUM_SECTORS - freeSectors);
    printf("The number of free sectors in the disk is: %u\n", freeSectors);
    printf("The maximum file size is: %u\n", MAX_FILE_SIZE);
    printf("The number of direct blocks per file header is: %u\n", NUM_DIRECT);
    printf("The number of blocks to fill with file data is: %u\n", FILE_DATA_SECTORS);

    FileHeader *bitH    = new FileHeader;
    FileHeader *dirH    = new FileHeader;
    Directory  *dir     = new Directory(directorySize); // we dont want to create a new Directory
    dir->FetchFrom(directoryFile);

    printf("--------------------------------\n");
    bitH->FetchFrom(FREE_MAP_SECTOR);
    bitH->Print("Bitmap");

    printf("--------------------------------\n");
    dirH->FetchFrom(DIRECTORY_SECTOR);
    dirH->Print("Directory");

    printf("--------------------------------\n");
    freeMap->Print();

    printf("--------------------------------\n");
    dir->Print();
    printf("--------------------------------\n");

    delete bitH;
    delete dirH;
    delete freeMap;
    delete dir;
}

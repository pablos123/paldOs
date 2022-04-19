/// Routines to manage a directory of file names.
///
/// The directory is a table of fixed length entries; each entry represents a
/// single file, and contains the file name, and the location of the file
/// header on disk.  The fixed size of each directory entry means that we
/// have the restriction of a fixed maximum size for file names.
///
/// The constructor initializes an empty directory of a certain size; we use
/// ReadFrom/WriteBack to fetch the contents of the directory from disk, and
/// to write back any modifications back to disk.
///
/// Also, this implementation has the restriction that the size of the
/// directory cannot expand.  In other words, once all the entries in the
/// directory are used, no more files can be created.  Fixing this is one of
/// the parts to the assignment.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "directory.hh"
#include "directory_entry.hh"
#include "file_header.hh"
#include "lib/utility.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>


/// Initialize a directory; initially, the directory is completely empty.  If
/// the disk is being formatted, an empty directory is all we need, but
/// otherwise, we need to call FetchFrom in order to initialize it from disk.
///
/// * `size` is the number of entries in the directory.
Directory::Directory(unsigned size, bool isNew)
{
    ASSERT(size > 0);
    raw.table = new DirectoryEntry [size];

    raw.tableSize = size;
    for (unsigned i = 0; i < raw.tableSize; i++) {
        raw.table[i].inUse = false;
    }
}

/// De-allocate directory data structure.
Directory::~Directory()
{
    delete [] raw.table;
}

/// Read the contents of the directory from disk.
///
/// * `file` is file containing the directory contents.
unsigned
Directory::FetchFrom(OpenFile *file, bool needTableSize)
{
    ASSERT(file != nullptr);
    DEBUG('0', "getting data fom disk...\n");
    DEBUG('0', "raw table memory is: %p\n", raw.table);
    DEBUG('0', "table size is: %d\n", raw.tableSize);
    int result = file->Read((char *) raw.table,
                raw.tableSize * sizeof (DirectoryEntry), true);

    if(needTableSize) {
        DEBUG('0', "table size before return is: %d", result);
        raw.tableSize = unsigned((unsigned) result/ sizeof (DirectoryEntry));
        DEBUG('0', "quantity of directoy entries is: %u\n", raw.tableSize);
        return raw.tableSize;
    }

    DEBUG('0', "bytes readed: %d\n", result);
    return (unsigned)result;
}

/// Write any modifications to the directory back to disk.
///
/// * `file` is a file to contain the new directory contents.
void
Directory::WriteBack(OpenFile *file, bool firstTime)
{
    ASSERT(file != nullptr);
    if(firstTime)
        file->WriteAt((char *) raw.table,
                    raw.tableSize * sizeof (DirectoryEntry), 0);
    else {
        DEBUG('0', "writing back to disk...\n");
        unsigned bytesToWrite = raw.tableSize * sizeof (DirectoryEntry);
        DEBUG('0',"Target bytes to write: %u\n", bytesToWrite);
        int result = file->Write((char *) raw.table, bytesToWrite, true);
        DEBUG('0', "bytes writed: %d\n", result);
        ASSERT(result);
    }
}

/// Look up file name in directory, and return its location in the table of
/// directory entries.  Return -1 if the name is not in the directory.
///
/// * `name` is the file name to look up.
int
Directory::FindIndex(const char *name)
{
    ASSERT(name != nullptr);

    for (unsigned i = 0; i < raw.tableSize; i++) {
        DEBUG('0', "Entry in use: %d, entryFileName: %s\n", raw.table[i].inUse, raw.table[i].name);
        if (raw.table[i].inUse
              && !strncmp(raw.table[i].name, name, FILE_NAME_MAX_LEN)) {
            return i;
        }
    }
    DEBUG('0', "Name %s not found in directory\n", name);
    return -1;  // name not in directory
}

/// Look up file name in directory, and return the disk sector number where
/// the file's header is stored.  Return -1 if the name is not in the
/// directory.
///
/// * `name` is the file name to look up.
int
Directory::Find(const char *name)
{
    ASSERT(name != nullptr);
    DEBUG('0', "Finding file in directory...\n");
    DEBUG('0', "Current table size of the directory: %u\n", raw.tableSize);
    int i = FindIndex(name);
    if (i != -1) {
        DEBUG('0', "File found! The index found is: %d and the associated sector\n", i,raw.table[i].sector);
        return raw.table[i].sector;
    }
    return -1;
}

/// Add a file into the directory.  Return true if successful; return false
/// if the file name is already in the directory, or if the directory is
/// completely full, and has no more space for additional file names.
///
/// * `name` is the name of the file being added.
/// * `newSector` is the disk sector containing the added file's header.
bool
Directory::Add(const char *name, int newSector)
{
    ASSERT(name != nullptr);

    if (FindIndex(name) != -1) {
        return false;
    }

    DEBUG('0', "adding %s\n", name);
    unsigned i = 0;
    for (; i < raw.tableSize; i++) {
        if (!raw.table[i].inUse) {
            raw.table[i].inUse = true;
            strncpy(raw.table[i].name, name, FILE_NAME_MAX_LEN);
            raw.table[i].sector = newSector;
            DEBUG('0', "%s added!\n", raw.table[i].name);
            return true;
        }
    }

    DEBUG('0', "bad size, resizing...\n");
    unsigned newTableSize = raw.tableSize + 1;

    DirectoryEntry * temp = raw.table;

    DirectoryEntry* newTable = new DirectoryEntry[newTableSize];
    memcpy(newTable, raw.table, raw.tableSize * sizeof(DirectoryEntry));

    raw.table =  newTable;
    raw.tableSize = newTableSize;
    DEBUG('0', "old: %p new: %p\n", temp, raw.table);
    delete [] temp;

    for (unsigned j = i; j < raw.tableSize; j++) {
        raw.table[j].inUse = false;
    }
    // Now we can mark the deseared entry to create
    raw.table[i].inUse = true;
    strncpy(raw.table[i].name, name, FILE_NAME_MAX_LEN);
    raw.table[i].sector = newSector;
    DEBUG('0',"Final table size: %u\n", raw.tableSize);

    DEBUG('0',"name in newTable: %s\n", newTable[0].name);


    fileSystem->SetDirectorySize(newTableSize);

    return true;
}

/// Remove a file name from the directory.   Return true if successful;
/// return false if the file is not in the directory.
///
/// * `name` is the file name to be removed.
bool
Directory::Remove(const char *name)
{
    ASSERT(name != nullptr);

    int i = FindIndex(name);
    if (i == -1) {
        return false;  // name not in directory
    }
    raw.table[i].inUse = false;
    return true;
}

/// List all the file names in the directory.
void
Directory::List() const
{
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            printf("%s\n", raw.table[i].name);
        }
    }
}

/// List all the file names in the directory, their `FileHeader` locations,
/// and the contents of each file.  For debugging.
void
Directory::Print() const
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    DEBUG('0', "printing with table size: %u\n", raw.tableSize);
    for (unsigned i = 0; i < raw.tableSize; i++) {  // !!!!!!!!!
        if (raw.table[i].inUse) {
            printf("\nDirectory entry:\n"
                   "    name: %s\n"
                   "    sector: %u\n",
                   raw.table[i].name, raw.table[i].sector);

            hdr->FetchFrom(raw.table[i].sector);
            hdr->Print(nullptr);

            unsigned nextSector = hdr->GetRaw()->nextFileHeader;
            while(nextSector) {
                hdr->FetchFrom(nextSector);
                hdr->Print(nullptr);
                nextSector = hdr->GetRaw()->nextFileHeader;
            }
        }
    }
    printf("\n");
    delete hdr;
}

const RawDirectory *
Directory::GetRaw() const
{
    return &raw;
}

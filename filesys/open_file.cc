/// Routines to manage an open Nachos file.  As in UNIX, a file must be open
/// before we can read or write to it.  Once we are all done, we can close it
/// (in Nachos, by deleting the `OpenFile` data structure).
///
/// Also as in UNIX, for convenience, we keep the file header in memory while
/// the file is open.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "open_file.hh"
#include "file_header.hh"
#include "threads/system.hh"
#include "threads/channel.hh"
#include <stdio.h>
#include <string.h>

/// Open a Nachos file for reading and writing.  Bring the file header into
/// memory while the file is open.
///
/// * `sector` is the location on disk of the file header for this file.
OpenFile::OpenFile(int sectorParam, bool isBinParam)
{
    isBin = isBinParam;
    if(isBin){
        file = sectorParam; // when called this int will be the fileDescriptor
        currentOffset = 0;
    } else {
        hdr = new FileHeader;
        hdr->FetchFrom(sectorParam);
        seekPosition = 0;
        sector = sectorParam;
        currentSector = sectorParam;
        if(openFilesTable[sector]->count == 0) {    // no one else has the file open
            Lock* writeLock = new Lock("Write Lock");
            openFilesTable[sector]->writeLock = writeLock;

            if( openFilesTable[sector]->removeLock == nullptr ) { // We do not have a removelock yet
                Lock* removeLock = new Lock("Remove Lock");
                openFilesTable[sector]->removeLock = removeLock;
            }
            if( openFilesTable[sector]->closeLock == nullptr ) { // We do not have a removelock yet
                Lock* closeLock = new Lock("Close Lock");
                openFilesTable[sector]->closeLock = closeLock;
            }
        }
        openFilesTable[sector]->count++; // add one user to the count of the threads that have the file open
    }
}

/// Close a Nachos file, de-allocating any in-memory data structures.
OpenFile::~OpenFile()
{
    if(isBin) {
        SystemDep::Close(file);
        return;
    }

    DEBUG('f', "Removing open file\n");
    // Decrease the counter meaning one less open file for this sector
    openFilesTable[sector]->closeLock->Acquire();
    if(openFilesTable[sector]->count > 0)
        openFilesTable[sector]->count--;
    else { // To support more close calls than open calls
        openFilesTable[sector]->closeLock->Release();
        return;
    }
    openFilesTable[sector]->closeLock->Release();

    // If this is the last open file of this sector free the lock
    DEBUG('f', "The count for the file is: %d\n", openFilesTable[sector]->count);
    if(openFilesTable[sector]->count == 0 ) {
        if(openFilesTable[sector]->removing) {
            SpaceId removerSpaceId = openFilesTable[sector]->removerSpaceId;
            if(runningProcesses->HasKey(removerSpaceId)){
                DEBUG('f', "About to send msg to remover...\n");
                runningProcesses->Get(removerSpaceId)->GetRemoveChannel()->Send(0);
            } // despierto al hilo que esta esperando que los hilos cierren el archivo que quiere borrar
        }

        delete openFilesTable[sector]->writeLock;
    }
    delete hdr;
}

// More meaningfull name for the deconsructor
void
OpenFile::Close()
{
  this->~OpenFile();
}

/// Change the current location within the open file -- the point at which
/// the next `Read` or `Write` will start from.
///
/// * `position` is the location within the file for the next `Read`/`Write`.
void
OpenFile::Seek(unsigned position)
{
    seekPosition = position;
}

/// OpenFile::Read/Write
///
/// Read/write a portion of a file, starting from `seekPosition`.  Return the
/// number of bytes actually written or read, and as a side effect, increment
/// the current position within the file.
///
/// Implemented using the more primitive `ReadAt`/`WriteAt`.
///
/// * `into` is the buffer to contain the data to be read from disk.
/// * `from` is the buffer containing the data to be written to disk.
/// * `numBytes` is the number of bytes to transfer.

int
OpenFile::Read(char *into, unsigned numBytes, bool isDirectory)
{
    ASSERT(into != nullptr);
    ASSERT(numBytes > 0);

    if(isBin){
        int numRead = ReadAt(into, numBytes, currentOffset);
        currentOffset += numRead;
        return numRead;
    }

    if(isDirectory) {
        hdr->FetchFrom(sector);
        seekPosition = 0;
    }

    int result = 0;

    if (seekPosition + numBytes > hdr->GetRaw()->numBytes) {
        DEBUG('w', "seekPostion is: %u, numBytes is: %u, maximuxbytes is:%u\n",seekPosition,numBytes,hdr->GetRaw()->numBytes);
        result = ReadAt(into, numBytes, seekPosition);
        seekPosition += result;
        numBytes -= result;
        char* temp = into + result;
        DEBUG('w', "bytes readed: %u\n", result);
        unsigned nextSector = hdr->GetRaw()->nextFileHeader;
        DEBUG('w', "READING the next file header sector is: %u\n", nextSector);
        while(nextSector && numBytes > 0) {
            seekPosition = 0;
            hdr->FetchFrom(nextSector);
            unsigned result_tmp = ReadAt(temp, numBytes, seekPosition);
            temp+= result_tmp;
            numBytes -= result_tmp;
            result += result_tmp;
            DEBUG('w', "bytes readed: %u\n", result);
            seekPosition += result_tmp;

            nextSector = hdr->GetRaw()->nextFileHeader;
            DEBUG('w', "READING the next file header sector is: %u\n", nextSector);
        }
    } else {
        DEBUG('w', "going to read with read at!\n");
        result = ReadAt(into, numBytes, seekPosition);
        DEBUG('w', "bytes readed: %u\n", result);

        seekPosition += result;
    }

    return result;
}

int
OpenFile::Write(const char *from, unsigned numBytes, bool isDirectory)
{
    DEBUG('w',"Attempting to write: %u bytes\n", numBytes);

    ASSERT(from != nullptr);
    ASSERT(numBytes > 0);

    if(isBin) {
        int numWritten = WriteAt(from, numBytes, currentOffset);
        currentOffset += numWritten;
        return numWritten;
    }

    openFilesTable[sector]->writeLock->Acquire();

    if(isDirectory) {
        seekPosition = 0;
        currentSector = sector;
    }

    // To support concurrency, because the hdr maybe changed in other thread's execution
    DEBUG('w', "the current sector is: %u\n", currentSector);
    hdr->FetchFrom(currentSector);

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(fileSystem->GetFreeMap());

    int result  = 0;

    // We limit the amount to write in the current FH.
    unsigned maxBytesToWrite = NUM_DIRECT * SECTOR_SIZE - seekPosition;
    // This is for using the space left in the current FH
    unsigned bytesToWrite =  numBytes > maxBytesToWrite ? maxBytesToWrite : numBytes;

    bool success = true;
    unsigned result_tmp = 0;

    DEBUG('w', "The bytes to write are %u\n", bytesToWrite);

    // If seekPosition is equal to NUM_DIRECT * SECTOR_SIZE so, bytesToWrite will be 0
    if(bytesToWrite > 0) {  // We can write in the current file header

        DEBUG('w', "writing in the same file header\n");

        if(seekPosition + bytesToWrite > hdr->GetRaw()->numBytes) {
            unsigned bytesToAllocate = (seekPosition + bytesToWrite) - hdr->GetRaw()->numBytes;
            DEBUG('w',"before allocate hdr memory: %p\n", hdr->GetRaw());
            DEBUG('w',"before allocate: numBytes: %u, numSectors: %u\n", hdr->GetRaw()->numBytes,hdr->GetRaw()->numSectors);
            for(unsigned i = 0; i < hdr->GetRaw()->numSectors; i++){
                DEBUG('w',"%u\n", hdr->GetRaw()->dataSectors[i]);
            }
            success = hdr->Allocate(freeMap, bytesToAllocate);
            if(!success) {
                delete freeMap;
                openFilesTable[sector]->writeLock->Release();
                return result;
            }
            DEBUG('w', "Writing file header and bitmap back to disk...\n");
            freeMap->WriteBack(fileSystem->GetFreeMap());
            // We need to write back the hdr when there is a change in the raw file header
            DEBUG('w',"The currentSector of the current hdr is: %u\n", currentSector);
            hdr->WriteBack(currentSector);
            DEBUG('w', "The struct raw %p has: numbytes %u, numsectors %u\n", hdr->GetRaw(), hdr->GetRaw()->numBytes, hdr->GetRaw()->numSectors);
        }

        DEBUG('w', "going to write in seek position %d\n", seekPosition);
        result_tmp = WriteAt(from, bytesToWrite, seekPosition);

        ////////////////// Updating data //////////////////////
        result += result_tmp;

        DEBUG('w', "the bytes writed are: %u\n", result_tmp);
        from += result_tmp; //Move the array of char pointers result_tmp times

        seekPosition += result_tmp;
        numBytes -= result_tmp;

        DEBUG('w', "The bytes left to write are: %u\n", numBytes);
    }


    // Travel the list of fileheaders if necessary and it exists
    // if it exists that means that another thread created the fileheader
    // previously
    unsigned iterFh = hdr->GetRaw()->nextFileHeader;
    DEBUG('w', "the next file header for the condition is: %u\n", iterFh);
    while(numBytes > 0 && iterFh != 0) {
        hdr->FetchFrom(iterFh);

        DEBUG('w', "the numBytes of the hdr is: %u\n", hdr->GetRaw()->numBytes);

        // We limit the amount to write in the current FH.
        maxBytesToWrite = NUM_DIRECT * SECTOR_SIZE;
        // This is for using the space left in the current FH
        bytesToWrite =  numBytes > maxBytesToWrite ? maxBytesToWrite : numBytes;

        if(bytesToWrite > hdr->GetRaw()->numBytes) {
            DEBUG('w', "Requiring memory in existing fh\n");
            unsigned bytesToAllocate =  bytesToWrite - hdr->GetRaw()->numBytes;
            success = hdr->Allocate(freeMap, bytesToAllocate);
            if(!success) {
                delete freeMap;
                openFilesTable[sector]->writeLock->Release();
                return result;
            }
            DEBUG('w',"Flushing allocated disk memory...\n");
            // We need to write back the hdr when there is a change in the raw file header
            freeMap->WriteBack(fileSystem->GetFreeMap());
            hdr->WriteBack(iterFh);
        }

        //update the current sector
        currentSector = iterFh;

        seekPosition = 0;
        result_tmp = WriteAt(from, bytesToWrite, seekPosition);
        result += result_tmp;
        from += result_tmp; //Move the array of char pointers reslt_tmp times

        seekPosition += result_tmp;
        numBytes -= result_tmp;  // This will be always zero at some point

        iterFh = hdr->GetRaw()->nextFileHeader;
        DEBUG('w',"iter fh is %u, number of bytes %u\n", iterFh, numBytes);
    }


    if (numBytes > 0) {
        DEBUG('w',"a new file header is needed!!!!!!!\n");

        //Create a list of sectors for all the fileheaders
        unsigned totalSectors = DivRoundUp(numBytes, SECTOR_SIZE);
        unsigned fileHeaderSectors = DivRoundUp(totalSectors, NUM_DIRECT);

        if(freeMap->CountClear() < fileHeaderSectors){
            delete freeMap;
            DEBUG('f', "There is not enough disk space for the file's FH\n");
            openFilesTable[sector]->writeLock->Release();
            return result;
        }

        int* sectors = new int[fileHeaderSectors];
        for(unsigned i = 0; i < fileHeaderSectors; ++i) {
            sectors[i] = freeMap->Find();
            freeMap->WriteBack(fileSystem->GetFreeMap());
        }
        DEBUG('w', "Sectors found! Lets create file headers now...\n");

        // The first file header of the linked list of file headers
        unsigned maxSize = NUM_DIRECT * SECTOR_SIZE;
        unsigned bytesToAllocate = numBytes <  maxSize ? numBytes : maxSize;

        DEBUG('w', "Allocating a fileheader with bytes to allocate: %u\n", bytesToAllocate);
        FileHeader *firstHeader = new FileHeader;
        firstHeader->GetRaw()->nextFileHeader = 0;
        firstHeader->GetRaw()->numBytes = 0;
        firstHeader->GetRaw()->numSectors = 0;

        DEBUG('9', "alocating again...\n");
        success = firstHeader->Allocate(freeMap, bytesToAllocate);
        if(!success) {
            DEBUG('w', "Error al querer alocar el frist header!\n");
            delete freeMap;
            delete firstHeader;
            delete [] sectors;
            openFilesTable[sector]->writeLock->Release();
            return result;
        }

        DEBUG('w', "Writing the bitmap back to disk...\n");
        freeMap->WriteBack(fileSystem->GetFreeMap());

        DEBUG('w', "Writing new file header back to disk in the sector: %u...\n", sectors[0]);
        firstHeader->WriteBack(sectors[0]);

        hdr->GetRaw()->nextFileHeader = sectors[0];
        DEBUG('w', "Writing back the original hdr with next header: %u\n", sectors[0]);
        hdr->WriteBack(currentSector); // !!!!!!!!

        FileHeader * fhToDestroy = hdr;
        //Update the hdr of the object
        hdr = firstHeader;

        delete fhToDestroy;

        DEBUG('w', "hdr is: %p, and the raw is: %p\n", hdr, hdr->GetRaw());

        ////////////////// Updating data //////////////////////
        seekPosition = 0;

        result_tmp = WriteAt(from, bytesToAllocate, seekPosition);
        DEBUG('w', "The bytes writed in the current FH are: %u\n", result_tmp);
        result += result_tmp;
        from += result_tmp; //Move the array of char pointers result_tmp times

        seekPosition += result_tmp;
        numBytes -= result_tmp;  // This will be always zero at some point

        FileHeader** fileHeaders = new FileHeader*[fileHeaderSectors];
        fileHeaders[0] = firstHeader;

        FileHeader* previousFileHeader = firstHeader;

        unsigned allocatedFileHeaders = 1;
        while(allocatedFileHeaders < fileHeaderSectors && success) {

            bytesToAllocate = numBytes <  maxSize ? numBytes : maxSize;

            DEBUG('w', "Creating a new file header!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            FileHeader *newFileHeader = new FileHeader;
            newFileHeader->GetRaw()->nextFileHeader = 0;
            newFileHeader->GetRaw()->numBytes = 0;
            newFileHeader->GetRaw()->numSectors = 0;
            if(freeMap->CountClear() < DivRoundUp(bytesToAllocate, SECTOR_SIZE)) {
                bytesToAllocate = freeMap->CountClear() * SECTOR_SIZE;
            }

            if(bytesToAllocate > 0)
                success = newFileHeader->Allocate(freeMap, bytesToAllocate);
            else
                success = false;

            if(success) {
                DEBUG('w', "Writing file headers and bitmap back to disk...");
                freeMap->WriteBack(fileSystem->GetFreeMap());
                newFileHeader->WriteBack(sectors[allocatedFileHeaders]);

                previousFileHeader->GetRaw()->nextFileHeader = sectors[allocatedFileHeaders];
                previousFileHeader->WriteBack(sectors[allocatedFileHeaders - 1]);

                ////////////////// Updating data //////////////////////
                seekPosition = 0;
                hdr = newFileHeader;
                result_tmp = WriteAt(from, bytesToAllocate, seekPosition);
                result += result_tmp;
                from += result_tmp; //Move the array of char pointers reslt_tmp times
                DEBUG('w', "the size of from is: %u\n", sizeof(from));

                seekPosition += result_tmp;
                numBytes -= result_tmp;  // This will be always zero at some point

                previousFileHeader = newFileHeader;
                fileHeaders[allocatedFileHeaders] = newFileHeader;
                ++allocatedFileHeaders;
            }
        }

        // Delete the created file headers
        for(unsigned i = 0; i < allocatedFileHeaders - 1; ++i) {
            delete fileHeaders[i];
        }
        delete [] fileHeaders;

        // This is for clearing the sectors allocated previously if some of the allocate fails
        if(!success)
            for(unsigned i = allocatedFileHeaders; i < fileHeaderSectors;++i)
                freeMap->Clear(sectors[i]);

        // Set the current sector as the last sector allocated for the last file header
        currentSector = sectors[allocatedFileHeaders - 1];

        delete [] sectors;
    }
    delete freeMap;
    openFilesTable[sector]->writeLock->Release();

    DEBUG('w',"The bytes writed are: %u\n", result);

    return result;
}

/// OpenFile::ReadAt/WriteAt
///
/// Read/write a portion of a file, starting at `position`.  Return the
/// number of bytes actually written or read, but has no side effects (except
/// that `Write` modifies the file, of course).
///
/// There is no guarantee the request starts or ends on an even disk sector
/// boundary; however the disk only knows how to read/write a whole disk
/// sector at a time.  Thus:
///
/// For ReadAt:
///     We read in all of the full or partial sectors that are part of the
///     request, but we only copy the part we are interested in.
/// For WriteAt:
///     We must first read in any sectors that will be partially written, so
///     that we do not overwrite the unmodified portion.  We then copy in the
///     data that will be modified, and write back all the full or partial
///     sectors that are part of the request.
///
/// * `into` is the buffer to contain the data to be read from disk.
/// * `from` is the buffer containing the data to be written to disk.
/// * `numBytes` is the number of bytes to transfer.
/// * `position` is the offset within the file of the first byte to be
///   read/written.

int
OpenFile::ReadAt(char *into, unsigned numBytes, unsigned position)
{
    ASSERT(into != nullptr);
    ASSERT(numBytes > 0);

    if(isBin) {
        SystemDep::Lseek(file, position, 0);
        return SystemDep::ReadPartial(file, into, numBytes);
    }

    unsigned fileLength = hdr->FileLength(); // raw.numBytes (size): 416
    unsigned firstSector, lastSector, numSectors;
    char *buf;

    if (position >= fileLength) {
        DEBUG('f', "Reading error position greater or equal than fileLength.\n");
        return 0;  // Check request.
    }
    if (position + numBytes > fileLength) {
        numBytes = fileLength - position;
    }
    DEBUG('f', "Reading %u bytes at %u, from file of length %u.\n",
          numBytes, position, fileLength);

    firstSector = DivRoundDown(position, SECTOR_SIZE);
    lastSector = DivRoundDown(position + numBytes - 1, SECTOR_SIZE);
    numSectors = 1 + lastSector - firstSector;

    // Read in all the full and partial sectors that we need.
    buf = new char [numSectors * SECTOR_SIZE];
    for (unsigned i = firstSector; i <= lastSector; i++) {
        unsigned sectorToRead = hdr->ByteToSector(i * SECTOR_SIZE);
        DEBUG('9', "Sector to read in ReadAt: %u\n", sectorToRead);
        synchDisk->ReadSector(sectorToRead,
                              &buf[(i - firstSector) * SECTOR_SIZE]);
    }

    // Copy the part we want.
    memcpy(into, &buf[position - firstSector * SECTOR_SIZE], numBytes);
    delete [] buf;
    return numBytes;
}

int
OpenFile::WriteAt(const char *from, unsigned numBytes, unsigned position)
{
    ASSERT(from != nullptr);
    ASSERT(numBytes > 0);

    if(isBin) {
        SystemDep::Lseek(file, position, 0);
        SystemDep::WriteFile(file, from, numBytes);
        return numBytes;
    }

    unsigned fileLength = hdr->FileLength();
    unsigned firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if (position >= fileLength) {
        return 0;  // Check request.
    }
    if (position + numBytes > fileLength) {
        numBytes = fileLength - position;
    }
    DEBUG('f', "Writing %u bytes at %u, from file of length %u.\n",
          numBytes, position, fileLength);

    firstSector = DivRoundDown(position, SECTOR_SIZE);
    lastSector  = DivRoundDown(position + numBytes - 1, SECTOR_SIZE);
    numSectors  = 1 + lastSector - firstSector;

    buf = new char [numSectors * SECTOR_SIZE];

    firstAligned = position == firstSector * SECTOR_SIZE;
    lastAligned  = position + numBytes == (lastSector + 1) * SECTOR_SIZE;

    // Read in first and last sector, if they are to be partially modified.
    if (!firstAligned) {
        ReadAt(buf, SECTOR_SIZE, firstSector * SECTOR_SIZE);
    }
    if (!lastAligned && (firstSector != lastSector || firstAligned)) {
        ReadAt(&buf[(lastSector - firstSector) * SECTOR_SIZE],
               SECTOR_SIZE, lastSector * SECTOR_SIZE);
    }

    // Copy in the bytes we want to change.
    memcpy(&buf[position - firstSector * SECTOR_SIZE], from, numBytes);

    // Write modified sectors back.
    for (unsigned i = firstSector; i <= lastSector; i++) {
        synchDisk->WriteSector(hdr->ByteToSector(i * SECTOR_SIZE),
                               &buf[(i - firstSector) * SECTOR_SIZE]);
    }
    delete [] buf;
    return numBytes;
}

/// Return the number of bytes in the file.
unsigned
OpenFile::Length() const
{
    if(isBin) {
        SystemDep::Lseek(file, 0, 2);
        return SystemDep::Tell(file);
    }
    return hdr->FileLength();
}

int
OpenFile::GetSector()
{
    return sector;
}

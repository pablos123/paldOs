/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_RAWFILEHEADER__HH
#define NACHOS_FILESYS_RAWFILEHEADER__HH


#include "machine/disk.hh"


// We need to support indirect addressing, so the MAX_FILE_SIZE will change.
// The sector count is NUM_SECTORS
// Each file header has NUM_DIRECT - 2 disk sectors for the file.
// Note that the dataSectors count is NUM_DIRECT - 2, this is for keeping the 128 bytes structure
// for consistency with the disk sector size.
// We need to find how many times FileHeaders + NUM_DIRECT - 2 fits in
// NUM_SECTORS, so:

// times_fh_fits_num_sector = int(NUM_SECTORS/NUM_DIRECT - 1)
// file_data_sectors = NUM_SECTORS - times_fh_fits_num_sector
// bytes_for_file_data_sectors = file_data_sectors * SECTOR SIZE


static const unsigned NUM_DIRECT
  = ((SECTOR_SIZE - 2 * sizeof (int)) / sizeof (int)) - 1;
const unsigned TIMES_FH_FITS = int(NUM_SECTORS / (NUM_DIRECT + 1));
const unsigned MAX_FILE_SIZE = (NUM_SECTORS - TIMES_FH_FITS) * SECTOR_SIZE;

struct RawFileHeader {
    unsigned numBytes;  ///< Number of bytes in the file.
    unsigned numSectors;  ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                       ///< block in the file.
    unsigned nextFileHeader;  // Number to the next
                              // FileHeader of the file
};
typedef struct RawFileHeader* RawFileHeaderNode;

#endif

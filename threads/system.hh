/// All global variables used in Nachos are defined here.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_SYSTEM__HH
#define NACHOS_THREADS_SYSTEM__HH


#include "thread.hh"
#include "lock.hh"
#include "scheduler.hh"
#include "lib/utility.hh"
#include "machine/interrupt.hh"
#include "machine/statistics.hh"
#include "machine/timer.hh"
#include "lib/bitmap.hh"

/// Initialization and cleanup routines.

// Initialization, called before anything else.
extern void Initialize(int argc, char **argv);

// Cleanup, called when Nachos is done.
extern void Cleanup();

#ifdef SWAP
typedef struct _coreMapEntry{
    SpaceId spaceId;
    unsigned virtualPage;
#ifdef PRPOLICY_LRU
    unsigned last_use_counter;  // this will represent the last recently use page.
                                // to search for the victim we will search directly for the minimun value of the array
#endif
}* CoreMapEntry;

extern CoreMapEntry* coreMap;
#endif

#ifdef PRPOLICY_FIFO
extern unsigned fifo_counter;
#endif

#ifdef PRPOLICY_LRU
extern unsigned references_done;
#endif

extern Thread *currentThread;        ///< The thread holding the CPU.
extern Thread *threadToBeDestroyed;  ///< The thread that just finished.
extern Scheduler *scheduler;         ///< The ready list.
extern Interrupt *interrupt;         ///< Interrupt status.
extern Statistics *stats;            ///< Performance metrics.
extern Timer *timer;                 ///< The hardware alarm clock.

extern const unsigned NUMBER_OF_TRIES;

#ifdef USER_PROGRAM
#include "machine/machine.hh"
#include "machine/synch_console.hh"

extern Machine *machine;  // User program memory and registers.
extern Bitmap* addressesBitMap;       ///< the addresses bit map
extern Table<Thread*> *runningProcesses;
extern SynchConsole* consoleSys;
#endif

#ifdef FILESYS_NEEDED  // *FILESYS* or *FILESYS_STUB*.
#include "filesys/file_system.hh"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "filesys/synch_disk.hh"
extern SynchDisk *synchDisk;
typedef struct _openFileEntry {
    int count;
    bool removing;
    bool removed;
    Lock* writeLock;
    Lock* removeLock;
    Lock* closeLock;
    SpaceId removerSpaceId;
}* OpenFileEntry;

extern OpenFileEntry* openFilesTable;
extern Lock* filesysCreateLock;

#endif

#ifdef NETWORK
#include "network/post.hh"
extern PostOffice *postOffice;
#endif


#endif

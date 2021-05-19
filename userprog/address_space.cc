/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"

#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

    Executable exe (executable_file);
    ASSERT(exe.CheckMagic()); //check if the executable is a nachos binary

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE * PAGE_SIZE;

    ASSERT(numPages <= addressesBitMap->CountClear()); //usar countClear
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
          // For now, virtual page number = physical page number.
        pageTable[i].physicalPage = addressesBitMap->Find();
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    
    for(unsigned i = 0; i < numPages; i++) {
      DEBUG('a', "use el bit %u\n",
            pageTable[i].physicalPage);
    }

    char *mainMemory = machine->GetMMU()->mainMemory;
   
    for(unsigned i = 0; i < numPages; i++) {
      memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE); //looks fine
    }

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment. 
    //memset(mainMemory, 0, size);  //rellenar con 0 SOLO las paginas de este proceso (no desde main memory)

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();

    DEBUG('a', "Datasize %u\n",
      initDataSize);

    unsigned codePages = DivRoundUp(codeSize, PAGE_SIZE);
    DEBUG('a', "code pages: %u, %u\n", codePages, PAGE_SIZE);
    
    if (codeSize > 0) {
        uint32_t virtualAddr = exe.GetCodeAddr(); //ya no es necesario...

        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codeSize);

        unsigned pageCounter = 0;
        unsigned wannaRead = PAGE_SIZE;
        for(unsigned i = 0; i != codePages; i++) {
            DEBUG('a', "EL I de code es: %u\n", i);

            unsigned lecture = wannaRead > codeSize ? codeSize - i * PAGE_SIZE : PAGE_SIZE;
            uint32_t addressToWrite = pageTable[i].physicalPage * PAGE_SIZE;
            uint32_t offset = PAGE_SIZE * pageCounter;

            DEBUG('a', "writing %u bytes to %u\n", lecture, addressToWrite);
            
            exe.ReadCodeBlock(&mainMemory[addressToWrite], lecture, offset);
            wannaRead += PAGE_SIZE;
            pageCounter++;
        }
        DEBUG('a', "totalRead: %u\n", wannaRead);
        //exe.ReadCodeBlock(&mainMemory[virtualAddr], codeSize, 0); //leer de a paginas (PAGE_SIZE), ya no está mapeado uno a uno
    }
    if (initDataSize > 0) {
        unsigned dataPages = DivRoundUp(initDataSize, PAGE_SIZE);

        uint32_t virtualAddr = exe.GetInitDataAddr(); //ya no es necesario...
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        DEBUG('a', "data pages: %u, %u\n", dataPages, PAGE_SIZE);

        unsigned pageCounter = 0;
        unsigned wannaRead = PAGE_SIZE;
        for(unsigned i = codePages; i < codePages + dataPages; i++) {
            DEBUG('a', "EL I de data es: %u\n", i);

            unsigned lecture = wannaRead > initDataSize ? initDataSize - pageCounter * PAGE_SIZE : PAGE_SIZE;
            uint32_t addressToWrite = pageTable[i].physicalPage * PAGE_SIZE;
            uint32_t offset = PAGE_SIZE * pageCounter;

            DEBUG('a', "writing %u bytes to %u\n", lecture, addressToWrite);
            
            exe.ReadDataBlock(&mainMemory[addressToWrite], lecture, offset);
            wannaRead += PAGE_SIZE;
            pageCounter++;
        }
    }
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    //int index; //check type
    for (unsigned i = 0; i < numPages; i++) {
      addressesBitMap->Clear(pageTable[i].physicalPage);
    }
    delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
}

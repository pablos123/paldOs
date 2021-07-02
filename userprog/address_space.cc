/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"
#include <limits.h>
#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

#ifdef DEMAND_LOADING
    exeFile = executable_file;
#endif

    Executable exe (executable_file);
    ASSERT(exe.CheckMagic()); //check if the executable is a nachos binary

#ifdef DEMAND_LOADING
    codeSize = exe.GetCodeSize();
    initDataSize = exe.GetInitDataSize();

    initDataFileAddr = exe.GetInFileInitDataAddr();
    codeFileAddr = exe.GetInFileCodeAddr();

    exeSize = exe.GetSize();

    DEBUG('a',"Loading information: codeAddr: %d initDataAddr: %d, ", exe.GetCodeAddr(), exe.GetInitDataAddr());
#endif

    // We need to increase the size to leave room for the stack.
    unsigned size = exe.GetSize() + USER_STACK_SIZE;

    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    DEBUG('a', "clean count: %u, numPages: %u, size: %u\n", addressesBitMap->CountClear(), numPages, size);

    ASSERT(numPages <= addressesBitMap->CountClear()); //ver que onda este assert, habria que dejarlo si no hay DL
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];

    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
#ifndef DEMAND_LOADING
        pageTable[i].physicalPage = addressesBitMap->Find();
        pageTable[i].valid        = true;
#else
        pageTable[i].physicalPage = INT_MAX;
        pageTable[i].valid        = false;
#endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

#ifndef DEMAND_LOADING

    DEBUG('e', "Not using demand loading...\n");
    //Debug:
    for(unsigned i = 0; i < numPages; i++)
      DEBUG('a', "using%u\n", pageTable[i].physicalPage);

    char *mainMemory = machine->GetMMU()->mainMemory;
    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.
    //Cleaning the memory that will be used...

    for(unsigned i = 0; i < numPages; i++) {
      memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE); //looks fine
    }

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();

    if (codeSize > 0) {
        uint32_t virtualAddr = exe.GetCodeAddr();

        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n", virtualAddr, codeSize);

        for(unsigned i = 0; i < codeSize; i++) {

            uint32_t frame  = (uint32_t)((virtualAddr + i) / PAGE_SIZE);  //obtengo el numero de pagina corresp al bit que quiero leer
            uint32_t offset = (virtualAddr + i) % PAGE_SIZE ; //dentro de esa pagina, me ubico en donde voy a leer
            uint32_t physicalAddressToWrite = pageTable[frame].physicalPage * PAGE_SIZE + offset;

            exe.ReadCodeBlock(&mainMemory[physicalAddressToWrite], 1, i); //leo un byte de la memoria fisica luego de moverme 'i' offset
        }
    }
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n", virtualAddr, initDataSize);

        for(unsigned i = 0; i < initDataSize; i++) {

            uint32_t frame  = (uint32_t)((virtualAddr + i) / PAGE_SIZE);
            uint32_t offset = (virtualAddr + i) % PAGE_SIZE ;
            uint32_t physicalAddressToWrite = pageTable[frame].physicalPage * PAGE_SIZE + offset;

            exe.ReadDataBlock(&mainMemory[physicalAddressToWrite], 1, i);
        }
    }
#else
  DEBUG('e', "Using demand loading...\n");
#endif
}

#ifdef DEMAND_LOADING
void
AddressSpace::LoadPage(unsigned vpnAddress, unsigned physicalPage) {

    ASSERT(vpnAddress >= 0);
    ASSERT(physicalPage != INT_MAX); //i  have a valid frame

    Executable exe (exeFile);

    DEBUG('e', "Loading page..., physicalPage: %d, vpnAddress: %d\n", physicalPage, vpnAddress);

    // Get the physical address to write into
    uint32_t physicalAddressToWrite = physicalPage * PAGE_SIZE;

    // Clean the memory
    char *mainMemory = machine->GetMMU()->mainMemory;
    memset(&mainMemory[physicalAddressToWrite], 0, PAGE_SIZE); //looks fine

    int vpn = vpnAddress / PAGE_SIZE;
    unsigned vpnAddressToRead = vpn * PAGE_SIZE;

    unsigned readed = 0; // I need to ensure that i have readed PAGE_SIZE bytes

    if (codeSize > 0 && vpnAddressToRead < codeSize) { // C lazyness
        DEBUG('e', "Reading code...\n");
        uint32_t toRead = codeSize - vpnAddressToRead < PAGE_SIZE ? codeSize - vpnAddressToRead : PAGE_SIZE;

        DEBUG('e',"Amount to be read: %d \n", toRead);
        exe.ReadCodeBlock(&mainMemory[physicalAddressToWrite], PAGE_SIZE, vpnAddressToRead);

        readed += toRead; //to check if there is some data left to read
    }

    if (initDataSize > 0 && vpnAddressToRead + readed < exe.GetInitDataAddr() + initDataSize &&
        readed != PAGE_SIZE) {
        DEBUG('e', "Reading data...\n");
        uint32_t toRead = PAGE_SIZE - readed ;

        // exe =|code....||data...||stadijwdjwidj|
        readed ? //if read any bytes in the code section and i have not completed the PAGE_SIZE
            exe.ReadDataBlock(&mainMemory[physicalAddressToWrite + readed], toRead,  0)
        :
            exe.ReadDataBlock(&mainMemory[physicalAddressToWrite], toRead, vpnAddressToRead - codeSize);
    }

    if (readed != PAGE_SIZE) { // Completes a page if needed
        // Stack memory
    }

    return;
}
#endif

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    char *mainMemory = machine->GetMMU()->mainMemory;

    for (unsigned i = 0; i < numPages; i++) {
#ifdef DEMAND_LOADING
        if(pageTable[i].physicalPage == INT_MAX)
            continue;
#endif
      addressesBitMap->Clear(pageTable[i].physicalPage);
      memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE); //looks fine
    }
#ifdef DEMAND_LOADING
    delete exeFile;
#endif
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

TranslationEntry*
AddressSpace::getPageTableEntry(unsigned vpn) {
  return &pageTable[vpn];
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

#ifndef USE_TLB
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
#else

    //tenemos TLB, la limpiamos para cambiar de proceso
    DEBUG('t', "Vaciando la TLB \n");
    for(unsigned i=0; i < TLB_SIZE; i++){
      machine->GetMMU()->tlb[i].valid = false;
    }
#endif
}

/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"
#include <time.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file, SpaceId spaceId)
{
    ASSERT(executable_file != nullptr);

#ifdef DEMAND_LOADING
    exeFile = executable_file;
    #ifdef SWAP
        char* swapFile = new char[30];
        sprintf(swapFile, "userprog/SWAP/SWAP.%d", spaceId);
        #ifdef FILESYS
        if(!fileSystem->Create(swapFile, 0, true)) {
        #else
        if(!fileSystem->Create(swapFile, 0)) {
        #endif
            DEBUG('a', "Error: Swap file not created.\n");
        }

        // We can have the approach of open the file every time we write in swap, but for testing we are forcing the OS to have
        // very poor number of physical pages.
        #ifdef FILESYS
        openSwapFile = fileSystem->Open(swapFile, true);
        #else
        openSwapFile = fileSystem->Open(swapFile);
        #endif

        delete [] swapFile;
        if(openSwapFile == nullptr) {
            DEBUG('a', "Cannot open SWAP FILE!!!!\n");
            ASSERT(false);
        }

        addressSpaceId = spaceId;  //to replace the coreMap later
    #endif
#endif

    Executable exe (executable_file);
    ASSERT(exe.CheckMagic()); //check if the executable is a nachos binary

#ifdef DEMAND_LOADING
    codeSize = exe.GetCodeSize();
    initDataSize = exe.GetInitDataSize();

    dataVirtualAddr = exe.GetInitDataAddr();

    DEBUG('a',"Loading information: codeAddr: %d initDataAddr: %d, ", exe.GetCodeAddr(), exe.GetInitDataAddr());
#endif

    // We need to increase the size to leave room for the stack.
    unsigned size = exe.GetSize() + USER_STACK_SIZE;

    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    DEBUG('a', "clean count: %u, numPages: %u, size: %u\n", addressesBitMap->CountClear(), numPages, size);
#ifndef SWAP
    // por enunciado tenemos que todos los programas entran en memoria con demand loading
    // Check we are not trying to run anything too big -- at least until we
    // have virtual memory.
    ASSERT(numPages <= addressesBitMap->CountClear());
#endif
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

    DEBUG('a', "Not using demand loading...\n");

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
  DEBUG('a', "Using demand loading...\n");
#endif
}

#ifdef DEMAND_LOADING
void
AddressSpace::LoadPage(unsigned vpnAddress, unsigned physicalPage) {

    ASSERT(vpnAddress >= 0);
    ASSERT(physicalPage != INT_MAX); //i  have a valid frame

    Executable exe (exeFile);

    DEBUG('a', "Loading page..., physicalPage: %d, vpnAddress: %d\n", physicalPage, vpnAddress);

    // Get the physical address to write into
    uint32_t physicalAddressToWrite = physicalPage * PAGE_SIZE;

    // Clean the memory
    char *mainMemory = machine->GetMMU()->mainMemory;
    memset(&mainMemory[physicalAddressToWrite], 0, PAGE_SIZE); //looks fine

    int vpn = vpnAddress / PAGE_SIZE;
    unsigned vpnAddressToRead = vpn * PAGE_SIZE;

    unsigned read = 0; // I need to ensure that i have read PAGE_SIZE bytes

    #ifdef SWAP
    if(pageTable[vpn].dirty) {
        DEBUG('a',"Reading from swap at position %d...\n", vpn * PAGE_SIZE);
        openSwapFile->ReadAt(&mainMemory[physicalAddressToWrite], PAGE_SIZE, vpn * PAGE_SIZE);
    } else { //read from the exe file
    #endif
        if (codeSize > 0 && vpnAddressToRead < codeSize) {
            DEBUG('a', "Reading code...\n");
            uint32_t toRead = codeSize - vpnAddressToRead < PAGE_SIZE ? codeSize - vpnAddressToRead : PAGE_SIZE;

            DEBUG('a',"Amount to be read: %d \n", toRead);
            exe.ReadCodeBlock(&mainMemory[physicalAddressToWrite], PAGE_SIZE, vpnAddressToRead);

            read += toRead; //to check if there is some data left to read
        }

        if (initDataSize > 0 && vpnAddressToRead + read < dataVirtualAddr + initDataSize && read != PAGE_SIZE) {

            // uint32_t toRead = PAGE_SIZE - read; // We're not sure if we need to the check cases like: |code segment... |data segment: 128 ... 128 12|. Suppose we didnt read any code bytes, so
            //                                         // here we're reading 128 bytes intead of just 12, maybe we're reaing garbage in some point. So we think of:

            uint32_t toRead = (dataVirtualAddr + initDataSize) - (vpnAddressToRead + read) < (PAGE_SIZE - read) ?
                                    (dataVirtualAddr + initDataSize) - (vpnAddressToRead + read)
                                :
                                    PAGE_SIZE - read;

            DEBUG('a', "Reading %d of data...\n", toRead);

            read ? //if read any bytes in the code section and i have not completed the PAGE_SIZE
                exe.ReadDataBlock(&mainMemory[physicalAddressToWrite + read], toRead,  0)
            :
                exe.ReadDataBlock(&mainMemory[physicalAddressToWrite], toRead, vpnAddressToRead - codeSize);

            read += toRead;
        }
    #ifdef SWAP
    }
    #endif

    if(vpnAddressToRead > codeSize + initDataSize) { // We are reading from the stack
        read = PAGE_SIZE; // memset already done previously
    };

#ifdef SWAP
    // //Update the coremap
    CoreMapEntry chosenCoreMapEntry = coreMap[physicalPage];
    chosenCoreMapEntry->spaceId = addressSpaceId;
    chosenCoreMapEntry->virtualPage = vpn;

    DEBUG('a',"Marking physical page %u, with virtualPage %u from process %d in the coremap\n", physicalPage, vpn, addressSpaceId);

    //DEBUG('a',"State of the coremap: \n");
    //for(unsigned i = 0; i < NUM_PHYS_PAGES; i++){
    //    DEBUG('a',"Physical page: %u, spaceId: %d, virtualPage of the mentioned process: %u \n", i, coreMap[i]->spaceId, coreMap[i]->virtualPage);
    //}
#endif

    DEBUG('a', "Finished loading page! :)\n");
    return;
}

#ifdef SWAP
unsigned
AddressSpace::EvacuatePage() {
    unsigned victim = PickVictim();
    DEBUG('a',"VICTIM PICKED in EvacuatePage: %u\n", victim);
    SpaceId victimSpace = coreMap[victim]->spaceId;

    if(runningProcesses->HasKey(victimSpace)) { // the victim process is alive
        TranslationEntry* entry = runningProcesses->Get(victimSpace)->space->getPageTableEntry(coreMap[victim]->virtualPage);

        for(unsigned i = 0; i < TLB_SIZE; ++i) { // save the bits if the page is in the TLB
            if(machine->GetMMU()->tlb[i].physicalPage == victim && machine->GetMMU()->tlb[i].valid) {
                machine->GetMMU()->tlb[i].valid = false;
                *entry = machine->GetMMU()->tlb[i];
            }
        }

        //if dirty, we put the midified virtualPage into the N block of the swap file
        DEBUG('a', "In evacuate page, the entry is: \n dirty: %d\n valid: %d\n", entry->dirty, entry->valid);
        if(entry->dirty) {
            char *mainMemory = machine->GetMMU()->mainMemory;
            unsigned physicalAddressToWrite = victim * PAGE_SIZE;
            DEBUG('a',"Writing into swap...\n");
            runningProcesses->Get(coreMap[victim]->spaceId)->space->openSwapFile->WriteAt(&mainMemory[physicalAddressToWrite], PAGE_SIZE, coreMap[victim]->virtualPage * PAGE_SIZE);   //save the evacuated information in the N file block
        }
        // we do not update the coremap here because it always has to happen, regardless there is an EvacuatePage or not

        entry->physicalPage = INT_MAX; // mark the entry out of the memory for the pageTable
        entry->valid = false; // mark the entry out of the memory for the machine
    }
    return victim;
}

unsigned
AddressSpace::PickVictim() {
    unsigned victim = 0;
#ifdef PRPOLICY_FIFO
    victim = fifo_counter % NUM_PHYS_PAGES;
    fifo_counter++;
#elif PRPOLICY_LRU
    if(references_done == UINT_MAX) {
        references_done = 0;
        for(unsigned i = 0; i < NUM_PHYS_PAGES; i++)    // to avoid using the same frame once the UINT_MAX is reached
            coreMap[i]->last_use_counter = 0;
    }

    unsigned min = UINT_MAX;
    for(unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
        if(coreMap[i]->last_use_counter < min) {
            min = coreMap[i]->last_use_counter;
            victim = i;
        }
    }

#else
    victim = rand() % NUM_PHYS_PAGES;
#endif

    return victim;
}

SpaceId
AddressSpace::GetSpaceId() {
    return addressSpaceId;
}
#endif

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
{
#ifdef SWAP
    for(unsigned i=0; i < TLB_SIZE; i++){
        if(machine->GetMMU()->tlb[i].valid){
            unsigned physicalPageToSave = machine->GetMMU()->tlb[i].physicalPage;
            TranslationEntry* entry = getPageTableEntry(coreMap[physicalPageToSave]->virtualPage);
            machine->GetMMU()->tlb[i].valid = false;
            *entry = machine->GetMMU()->tlb[i];
        }
    }
#endif
}

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
    DEBUG('a', "Cleaning TLB...\n");
    for(unsigned i=0; i < TLB_SIZE; i++){
      machine->GetMMU()->tlb[i].valid = false;
    }
#endif
}

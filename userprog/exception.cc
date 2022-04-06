/// Entry points into the Nachos kernel from user programs.
/// h
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "args.hh"
#include "exception.hh"
#include "filesys/directory_entry.hh"
#include "filesys/open_file.hh"
#include "threads/system.hh"
#include "machine/mmu.hh"    //for the page size

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}


///
/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
///

static void
StartProcess(void * voidargv)
{

    char** argv = (char**)voidargv;

    currentThread->space->InitRegisters();  // Set the initial register values.

    currentThread->space->RestoreState();   // Load page table register.

    unsigned argc = 0;

    if(argv != nullptr) {
        argc = WriteArgs(argv); //libera la memoria de argv
        DEBUG('e', "argv is not null and argc is: %d \n", argc);

        if(argc) {
            //Load a1 register (5)
            int argvaddr = machine->ReadRegister(STACK_REG);
            //Lo ultimo que hace WriteArgs es dejar el sp dentro de STACK_REG
            //luego de haber cargado todos los argumentos.
            machine->WriteRegister(STACK_REG, argvaddr - 24); //convencion de MIPS, restandole 24 seteo el stack pointer 24 bytes más abajo.

            DEBUG('e', "argvaddr is: %d\n", argvaddr);
            machine->WriteRegister(5, argvaddr);
        }
    } else {
        DEBUG('e', "argvaddr is null!: %p\n", argv);
    }

    machine->WriteRegister(4, argc);

    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                    // exits by doing the system call `Exit`.
}


/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!) :D
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT: {
            DEBUG('e', "Shutdown, initiated by user program.\n");

            interrupt->Halt();

            break;
        }

        case SC_EXIT: {
            int status = machine->ReadRegister(4);

            DEBUG('e', "The program finished with status %d.\n", status);

            currentThread->Finish(status);

            break;
        }

        case SC_EXEC: {
            int processAddr = machine->ReadRegister(4);
            int argvAddr = machine->ReadRegister(5);
            bool isJoinable = (bool)machine->ReadRegister(6);
            char** argv = nullptr;

            if (processAddr == 0) {
                DEBUG('e', "Error in Exec: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char* filename = new char[FILE_NAME_MAX_LEN + 1];

            if (! ReadStringFromUser(processAddr, filename, FILE_NAME_MAX_LEN + 1)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            if (argvAddr) {
                DEBUG('e', "argvAddr distinto de cero, con direccion: %d\n", argvAddr);
                argv = SaveArgs(argvAddr);
            }

            DEBUG('e', "el nombre del archivo a abrir es %s y direccion de memoria %p\n", filename, filename);

            ASSERT(filename != nullptr);

            DEBUG('e', "abriendo: %s\n", filename);

            OpenFile *executable = fileSystem->Open(filename);
            if (executable == nullptr) {
                DEBUG('e', "Unable to open file %s\n", filename);
                machine->WriteRegister(2, -1);
                break;
            }

            Thread *newThread = new Thread(filename, isJoinable);

            SpaceId spaceId = (SpaceId)runningProcesses->Add(newThread);

            AddressSpace *space = new AddressSpace(executable, spaceId);

            newThread->space = space;

#ifndef DEMAND_LOADING
            DEBUG('e', "Deleting executable...\n");
            delete executable;
#endif

            newThread->Fork(StartProcess, (void *) argv);

            machine->WriteRegister(2, spaceId);

            break;
        }

        case SC_JOIN: {
            SpaceId spaceId = machine->ReadRegister(4);

            DEBUG('e', "Joining process %d\n", spaceId);

            if(! runningProcesses->HasKey(spaceId)){
                DEBUG('e',"Error en Join: id del proceso inexistente");
                machine->WriteRegister(2, -1);
                break;
            }

            Thread* process = runningProcesses->Get(spaceId);
            int exitStatus = process->Join();

            machine->WriteRegister(2, exitStatus);

            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error in Create: address to filename string is null.\n");
                machine->WriteRegister(2, 1);
                break;
            }

            char* filename = new char[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, 1);
                break;
            }

            if(!fileSystem->Create(filename, 0)){
                DEBUG('e', "Error: File not created.\n");
                machine->WriteRegister(2, 1);
                break;
            }
            machine->WriteRegister(2, 0);

            DEBUG('e', "File `%s` created.\n", filename);
            break;
        }

        case SC_REMOVE:{
            DEBUG('e',"About to remove a file...\n");
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error in remove: address to filename string is null.\n");
                machine->WriteRegister(2, 1);
                break;
            }

            char* filename = new char[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, 1);
                break;
            }
            if(!fileSystem->Remove(filename)){
                DEBUG('e', "Error: File %s not removed.\n",filename);
                machine->WriteRegister(2, 1);
                break;
            }

            machine->WriteRegister(2, 0);
            DEBUG('e', "File `%s` removed.\n", filename);
            break;

        }


        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);

            if (filenameAddr == 0) {
                DEBUG('e', "Error in Open: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char* filename = new char[FILE_NAME_MAX_LEN + 1];

            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                    FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile* openedFile = fileSystem->Open(filename);

            if(openedFile == nullptr){
                DEBUG('e', "Error: File not opened.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            int fileDescriptor = currentThread->GetOpenedFilesTable()->Add(openedFile);
            if(fileDescriptor < 0) {
                DEBUG('e', "Error: File not added to table");
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, fileDescriptor);
            DEBUG('e', "File opened successfully!. Index assigned: %d \n", fileDescriptor);

            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);

            if(currentThread->GetOpenedFilesTable()->HasKey(fid) && fid != 0 && fid != 1) {
                currentThread->GetOpenedFilesTable()->Remove(fid);
                DEBUG('e', "File closed successfully!\n");
                machine->WriteRegister(2, 0);

            } else{
                DEBUG('e', "Error: File not closed. ID is not present in the table");
                machine->WriteRegister(2, 1);
            }

            break;
        }

        case SC_READ: {
            int usrStringAddr = machine->ReadRegister(4);
            int nbytes = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);

            if (usrStringAddr == 0) {
                DEBUG('e', "Error in Read: address string is null.\n");
                machine->WriteRegister(2, 0);
                break;
            }
            if (nbytes <= 0){
                DEBUG('e', "Error in Read: invalid number of bytes.\n");
                machine->WriteRegister(2, 0);
                break;
            }
            if(fid < 0){
                DEBUG('e', "Invalid file descriptor ID to Read\n");    //to avoid the assert in Remove function
                machine->WriteRegister(2, 0);
                break;
            }

            char buffer[nbytes + 1];

            if(fid == CONSOLE_INPUT) {

                for(int i = 0; i < nbytes; i++) {
                    buffer[i] = consoleSys->ReadConsole();
                }

                WriteBufferToUser(buffer, usrStringAddr, nbytes);
                machine->WriteRegister(2, nbytes);
            } else if(!currentThread->GetOpenedFilesTable()->HasKey(fid)) {
                DEBUG('e', "Error in read: Not an opened file\n");
                machine->WriteRegister(2, 0);
            } else {
                OpenFile* file = currentThread->GetOpenedFilesTable()->Get(fid);

                int bytesReaded = file->Read(buffer, nbytes);

                DEBUG('e', "Readed: %s, nrobytes: %d de %d, fromfileid: %d, fileaddr: %p\n", buffer, bytesReaded, nbytes, fid, file);

                if(bytesReaded <= 0) {
                    DEBUG('e', "error in read: wrong bytes readed\n");
                    machine->WriteRegister(2, 0);
                } else {
                    WriteBufferToUser(buffer, usrStringAddr, nbytes);
                    machine->WriteRegister(2, bytesReaded);
                }
            }
            break;
        }

        case SC_WRITE: {
            int usrStringAddr = machine->ReadRegister(4);
            int nbytes = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);


            if (usrStringAddr == 0) {
                DEBUG('e', "Error in Write: address string is null.\n");
                machine->WriteRegister(2, 0);
                break;
            }
            if (nbytes <= 0){
                DEBUG('e', "Error in Write: invalid number of bytes.\n");
                machine->WriteRegister(2, 0);
                break;
            }
            if(fid < 0){
                DEBUG('e', "Invalid file descriptor ID to Write\n");    //to avoid the assert in Remove function
                machine->WriteRegister(2, 0);
                break;
            }

            char* buffer = new char[nbytes + 1];
            ReadBufferFromUser(usrStringAddr, buffer, nbytes);
            DEBUG('e', "Write: %s, nrobytes: %d, hacia: %d\n", buffer, nbytes, usrStringAddr);

            if(fid == CONSOLE_OUTPUT) {
                for(int i = 0; i < nbytes; i++) {
                    consoleSys->WriteConsole(buffer[i]);
                }

                machine->WriteRegister(2, nbytes);
            } else if(!currentThread->GetOpenedFilesTable()->HasKey(fid)) {
                DEBUG('e', "Error in write: the file was not opened\n");
                machine->WriteRegister(2, 0);
            } else {

                OpenFile* file = currentThread->GetOpenedFilesTable()->Get(fid);

                int bytesWrited = file->Write(buffer, nbytes);

                if(bytesWrited <= 0) {
                    DEBUG('e', "error in write: wrong bytes writed");
                    machine->WriteRegister(2, 0);
                } else {
                    machine->WriteRegister(2, bytesWrited);
                    DEBUG('e', "SUCESS! %d writed \n", bytesWrited);
                }
            }

            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}

static void TLBPageFaultHandler(ExceptionType exc) {

#ifndef USE_TLB //Si no uso tlb entonces este error no deberíamos catchearlo (por ahora...)

    DefaultHandler(exc);

#else
    int vpnAddress = machine->ReadRegister(BAD_VADDR_REG);

    DEBUG('a',"There was a page fault. Searching... vpnAdress: %d \n", vpnAddress);

    int vpn = vpnAddress / PAGE_SIZE;
    DEBUG('a', "vpn: %d, numFaults: %lu ,indice de la TLB: %d \n", vpn, currentThread->numFaults, currentThread->numFaults % TLB_SIZE);

    TranslationEntry* pageTableEntry = currentThread->space->getPageTableEntry(vpn);

#ifdef DEMAND_LOADING

    pageTableEntry->valid = true;

    if(pageTableEntry->physicalPage == INT_MAX) {   // the page is not in main memory
        DEBUG('a', "Loading page that does not exists in memory (demand loading)\n");
        int possibleFrame = addressesBitMap->Find();
        unsigned frame = (unsigned)possibleFrame;
        DEBUG('a',"frame to use: %u, possibleFrame: %d\n",frame, possibleFrame);
#ifdef SWAP
        if(possibleFrame == -1) { //there aren't frames availables
            DEBUG('a', "I want to evacuate a page\n");
            DEBUG('a',"The page dirtyness is: %d\n", pageTableEntry->dirty);
            frame = currentThread->space->EvacuatePage(); //cleans up a physical page and updates the coreMap,
                                                             //returns the new physical page for use
            possibleFrame = frame;

            DEBUG('a',"frame to use: %u, possibleFrame: %d\n",frame, possibleFrame);
            ASSERT(possibleFrame != -1);
        }
#endif
        pageTableEntry->physicalPage = frame;

#ifdef PRPOLICY_LRU
        references_done++;
        coreMap[frame].last_use_counter = references_done;
#endif

        currentThread->space->LoadPage(vpnAddress, pageTableEntry->physicalPage);
    }
#endif

    unsigned tlbEntry = currentThread->numFaults % TLB_SIZE;

    if(machine->GetMMU()->tlb[tlbEntry].valid) {    // we are going to occupy a page that belongs to this proccess
        DEBUG('a', "The tlb entry was valid, saving the tlb state...\n");
        machine->GetMMU()->tlb[tlbEntry].valid = false;
        unsigned virtualPage = machine->GetMMU()->tlb[tlbEntry].virtualPage;
        TranslationEntry* entryToSave = currentThread->space->getPageTableEntry(virtualPage);
        *entryToSave = machine->GetMMU()->tlb[tlbEntry];
    }

    machine->GetMMU()->tlb[tlbEntry] = *pageTableEntry;

    currentThread->numFaults++;
#endif

}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &TLBPageFaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}

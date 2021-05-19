/// Entry points into the Nachos kernel from user programs.
///
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
#include "filesys/directory_entry.hh"
#include "filesys/open_file.hh"
#include "threads/system.hh"
#include "synch_console.hh"
#include "exception.hh"
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
StartProcess(void *argumentsAddr)
{
    StartPParam arguments = (StartPParam) argumentsAddr;

    char** argv = arguments->argv;

    char* filename = arguments->name;

    //DEBUG('e', "%s\n", argv[1]);
    DEBUG('e', "%s\n", filename);

    ASSERT(filename != nullptr);

    OpenFile *executable = fileSystem->Open(filename);
    if (executable == nullptr) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    
    AddressSpace *space = new AddressSpace(executable);
    currentThread->space = space;

    delete executable;

    space->InitRegisters();  // Set the initial register values.

    //Load a0 register (4)

    if(argv == nullptr) {
        machine->WriteRegister(4, 0);    
    } else {
        unsigned argc = WriteArgs(argv); //la otra es no hacer esto si argv es vacio
        machine->WriteRegister(4, argc);
        if(! argc) {
            //Load a1 register (5)
            int argvaddr = machine->ReadRegister(STACK_REG) + 16;
            machine->WriteRegister(5, argvaddr);
            //Lo ultimo que hace WriteArgs es dejar el sp dentro de STACK_REG
            //luego de haber cargado todos los argumentos.
        }
    }

    space->RestoreState();   // Load page table register.

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

        case SC_HALT:
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        case SC_EXIT: {
            int status = machine->ReadRegister(4);

            DEBUG('e', "The program finished with status %d.\n", status);


            currentThread->Finish(status);
        }

        case SC_EXEC: {
            int processAddr = machine->ReadRegister(4);
            int argvAddr = machine->ReadRegister(5);
            char** argv = nullptr;

            if (processAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, 1); //aca ojo porque esta haciendo seg fault no se que onda
            }
        
            char processname[FILE_NAME_MAX_LEN + 1];
            //DEBUG('e', "%d", sizeof processname);
            if (!ReadStringFromUser(processAddr,
                                    processname, sizeof processname)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, 1);
            }

            if (argvAddr != 0) {
                DEBUG('e', "argv distinto de cero.\n");
                argv = SaveArgs(argvAddr);
            }
            char filename[30];
            sprintf(filename, "userland/%s", processname);

            DEBUG('e', "el nombre del archivo a abrir es %s\n", filename);

            StartPParam param = (StartPParam)malloc(sizeof(struct _param));
            param->name = filename;
            param->argv = argv;
            //se podria hacer mas robusto checkeando mas cosas....
            
            Thread *newThread = new Thread("newProcess", true);

            SpaceId spaceId = (SpaceId)runningProcesses->Add(newThread); 

            newThread->Fork(StartProcess, (void *) param);
            currentThread->Yield(); //esto no estoy seguro, perdon si está mal yay

            //hacer tabla para meter con key= spaceId el thread, entonces cuando hago join hago join de esa key.
            machine->WriteRegister(2, spaceId);

            free(param);            
            break;
        }

        case SC_JOIN: {
            SpaceId spaceId = machine->ReadRegister(4);

            if(! runningProcesses->HasKey(spaceId)){
                DEBUG('e',"Error en Join: id del proceso inexistente");
                machine->WriteRegister(2,-1);
            }

            Thread* process = runningProcesses->Get(spaceId);
            int exitStatus = process->Join();

            delete (process->space); // liebramos la memoria fisica del proceso

            machine->WriteRegister(2, exitStatus);

            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, 1);
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, 1);
            }

            if(!fileSystem->Create(filename, 0)){
                DEBUG('e', "Error: File not created.\n");
                machine->WriteRegister(2, 1);
            }
            machine->WriteRegister(2, 0);

            DEBUG('e', "File `%s` created.\n", filename);
            break;
        }

        case SC_REMOVE:{
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, 1);
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, 1);
            }
            if(!fileSystem->Remove(filename)){
                DEBUG('e', "Error: File not removed.\n");
                machine->WriteRegister(2, 1);
            }

            machine->WriteRegister(2, 0);
            DEBUG('e', "File `%s` removed.\n", filename);
            break;

        }


        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);

            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
            }

            char filename[FILE_NAME_MAX_LEN + 1];

            if (!ReadStringFromUser(filenameAddr, filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                    FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
            }

            OpenFile* openedFile = fileSystem->Open(filename);

            if(openedFile == nullptr){
                DEBUG('e', "Error: File not opened.\n");
                machine->WriteRegister(2, -1);
            }
            
            int fileDescriptor = currentThread->GetOpenedFilesTable()->Add(openedFile);
            if(fileDescriptor < 0) {
                DEBUG('e', "Error: File not added to table");
                machine->WriteRegister(2, -1);
            }
            
            machine->WriteRegister(2, fileDescriptor);
            DEBUG('e', "File opened successfully!. Index assigned: %d \n", fileDescriptor);
    
            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);

            if(currentThread->GetOpenedFilesTable()->HasKey(fid)) {
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
                DEBUG('e', "Error: address string is null.\n");
                machine->WriteRegister(2, 0);
            }    
            if (nbytes <= 0){
                DEBUG('e', "Error: invalid number of bytes.\n");
                machine->WriteRegister(2, 0);
            }
            if(fid < 0){
                DEBUG('e', "Invalid file descriptor ID \n");    //to avoid the assert in Remove function
                machine->WriteRegister(2, 0);
            }

            char buffer[nbytes + 1];

            if(fid == CONSOLE_INPUT) {
                SynchConsole* console = new SynchConsole(nullptr, nullptr);
                for(int i = 0; i < nbytes; i++) {
                    buffer[i] = console->ReadConsole();
                }
                
                //delete console; adonde hacer esto??
                WriteBufferToUser(buffer, usrStringAddr, nbytes);
                machine->WriteRegister(2, nbytes);
                break;
            } else if(!currentThread->GetOpenedFilesTable()->HasKey(fid)) {
                DEBUG('e', "Error in read: Not an opened file\n");
                machine->WriteRegister(2, 0);
            } else {
                OpenFile* file = currentThread->GetOpenedFilesTable()->Get(fid);
        
                int bytesReaded = file->Read(buffer, nbytes);

                printf("Readed: %s, nrobytes: %d de %d, fromfileid: %d, fileaddr: %p\n", buffer, bytesReaded, nbytes, fid, file);

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
                DEBUG('e', "Error: address string is null.\n");
                machine->WriteRegister(2, 0);
            }    
            if (nbytes <= 0){
                DEBUG('e', "Error: invalid number of bytes.\n");
                machine->WriteRegister(2, 0);
            }
            if(fid < 0){
                DEBUG('e', "Invalid file descriptor ID \n");    //to avoid the assert in Remove function
                machine->WriteRegister(2, 0);
            }
            
            char buffer[nbytes + 1];
            ReadBufferFromUser(usrStringAddr, buffer, nbytes);
            printf("Write: %s, nrobytes: %d, hacia: %d\n", buffer, nbytes, usrStringAddr);
            
            if(fid == CONSOLE_OUTPUT) {
                SynchConsole* console = new SynchConsole(nullptr, nullptr);
                for(int i = 0; i < nbytes; i++) {
                    console->WriteConsole(buffer[i]);
                }
                
                //delete console; adonde hacer esto??
                machine->WriteRegister(2, nbytes);
                break;
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


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}

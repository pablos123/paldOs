#include "synch_console.hh"
#include <stdio.h>


static void
ReadAvail(void *arg)
{
    ASSERT(arg != nullptr);
    SynchConsole *console = (SynchConsole *) arg;
    console->ReadAvailSynch();
}

static void
WriteDone(void *arg)
{
    ASSERT(arg != nullptr);
    SynchConsole *console = (SynchConsole *) arg;
    console->WriteDoneSynch();
}

/// Disk interrupt handler.  Wake up any thread waiting for the disk
/// request to finish.
void
SynchConsole::ReadAvailSynch()
{
    readAvail->V();
}

/// Disk interrupt handler.  Wake up any thread waiting for the disk
/// request to finish.
void
SynchConsole::WriteDoneSynch()
{
    writeDone->V();
}


/// Initialize a SynchConsole, 
SynchConsole::SynchConsole(const char *readFile, const char *writeFile)
{
    readAvail = new Semaphore("synch read console", 0);
    writeDone = new Semaphore("synch write console", 0);
    lock = new Lock("synch console");
    console = new Console(readFile, writeFile, ReadAvail, WriteDone, this);
}

/// De-allocate data structures needed for the console abstraction.
SynchConsole::~SynchConsole()
{
    delete console;
    delete lock;
    delete readAvail;
    delete writeDone;
}

/// Read the contents of a console into a buffer.  Return only after the
/// data has been read.
char
SynchConsole::ReadConsole()
{
    readAvail->P(); //wait until there is something to be read
    lock->Acquire();  // only one thread can be reading of the console
    char character = console->GetChar();
    //writeDone->P();   // Wait for interrupt. //capaz que este no es necesario porque no me interesa el aoutput al toque
    lock->Release();

    return character;
}

/// Write the contents of a buffer in the console.  Return only
/// after the data has been written.
///
/// `ch` is the character to be written.
void
SynchConsole::WriteConsole(char ch)
{
    //ASSERT(ch != nullptr);

    //lock->Acquire();  // not only one disk I/O at a time: en este caso un hilo queriendo escribir no deberia bloquear a un hilo queriendo leer.
    console->PutChar(ch);
    writeDone->P();   // wait for interrupt
    //lock->Release();
}

/// Gets the private attribute console
Console*
SynchConsole::GetConsole()
{
    return console;
}
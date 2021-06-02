#ifndef NACHOS_USERPROG_SYNCHCONSOLE__HH
#define NACHOS_USERPROG_SYNCHCONSOLE__HH


#include "machine/console.hh"
#include "threads/lock.hh"
#include "threads/semaphore.hh"

class SynchConsole {
public:
   
    SynchConsole(const char *readFile, const char *writeFile);

    ~SynchConsole();

    char ReadConsole();
    void WriteConsole(char ch);

    void ReadAvailSynch();
    void WriteDoneSynch();

    Console* GetConsole();

private:
    Console *console;
    Semaphore *readAvail;
    Semaphore *writeDone;
    Lock *lockRead;
    Lock *lockWrite;
};

#endif 
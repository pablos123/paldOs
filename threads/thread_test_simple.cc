/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"
#include "semaphore.hh"
#include "lib/utility.hh"

#include <stdio.h>
#include <string.h>

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);
}

void
SimpleThreadSemaphore(void *semaphore_param) 
{
    // Reinterpret arg `name` as a string.
    char *name = (char *)((SemaphoreParam)semaphore_param)->debugName;
    Semaphore* semaphore = (Semaphore *)((SemaphoreParam)semaphore_param)->semaphore;
    
    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.

    for (unsigned num = 0; num < 10; num++) {
        DEBUG('s', "*** Thread `%s` is going to call P()\n", name);
        semaphore->P();
        printf("*** Thread `%s` is running with semaphores: iteration %u\n", name, num);
        semaphore->V();
        currentThread->Yield();
        DEBUG('s', "*** Thread `%s` called V()\n", name);
    }
    
    
    printf("!!! Thread `%s` has finished\n", name);
}



/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
char *name2 = new char [64], *name3 = new char [64],
*name4 = new char [64], *name5 = new char [64];

#ifndef SEMAPHORE_TEST
    strncpy(name2, "2nd", 64);
    Thread *newThread = new Thread(name2);
    newThread->Fork(SimpleThread, (void *) name2);

    strncpy(name3, "3nd", 64);
    Thread *newThread3 = new Thread(name3);
    newThread3->Fork(SimpleThread, (void *) name3);

    strncpy(name4, "4nd", 64);
    Thread *newThread4 = new Thread(name4);
    newThread4->Fork(SimpleThread, (void *) name4);

    strncpy(name5, "5nd", 64);
    Thread *newThread5 = new Thread(name5);
    newThread5->Fork(SimpleThread, (void *) name5);

    SimpleThread((void *) "1st");
#endif

#ifdef SEMAPHORE_TEST     
    char* name = new char [64];
    Semaphore* semaphore = new Semaphore("Semaforo test", 3);

    strncpy(name2, "2nd", 64);
    Thread *newThread = new Thread(name2);
    SemaphoreParam param2 = SemaphoreParamConstructor((void*)name2, semaphore);
    newThread->Fork(SimpleThreadSemaphore, (void *) param2);

    strncpy(name3, "3nd", 64);
    Thread *newThread3 = new Thread(name3);
    SemaphoreParam param3 = SemaphoreParamConstructor((void*)name3, semaphore);
    newThread3->Fork(SimpleThreadSemaphore, (void *) param3);

    strncpy(name4, "4nd", 64);
    Thread *newThread4 = new Thread(name4);
    SemaphoreParam param4 = SemaphoreParamConstructor((void*)name4, semaphore);
    newThread4->Fork(SimpleThreadSemaphore, (void *) param4);

    strncpy(name5, "5nd", 64);
    Thread *newThread5 = new Thread(name5);
    SemaphoreParam param5 = SemaphoreParamConstructor((void*)name5, semaphore);
    newThread5->Fork(SimpleThreadSemaphore, (void *) param5);

    strncpy(name, "1st", 64);
    SemaphoreParam param1 = SemaphoreParamConstructor((void *)name, semaphore);
    SimpleThreadSemaphore((void *) param1);

    SemaphoreParamDestructor(param1);
    SemaphoreParamDestructor(param2);
    SemaphoreParamDestructor(param3);
    SemaphoreParamDestructor(param4);
    SemaphoreParamDestructor(param5);
#endif
}
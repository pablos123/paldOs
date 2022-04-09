/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"
#include "semaphore.hh"
#include "condition.hh"
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
    const char *name = ((SemaphoreParam*)semaphore_param)->GetName();
    Semaphore* semaphore = ((SemaphoreParam*)semaphore_param)->GetSemaphore();

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
char *name1 = new char [8];
strncpy(name1, "1nd", 8);

char *name2 = new char [8];
strncpy(name2, "2nd", 8);

char *name3 = new char [8];
strncpy(name3, "3nd", 8);

char *name4 = new char [8];
strncpy(name4, "4nd", 8);

char *name5 = new char [8];
strncpy(name5, "5nd", 8);


#ifndef SEMAPHORE_TEST
    Thread *newThread = new Thread(name2);
    newThread->Fork(SimpleThread, (void *) name2);

    Thread *newThread3 = new Thread(name3);
    newThread3->Fork(SimpleThread, (void *) name3);

    Thread *newThread4 = new Thread(name4);
    newThread4->Fork(SimpleThread, (void *) name4);

    Thread *newThread5 = new Thread(name5);
    newThread5->Fork(SimpleThread, (void *) name5);

    SimpleThread((void *) name1);
#else
    Semaphore* semaphore = new Semaphore("Semaforo test", 3);

    Thread *newThread2 = new Thread(name2);
    SemaphoreParam* param2 = new SemaphoreParam(name2, semaphore);
    newThread2->Fork(SimpleThreadSemaphore, (void *) param2);

    Thread *newThread3 = new Thread(name3);
    SemaphoreParam* param3 = new SemaphoreParam(name3, semaphore);
    newThread3->Fork(SimpleThreadSemaphore, (void *) param3);

    Thread *newThread4 = new Thread(name4);
    SemaphoreParam* param4 = new SemaphoreParam(name4, semaphore);
    newThread4->Fork(SimpleThreadSemaphore, (void *) param4);

    Thread *newThread5 = new Thread(name5);
    SemaphoreParam* param5 = new SemaphoreParam(name5, semaphore);
    newThread5->Fork(SimpleThreadSemaphore, (void *) param5);

    SemaphoreParam* param1 = new SemaphoreParam(name1, semaphore);
    SimpleThreadSemaphore((void *) param1);

    delete param5;
    delete param4;
    delete param3;
    delete param2;
    delete param1;
    delete semaphore;
#endif

    delete [] name1;
    delete [] name2;
    delete [] name3;
    delete [] name4;
    delete [] name5;
}

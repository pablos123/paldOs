/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "condition.hh"
#include "system.hh"


/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Condition::Condition(const char* debugName, Lock* lock)
{
    name = debugName;
    conditionLock = lock;

    queue = new List<Semaphore *>;
}

/// We do not free the memory of the lock here
Condition::~Condition()
{
    DEBUG('t', "Removing condition.\n");
    while(! queue->IsEmpty())
        delete queue->Pop();
    delete queue;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    DEBUG('t', "Thread %s waiting...\n", currentThread->GetName());
    ASSERT(conditionLock->IsHeldByCurrentThread());

    Semaphore* new_semaphore = new Semaphore("dummy", 0); // We need to sleep right away

    queue->Append(new_semaphore);

    conditionLock->Release(); // Yielding thread...
    new_semaphore->P(); // Go to sleep

    conditionLock->Acquire();
}

void
Condition::Signal()
{
    ASSERT(conditionLock->IsHeldByCurrentThread());
    if(queue->IsEmpty()) return;
    Semaphore* semaphore = queue->Head();
    semaphore->V();
}

void
Condition::Broadcast()
{
    while(!queue->IsEmpty())
        Signal();
}

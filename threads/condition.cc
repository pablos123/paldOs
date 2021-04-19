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


/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Condition::Condition(const char* debugName, Lock* lock)
{
    this->name = debugName;
    this->conditionLock = lock;
    this->queue = new List<Semaphore *>;
}

Condition::~Condition()
{
    this->queue->~List();
    delete this;
}

const char *
Condition::GetName() const
{
    return name;
}

Lock*
Condition::GetLock() {
    return conditionLock;
}


void
Condition::Wait()
{
    ASSERT(conditionLock->IsHeldByCurrentThread()); //tiene que tener agarrado el candado
    //conditionLock->Release(); //si es dueño del candado entonces lo libera
    Semaphore* new_semaphore = new Semaphore("dummy", 0); //esto lo iniciamos en 0 para dormir.
    
    queue->Append(new_semaphore);
    
    conditionLock->Release(); //aca hago el yield que es lo mismo //si no hago el yield en release esto es un bardo
    new_semaphore->P(); //lo mandamos a dormir
    
    //ver si borrar acá el semaforo o en el destructor creo que ya esta xddx
    
    conditionLock->Acquire();
}

void
Condition::Signal()
{
    ASSERT(conditionLock->IsHeldByCurrentThread());
    if(queue->IsEmpty()) return;
    Semaphore* semaphore = queue->Pop();
    semaphore->V();
}

void
Condition::Broadcast()
{
    Semaphore* sem;
    for(; sem != nullptr; sem = queue->Pop()) {
        sem->V();
    }
}

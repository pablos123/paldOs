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

#include "lock.hh"
#include "system.hh"
#include <stdlib.h>

/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    this->name = debugName; //este name no me gusta mucho peeeero, depende tamben cuanto usemos este name no??
                            //entonces tenemos la discusion de si menos memoria por no poner el name o mayor
                            //reutilizacion al ponerlo.

    this->lock = new Semaphore(debugName, 1); //ya lo tenemos aca me parece.

    this->lockOwner = nullptr; //nadie es dueño del lock... todavia

} //esto parece que tambien pero no me acuerdo 

Lock::~Lock()
{
    delete this->lock;
    delete this;
}

const char *
Lock::GetName() const //accedo mas facil si existe la propiedad.
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(!this->IsHeldByCurrentThread());
    this->lock->P(); //Primero resto el semaforo dado que puede pasar que no pueda entrar pero cambio el dueño igual
                    //therefore hay alta explotacion con el assert
    this->lockOwner = currentThread;
}

void
Lock::Release()
{
    ASSERT(this->IsHeldByCurrentThread());
    this->lock->V();
    this->lockOwner = nullptr; //esto es representativo para la funcion del lock :)ç
                               //aunque no es necesario. Si es necesario! :D aldu la mas grande
    //currentThread->Yield(); lo sacamos ya que en las variables de condicion, en Wait() se llama a
    // P() y esta cuando hace Sleep() ya hace un relinquish/yield del CPU. Entonces,
    //cuando hacemos Realease() no deberíamos "duplicar" este relinquish
}

bool
Lock::IsHeldByCurrentThread() const
{
    return this->lockOwner == currentThread;
}

///Lock param library

LockParam LockParamConstructor(void* debugName, Lock* lock) {
    LockParam lockParam = (LockParam)malloc(sizeof(struct _lockParam));
    lockParam->debugName = debugName;
    lockParam->lock = lock;
    
    return lockParam;
}


void LockParamDestructor(LockParam lockParam) {
    free(lockParam);
    return;
}
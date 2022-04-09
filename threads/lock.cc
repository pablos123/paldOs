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
    name = debugName; //este name no me gusta mucho peeeero, depende tamben cuanto usemos este name no??
                            //entonces tenemos la discusion de si menos memoria por no poner el name o mayor
                            //reutilizacion al ponerlo.

    lock = new Semaphore(debugName, 1); //ya lo tenemos aca me parece.

    lockOwner = nullptr; //nadie es dueño del lock... todavia

    oldPriority = 0;

} //esto parece que tambien pero no me acuerdo

Lock::~Lock()
{
    delete lock;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(! this->IsHeldByCurrentThread());
#ifdef MULTILEVEL_PRIORITY_QUEUE
/*
    Herencia de prioridades:
    obtengo la propiedad del proceso de mayor prioridad
    guardo la prioridad del actual (el de baja prioridad, por ej)
    guardo dentro de la prioridad del actual como prioridad mayor
    antes del release bajo la prioridad del actual y la dejo como la que tenía originalmente

*/
    size_t ownerPriority;
    if(lockOwner == nullptr) {
        ownerPriority = currentThread->GetPriority();
    } else {
        ownerPriority = lockOwner->GetPriority();
    }
    size_t currentPriority = currentThread->GetPriority();
    if(ownerPriority < currentPriority){
        oldPriority = ownerPriority;
        lockOwner->SetPriority(currentPriority);   //le subimos al de baja prioridad para que termine de usar el lock mas rapido
        scheduler->ModifyPriority(lockOwner);
    }
#endif

    lock->P(); //Primero resto el semaforo dado que puede pasar que no pueda entrar pero cambio el dueño igual
               //therefore hay alta explotacion con el assert
    lockOwner = currentThread;
}

void
Lock::Release()
{
    ASSERT(this->IsHeldByCurrentThread());

    lockOwner = nullptr;//esto es representativo para la funcion del lock :)
                        //aunque no es necesario. Si es necesario! :D aldu la mas grande
    //currentThread->Yield(); lo sacamos ya que en las variables de condicion, en Wait() se llama a
    // P() y esta cuando hace Sleep() ya hace un relinquish/yield del CPU. Entonces,
    //cuando hacemos Realease() no deberíamos "duplicar" este relinquish
#ifdef MULTILEVEL_PRIORITY_QUEUE
    if(currentThread->GetPriority() != oldPriority){
        currentThread->SetPriority(oldPriority);
        scheduler->ModifyPriority(currentThread);
    }
#endif

    lock->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return lockOwner == currentThread;
}

//--------------LockParam---------------//
LockParam::LockParam(const char* paramName, Lock* lockParam, void* optionalParam) {
    name = paramName;

    lock = lockParam;

    optional = optionalParam;
}

LockParam::~LockParam(){}

//------------Getters---------------//
const char*
LockParam::GetName() {
    return name;
}

Lock*
LockParam::GetLock() {
    return lock;
}

void*
LockParam::GetOptional() {
    return optional;
}

/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_garden.hh"
#include "system.hh"
#include "semaphore.hh"
#include "lock.hh"
#include <stdio.h>


static const unsigned NUM_TURNSTILES = 5;
static const unsigned ITERATIONS_PER_TURNSTILE = 5;
static bool done[NUM_TURNSTILES];
static int count;

static void
Turnstile(void *n_)
{
    unsigned *n = (unsigned *) n_;

    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
        int temp = count;
        count = temp + 1;
        currentThread->Yield();  
    }
    printf("Turnstile %u finished. Count is now %u.\n", *n, count);
    done[*n] = true;
    delete n;
}

void
ThreadTestGarden()
{
    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        DEBUG('t', "Launching turnstile %u.\n", i);
        char *name = new char [16];
        sprintf(name, "Turnstile %u", i);
        unsigned *n = new unsigned;
        *n = i;
        Thread *t = new Thread(name);
        t->Fork(Turnstile, (void *) n);
    }

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}


static void
TurnstileSemaphore(void *param)
{
    unsigned *n = (unsigned *) ((SemaphoreParam) param)->debugName;
    Semaphore* semaphore = (Semaphore *)((SemaphoreParam) param)->semaphore;


    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
        int temp = count;
        semaphore->P(); //sleep() del actual si el value es 0
        count = temp + 1;
        semaphore->V(); //marca el ultimo hilo de la pila como ready
        currentThread->Yield(); //es necesario pasarle el control a otro hilo, en este caso en particular
                                //si no lo hacemos, luego en la función llamante se hace con el join ad-hoc
    }

    //Ejemplo lindo para ver el comportamiento de los semaforos, en particular
    //el P(), con la llamada a sleep() que ejecutara otro hilo si el actual se va a dormir
    // for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
    //     semaphore->P();
    //     currentThread->Yield();
    //     int temp = count;
    //     count = temp + 1;
    //     printf("Turnstile %u running...\n", *n);
    //     semaphore->V();
    // }

    printf("Turnstile %u finished. Count is now %u.\n", *n, count);
    done[*n] = true;
    delete n;
}

void
ThreadTestGardenSemaphore()
{
    Semaphore* semaphore = new Semaphore("Semaforo test", 1);

    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char [16];
        sprintf(name, "Turnstile %u", i);

        unsigned *n = new unsigned;
        *n = i;
        Thread *t = new Thread(name);

        SemaphoreParam param = SemaphoreParamConstructor((void*)n, semaphore);        
        t->Fork(TurnstileSemaphore, (void*)param);
    }

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}


static void
TurnstileLocks(void* lockParam)
{
    unsigned *n = (unsigned *) ((LockParam) lockParam)->debugName;
    Lock* lock = (Lock *)((LockParam) lockParam)->lock;

    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {

        lock->Acquire();
        int temp = count; //No entendemos por que no funciona si sacamos esto fuera del candado
        count = temp + 1;
        printf("Turnstile %u, %d\n", *n, count);
        lock->Release();
        currentThread->Yield();
    }
    printf("Turnstile %u finished. Count is now %u.\n", *n, count);
    done[*n] = true;
    delete n;
}


void ThreadTestGardenLocks() {

    Lock* lock = new Lock("Lock test");

    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char [16];
        sprintf(name, "Turnstile %u", i);

        unsigned *n = new unsigned;
        *n = i;
        Thread *t = new Thread(name);

        LockParam param = LockParamConstructor((void*)n, lock);        
        t->Fork(TurnstileLocks, (void*)param);
    }

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);

}
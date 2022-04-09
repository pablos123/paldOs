/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_prod_cons.hh"
#include "condition.hh"
#include "system.hh"
#include <stdio.h>

/*****************************************************************************************
///////////////////////////////////TEST PROD CONS/////////////////////////////////////////
******************************************************************************************/

#define MAX 2
#define CONSUMERS 4
#define LOOPS 10

int* buffer = nullptr;
int fillIndex = 0, useIndex = 0, total = 0, count = 0;

Lock* mutex = new Lock("mutex");

Condition* empty = new Condition("emptyCondition", mutex);
Condition* full = new Condition("fullCondition", mutex);

void ThreadTestProdCons() {
    /* Allocate space for the buffer*/
    buffer = new int[MAX];

    //Create the producer
    Thread* prod = new Thread("producer", true);
    prod->Fork(producer, nullptr);

    //Create the CONSUMERS
    Thread** cons      = new Thread*[CONSUMERS];
    int**  consumables = new int*[CONSUMERS];
    char** names       = new char*[CONSUMERS];

    for(int i = 0; i < CONSUMERS; i++){
        char* name = new char[10];
        sprintf(name, "Consumer %d", i);

        int* n = new int;
        *n = i;

        consumables[i] = n;
        names[i] = name;

        cons[i] = new Thread(name, true);
        cons[i]->Fork(consumer, (void*)n);
    }


    //Wait for all the threads to finish
    prod->Join();
    for(int i = 0; i < CONSUMERS; i++){
        cons[i]->Join();
    }

    for(int i = 0; i < CONSUMERS; ++i) delete consumables[i];
    delete [] consumables;

    for(int i = 0; i < CONSUMERS; ++i) delete [] names[i];
    delete [] names;

    delete [] buffer;
    delete [] cons;

    delete mutex;
    delete empty;
    delete full;

    printf("All CONSUMERS finished. Final count is %d (should be %d), all products were consumed!!!.\n",
           total, LOOPS);

    return;
}


/* Buffer Operations */
void put(int value) {
    buffer[fillIndex] = value;
    fillIndex = (fillIndex + 1) % MAX;
    count++;
    printf("Created the product %d\n", value);
}

int get() {
    int tmp = buffer[useIndex];
    useIndex = (useIndex + 1) % MAX;
    count--;
    return tmp;
}


void producer (void * arg) {
    for(int i = 0; i < LOOPS; i++) {
        mutex->Acquire();
        while(count == MAX)
            empty->Wait();
        put(i);

        full->Signal();

        mutex->Release();
    }
}

void consumer(void * arg) {
    int* Ncon = (int*)arg;
    for(int i = 0; i < LOOPS; i++){
        mutex->Acquire();

        while(count == 0) {
            printf("El consumidor %d entro a esperar\n", *Ncon);
            full->Wait();
        }

        int tmp = get();
        empty->Signal();

        //Count the consumed product
        total++;

        printf("Producto %d consumido por %d\n", tmp, *Ncon);
        mutex->Release();
    }
}

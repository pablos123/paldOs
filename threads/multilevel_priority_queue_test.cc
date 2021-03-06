#include "multilevel_priority_queue_test.hh"
#include "thread.hh"
#include "system.hh"
#include "condition.hh"
#include <stdio.h>

void foo(void* v) {
    for(int i = 0; i < 5; ++i){
        printf("Ejecutando el hilo %s con prioridad: %ld\n",
            currentThread->GetName(), currentThread->GetPriority());
    }

    currentThread->Yield();
    return;
}


void MultilevelPriorityQueueTest() {
    Thread** threads = new Thread*[8];
    char** names = new char*[8];

    threads[0] = new Thread("bigpriority", true, 30);
    threads[0]->Fork(foo, nullptr);

    for(int i = 1; i < 6; ++i) {
        char *name = new char[16];
        sprintf(name, "p%d", i);
        Thread* newThread = new Thread(name, true, i);
        printf("Lanzando el hilo %s, con prioridad %d\n", name, i);
        newThread->Fork(foo, nullptr);
        threads[i] = newThread;
        names[i - 1] = name;
    }

    threads[6] = new Thread("lowpriorityafter", true, 3);
    threads[6]->Fork(foo, nullptr);
    threads[7] = new Thread("bigpriority", true, 37);
    threads[7]->Fork(foo, nullptr);

    for(int i = 0; i < 8; ++i)
        threads[i]->Join();

    for(int i = 0; i < 5; ++i)
        delete names[i];

    delete [] names;

    delete [] threads;

    return;
};

#include "test.hh"
#include "thread.hh"
#include "system.hh"
#include "condition.hh"
#include <stdio.h>

Lock* lockDeTestPrioridad = new Lock("lockPrioridad");
Condition* conditionTestPrioridad = new Condition("conditionLocko", lockDeTestPrioridad);

void foo(void* v) {
    for(int i = 0; i < 5; ++i){
        DEBUG('t', "Ejecutando el hilo con prioridad: %ld\n", currentThread->GetPriority());
        printf("Ejecutando el hilo %s con prioridad: %ld\n", currentThread->GetName(), currentThread->GetPriority());
    }

    return;
}

void MultilevelPriorityQueueTest() {
    Thread* threads[5];
    for(int i = 0; i < 5; ++i) {
        char *name = new char[16];
        sprintf(name, "p%d", i);
        Thread* newThread = new Thread(name, true, i);
        printf("Lanzando el hilo %s, con prioridad %d\n", name, i);
        newThread->Fork(foo, nullptr);
        threads[i] = newThread;
    }

    for(int i = 0; i < 5; ++i) {
        threads[i]->Join();
    }
    
    return;
};


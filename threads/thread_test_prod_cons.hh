#ifndef NACHOS_THREADS_THREADTESTPRODCONS__HH
#define NACHOS_THREADS_THREADTESTPRODCONS__HH

void ThreadTestProdCons();

void producer (void * arg);

void consumer (void * arg);

void put(int value);

int get();

#endif

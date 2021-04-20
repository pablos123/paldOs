/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "condition.hh"
#include "system.hh"
#include "channel.hh"

#include <stdio.h>
#include <stdlib.h>

typedef struct _channelParam {
    Channel* channel;
    int i;
}* ChannelParam;

typedef struct _channelParam2 {
    Channel* channel;
    int* i;
}* ChannelParam2;

static const unsigned MAX = 5;

/*::::::::::::::VARIABELS:::::::::::::::::::::::*/
size_t loops = MAX, consumers = MAX, products = 0;
int buffer[MAX];
int fillIndex = 0, useIndex = 0, count = 0;
static bool done[MAX]; //pongo el mas uno porque el productor tambien es un hilo 

Lock* lock = new Lock("CondLock"); 
Condition* cond_var = new Condition("CondVariable", lock);

/*:::::::::::::::::::::::::::::::::::::*/

static int get() {
    int tmp = buffer[useIndex];
    useIndex = (useIndex + 1) % MAX;
    printf("useIndex: %d\n", useIndex);
    count--;
    return tmp;
}

static void put(int value){
    buffer[fillIndex] = value;
    fillIndex = (fillIndex + 1) % MAX; 
    count++;
}


void Producer(void* v) {
    for(size_t i = 0; i < loops; i++){
        cond_var->GetLock()->Acquire();
        if(count == MAX) {
            cond_var->Wait();
        }
        put(i);
        printf("Insertando %ld con el productor\n", i);
        cond_var->Signal();
        cond_var->GetLock()->Release();  
        currentThread->Yield();      
    }
}

void Consumer(void* v) {
    size_t* consumerNumber = (size_t*)v;
    
    for (size_t i = 0; i < loops; i++) { //chequeo todos los productos que puedo llegar a agarrar
        cond_var->GetLock()->Acquire();
        if (count == 0) {
            printf("El cosumidor %ld entro a esperar\n", *consumerNumber);
            cond_var->Wait();
        }
        int tmp = get();
        printf("Producto %d consumido por el consumidor %ld \n", tmp, *consumerNumber);
        products++;
         if (products == MAX) {     
            for (size_t j = 0; j < consumers; j++) done[j] = true;
            return;
        }

        cond_var->Signal();
        cond_var->GetLock()->Release();
        currentThread->Yield();
    }
    done[*consumerNumber] = true; //este me interesa porque quiero saber si termine de chequear
    return;
}


void
ThreadTestProdCons()
{
    
    Thread* producer = new Thread("producer");
    producer->Fork(Producer, (void*)"dummyparam");

    for (size_t i = 0; i < consumers; i++) {
        printf("Launching consumer %lu.\n", i);
        char *name = new char [16];
        sprintf(name, "Consumer %lu", i);

        Thread *consumer = new Thread(name);
        size_t * caca = (size_t*)malloc(sizeof(size_t));
        *caca = i;
        consumer->Fork(Consumer, (void*)caca);
    }
    
    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (size_t i = 0; i < consumers; i++) {
        while (!done[i]) {
            currentThread->Yield(); //aca recien va a parar a producer
        }
    }
    printf("\nAll consumers finish consuming :D\n");
}



void senderTest(void* param) {
    ChannelParam channelParam = (ChannelParam)param;
    Channel* channel = (Channel*)(channelParam->channel);
    channel->Send(channelParam->i);
}

void receiverTest(void* param) {
    ChannelParam2 channelParam = (ChannelParam2)param;
    Channel* channel = (Channel*)(channelParam->channel);
    channel->Receive(channelParam->i);
}


void ThreadTestChannel() {
    int i = 0;
    printf("Mandando mensaje: %d\n", i);
    Channel* channel = new Channel("canal1");
    
    Thread* receiver = new Thread("Hilo1", true); 
    Thread* sender = new Thread("Hilo2", true); 
    
    ChannelParam param = (ChannelParam)malloc(sizeof(struct _channelParam));
    param->channel = channel;
    param->i = i;
    sender->Fork(senderTest, (void*) param);
    
    int* j = (int*)malloc(sizeof(int));
    *j = 123;
    ChannelParam2 param2 = (ChannelParam2)malloc(sizeof(struct _channelParam2));
    param2->channel = channel;
    param2->i = j;
    receiver->Fork(receiverTest, (void*) param2);

    sender->Join();
    receiver->Join(); ///saque un join  porque uno de los dos esta esperando y entonces con el otro quiero mandarle la señal :D
                        //si voy a esperar entonces no me sirve, un owr around podría ser, agarrar y fijarme si esta
    
    printf("Recibiendo mensaje: %d\n", *j);
    free(j);

    return;
}
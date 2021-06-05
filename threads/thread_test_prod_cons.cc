/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "condition.hh"
#include "system.hh"
#include "channel.hh"
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************************************
///////////////////////////////////TEST PROD CONS/////////////////////////////////////////
******************************************************************************************/
#define max 2
#define consumers 4
#define loops 10

int * buffer;
int fillIndex, useIndex, total, count = 0;


/************************
  Function declarations
*************************/
void producer (void * arg);
void consumer (void * arg);
static void put(int value);
static int get();
/**************************************/


void ThreadTestProdCons() {
    /* Allocate space for the buffer*/
    buffer = (int*)malloc(sizeof(int) * max);

    //Create the producer
    Thread* prod = new Thread("producer", true);
    prod->Fork(producer, nullptr);

    //Create the consumers
    Thread* cons[consumers];      
    for(int i = 0; i < consumers; i++){
        char name[10];
        sprintf(name, "con%d", i);
        int* n = (int*)calloc(1, sizeof(int));
        *n = i;
        cons[i] = new Thread(name, true);
        cons[i]->Fork(consumer, (void*)n);
    }

    //Wait for all the threads to finish
    prod->Join();
    for(int i = 0; i < consumers; i++){
        cons[i]->Join();
    }
    
    printf("All consumers finished. Final count is %d (should be %d), all products were consumed!!!.\n",
           total, loops);

    return;
}


/* Buffer Operations */
static void put(int value) {
    buffer[fillIndex] = value;
    fillIndex = (fillIndex + 1) % max;
    count++;
}

static int get() {
    int tmp = buffer[useIndex];
    useIndex = (useIndex + 1) % max;
    count--;
    return tmp;
}

Lock* mutex = new Lock("mutex");

Condition* empty = new Condition("emptyCondition", mutex);
Condition* full = new Condition("fullCondition", mutex);

void producer (void * arg) {
    for(int i = 0; i < loops; i++) {
        printf("Created the product %d\n", i);
        mutex->Acquire();
        while(count == max)
            empty->Wait();
        put(i);

        full->Signal();
        
        mutex->Release();
    }
    return;
}

void consumer (void * arg) {
    int* Ncon = (int*)arg;
    for(int i = 0; i < loops; i++){
        mutex->Acquire();

        while(count == 0) {
            printf("El consumidor %d entro a esperar\n", *Ncon);
            full->Wait();
        }

        int tmp = get();
        empty->Signal();

        //Count the consumed product
        total++;

        mutex->Release();
        printf("Producto %d consumido por %d\n", tmp, *Ncon);
    }
    return;
}

/*****************************************************************************************
///////////////////////////////////TEST CHANNEL///////////////////////////////////////////
******************************************************************************************/

typedef struct _channelParam {
    Channel* channel;
    int i;
}* ChannelParam;

typedef struct _channelParam2 {
    Channel* channel;
    int* i;
}* ChannelParam2;

static void senderTest(void* param) {
    ChannelParam channelParam = (ChannelParam)param;
    Channel* channel = (Channel*)(channelParam->channel);
    channel->Send(channelParam->i);
}

static void receiverTest(void* param) {
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
    receiver->Join();
    ///saque un join  porque uno de los dos esta esperando y entonces con el otro quiero mandarle la señal :D
    //si voy a esperar entonces no me sirve, un owr around podría ser, agarrar y fijarme si esta

    printf("Recibiendo mensaje: %d\n", *j);
    free(j);

    return;
}
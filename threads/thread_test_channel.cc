/*****************************************************************************************
///////////////////////////////////CHANNEL TEST///////////////////////////////////////////
******************************************************************************************/

#include "channel.hh"
#include "thread_test_channel.hh"
#include <stdio.h>

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
    int i = 123;
    printf("Mandando mensaje: %d\n", i);
    Channel* channel = new Channel("canal1");

    Thread* receiver = new Thread("Hilo1", true);
    Thread* sender = new Thread("Hilo2", true);

    ChannelParam param = new struct _channelParam;
    param->channel = channel;
    param->i = i;
    sender->Fork(senderTest, (void*) param);

    int* j = new int;
    *j = 0;
    ChannelParam2 param2 = new struct _channelParam2;
    param2->channel = channel;
    param2->i = j;
    receiver->Fork(receiverTest, (void*) param2);

    sender->Join();
    receiver->Join();

    printf("Recibiendo mensaje: %d\n", *j);

    delete j;

    delete param;
    delete param2;

    delete channel;

    return;
}

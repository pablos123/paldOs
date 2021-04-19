#include "channel.hh"

Channel::Channel(const char * name) {
    this->name = name;
}

Channel::~Channel() {
    delete this;
}

void Channel::Send(int message){


}

void Channel::Receive(int* message){


}
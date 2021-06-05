#include "channel.hh"

Channel::Channel(const char * debugName) 
{
    name = debugName;

    buzon = new List<int>;
    lock = new Lock(name);

    conditionForSenders = new Condition(name, lock);
    conditionForReceivers = new Condition(name, lock);
}

Channel::~Channel() {
    delete buzon;
    delete conditionForSenders; // not freeing the lock here
    delete conditionForReceivers;
    delete lock;
}

void Channel::Send(int message){
    lock->Acquire(); //tomamos el lock
    buzon->Append(message);
    conditionForReceivers->Signal();
    conditionForSenders->Wait(); //quiero esperar (y que otros senders no manden cosas) hasta que 
                                // se llame a Receive() y hagan el Pop del buffer en el otro lado
    lock->Release();
}

void 
Channel::Receive(int* message) //como usuario llamo a receive y espero que el resultado exista en la direccion de memoria de message
{
    //se llamo a receive, por lo tanto hacemos la signal aca

    lock->Acquire(); //tomamos el lock
    //aca podriamos chequear si esta vacio el buzon
    if(buzon->IsEmpty()){
        conditionForReceivers->Wait();  //lo mando a dormir hasta que un sender mande algo
    }

    *message = buzon->Pop();
    conditionForSenders->Signal(); //mando la señal al sender que esta esperando para seguir mandando

    lock->Release();
}
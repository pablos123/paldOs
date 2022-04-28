#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

//Esto define la calse Channel que implementa canales entre threads.
//La clase tiene un nombre y bla bla...

#include "condition.hh"

class Channel {
public:

    /// Constructor: set up the channel.
    Channel(const char *debugName);

    ~Channel();

    /// For debugging.
    const char *GetName() const;

    /// Operations on the channel.
    ///
    /// Both must be *atomic*.
    void Send(int message);
    void Receive(int *message);



private:

    /// For debugging.
    const char *name;
    List<int>* buzon;
    Lock* lock;
    Condition* conditionForReceivers;
    Condition* conditionForSenders;
};

#endif //NACHOS_THREADS_CHANNEL__HH
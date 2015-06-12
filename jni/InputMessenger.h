#ifndef MESSENGER
#define MESSENGER

#include "touch_vcr.h"
#include "Message.h"
#include <queue>

class InputMessenger {

public:
    InputMessenger();
    void send(Message msg);

    // All events are based off of android's monotonic clock.  Reset sends the timebase for all forthcoming
    // events, so that we can capture one set of events and replay them later
    void add_msg(Message msg);
    int dequeue(int32_t now, Message &msg);
    void fill_queue();
    bool isEmpty();

    void setInFD(int fd) { inFD = fd; };
    void setOutFD(int fd) { outFD = fd; };
private:
    std::queue<Message> msgQ;

    int inFD;
    int outFD;

    int32_t mMotionStart;
    int32_t mTimebase;
    
    static const int MAX_MSG_LENGTH = 200;
    char msgBuffer[MAX_MSG_LENGTH];
    int bufferIdx;

    void clear_buffer();
};

#endif

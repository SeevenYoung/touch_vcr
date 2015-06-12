#include "InputMessenger.h"
#include <algorithm>

extern bool VERBOSE;

// TODO allow alternate message formats
// TODO have clients construct messages and send them

InputMessenger::InputMessenger() {
    mTimebase = -1;
    mMotionStart = -1;
    bufferIdx = 0;
    clear_buffer();
}

void InputMessenger::send(Message msg) {
    // For now just dump.  Depending on config, we'll tell it to emit a different file format
    msg.dump(outFD);
}

void InputMessenger::add_msg(Message msg) {
    msgQ.push(msg);
    // TODO implement a limit here
}

void InputMessenger::clear_buffer() {
    for(int i = 0; i < MAX_MSG_LENGTH; i++ ) {
        msgBuffer[i] = 0;
    }
}

// TODO bail out with errors
void InputMessenger::fill_queue() {
    while(read(inFD,msgBuffer + bufferIdx,1) > 0) {
        if(msgBuffer[bufferIdx] == '\n') {
            msgBuffer[bufferIdx] = 0;
            bufferIdx = 0;
            Message msg;
            if( Message::fromString(std::string(msgBuffer), msg) ) {
                add_msg(msg);
                if(VERBOSE) printf( "Adding message %s\n", msgBuffer);
            } else {
                fprintf( stderr, "Failed to parse message %s\n", msgBuffer );
            }
        } else {
            bufferIdx += 1;
        }
    
        if(bufferIdx >= MAX_MSG_LENGTH-1) {
            fprintf(stderr, "Max message length exceeded, unable to parse %s", msgBuffer);
            bufferIdx = 0;
            clear_buffer();
            return;
        }
    }
    if(VERBOSE) fprintf(stderr, "Done filling queue\n");
}

// Returns the time until the next message
int InputMessenger::dequeue(int32_t now, Message &msg) {
    if( msgQ.empty() ) {
        return -1;
    }
    
    if(VERBOSE) printf("Pulling message %d\n", msg.getTimestamp());
    msg = msgQ.front();

    if(mMotionStart < 1) {
        mMotionStart = now;
    }

    // If there's no timebase, take it from this message
    if( mTimebase < 1 ) {
        mTimebase = msg.getTimestamp();
    }

    int myDelta = now - mMotionStart;
    int nextDelta = msg.getTimestamp() - mTimebase;
    if(VERBOSE) printf("myDelta %d\tnextDelta%d\n",myDelta,nextDelta);

    if( myDelta >= nextDelta ) {
        // If we read a reset, update the timebase
        if( msg.isReset() ) {
            mTimebase = msg.getTimestamp();
            mMotionStart = now;
        }

        msgQ.pop();

        // Signal that it's time to play the message
        return 0;
    } else {
        // Return how long we have to wait to play the next message
        int delay = nextDelta - myDelta;
        return (delay < 0) ? 0 : delay;
    }
}

bool InputMessenger::isEmpty() {
    if( msgQ.empty() ) {
        return true;
    }
    return false;
}

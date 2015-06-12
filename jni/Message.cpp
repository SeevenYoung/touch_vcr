#include "Message.h"
#include "stdio.h"

Message::Message() {
    mType = UNSET;
    mTimestamp = -1;
    mTrackingID = -1;
    mX = -1;
    mY = -1;
}

// TODO this will not scale to more message types
bool Message::fromString(std::string msgText, Message &msg) {
    size_t delim, last_delim;

    // TODO make delimiter a constant
    last_delim = 0;
    delim = msgText.find(' ');
    if(msgText.substr(0, delim).compare("reset") == 0) {
        msg.setType(RESET);
    } else if(msgText.substr(0, delim).compare("stop") == 0) {
        msg.setType(STOP);
    } else if(msgText.substr(0, delim).compare("sync") == 0) {
        msg.setType(SYNC);
    } else {
        return false;
    }

    last_delim = delim + 1;
    delim = msgText.find(' ', last_delim);
    std::string ts = msgText.substr(last_delim, delim);
    msg.setTimestamp( strtol(ts.c_str(), NULL, 10) );

    if( msg.isSync() || msg.isStop() ) {
        last_delim = delim + 1;
        delim = msgText.find(' ', last_delim);
        std::string id = msgText.substr(last_delim, delim);
        msg.setTrackingID( strtol(id.c_str(), NULL, 10) );
    }

    if( msg.isSync() ) {
        last_delim = delim + 1;
        delim = msgText.find(' ', last_delim);
        std::string x = msgText.substr(last_delim, delim);
        msg.setX( strtol(x.c_str(), NULL, 10) );

        last_delim = delim + 1;
        delim = msgText.find(' ', last_delim);
        std::string y = msgText.substr(last_delim, delim);
        msg.setY( strtol(y.c_str(), NULL, 10) );
    }

    return true;
}

Message Message::Reset(int32_t timestamp) {
    Message msg;
    msg.setType(RESET);
    msg.setTimestamp(timestamp);
    return msg;
}

Message Message::Stop(int32_t timestamp, int32_t trackingID) {
    Message msg;
    msg.setType(STOP);
    msg.setTimestamp(timestamp);
    msg.setTrackingID(trackingID);
    return msg;
}

Message Message::Sync(int32_t timestamp, int32_t trackingID, int32_t x, int32_t y) {
    Message msg;
    msg.setType(SYNC);
    msg.setTimestamp(timestamp);
    msg.setTrackingID(trackingID);
    msg.setX(x);
    msg.setY(y);
    return msg;
}

void Message::dump( int fd ) {
    FILE* output = fdopen(fd, "w");
    if( isReset() ) {
        fprintf( output, "reset %d\n", mTimestamp );
    } else if( isStop() ) {
        fprintf( output, "stop %d %d\n", mTimestamp, mTrackingID );
    } else if( isSync() ) {
        fprintf( output, "sync %d %d %d %d\n", mTimestamp, mTrackingID, mX, mY );
    } else {
        fprintf(stderr, "Unknown message format\n");
    }
    fflush(output);
}

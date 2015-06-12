#ifndef MESSAGE
#define MESSAGE

#include "touch_vcr.h"
#include <string>

enum msg_type { 
    UNSET,
    SYNC,
    STOP,
    RESET
};

class Message {
public:
    Message();

    static bool fromString(std::string msgText, Message &msg);

    // TODO Serialization/deserialization
    // TODO binary formats
    static Message Reset(int32_t timestamp);
    static Message Stop(int32_t timestamp, int32_t trackingID);
    static Message Sync(int32_t timestamp, int32_t trackingID, int32_t x, int32_t y);

    inline int32_t getTimestamp() const { return mTimestamp; }
    inline int32_t getTrackingID() const { return mTrackingID; }
    inline int32_t getX() const { return mX; }
    inline int32_t getY() const { return mY; }

    inline bool isUnset() const { return mType == UNSET; }
    inline bool isReset() const { return mType == RESET; } 
    inline bool isStop() const { return mType == STOP; }
    inline bool isSync() const { return mType == SYNC; }

    void dump( int fd );
private:
    inline int32_t setTimestamp(int32_t ts) { mTimestamp = ts; }
    inline int32_t setTrackingID(int32_t id) { mTrackingID = id; }
    inline int32_t setX(int32_t x) { mX = x; }
    inline int32_t setY(int32_t y) { mY = y; }

    inline int32_t getType() const { return mType; }
    inline int32_t setType(msg_type type) { mType = type; }

    // Time of event in ms
    // Will overflow if the phone is not rebooted within 9 months
    int32_t mTimestamp;
    int32_t mTrackingID;
    msg_type mType;
    
    // In the future, this could be a varying payload for different message types
    int32_t mX;
    int32_t mY;

};

#endif

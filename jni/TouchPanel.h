#ifndef TOUCHPANEL
#define TOUCHPANEL

#include "touch_vcr.h"
#include "InputMessenger.h"
#include "Message.h"
#include "Clock.h"

enum SlotState {
    IN_USE,
    DONE,
    NOT_IN_USE
};

/* Keeps track of the state of multi-touch protocol. */
class TouchPanel {
public:
    class Slot {
    public:
        inline SlotState isInUse() const { return mState; }
        inline int32_t getX() const { return mAbsMTPositionX; }
        inline int32_t getY() const { return mAbsMTPositionY; }
        inline int32_t getTouchMajor() const { return mAbsMTTouchMajor; }
        inline int32_t getTouchMinor() const {
            return mHaveAbsMTTouchMinor ? mAbsMTTouchMinor : mAbsMTTouchMajor; }
        inline int32_t getToolMajor() const { return mAbsMTWidthMajor; }
        inline int32_t getToolMinor() const {
            return mHaveAbsMTWidthMinor ? mAbsMTWidthMinor : mAbsMTWidthMajor; }
        inline int32_t getOrientation() const { return mAbsMTOrientation; }
        inline int32_t getTrackingId() const { return mAbsMTTrackingId; }
        inline int32_t getPressure() const { return mAbsMTPressure; }
        inline int32_t getDistance() const { return mAbsMTDistance; }
        inline int32_t getToolType() const;

    private:
        friend class TouchPanel;
        SlotState mState;
        bool mHaveAbsMTTouchMinor;
        bool mHaveAbsMTWidthMinor;
        bool mHaveAbsMTToolType;

        int32_t mAbsMTPositionX;
        int32_t mAbsMTPositionY;
        int32_t mAbsMTTouchMajor;
        int32_t mAbsMTTouchMinor;
        int32_t mAbsMTWidthMajor;
        int32_t mAbsMTWidthMinor;
        int32_t mAbsMTOrientation;
        int32_t mAbsMTTrackingId;
        int32_t mAbsMTPressure;
        int32_t mAbsMTDistance;
        int32_t mAbsMTToolType;

        Slot();
        void clear();
    };

    TouchPanel(const char* device, int numSlots, InputMessenger* messenger, int screenWidth, int screenHeight);
    ~TouchPanel();

    void replay( Message msg, int now );
    void configure(size_t slotCount, bool usingSlotsProtocol);
    void reset();
    void process(const input_event* rawEvent);
    void finishSync();
    int openDevice();

    inline size_t getSlotCount() const { return mSlotCount; }
    inline const Slot* getSlot(size_t index) const { return &mSlots[index]; }

private:
    int32_t mDeviceFD;

    int32_t mCurrentSlot;
    Slot* mSlots;
    size_t mSlotCount;
    bool mUsingSlotsProtocol;
    const char* mDeviceName;

    float mXScale;
    float mYScale;

    int screenWidth;
    int screenHeight;

    InputMessenger* mMessenger;
    Clock mInputClock;

    void clearSlots(int32_t initialSlot);
    bool getAbsoluteAxisValue(int32_t axis, int32_t* outValue);
    bool getAbsoluteAxisInfo(int32_t axis, input_absinfo* outValue);
    bool readConfig();
    void readSlotsConfig();
    void send_event(int code, int type, int value);
};

#endif // TOUCHPANEL

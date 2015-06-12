#include "TouchPanel.h"
#include <fcntl.h>
#include <linux/fb.h>

TouchPanel::TouchPanel(const char* device, int slotCount, InputMessenger* messenger, int width, int height) : 
    mDeviceName(device), mSlotCount(slotCount), mMessenger(messenger), screenWidth(width), screenHeight(height) {
    delete[] mSlots;
    mSlots = new Slot[slotCount];
    mUsingSlotsProtocol = true;
}

void TouchPanel::reset() {
    // Unfortunately there is no way to read the initial contents of the slots.
    // So when we reset the accumulator, we must assume they are all zeroes.
    int32_t initialSlot = -1;
    
    if(mUsingSlotsProtocol) {
        // Query the driver for the current slot index and use it as the initial slot
        // before we start reading events from the device.  It is possible that the
        // current slot index will not be the same as it was when the first event was
        // written into the evdev buffer, which means the input mapper could start
        // out of sync with the initial state of the events in the evdev buffer.
        // In the extremely unlikely case that this happens, the data from
        // two slots will be confused until the next ABS_MT_SLOT event is received.
        // This can cause the touch point to "jump", but at least there will be
        // no stuck touches.
        bool status = getAbsoluteAxisValue(ABS_MT_SLOT, &initialSlot); 
        if (!status) {
            fprintf(stderr, "Could not retrieve current multitouch slot index.  status=%d", status); 
        }
    }

    clearSlots(initialSlot);
    fprintf(stderr, "Got initial slot %d\n", initialSlot);
}

bool TouchPanel::getAbsoluteAxisValue(int32_t axis, int32_t* outValue)  {
    *outValue = 0;
    bool status;
    struct input_absinfo info;

    status = getAbsoluteAxisInfo(axis, &info);
    if(status) {
        *outValue = info.value;
    }

    return status;
}

bool TouchPanel::getAbsoluteAxisInfo(int32_t axis, input_absinfo* outInfo)  {
    if (axis >= 0 && axis <= ABS_MAX) {
        if(ioctl(mDeviceFD, EVIOCGABS(axis), outInfo)) {
            fprintf(stderr, "Error reading absolute controller %d for device %s fd %d, errno=%d",
                 axis, mDeviceName, mDeviceFD, errno);
            return false;
        }

        return true;
    }
    fprintf(stderr, "Bad axis %d for device %s fd %d, errno=%d",
             axis, mDeviceName, mDeviceFD, errno);
    return false;
}

int TouchPanel::openDevice()
{
    // TODO: determine if this device uses the multislot protocol
    // TODO: read config/calibration values
    
    int version;
    char name[80];
    struct input_id id;

    mDeviceFD = open(mDeviceName, O_RDWR);
    if(mDeviceFD < 0) {
        fprintf(stderr, "could not open %s, %s\n", mDeviceName, strerror(errno));
        return -1;
    }
    
    if(ioctl(mDeviceFD, EVIOCGVERSION, &version)) {
        fprintf(stderr, "could not get driver version for %s, %s\n", mDeviceName, strerror(errno));
        return -1;
    }
    if(ioctl(mDeviceFD, EVIOCGID, &id)) {
        fprintf(stderr, "could not get driver id for %s, %s\n", mDeviceName, strerror(errno));
        return -1;
    }
    name[sizeof(name) - 1] = '\0';
    if(ioctl(mDeviceFD, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        fprintf(stderr, "could not get device name for %s, %s\n", mDeviceName, strerror(errno));
        name[0] = '\0';
    }

    if(readConfig() < 1) {
        fprintf(stderr, "could not read axis configuration for %s, %s\n", mDeviceName, strerror(errno));
    }

    return mDeviceFD;
}

/* How to get device resolution
void set_device_details(const char *device) {
    int fb = open(device, O_RDONLY);
    if (fb >= 0) {
        struct fb_var_screeninfo vinfo;
        if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            uint32_t bytespp;
            if (vinfoToPixelFormat(vinfo, &bytespp, &f) == NO_ERROR) {
                size_t offset = (vinfo.xoffset +
vinfo.yoffset*vinfo.xres) * bytespp;
                w = vinfo.xres;
                h = vinfo.yres;
                size = w*h*bytespp;
                mapsize = offset + size;
                mapbase = mmap(0, mapsize, PROT_READ, MAP_PRIVATE, fb,
0);
                if (mapbase != MAP_FAILED) {
                    base = (void const *)((char const *)mapbase +
offset);
                }
            }
        }
        close(fb);
    }
}
*/

// Read the width, height, and whether the panel is using a slots protocol
void TouchPanel::readSlotsConfig() {
    uint8_t bits[ABS_MAX/8];
    int res;

    res = ioctl(mDeviceFD, EVIOCGBIT(EV_ABS, ABS_MAX), bits);

    int index = (int) floor(ABS_MT_SLOT / 8.0); 
    int offset = ABS_MT_SLOT % 8;
    if( bits[index] >> offset & 1 ) {
        fprintf(stderr, "Using slots\n");
        mUsingSlotsProtocol = true;
    } else {
        fprintf(stderr, "Not using slots\n");
        mUsingSlotsProtocol = false;
    }
}


bool TouchPanel::readConfig() {
    input_absinfo info;

    // Grab the resolution from the device
    readSlotsConfig();

    if(getAbsoluteAxisInfo(ABS_MT_POSITION_X, &info) < 1) {
        return false;
    }
    mXScale = float(screenWidth) / (info.maximum - info.minimum + 1);

    if(getAbsoluteAxisInfo(ABS_MT_POSITION_Y, &info) < 1) {
        return false;
    }
    mYScale = float(screenHeight) / (info.maximum - info.minimum + 1);

    fprintf(stderr, "xScale: %f, yScale: %f\n", mXScale, mYScale);
    return true;
}

void TouchPanel::clearSlots(int32_t initialSlot) {
    if (mSlots) {
        for (size_t i = 0; i < mSlotCount; i++) {
            mSlots[i].clear();
        }
    }
    mCurrentSlot = initialSlot;
}

void TouchPanel::process(const input_event* rawEvent) {
    if (rawEvent->type == EV_ABS) {
        bool newSlot = false;
        if (mUsingSlotsProtocol) {
            if (rawEvent->code == ABS_MT_SLOT) {
                mCurrentSlot = rawEvent->value;
                newSlot = true;
            }
        } else {
            if (rawEvent->code == ABS_MT_TRACKING_ID) {
                mCurrentSlot = rawEvent->value;
            }
        }

        if (mCurrentSlot < 0 || size_t(mCurrentSlot) >= mSlotCount) {
            if (newSlot) {
                fprintf(stderr,"MultiTouch device emitted invalid slot index %d but it "
                        "should be between 0 and %d; ignoring this slot.",
                        mCurrentSlot, mSlotCount - 1);
            }
        } else {
            Slot* slot = &mSlots[mCurrentSlot];

            switch (rawEvent->code) {
            case ABS_MT_POSITION_X:
                slot->mState = IN_USE;
                slot->mAbsMTPositionX = rawEvent->value;
                break;
            case ABS_MT_POSITION_Y:
                slot->mState = IN_USE;
                slot->mAbsMTPositionY = rawEvent->value;
                break;
            case ABS_MT_TOUCH_MAJOR:
                slot->mState = IN_USE;
                slot->mAbsMTTouchMajor = rawEvent->value;
                break;
            case ABS_MT_TOUCH_MINOR:
                slot->mState = IN_USE;
                slot->mAbsMTTouchMinor = rawEvent->value;
                slot->mHaveAbsMTTouchMinor = true;
                break;
            case ABS_MT_WIDTH_MAJOR:
                slot->mState = IN_USE;
                slot->mAbsMTWidthMajor = rawEvent->value;
                break;
            case ABS_MT_WIDTH_MINOR:
                slot->mState = IN_USE;
                slot->mAbsMTWidthMinor = rawEvent->value;
                slot->mHaveAbsMTWidthMinor = true;
                break;
            case ABS_MT_ORIENTATION:
                slot->mState = IN_USE;
                slot->mAbsMTOrientation = rawEvent->value;
                break;
            case ABS_MT_TRACKING_ID:
                if (mUsingSlotsProtocol && rawEvent->value < 0) {
                    // TODO - need to capture this
                    // The slot is no longer in use but it retains its previous contents,
                    // which may be reused for subsequent touches.
                    slot->mState = DONE;
                } else {
                    slot->mState = IN_USE;
                    slot->mAbsMTTrackingId = rawEvent->value;
                }
            case ABS_MT_PRESSURE:
                break;
                slot->mState = IN_USE;
                slot->mAbsMTPressure = rawEvent->value;
                break;
            case ABS_MT_DISTANCE:
                slot->mState = IN_USE;
                slot->mAbsMTDistance = rawEvent->value;
                break;
            case ABS_MT_TOOL_TYPE:
                slot->mState = IN_USE;
                slot->mAbsMTToolType = rawEvent->value;
                slot->mHaveAbsMTToolType = true;
                break;
            }
        }
    } else if (rawEvent->type == EV_SYN && rawEvent->code == SYN_MT_REPORT) {
        // MultiTouch Sync: The driver has returned all data for *one* of the pointers.
        if(mUsingSlotsProtocol) {
            mCurrentSlot += 1;
        }
    } else if( rawEvent->type == EV_SYN && rawEvent->code == SYN_REPORT) {
        int32_t timestamp = (rawEvent->time.tv_sec*1000) + (rawEvent->time.tv_usec/1000);
        // Iterate over the slots.  If a slot is done, report that
        for(int i = 0; i < mSlotCount; i++ ) {
            Slot* slot = &mSlots[i];
            if(slot->mState == DONE) {
                slot->mState = NOT_IN_USE;    
                Message msg;
                msg = Message::Stop(timestamp, slot->getTrackingId()); 
                mMessenger->send(msg);
            }
            if(slot->mState == IN_USE) {
                int32_t x = slot->getX() * mXScale;
                int32_t y = slot->getY() * mYScale;
                Message msg;
                msg = Message::Sync(timestamp, slot->getTrackingId(), x, y);
                mMessenger->send(msg);
                if(!mUsingSlotsProtocol) {
                    slot->mState = DONE;
                }
            }
        }
    }
}
/*
#ifdef DEBUG
        printf("Replaying sync %d %d %d %d\n", msg.getTimestamp(), msg.getTrackingID(), msg.getX(), msg.getY() );
#endif
        send_event(EV_ABS, ABS_MT_SLOT, 0); 
        send_event(EV_ABS, ABS_MT_TRACKING_ID, msg.getTrackingID()); 
        send_event(EV_ABS, ABS_MT_POSITION_X, msg.getX()/mXScale); 
        send_event(EV_ABS, ABS_MT_POSITION_Y, msg.getY()/mYScale); 
        send_event(EV_SYN, SYN_REPORT, 0); 
    }
    if( msg.isStop() ) {
#ifdef DEBUG
        printf("Replaying stop %d\n", msg.getTimestamp() );
#endif
        send_event(EV_ABS, ABS_MT_SLOT, 0); 
        send_event(EV_ABS, ABS_MT_TRACKING_ID, -1); 
        send_event(EV_SYN, SYN_REPORT, 0); 
    }
*/

void TouchPanel::replay( Message msg, int now ) {
    if( msg.isSync() ) {
#ifdef DEBUG
        printf("Replaying sync %d %d %d %d\n", msg.getTimestamp(), msg.getTrackingID(), msg.getX(), msg.getY() );
#endif
        if(mUsingSlotsProtocol) {
            send_event(EV_ABS, ABS_MT_TRACKING_ID, msg.getTrackingID()); 
        } else {
            send_event(EV_ABS, ABS_MT_TRACKING_ID, 0); 
        }
        send_event(EV_ABS, ABS_MT_POSITION_X, msg.getX()/mXScale); 
        send_event(EV_ABS, ABS_MT_POSITION_Y, msg.getY()/mYScale); 
        send_event(EV_ABS, ABS_MT_PRESSURE, 30); 
        if(!mUsingSlotsProtocol) {
            send_event(EV_SYN, SYN_MT_REPORT, 0); 
        }
        send_event(EV_SYN, SYN_REPORT, 0); 
    }
    if( msg.isStop() ) {
#ifdef DEBUG
        printf("Replaying stop %d\n", msg.getTimestamp() );
#endif
        if(mUsingSlotsProtocol) {
            send_event(EV_ABS, ABS_MT_TRACKING_ID, -1); 
        } else {
            send_event(EV_SYN, SYN_MT_REPORT, 0); 
        }
        send_event(EV_SYN, SYN_REPORT, 0); 
    }
}

void TouchPanel::send_event(int type, int code, int value) {
    /* Emergency backup method
     * slow, but reliable
    char cmd[100];
    sprintf(cmd, "sendevent /dev/input/event3 %d %d %d",type, code, value); 
    system(cmd); */
    struct input_event event;
    int res;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    res = write(mDeviceFD, &event, sizeof(event));
}

void TouchPanel::finishSync() {
    if (!mUsingSlotsProtocol) {
        clearSlots(-1);
    }
}


// --- TouchPanel::Slot ---

TouchPanel::Slot::Slot() {
    clear();
}

void TouchPanel::Slot::clear() {
    mState = NOT_IN_USE;
    mHaveAbsMTTouchMinor = false;
    mHaveAbsMTWidthMinor = false;
    mHaveAbsMTToolType = false;
    mAbsMTPositionX = 0;
    mAbsMTPositionY = 0;
    mAbsMTTouchMajor = 0;
    mAbsMTTouchMinor = 0;
    mAbsMTWidthMajor = 0;
    mAbsMTWidthMinor = 0;
    mAbsMTOrientation = 0;
    mAbsMTTrackingId = -1;
    mAbsMTPressure = 0;
    mAbsMTDistance = 0;
    mAbsMTToolType = 0;
}


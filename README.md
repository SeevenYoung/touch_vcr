# Touch VCR

This is a simple tool for recording and playing back touch events on Android devices. It records
all touch events with a timestamp in a simple space-delimited log. It can also read the log
and reproduce the recorded touch input exactly. It's useful for recording and replaying gestures
during testing.

# Building

Build with the Android NDK. It's pretty simple, so any version should work. If you're on a device
older than API 16 you may need to modify Application.mk.

# Installation

Touch VCR requires root permissions. You can install it on a rooted device as follows:

    ndk-build && adb push libs/armeabi/touch_vcr /sdcard/
    adb shell
    su

Here you'll need to grant your shell sudo on your device.

    cp /sdcard/touch_vcr .
    chmod 777 touch_vcr

# Usage

Simply run it to record all touch events.

    ./touch_vcr > touches.txt

Swipe around on the device a bit and kill it with ctrl-c when you're done. touches.txt should look
like this

    sync 2662 84 288 433
    sync 2670 84 291 433
    sync 2678 84 293 433
    sync 2686 84 296 433
    sync 2694 84 299 432
    stop 2702 84
    sync 4054 85 291 373
    sync 4062 85 287 373
    sync 4077 85 274 373
    sync 4088 85 270 374
    sync 4094 85 260 376
    sync 4102 85 251 379
    sync 4110 85 240 384
    sync 4118 85 215 399
    stop 4125 85

Each line is an input event from the touch panel. The values are 
- Type of event ('stop' when finger lifted)
- Timestamp in millis since program start
- Finger id (for handling multitouch)
- touch x coordinate
- touch y coordinate

For ease of use with different devices, all x,y coordinates for touches are normalized to a 360x640
resolution.

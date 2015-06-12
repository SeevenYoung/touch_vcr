#include "Clock.h"

Clock::Clock() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    mStartTimeSeconds = t.tv_sec;
}

int32_t Clock::getTimestamp(const timeval &time) {
    return (time.tv_sec - mStartTimeSeconds)*1000LL + time.tv_usec/1000LL;
}

int32_t Clock::getTimestamp(const timespec &time) {
    return (time.tv_sec - mStartTimeSeconds)*1000LL + time.tv_nsec/1000000LL;
}

int32_t Clock::getTimestampStart() {
    return (int32_t)(mStartTimeSeconds)*1000LL;
}

int32_t Clock::getTimestampNow() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return getTimestamp(t);
}

#ifndef CLOCK
#define CLOCK

#include <linux/input.h>
#include <time.h>

class Clock {
 public: 
  Clock();

  int32_t getTimestamp(const timeval &time);
  int32_t getTimestamp(const timespec &time);

  int32_t getTimestampNow();
  int32_t getTimestampStart();

 private:
  int32_t mStartTimeSeconds;
};

#endif

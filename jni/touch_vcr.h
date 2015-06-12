#ifndef EVENTCAT_HEADER
#define EVENTCAT_HEADER

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <time.h>

typedef int64_t nsecs_t;

// Stealing multitouch defines from the kernel since
// the NDK seems to lack them
#define EVIOCGMTSLOTS(len)      _IOC(_IOC_READ, 'E', 0x0a, len)

#define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR      0x31    /* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR      0x32    /* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR      0x33    /* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION      0x34    /* Ellipse orientation */
#define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
#define ABS_MT_BLOB_ID          0x38    /* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#define ABS_MT_PRESSURE         0x3a    /* Pressure on contact area */
#define ABS_MT_DISTANCE         0x3b    /* Contact hover distance */

/*
 * MT_TOOL types
 */
#define MT_TOOL_FINGER          0
#define MT_TOOL_PEN             1
#define MT_TOOL_MAX             1

/*
 * Synchronization events.
 */
#define SYN_REPORT              0
#define SYN_CONFIG              1
#define SYN_MT_REPORT           2
#define SYN_DROPPED             3

#endif // EVENTCAT_HEADER

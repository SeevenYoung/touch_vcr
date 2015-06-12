#include "touch_vcr.h"
#include "TouchPanel.h"
#include "InputMessenger.h"
#include "Clock.h"

#include "sys/system_properties.h"

bool VERBOSE = false;
bool SCALE_NHD = false;
const int MAX_PATH = 256;

static struct pollfd ufds[2];

bool is_touch_device(const char *devname) 
{
    int fd;
    int res;
    uint8_t *bits = NULL;
    ssize_t bits_size = 0;

    fd = open(devname, O_RDONLY);

    while(1) {
        res = ioctl(fd, EVIOCGBIT(EV_ABS, bits_size), bits);
        if(res < bits_size)
            break;
        bits_size = res + 16;
        bits = (uint8_t*) realloc(bits, bits_size * 2);
        if(bits == NULL) {
            fprintf(stderr, "failed to allocate buffer of size %d\n", (int)bits_size);
            return 1;
        }
    }

    // Check if the device produces multi-touch events.
    int i,k;
    for( i = 0; i < res; i++ ) {
        for(k = 0; k < 8; k++) {
            if(bits[i] & 1 << k) {
                if( (i*8+k) == ABS_MT_POSITION_X ) {
                    close(fd);
                    return true;
                }
            }
        }
    }
    close(fd);
    return false;
}

bool scan_devices(const char *dirname, char *devname)
{
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    int fd;
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        if(is_touch_device(devname)) {
            return true;
        }   
    }
    closedir(dir);
    return false;
}

static void usage(int argc, char *argv[]) {
    fprintf(stderr, "Usage: %s [options] <device>\n", argv[0]);
    fprintf(stderr, "    -b: use binary formatted data (default is ASCII) (NOT IMPLEMENTED)\n");
    fprintf(stderr, "    -d: print extra debugging on stderr\n");
    fprintf(stderr, "    -s: scale all touches to nHD (360x640)\n");
    fprintf(stderr, "    -q: quit when stdin is closed (good for catting files) (NOT IMPLEMNTED)\n");
    fprintf(stderr, "    -x<width>: width of screen (default 720)\n");
    fprintf(stderr, "    -y<height>: height of screen (default 1280)\n");
    fprintf(stderr, "If a device isn't specified, it will be inferred\n");
    fprintf(stderr, "from the product name\n");
}

/* NOTE: The devices we care about only have two event devices - the touch panel and the power+volume buttons.
*/ 
int main(int argc, char *argv[])
{
    TouchPanel *touchPanel;
    InputMessenger* messenger;
    Clock clock;
    char device[MAX_PATH];
    int pollres = 0;
    int res = 0;
    input_event event;
    long current;

    // Default to thinking we have a NHD screen
    int screenWidth = 360;
    int screenHeight = 640;

    char product[PROP_VALUE_MAX];
    __system_property_get("ro.product.name",product);
    printf("Product: %s\n", product);

    scan_devices("/dev/input", device);
    fprintf(stderr, "Detected multitouch device: %s\n", device);

    int c;
    opterr = 0;
    do {
        c = getopt(argc, argv, "bdsh");
        if (c == EOF)
            break;
        switch (c) {
        case 'b':
            break;
        case 's':
            fprintf(stderr, "Scaling all touches to nHD resolution\n");
            SCALE_NHD = true;
            break;
        case 'v':
            VERBOSE = true;
            break;
        case 'h':
            usage(argc, argv);
            exit(1);
        case 'x':
            screenWidth = atoi(optarg);
            break;
        case 'y':
            screenHeight = atoi(optarg);
            break;
        }
    } while(1);

    if (optind + 1 == argc) {
        strcpy(device, argv[optind]);
        optind++;
    }
    if (optind != argc) {
        usage(argc, argv);
        exit(1);
    }

    messenger = new InputMessenger();

    if( SCALE_NHD ) {
        screenWidth = 360;
        screenHeight = 640;
    }
    touchPanel = new TouchPanel(device, 4, messenger, screenWidth, screenHeight);
    
    ufds[0].fd = touchPanel->openDevice();
    ufds[0].events = POLLIN;

    messenger->setInFD( STDIN_FILENO );
    messenger->setOutFD( STDOUT_FILENO );

    // Make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0); /* get current file status flags */
    flags |= O_NONBLOCK;        /* turn off blocking flag */
    fcntl(STDIN_FILENO, F_SETFL, flags);     /* set up non-blocking read */

    ufds[1].fd = STDIN_FILENO;
    ufds[1].events = POLLIN;

    // Device discovery and setup (based on which phone this is)
    if(VERBOSE) printf("Starting input polling %d\n", clock.getTimestampStart());

    int pollTimeout = -1;
    int now = 0;
    Message msg;

    while(1) {
        now = clock.getTimestampNow();
        if(!messenger->isEmpty()) {
            pollTimeout = messenger->dequeue(now, msg);
            while( pollTimeout == 0 )  {
                touchPanel->replay(msg, now);
                pollTimeout = messenger->dequeue(now, msg);
                if(VERBOSE) fprintf(stderr, "Set poll timeout to %d\n", pollTimeout);
            }
        }
        pollres = poll(ufds, 2, pollTimeout);

        // Input from touch panel
        if(ufds[0].revents & POLLIN) {
            if(VERBOSE) fprintf(stderr, "Saw event\n");
            res = read(ufds[0].fd, &event, sizeof(event));
            if(res < (int)sizeof(event)) {
                fprintf(stderr, "could not get event\n");
                return 1;
            }
            
            touchPanel->process(&event);
        }

        // Input from STDIN
        if(ufds[1].revents & POLLIN) {
            if(VERBOSE) fprintf(stderr, "Saw stdin\n");
            messenger->fill_queue();
        }

        // If we get POLLHUP, then stdin has closed
        if(ufds[1].revents & POLLHUP) {
            ufds[1].fd = -1;
        }
    
    }

    return 0;
}


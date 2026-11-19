#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#define ROTATION_0  0
#define ROTATION_90  1
#define ROTATION_180  2
#define ROTATION_270  3
#define RAW_WIDTH_DEVICE 1800
#define RAW_HEIGHT_DEVICE 2880
#define RAW_RATIO_DEVICE 10

static const struct timespec t50ms = {0, 50 * 1000 * 1000};

int send_event(int fd, __u16 type, __u16 code, __s32 value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    if (write(fd, &ev, sizeof(ev)) < sizeof(ev)) {
        fprintf(stderr, "Write error!\n");
        return -1;
    }
    return 0;
}

void mapCoordinatesSH(__u16 *x, __u16 *y, int rotation) {
    int map_x, map_y;
    switch (rotation) {
        case ROTATION_0:
            map_x = *x;
            map_y = *y;
            break;
        case ROTATION_90:
            map_x = RAW_WIDTH_DEVICE - *y;
            map_y = *x;
            break;
        case ROTATION_180:
            map_x = RAW_WIDTH_DEVICE - *x;
            map_y = RAW_HEIGHT_DEVICE - *y;
            break;
        case ROTATION_270:
            map_x = *y;
            map_y = RAW_HEIGHT_DEVICE - *x;
            break;
        default:
            map_x = *x;
            map_y = *y;
            break;
    }
    *x = map_x;     
    *y = map_y;
}


int performance_click_write(int fd, __u16 x, __u16 y) {
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, 1);
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, x * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, y * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 8);
    send_event(fd, EV_ABS, ABS_MT_PRESSURE, 1);
    send_event(fd, EV_KEY, BTN_TOUCH, 1);
    send_event(fd, EV_SYN, SYN_REPORT, 0);
    nanosleep(&t50ms, NULL);
    send_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
    send_event(fd, EV_ABS, ABS_MT_PRESSURE, 0);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    send_event(fd, EV_KEY, BTN_TOUCH, 0);
    send_event(fd, EV_SYN, SYN_REPORT, 0);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc >= 5) {
        int fd = open(argv[1], O_WRONLY);
        if (fd < 0) {
            perror("Input device not found!\n");
            return 1;
        }
        __u16 x = (__u16) atoi(argv[2]);
        __u16 y = (__u16) atoi(argv[3]);
        char rotation = (char) atoi(argv[4]);
        mapCoordinatesSH(&x, &y, rotation);
        int ret = performance_click_write(fd, x, y);
        close(fd);
        return ret;
    } else {
        perror("args to short!\n");
        return 1;
    }
}